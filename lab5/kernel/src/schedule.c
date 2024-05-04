#include "schedule.h"
#include "uart1.h"
#include "exception.h"
#include "memory.h"
#include "timer.h"

list_head_t *run_queue;
list_head_t *wait_queue;
list_head_t *zombie_queue;

thread_t threads[PIDMAX + 1];
thread_t *curr_thread;

int pid_history = 0;

void init_thread_sched()
{
    //el1_interrupt_disable();
    run_queue = kmalloc(sizeof(list_head_t));
    wait_queue = kmalloc(sizeof(list_head_t));
    zombie_queue = kmalloc(sizeof(list_head_t));
    INIT_LIST_HEAD(run_queue);
    INIT_LIST_HEAD(wait_queue);
    INIT_LIST_HEAD(zombie_queue);

    //init pids
    for (int i = 0; i <= PIDMAX; i++)
    {
        threads[i].isused = 0;
        threads[i].pid = i;
        threads[i].iszombie = 0;
    }

    /* https://stackoverflow.com/questions/64856566/what-is-the-purpose-of-thread-id-registers-like-tpidr-el0-tpidr-el1-in-arm */
    asm volatile("msr tpidr_el1, %0" ::"r"(kmalloc(sizeof(thread_t)))); /// malloc a space for current kernel thread to prevent crash

    thread_t* idlethread = thread_create(idle);
    curr_thread = idlethread;
    //el1_interrupt_enable();
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
    }while (list_is_head(&curr_thread->listhead, run_queue) || curr_thread->iszombie);
    switch_to(get_current(), &curr_thread->context);
    //el1_interrupt_enable();
}

void kill_zombies(){
    //el1_interrupt_disable();
    //while(!list_empty(zombie_queue))
    list_head_t *curr;
    list_for_each(curr,run_queue)
    {
        if (((thread_t *)curr)->iszombie)
        {
            list_head_t *prev_curr = curr->prev;
            list_del_entry(curr);
            kfree(((thread_t *)curr)->stack_alloced_ptr);        // free stack
            kfree(((thread_t *)curr)->kernel_stack_alloced_ptr); // free stack
            //kfree(((thread_t *)curr)->data); // free data (don't free data because of fork)
            ((thread_t *)curr)->iszombie = 0;
            ((thread_t *)curr)->isused = 0;
            curr = prev_curr;
        }
    }
    //el1_interrupt_enable();
}

int exec_thread(char *data, unsigned int filesize)
{
    thread_t *t = thread_create(data);
    t->data = kmalloc(filesize);
    t->datasize = filesize;
    t->context.lr = (unsigned long)t->data;
    //copy file into data
    for (int i = 0; i < filesize;i++)
    {
        t->data[i] = data[i];
    }

    curr_thread = t;
    add_timer(schedule_timer, 1, "", 0);

    // eret to exception level 0
    asm("msr tpidr_el1, %0\n\t" // &t->context
        "msr elr_el1, %1\n\t"   // t->context.lr
        "msr spsr_el1, xzr\n\t" // enable interrupt in EL0. You can do it by setting spsr_el1 to 0 before returning to EL0.
        "msr sp_el0, %2\n\t"    // t->context.sp
        "mov sp, %3\n\t"        // t->kernel_stack_alloced_ptr + KSTACK_SIZE
        "eret\n\t" ::"r"(&t->context),"r"(t->context.lr), "r"(t->context.sp), "r"(t->kernel_stack_alloced_ptr + KSTACK_SIZE));

    return 0;
}

thread_t *thread_create(void *start)
{
    //el1_interrupt_disable();
    thread_t *r;
    for (int i = pid_history; i <= PIDMAX; i++)
    {
        if (!threads[i].isused)
        {
            r = &threads[i];
            pid_history = i;
            break;
        }
    }
    uart_sendline("Thread create : %d\n", r->pid);
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

    list_add(&r->listhead, run_queue);
    //el1_interrupt_enable();
    return r;
}

void thread_exit(){
    uart_sendline("Thread PID %d exit \n", curr_thread->pid);
    // el1_interrupt_disable();
    // list_del_entry(&curr_thread->listhead);
    curr_thread->iszombie = 1;
    // list_add(&curr_thread->listhead, zombie_queue);
    schedule();
    //el1_interrupt_enable();
}

void schedule_timer(char* notuse){
    unsigned long long cntfrq_el0;
    __asm__ __volatile__("mrs %0, cntfrq_el0\n\t": "=r"(cntfrq_el0)); //tick frequency
    add_timer(schedule_timer, cntfrq_el0 >> 5, "", 1);
}

void foo(){
    for (int i = 0; i < 10; ++i)
    {
        uart_sendline("Thread id: %d %d\n", curr_thread->pid, i);
        int r = 1000000;
        while (r--) { asm volatile("nop"); }
        schedule();
    }
    thread_exit();
}