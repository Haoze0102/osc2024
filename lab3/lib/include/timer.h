#ifndef __TIMER_H__
#define __TIMER_H__

#include "list.h"
#include "malloc.h"
#include "printf.h"
#include "regs.h"
#include "stdint.h"
#include "string.h"

#define CORE0_TIMER_IRQ_CTRL 0x40000040

typedef struct timer_event {
    struct list_head node;
    unsigned long long tval;  //store as tick time after cpu start
    void *callback; // interrupt -> timer_callback -> callback(args)
    char* args; // need to free the string by event callback function
} timer_event_t;

void enable_core_timer();
void core_timer_handler();
void timer_list_init();
uint64_t get_absolute_time(uint64_t offset);
void add_timer(void* callback, char* args, uint64_t timeout);
void show_timer_list();
void sleep(uint64_t timeout);

void show_msg_callback(char* args);
void show_time_callback(char* args);

void set_relative_timeout(uint64_t timeout);  // relative -> cntp_tval_el0
void set_absolute_timeout(uint64_t timeout);  // absoulute -> cntp_cval_el0

#endif