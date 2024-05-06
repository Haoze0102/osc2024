#include "schedule.h"
#include "uart1.h"
#include "exception.h"
#include "memory.h"
#include "timer.h"
#include "signal.h"

list_head_t *run_queue;

thread_t threads[PIDMAX + 1];
thread_t *curr_thread;

int pid_history = 0;

void init_thread_sched()
{
    // init thread freelist and run_queue
    run_queue = kmalloc(sizeof(list_head_t));
    INIT_LIST_HEAD(run_queue);

    //init pids
    for (int i = 0; i <= PIDMAX; i++)
    {
        threads[i].isused = 0;
        threads[i].pid = i;
        threads[i].iszombie = 0;
    }

    /* https://stackoverflow.com/questions/64856566/what-is-the-purpose-of-thread-id-registers-like-tpidr-el0-tpidr-el1-in-arm */
    asm volatile("msr tpidr_el1, %0" ::"r"(kmalloc(sizeof(thread_t)))); /// set tpidr_el1 to thread pointer

    thread_t* idlethread = thread_create(idle);
    curr_thread = idlethread;
}

void idle(){
    while(1)
    {
        kill_zombies();   //reclaim threads marked as DEAD
        schedule();       //switch to next thread in run queue
    }
}

void schedule(){
    //el1_interrupt_disable();
    //uart_sendline("From pid-%d to ", curr_thread->pid);
    //curr_thread = (thread_t*)curr_thread->listhead.next;
    // ignore run_queue head
    //if(list_is_head(&curr_thread->listhead,run_queue))
    do{
        curr_thread = (thread_t *)curr_thread->listhead.next;
    } while (list_is_head(&curr_thread->listhead, run_queue)); // find a runnable thread
    switch_to(get_current(), &curr_thread->context);
}

void kill_zombies(){
    list_head_t *curr;
    list_for_each(curr,run_queue)
    {
        if (((thread_t *)curr)->iszombie)
        {
            list_del_entry(curr);
            kfree(((thread_t *)curr)->stack_alloced_ptr);        // free stack
            kfree(((thread_t *)curr)->kernel_stack_alloced_ptr); // free stack
            //kfree(((thread_t *)curr)->data);                   // Don't free data because children may use data
            ((thread_t *)curr)->iszombie = 0;
            ((thread_t *)curr)->isused = 0;
        }
    }
}

int exec_thread(char *data, unsigned int filesize)
{
    thread_t *t = thread_create(data);
    t->data = kmalloc(filesize);
    t->datasize = filesize;
    t->context.lr = (unsigned long)t->data; // set return address to program if function call completes
    //copy file into data
    for (int i = 0; i < filesize;i++)
    {
        t->data[i] = data[i];
    }

    curr_thread = t;
    add_timer(schedule_timer, 1, "", 0);

    // eret to exception level 0
    asm("msr tpidr_el1, %0\n\t" // Hold the "kernel(el1)" thread structure information
        "msr elr_el1, %1\n\t"   // When el0 -> el1, store return address for el1 -> el0
        "msr spsr_el1, xzr\n\t" // Enable interrupt in EL0 -> Used for thread scheduler
        "msr sp_el0, %2\n\t"    // el0 stack pointer for el1 process
        "mov sp, %3\n\t"        // sp is reference for the same el process. For example, el2 cannot use sp_el2, it has to use sp to find its own stack.
        "eret\n\t" ::"r"(&t->context),"r"(t->context.lr), "r"(t->context.sp), "r"(t->kernel_stack_alloced_ptr + KSTACK_SIZE));

    return 0;
}

thread_t *thread_create(void *start)
{
    thread_t *r;
    // find usable PID, don't use the previous one
    if( pid_history > PIDMAX ) return 0;
    if (!threads[pid_history].isused){
        r = &threads[pid_history];
        pid_history += 1;
    }
    else return 0;
    //uart_sendline("Thread create : %d\n", r->pid);
    r->iszombie = 0; // Initialize to indicate the thread is not a zombie.
    r->isused = 1; // Mark the thread as used.
    r->context.lr = (unsigned long long)start; // Set the link register to the start function, defining the entry point for this thread.

    // Allocate stack space for this thread. `USTACK_SIZE` is the size of the stack.
    r->stack_alloced_ptr = kmalloc(USTACK_SIZE);
    r->kernel_stack_alloced_ptr = kmalloc(KSTACK_SIZE);
    // The stack pointer should point to the top of the stack, so we add `USTACK_SIZE` to the base pointer.
    r->context.sp = (unsigned long long)r->stack_alloced_ptr + USTACK_SIZE;
    // Set the frame pointer (fp) to the top of the stack. This is typically used for stack frame linkage.
    r->context.fp = r->context.sp;

    // Advanced 1 POSIX signal
    r->signal_inProcess = 0;

    //initial all signal handler with signal_default_handler (kill thread)
    for (int i = 0; i < SIGNAL_MAX;i++)
    {
        r->signal_handler[i] = signal_default_handler;
        r->sigcount[i] = 0;
    }
    list_add(&r->listhead, run_queue);

    return r;
}

void thread_exit(){
    // thread cannot deallocate the stack while still using it, wait for someone to recycle it.
    // In this lab, idle thread handles this task, instead of parent thread.
    curr_thread->iszombie = 1;
    schedule();
}

void schedule_timer(char* notuse){
    unsigned long long cntfrq_el0;
    __asm__ __volatile__("mrs %0, cntfrq_el0\n\t": "=r"(cntfrq_el0)); //tick frequency
    add_timer(schedule_timer, cntfrq_el0 >> 5, "", 1);
    // 32 * default timer -> trigger next schedule timer
}

void foo(){
    // Lab5 Basic 1 Test function
    for (int i = 0; i < 10; ++i)
    {
        uart_sendline("Thread id: %d %d\n", curr_thread->pid, i);
        int r = 1000000;
        while (r--) { asm volatile("nop"); }
        schedule();
    }
    thread_exit();
}