/*
 * $Header$
 *
 * Copyright 2006 Massachusetts Institute of Technology.
 * All Rights Reserved.
 *
 * Export of this software from the United States of America may
 * require a specific license from the United States Government.
 * It is the responsibility of any person or organization contemplating
 * export to obtain such a license before exporting.
 *
 * WITHIN THAT CONSTRAINT, permission to use, copy, modify, and
 * distribute this software and its documentation for any purpose and
 * without fee is hereby granted, provided that the above copyright
 * notice appear in all copies and that both that copyright notice and
 * this permission notice appear in supporting documentation, and that
 * the name of M.I.T. not be used in advertising or publicity pertaining
 * to distribution of the software without specific, written prior
 * permission.  Furthermore if you modify this software you must label
 * your software as modified software and not distribute it in such a
 * fashion that it might be confused with the original M.I.T. software.
 * M.I.T. makes no representations about the suitability of
 * this software for any purpose.  It is provided "as is" without express
 * or implied warranty.
 */

#ifndef LEAN_CLIENT

#include "k5_mig_client.h"
#include "krb5-ipc.h"
#include "k5_mig_request.h"
#include "k5_mig_replyServer.h"
#include "k5-thread.h"

#include <mach/mach.h>
#include <mach/task_special_ports.h>
#include <servers/bootstrap.h>
#include <dlfcn.h>


/* ------------------------------------------------------------------------ */

/* Number of services available.  Update if modifying the lists below */
#define KIPC_SERVICE_COUNT 2

/* This struct exists to hold the per-thread connection port used for ipc
 * messages to the server.  Each thread is issued a separate connection 
 * port so that the server can distinguish between threads in the same 
 * application. */

typedef struct k5_ipc_connection {
    const char *service_id;
    mach_port_t port;
} *k5_ipc_connection;

typedef struct k5_ipc_connection_info {
    struct k5_ipc_connection connections[KIPC_SERVICE_COUNT];
    boolean_t server_died;
    k5_ipc_stream reply_stream;
} *k5_ipc_connection_info;

/* initializer for k5_ipc_request_port to fill in server names in TLS */
static const char *k5_ipc_known_services[KIPC_SERVICE_COUNT] = { 
"edu.mit.Kerberos.CCacheServer",
"edu.mit.Kerberos.KerberosAgent" };


/* ------------------------------------------------------------------------ */

/* This struct exists to store the global service port shared between all
 * threads.  Note that there is one of these ports per server, whereas
 * there is one connection port per thread.  Thus this is global and mutexed,
 * whereas the connection ports below are in TLS */

typedef struct k5_ipc_service_port {
    const char *service_id;
    mach_port_t service_port;
} k5_ipc_service_port;

/* global service ports and mutex to protect it */
static k5_mutex_t g_service_ports_mutex = K5_MUTEX_PARTIAL_INITIALIZER;
static k5_ipc_service_port g_service_ports[KIPC_SERVICE_COUNT] = { 
{ "edu.mit.Kerberos.CCacheServer", MACH_PORT_NULL },
{ "edu.mit.Kerberos.KerberosAgent", MACH_PORT_NULL } };

static int use_uid = 0;
static uid_t targetuid;

static void
clear_cache(void)
{      
    k5_ipc_connection_info cinfo;
    int i;
        
    /* assume lock is taken */
    for (i = 0; i < KIPC_SERVICE_COUNT; i++) {
	if (MACH_PORT_VALID (g_service_ports[i].service_port)) {
	    mach_port_destroy (mach_task_self (), 
			       g_service_ports[i].service_port); 
	    g_service_ports[i].service_port = MACH_PORT_NULL;
	}
    }

    cinfo = k5_getspecific (K5_KEY_IPC_CONNECTION_INFO);
    if (cinfo) {
	for (i = 0; i < KIPC_SERVICE_COUNT; i++) {
            if (MACH_PORT_VALID (cinfo->connections[i].port))
		mach_port_deallocate(mach_task_self(), 
				     cinfo->connections[i].port);
	    cinfo->connections[i].port = MACH_PORT_NULL;
	}
    }
}


