#include "signal.h"
#include "syscall.h"
#include "schedule.h"
#include "memory.h"
#include "mmu.h"

extern thread_t *curr_thread;

void check_signal(trapframe_t *tpf)
{
    lock();
    if(curr_thread->signal_is_checking)
    {
        unlock();
        return;
    }
    //prevent nested running signal handler
    curr_thread->signal_is_checking = 1;
    unlock();
    for (int i = 0; i <= SIGNAL_MAX; i++)
    {
        // before running the handler, you should save the original context
        store_context(&curr_thread->signal_saved_context);
        if(curr_thread->sigcount[i]>0)
        {
            uart_sendline("catch!!!\n");
            lock();
            curr_thread->sigcount[i]--;
            unlock();
            run_signal(tpf,i);
        }
    }
    lock();
    curr_thread->signal_is_checking = 0;
    unlock();
}

void run_signal(trapframe_t* tpf,int signal)
{
    uart_sendline("run_signal\n");
    curr_thread->curr_signal_handler = curr_thread->signal_handler[signal];

    //run default handler in kernel
    if (curr_thread->curr_signal_handler == signal_default_handler)
    {
        signal_default_handler();
        return;
    }
    uart_sendline("USER_SIGNAL_WRAPPER_VA:%x\n",USER_SIGNAL_WRAPPER_VA);
    uart_sendline("offset:%x\n",(size_t)signal_handler_wrapper % 0x1000);
    uart_sendline("offset:%x\n",USER_SIGNAL_WRAPPER_VA + ((size_t)signal_handler_wrapper % 0x1000));
    uart_sendline("signal_handler_wrapper:%x\n",&signal_handler_wrapper);
    //on the other hand, other handler should be run in user mode
    //during execution, the handler requires a user stack. 
    //The kernel should allocate a new stack for the handler and then recycle it after it completes
    asm("msr elr_el1, %0\n\t"
        "msr sp_el0, %1\n\t"
        "msr spsr_el1, %2\n\t"
        "mov x0, %3\n\t" // store handler in x0
        "eret\n\t"
        :: "r"(USER_SIGNAL_WRAPPER_VA),
           "r"(tpf->sp_el0),
           "r"(tpf->spsr_el1),
           "r"(curr_thread->curr_signal_handler));
}

void signal_handler_wrapper()
{
    // here to exec other handler
    // but still in EL0
    int i = 0;
    uart_sendline("user:%x\n", &i);
    //elr_el1 set to function -> call function by x0
    //system call sigreturn, back to EL1
    uart_sendline("Other handler\n");
    asm("blr x0\n\t"
        "mov x8,50\n\t"
        "svc 0\n\t");
}

void signal_default_handler()
{
    uart_sendline("signal_default_handler\n");
    kill(0,curr_thread->pid);
}