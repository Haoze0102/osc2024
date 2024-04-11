#ifndef __EXCEPTIONS_H__
#define __EXCEPTIONS_H__

#include "mmio.h"
#include "printf.h"
#include "task.h"
#include "timer.h"
#include "uart.h"

#define ARM_INT_REG_BASE (0x3f00b000)  // ARM interrupt register base
#define IRQ_BASIC_PEND (ARM_INT_REG_BASE + 0x200) //The basic pending register shows which interrupt are pending.
#define IRQ_PEND_1 (ARM_INT_REG_BASE + 0x204)  //This register holds ALL interrupts 0..31 from the GPU side.
#define IRQ_PEND_2 (ARM_INT_REG_BASE + 0x208)  //This register holds ALL interrupts 32..63 from the GPU side.
#define FIQ_CTRL (ARM_INT_REG_BASE + 0x20c)
#define IRQ_ENABLE_1 (ARM_INT_REG_BASE + 0x210)  // set [29] to 1 to enable AUX interrupt
#define IRQ_ENABLE_2 (ARM_INT_REG_BASE + 0x214)
#define IRQ_ENABLE_BASIC (ARM_INT_REG_BASE + 0x218)
#define IRQ_DISABLE_1 (ARM_INT_REG_BASE + 0x21c)
#define IRQ_DISABLE_2 (ARM_INT_REG_BASE + 0x220)
#define IRQ_DISABLE_BASIC (ARM_INT_REG_BASE + 0x224)

// IRQ & FIQ source registers
#define CORE0_IRQ_SRC 0x40000060
#define CORE1_IRQ_SRC 0x40000064
#define CORE2_IRQ_SRC 0x40000068
#define CORE3_IRQ_SRC 0x4000006C
#define CORE0_FIQ_SRC 0x40000070
#define CORE1_FIQ_SRC 0x40000074
#define CORE2_FIQ_SRC 0x40000078
#define CORE3_FIQ_SRC 0x4000007C

#define IRQ_PEND_AUX_INT (1 << 29)  // Mini UART

#define SRC_CNTPSIRQ_INT (1 << 0)
#define SRC_CNTPNSIRQ_INT (1 << 1)
#define SRC_CNTHPIRQ_INT (1 << 2)
#define SRC_CNTVIRQ_INT (1 << 3)
#define SRC_MBOX0_INT (1 << 4)
#define SRC_MBOX1_INT (1 << 5)
#define SRC_MBOX2_INT (1 << 6)
#define SRC_MBOX3_INT (1 << 7)
#define SRC_GPU_INT (1 << 8)
#define SRC_PMU_INT (1 << 9)
#define SRC_AXI_OUTSTANDING_INT (1 << 10)
#define SRC_LOCAL_TIME_INT (1 << 11)

void irq_handler();
void invalid_handler();
void sync_handler();
void enable_interrupt();
void disable_interrupt();

#endif