void
krb5_ipc_client_set_target_uid(uid_t uid)
{
    int err;

    err = k5_mutex_lock (&g_service_ports_mutex);
    if (!err) {
	use_uid = 1;
	targetuid = uid;
	clear_cache();
        k5_mutex_unlock (&g_service_ports_mutex);
    }
}

void
krb5_ipc_client_clear_target(void)
{
    int err;

    err = k5_mutex_lock (&g_service_ports_mutex);
    if (!err) {
	use_uid = 0;
	clear_cache();
        k5_mutex_unlock (&g_service_ports_mutex);
    }
}



/* ------------------------------------------------------------------------ */

static void k5_ipc_client_cinfo_free (void *io_cinfo)
{
    if (io_cinfo) {
        k5_ipc_connection_info cinfo = io_cinfo;
        int i;
        
        for (i = 0; i < KIPC_SERVICE_COUNT; i++) {
            if (MACH_PORT_VALID (cinfo->connections[i].port)) {
                mach_port_mod_refs (mach_task_self(), 
                                    cinfo->connections[i].port, 
                                    MACH_PORT_RIGHT_SEND, -1 );
                cinfo->connections[i].port = MACH_PORT_NULL;
            }
        }
        /* reply_stream will always be freed by k5_ipc_send_request() */
        free (cinfo);
    }
}

/* ------------------------------------------------------------------------ */

static int k5_ipc_client_cinfo_allocate (k5_ipc_connection_info *out_cinfo)
{
    int err = 0;
    k5_ipc_connection_info cinfo = NULL;
    
    cinfo = malloc (sizeof (*cinfo));
    if (!cinfo) { err = ENOMEM; }
    
    if (!err) {
        int i;
        
        cinfo->server_died = 0;
        cinfo->reply_stream = NULL;
        
        for (i = 0; i < KIPC_SERVICE_COUNT; i++) {
            cinfo->connections[i].service_id = k5_ipc_known_services[i];
            cinfo->connections[i].port = MACH_PORT_NULL;
        }
    }
    
    if (!err) {
        *out_cinfo = cinfo;
        cinfo = NULL;
    }
    
    k5_ipc_client_cinfo_free (cinfo);
    
    return err;
}


#pragma mark -

MAKE_INIT_FUNCTION(k5_cli_ipc_thread_init);
MAKE_FINI_FUNCTION(k5_cli_ipc_thread_fini);

/* ------------------------------------------------------------------------ */

static int k5_cli_ipc_thread_init (void)
{
    int err = 0;
    
    err = k5_key_register (K5_KEY_IPC_CONNECTION_INFO, 
                           k5_ipc_client_cinfo_free);
    
    if (!err) {
        err = k5_mutex_finish_init (&g_service_ports_mutex);
    }
    
    return err;
}

/* ------------------------------------------------------------------------ */

static void k5_cli_ipc_thread_fini (void)
{    
    int err = 0;
    
    err = k5_mutex_lock (&g_service_ports_mutex);

    if (!err) {
	clear_cache();
        k5_mutex_unlock (&g_service_ports_mutex);
    }
    
    k5_key_delete (K5_KEY_IPC_CONNECTION_INFO);
    k5_mutex_destroy (&g_service_ports_mutex);
}

/* SPI */ kern_return_t
bootstrap_look_up_per_user(mach_port_t, name_t, uid_t, mach_port_t *);


