#ifndef _TIMER_H_
#define _TIMER_H_

#include "list.h"
//https://github.com/Tekki/raspberrypi-documentation/blob/master/hardware/raspberrypi/bcm2836/QA7_rev3.4.pdf p13
#define CORE0_TIMER_IRQ_CTRL PHYS_TO_VIRT(0x40000040)

void core_timer_enable();
void core_timer_disable();
void core_timer_handler();

typedef struct timer_event {
    struct list_head listhead;
    unsigned long long interrupt_time;  //store as tick time after cpu start
    void *callback; // interrupt -> timer_callback -> callback(args)
    char* args; // need to free the string by event callback function
} timer_event_t;

void               add_timer(void *callback, unsigned long long timeout, char *args, int bytick);
unsigned long long get_tick_plus_s(unsigned long long second);
void               set_core_timer_interrupt(unsigned long long expired_time);
void               set_core_timer_interrupt_by_tick(unsigned long long tick);
void               timer_list_init();
int                timer_list_get_size();


#endif /* _TIMER_H_ */
