#include "exception.h"

void irq_handler() {
    uint64_t c0_int_src = mmio_read(CORE0_IRQ_SRC),
             irq_pend_1 = mmio_read(IRQ_PEND_1);
    // async_printf("IRQ_PEND_1: 0x%lX" ENDL, mmio_read(IRQ_PEND_1));
    // async_printf("CORE0_IRQ_SRC: 0x%lX" ENDL, c0_int_src);

    // [ Lab3 - AD2 ] 2. move data from the device’s buffer through DMA, or manually copy,
    // TODO: necessary???
    // printf("[START] irq_handler()" ENDL);
    // [ Lab3 - AD2 ] 3. enqueues the processing task to the event queue,
    // classify the interrupt type, and call add_task()
    if (c0_int_src & SRC_CNTPNSIRQ_INT) {
        core_timer_handler();
    } else {  // TODO: better condition
        uart_int_handler();
    }

    // [ Lab3 - AD2 ] 4. do the tasks with interrupts enabled,
    run_task();
    // printf("[END] irq_handler()" ENDL);
}

void invalid_handler(uint32_t x0) {
    printf("[Invalid Exception] %d" ENDL, x0);
    while (1)
        ;
}

void sync_handler() {  // handle svc0
    printf("sync_handler()" ENDL);
    uint64_t spsr_el1, elr_el1, esr_el1;
    get_reg(spsr_el1, spsr_el1);
    get_reg(elr_el1, elr_el1);
    get_reg(esr_el1, esr_el1);

    printf("0x%lX, 0x%lX, 0x%lX\r\n", spsr_el1, elr_el1, esr_el1);
}

// DAIF, Interrupt Mask Bits on EL1
void enable_interrupt() {
    asm volatile("msr DAIFClr, 0xf");
}

void disable_interrupt() {
    asm volatile("msr DAIFSet, 0xf");
}