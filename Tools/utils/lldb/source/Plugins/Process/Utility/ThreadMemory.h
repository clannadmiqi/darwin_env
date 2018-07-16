//===-- ThreadMemory.h -----------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef liblldb_ThreadMemory_h_
#define liblldb_ThreadMemory_h_

#include "lldb/Target/Thread.h"

class ThreadMemory :
    public lldb_private::Thread
{
public:

    ThreadMemory (lldb_private::Process &process, 
                   lldb::tid_t tid,
                   const lldb::ValueObjectSP &thread_info_valobj_sp);

    virtual 
    ~ThreadMemory();

    //------------------------------------------------------------------
    // lldb_private::Thread methods
    //------------------------------------------------------------------
    virtual void
    RefreshStateAfterStop();

    virtual lldb::RegisterContextSP
    GetRegisterContext ();

    virtual lldb::RegisterContextSP
    CreateRegisterContextForFrame (lldb_private::StackFrame *frame);

    virtual lldb::StopInfoSP
    GetPrivateStopReason ();

    virtual bool
    WillResume (lldb::StateType resume_state);

    lldb::ValueObjectSP &
    GetValueObject ()
    {
        return m_thread_info_valobj_sp;
    }

protected:
    //------------------------------------------------------------------
    // For ThreadMemory and subclasses
    //------------------------------------------------------------------
    lldb::ValueObjectSP m_thread_info_valobj_sp;
    
private:
    //------------------------------------------------------------------
    // For ThreadMemory only
    //------------------------------------------------------------------
    DISALLOW_COPY_AND_ASSIGN (ThreadMemory);
};

#endif  // liblldb_ThreadMemory_h_
