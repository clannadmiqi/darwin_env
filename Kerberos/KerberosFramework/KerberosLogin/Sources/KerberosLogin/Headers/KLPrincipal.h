/*
 * KLPrincipal.h
 *
 * $Header: /cvs/kfm/KerberosFramework/KerberosLogin/Sources/KerberosLogin/Headers/KLPrincipal.h,v 1.2 2003/09/02 15:03:39 lxs Exp $
 *
 * Copyright 2003 Massachusetts Institute of Technology.
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

KLStatus __KLCreatePrincipalFromTriplet (const char  *inName,
                                         const char  *inInstance,
                                         const char  *inRealm,
                                         KLKerberosVersion  inKerberosVersion,
                                         KLPrincipal *outPrincipal);

KLStatus __KLGetTripletFromPrincipal (KLPrincipal         inPrincipal,
                                      KLKerberosVersion   inKerberosVersion,
                                      char              **outName,
                                      char              **outInstance,
                                      char              **outRealm);

KLStatus __KLCreatePrincipalFromKerberos5Principal (krb5_principal inPrincipal,
                                                    KLPrincipal *outPrincipal);

krb5_principal __KLGetKerberos5PrincipalFromPrincipal (KLPrincipal inPrincipal);

KLStatus __KLGetRealmFromPrincipal (KLPrincipal inPrincipal, KLKerberosVersion inKerberosVersion, char **outRealm);

KLBoolean __KLPrincipalIsTicketGrantingService (KLPrincipal inPrincipal, KLKerberosVersion inVersion);
