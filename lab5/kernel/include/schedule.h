#ifndef _SCHED_H_
#define _SCHED_H_

#include "u_list.h"

#define PIDMAX 32768 // RPi3B pid_max: default: 32768 minimum: 301
#define USTACK_SIZE 0x10000 // User stack size
#define KSTACK_SIZE 0x10000 // Kernel stack size
#define SIGNAL_MAX  64 // number of sugnal can be use

extern void  switch_to(void *curr_context, void *next_context);
extern void* get_current();
extern void  store_context(void *curr_context);
extern void  load_context(void *curr_context);

typedef struct thread_context
{
    /* X19-X29 Callee-saved registers (X19-X29) */
    /* X29 is the frame pointer register (FP).
       X30 is the link register (LR). */
    unsigned long x19;
    unsigned long x20;
    unsigned long x21;
    unsigned long x22;
    unsigned long x23;
    unsigned long x24;
    unsigned long x25;
    unsigned long x26;
    unsigned long x27;
    unsigned long x28;
    unsigned long fp;
    unsigned long lr;
    unsigned long sp;
} thread_context_t;

/* https://zhuanlan.zhihu.com/p/473736908 */
typedef struct thread
{
    list_head_t      listhead;
    thread_context_t context;
    char*            data;
    unsigned int     datasize;
    int              iszombie;
    int              pid;
    int              isused;
    char*            stack_alloced_ptr;
    char*            kernel_stack_alloced_ptr;
    void             (*signal_handler[SIGNAL_MAX+1])();
    int              sigcount[SIGNAL_MAX+1];
    void             (*curr_signal_handler)();
    int              signal_inProcess;
    thread_context_t signal_savedContext;
} thread_t;

void schedule_timer(char *notuse);
void init_thread_sched();
void idle();
void schedule();
void kill_zombies();
void thread_exit();
thread_t *thread_create(void *start);
int exec_thread(char *data, unsigned int filesize);

void foo();

#endif /* _SCHED_H_ */