static int
get_service_port(char *service, mach_port_t *sport)
{
    int ret;

    if (use_uid) {
	mach_port_t rootbs;

        ret = task_get_bootstrap_port(mach_task_self(), &rootbs);
	if (ret)
	    return ret;

	/* search up the namespace tree to the root */
	while (1) {
	    kern_return_t kr;
	    mach_port_t parent;
	    kr = bootstrap_parent(rootbs, &parent);

	    if (kr == KERN_SUCCESS) {
		if (parent == MACH_PORT_NULL || parent == rootbs)
		    break;
	    } else
		break;
	    mach_port_deallocate(mach_task_self(), rootbs);
	    rootbs = parent;
	}

	ret = bootstrap_look_up_per_user(rootbs, service, targetuid,
					 sport);
	mach_port_deallocate(mach_task_self(), rootbs);
    } else {
	mach_port_t boot_port;

        ret = task_get_bootstrap_port(mach_task_self(), &boot_port);
	if (ret)
	    return ret;
	ret = bootstrap_look_up (boot_port, service, sport);
	mach_port_deallocate(mach_task_self(), boot_port);
    }

    return ret;
}

#pragma mark -

/* ------------------------------------------------------------------------ */

static kern_return_t k5_ipc_client_lookup_server (const char  *in_service_id,
                                                  boolean_t    in_launch_if_necessary,
                                                  boolean_t    in_use_cached_port,
                                                  mach_port_t *out_service_port) 
{
    kern_return_t err = 0;
    kern_return_t lock_err = 0;
    mach_port_t k5_service_port = MACH_PORT_NULL;
    boolean_t found_entry = 0;
    int i;
    
    in_launch_if_necessary = 1;

    if (!in_service_id   ) { err = EINVAL; }
    if (!out_service_port) { err = EINVAL; }
    
    if (!err) {
        lock_err = k5_mutex_lock (&g_service_ports_mutex);
        if (lock_err) { err = lock_err; }
    }
    
    for (i = 0; !err && i < KIPC_SERVICE_COUNT; i++) {
        if (!strcmp (in_service_id, g_service_ports[i].service_id)) {
            found_entry = 1;
            if (in_use_cached_port) {
                k5_service_port = g_service_ports[i].service_port;
            }
            break;
        }
    }
    
    if (!err && (!MACH_PORT_VALID (k5_service_port) || !in_use_cached_port)) {
        char *service = NULL;
        
        if (!err && !in_launch_if_necessary) {
            char *lookup = NULL;
            mach_port_t lookup_port = MACH_PORT_NULL;
            
            int w = asprintf (&lookup, "%s%s", 
                              in_service_id, K5_MIG_LOOKUP_SUFFIX);
            if (w < 0) { err = ENOMEM; }
            
	    /* Use the lookup name because the service name will return 
	     * a valid port even if the server isn't running */
            if (!err) {
		err = get_service_port (lookup, &lookup_port);
		free (lookup);
            }
            
            if (MACH_PORT_VALID (lookup_port)) { 
                mach_port_deallocate (mach_task_self (), lookup_port); 
            }
        }
        
        if (!err) {
            int w = asprintf (&service, "%s%s", 
                              in_service_id, K5_MIG_SERVICE_SUFFIX);
            if (w < 0) { err = ENOMEM; }
        }
        
        if (!err) {
	    err = get_service_port (service, &k5_service_port);
	    free (service);
            
            if (!err && found_entry) {
                /* Free old port if it is valid */
                if (!err && MACH_PORT_VALID (g_service_ports[i].service_port)) {
                    mach_port_deallocate (mach_task_self (), 
                                          g_service_ports[i].service_port);
                }
                
                g_service_ports[i].service_port = k5_service_port;
            }
        }
    }
    
    if (!err) {
        *out_service_port = k5_service_port;
    }
    
    if (!lock_err) { k5_mutex_unlock (&g_service_ports_mutex); }
    
    return err;
}

#pragma mark -

/* ------------------------------------------------------------------------ */

