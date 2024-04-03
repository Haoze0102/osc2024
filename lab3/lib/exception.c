#include "exception.h"

void irq_handler() {
    uint64_t c0_int_src = mmio_read(CORE0_IRQ_SRC),
             irq_pend_1 = mmio_read(IRQ_PEND_1);

    if (c0_int_src & SRC_CNTPNSIRQ_INT) { // FIQ control. This bit is for Physical Timer 
        core_timer_handler();
        // async_printf("IRQ_PEND_1: 0x%lx" ENDL, mmio_read(IRQ_PEND_1)); // This register holds ALL interrupts 0..31 from the GPU side.
        // async_printf("CORE0_IRQ_SRC: 0x%lx" ENDL, c0_int_src); //  Core 0 Timers interrupt control
    } else {
        uart_int_handler();
    }
}

void invalid_handler(uint32_t x0) {
    printf("[Invalid Exception] %d" ENDL, x0);
}

void sync_handler() {
    printf("sync_handler()" ENDL);
}

void enable_interrupt() {
    asm volatile("msr DAIFClr, 0xf"); //DAIF, Interrupt Mask Bits
}

void disable_interrupt() {
    asm volatile("msr DAIFSet, 0xf");
}