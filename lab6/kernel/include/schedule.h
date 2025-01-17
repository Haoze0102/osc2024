#ifndef _SCHED_H_
#define _SCHED_H_

#include "list.h"

#define PIDMAX      32768 // RPi3B pid_max: default: 32768 minimum: 301
#define USTACK_SIZE 0x4000 // User stack size
#define KSTACK_SIZE 0x4000 // Kernel stack size
#define SIGNAL_MAX  64 // number of sugnal can be use

extern void switch_to(void *curr_context, void *next_context);
extern void store_context(void *curr_context);
extern void load_context(void *curr_context);
extern void *get_current();

// arch/arm64/include/asm/processor.h - cpu_context
typedef struct thread_context
{
    /* X19-X29 Callee-saved registers (X19-X29)
       X29 is the frame pointer register (FP).
       X30 is the link register (LR). 
       callee saved registers: the called function will preserve them and restore them before returning*/
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
    unsigned long fp; // base pointer for local variable in stack
    unsigned long lr; // store return address
    unsigned long sp; // stack pointer, varys from function calls
    void* pgd;   // use for MMU mapping (user space)
} thread_context_t;

/* https://zhuanlan.zhihu.com/p/473736908 */
typedef struct thread
{
    list_head_t      listhead;                            // Freelist node
    thread_context_t context;                             // Thread registers
    char*            data;                                // Process itself
    unsigned int     datasize;                            // Process size
    int              iszombie;                            // Process statement
    int              pid;                                 // Process ID
    int              isused;                              // Freelist node statement
    char*            stack_alloced_ptr;                   // Process Stack (Process itself)
    char*            kernel_stack_alloced_ptr;            // Process Stack (Kernel syscall)
    void             (*signal_handler[SIGNAL_MAX+1])();   // Signal handlers for different signal
    int              sigcount[SIGNAL_MAX+1];              // Signal Pending buffer
    void             (*curr_signal_handler)();            // Allow Signal handler overwritten by others
    int              signal_is_checking;                    // Signal Processing Lock
    thread_context_t signal_saved_context;                 // Store registers before signal handler involving
} thread_t;

extern thread_t    *curr_thread;
extern list_head_t *run_queue;
extern list_head_t *wait_queue;
extern thread_t    threads[PIDMAX + 1];

void schedule_timer(char *notuse);
void init_thread_sched();
void idle();
void schedule();
void kill_zombies();
void thread_exit();
thread_t *thread_create(void *start, unsigned int filesize);
int exec_thread(char *data, unsigned int filesize);


#endif /* _SCHED_H_ */