static boolean_t k5_ipc_reply_demux (mach_msg_header_t *request, 
                                     mach_msg_header_t *reply) 
{
    boolean_t handled = 0;
    
    if (CALL_INIT_FUNCTION (k5_cli_ipc_thread_init) != 0) {
        return 0;
    }    
    
    if (!handled && request->msgh_id == MACH_NOTIFY_NO_SENDERS) {
        k5_ipc_connection_info cinfo = k5_getspecific (K5_KEY_IPC_CONNECTION_INFO);
        if (cinfo) { 
            cinfo->server_died = 1;
        }
        
        handled = 1; /* server died */
    }
    
    if (!handled) {
        handled = k5_ipc_reply_server (request, reply);
    }
    
    return handled;    
}

/* ------------------------------------------------------------------------ */

kern_return_t k5_ipc_client_reply (mach_port_t             in_reply_port,
                                   k5_ipc_inl_reply_t      in_inl_reply,
                                   mach_msg_type_number_t  in_inl_replyCnt,
                                   k5_ipc_ool_reply_t      in_ool_reply,
                                   mach_msg_type_number_t  in_ool_replyCnt)
{
    kern_return_t err = KERN_SUCCESS;
    k5_ipc_connection_info cinfo = NULL;
    
    if (!err) {
        err = CALL_INIT_FUNCTION (k5_cli_ipc_thread_init);
    }
    
    if (!err) {
        cinfo = k5_getspecific (K5_KEY_IPC_CONNECTION_INFO);
        if (!cinfo || !cinfo->reply_stream) { err = EINVAL; }
    }
    
    if (!err) {
        if (in_inl_replyCnt) {
            err = k5_ipc_stream_write (cinfo->reply_stream, 
                                       in_inl_reply, in_inl_replyCnt);
            
        } else if (in_ool_replyCnt) {
            err = k5_ipc_stream_write (cinfo->reply_stream, 
                                       in_ool_reply, in_ool_replyCnt);
            
        } else {
            err = EINVAL;
        }
    }
    
    if (in_ool_replyCnt) { vm_deallocate (mach_task_self (), 
                                          (vm_address_t) in_ool_reply, 
                                          in_ool_replyCnt); }
    
    return err;
}

#pragma mark -

/* ------------------------------------------------------------------------ */

