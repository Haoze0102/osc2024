#include "timer.h"

struct list_head* timer_event_list;

void enable_core_timer() {
    asm volatile(
        "mov    x0, 1\n\t"
        "msr    cntp_ctl_el0, x0\n\t");

    *(uint32_t*)CORE0_TIMER_IRQ_CTRL = 2;  // enable rip3 timer interrupt

    struct list_head* curr;
}

void disable_core_timer() {
    *(uint32_t*)CORE0_TIMER_IRQ_CTRL = 0;  // disable rip3 timer interrupt
}

void core_timer_handler() {
    disable_core_timer();  // [ Lab3 - AD2 ] 1. masks the device’s interrupt line,
    add_task(core_timer_callback, PRIORITY_NORMAL);
}

void core_timer_callback() {
    // return;
    // if there is no timer event, set a huge expire time
    if (list_empty(timer_event_list)) {
        //printf("[+] timer_event_list is empty" ENDL);
        set_relative_timeout(65535);
        enable_core_timer();
        return;
    }

    // trigger the first callback
    ((void (*)(char*))((timer_event_t*)timer_event_list->next)->callback)(((timer_event_t*)timer_event_list->next)->args);

    // remove the first event
    list_rotate_left(timer_event_list);
    list_del(timer_event_list->prev);
    free();

    // if there is next event, set next timeout
    if (list_empty(timer_event_list)) {
        set_relative_timeout(65535);
    } else {
        set_absolute_timeout(((timer_event_t*)timer_event_list->next)->tval);
    }

    // [ Lab3 - AD2 ] 5. unmasks the interrupt line to get the next interrupt at the end of the task.
    enable_core_timer();
}

void timer_list_init() {
    timer_event_list = malloc(sizeof(struct list_head));
    INIT_LIST_HEAD(timer_event_list);
}

uint64_t get_absolute_time(uint64_t offset) {
    uint64_t cntpct_el0, cntfrq_el0;
    get_reg(cntpct_el0, cntpct_el0);
    get_reg(cntfrq_el0, cntfrq_el0);
    return cntpct_el0 + cntfrq_el0 * offset;
}

void add_timer(void* callback, char* args, uint64_t timeout) {
    timer_event_t* new_timer_event = malloc(sizeof(timer_event_t));
    INIT_LIST_HEAD(&new_timer_event->node);
    new_timer_event->args = malloc(strlen(args) + 1);
    strcpy(new_timer_event->args, args);
    new_timer_event->callback = callback;
    new_timer_event->tval = get_absolute_time(timeout);

    // set first event interrupt
    if (list_empty(timer_event_list)) set_absolute_timeout(new_timer_event->tval);

    // insert node
    struct list_head* curr;
    //bool inserted = false;
    list_for_each(curr, timer_event_list) {
        if (new_timer_event->tval < ((timer_event_t*)curr)->tval) {
            list_add(&new_timer_event->node, curr->prev);
            break;
        }
    }
    // if the timer_event is the biggest, run this code block
    if(list_is_head(curr,timer_event_list))
    {
        // put the biggest to tail
        list_add_tail(&new_timer_event->node,timer_event_list);
    }
    // set interrupt to first event
    set_absolute_timeout(((timer_event_t*)timer_event_list->next)->tval);
}

void show_timer_list() {
    struct list_head* curr;
    bool inserted = false;
    list_for_each(curr, timer_event_list) {
        printf("%u -> ", ((timer_event_t*)curr)->tval);
    }
    printf(ENDL);
}

void sleep(uint64_t timeout) {
    add_timer(NULL, NULL, 2);
}

void show_msg_callback(char* args) {
    // async_printf("[+] show_msg_callback(%s)" ENDL, args);
    printf("[+] show_msg_callback() -> %s" ENDL, args);
}

void show_time_callback(char* args) {
    uint64_t cntpct_el0, cntfrq_el0;
    get_reg(cntpct_el0, cntpct_el0);
    get_reg(cntfrq_el0, cntfrq_el0);
    // async_printf("[+] show_time_callback() -> %02ds" ENDL, cntpct_el0 / cntfrq_el0);
    printf("[+] show_time_callback() -> %02ds" ENDL, cntpct_el0 / cntfrq_el0);

    add_timer(show_time_callback, args, 2);
}

// set timer interrupt time to [expired_time] seconds after now (relatively)
void set_relative_timeout(uint64_t timeout) {  // relative -> cntp_tval_el0
    asm volatile(
        "mrs    x1, cntfrq_el0\n\t"  // cntfrq_el0 -> frequency of the timer
        "mul    x1, x1, %0\n\t"      // cntpct_el0 = cntfrq_el0 * seconds: relative timer to cntfrq_el0
        "msr    cntp_tval_el0, x1\n\t" ::"r"(timeout));  // Set expired time to cntp_tval_el0, which stores time value of EL1 physical timer.
}

void set_absolute_timeout(uint64_t timeout) {  // absoulute -> cntp_cval_el0
    asm volatile(
        "msr    cntp_cval_el0, %0\n\t"
        : "=r"(timeout));
}
