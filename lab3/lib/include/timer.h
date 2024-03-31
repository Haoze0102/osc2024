#ifndef __TIMER_H__
#define __TIMER_H__

#include "printf.h"
#include "stdint.h"
#include "string.h"

#define CORE0_TIMER_IRQ_CTRL 0x40000040

void enable_core_timer();

void core_timer_handler();

#endif