int32_t k5_ipc_send_request (const char    *in_service_id,
                             int32_t        in_launch_server,
                             k5_ipc_stream  in_request_stream,
                             k5_ipc_stream *out_reply_stream)
{
    int err = 0;
    int32_t done = 0;
    int32_t try_count = 0;
    mach_port_t server_port = MACH_PORT_NULL;
    k5_ipc_connection_info cinfo = NULL;
    k5_ipc_connection connection = NULL;
    mach_port_t reply_port = MACH_PORT_NULL;
    const char *inl_request = NULL; /* char * so we can pass the buffer in directly */
    mach_msg_type_number_t inl_request_length = 0;
    k5_ipc_ool_request_t ool_request = NULL;
    mach_msg_type_number_t ool_request_length = 0;

    if (!in_request_stream) { err = EINVAL; }
    if (!out_reply_stream ) { err = EINVAL; }
    
    if (!err) {
        err = CALL_INIT_FUNCTION (k5_cli_ipc_thread_init);
    }    
    
    if (!err) {
        /* depending on how big the message is, use the fast inline buffer or  
         * the slow dynamically allocated buffer */
        mach_msg_type_number_t request_length = k5_ipc_stream_size (in_request_stream);
        
        if (request_length > K5_IPC_MAX_INL_MSG_SIZE) {
            /*dprintf ("%s choosing out of line buffer (size is %d)", 
             *                  __FUNCTION__, request_length); */
            
            err = vm_read (mach_task_self (), 
                           (vm_address_t) k5_ipc_stream_data (in_request_stream), 
                           request_length, 
                           (vm_address_t *) &ool_request, 
                           &ool_request_length);        
        } else {
            /*dprintf ("%s choosing in line buffer (size is %d)",
             *                  __FUNCTION__, request_length); */
            
            inl_request_length = request_length;
            inl_request = k5_ipc_stream_data (in_request_stream);
        }
    }

    if (!err) {
        cinfo = k5_getspecific (K5_KEY_IPC_CONNECTION_INFO);

        if (!cinfo) {
            err = k5_ipc_client_cinfo_allocate (&cinfo);

            if (!err) {
                err = k5_setspecific (K5_KEY_IPC_CONNECTION_INFO, cinfo);
            }
        }
        
        if (!err) {
            int i, found = 0;

            for (i = 0; i < KIPC_SERVICE_COUNT; i++) {
                if (!strcmp (in_service_id, cinfo->connections[i].service_id)) {
                    found = 1;
                    connection = &cinfo->connections[i];
                    break;
                }
            }
            
            if (!found) { err = EINVAL; }
        }
    }
    
    if (!err) {
        err = mach_port_allocate (mach_task_self (), MACH_PORT_RIGHT_RECEIVE, 
                                  &reply_port);
    }
    
    while (!err && !done) {
        if (!err && !MACH_PORT_VALID (connection->port)) {
	    err = k5_ipc_client_lookup_server (in_service_id, in_launch_server, 
					       try_count == 0 ? TRUE : FALSE,
					       &server_port);
	    if (err)
		continue;

	    err = k5_ipc_client_create_client_connection(server_port, 
							 &connection->port);
	    if (err)
		continue;
        }
        
        if (!err) {
            err = k5_ipc_client_request (connection->port, reply_port,
                                         inl_request, inl_request_length,
                                         ool_request, ool_request_length);
            
        }
        
        if (err == MACH_SEND_INVALID_DEST) {
            if (try_count < 2) { 
                try_count++;
                err = 0;
            }

            if (MACH_PORT_VALID (connection->port)) {
                mach_port_mod_refs (mach_task_self(), connection->port, 
                                    MACH_PORT_RIGHT_SEND, -1 );
                connection->port = MACH_PORT_NULL;
            }    

        } else {
            /* Talked to server, though we may have gotten an error */
            done = 1;
            
            /* Because we use ",dealloc" ool_request will be freed by mach. 
            * Don't double free it. */
            ool_request = NULL; 
            ool_request_length = 0;                
        }            
    }
    
    if (!err) {
        err = k5_ipc_stream_new (&cinfo->reply_stream);
    }
    
    if (!err) {
        mach_port_t old_notification_target = MACH_PORT_NULL;

        /* request no-senders notification so we know when server dies */
        err = mach_port_request_notification (mach_task_self (), reply_port, 
                                              MACH_NOTIFY_NO_SENDERS, 1, 
                                              reply_port, 
                                              MACH_MSG_TYPE_MAKE_SEND_ONCE, 
                                              &old_notification_target);
        
        if (!err && old_notification_target != MACH_PORT_NULL) {
            mach_port_deallocate (mach_task_self (), old_notification_target);
        }
    }
    
    if (!err) {
        cinfo->server_died = 0;
        
        err = mach_msg_server_once (k5_ipc_reply_demux, K5_IPC_MAX_MSG_SIZE, 
                                    reply_port, MACH_MSG_TIMEOUT_NONE);
        
        if (!err && cinfo->server_died) {
            err = ENOTCONN;
        }
    }
    
    if (err == BOOTSTRAP_UNKNOWN_SERVICE && !in_launch_server) {
        err = 0;  /* If server is not running just return an empty stream. */
    }
    
    if (!err) {
        *out_reply_stream = cinfo->reply_stream;
        cinfo->reply_stream = NULL;
    }
 
    if (reply_port != MACH_PORT_NULL) { 
        mach_port_destroy (mach_task_self (), reply_port); 
    }
    if (ool_request_length) { 
        vm_deallocate (mach_task_self (), 
                       (vm_address_t) ool_request, ool_request_length); 
    }
    if (cinfo && cinfo->reply_stream) { 
        k5_ipc_stream_release (cinfo->reply_stream); 
        cinfo->reply_stream = NULL;
    }
    
    return err;    
}

#endif /* LEAN CLIENT */
