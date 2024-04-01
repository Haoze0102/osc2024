#include "timer.h"

void enable_core_timer() {
    asm volatile(
        "mov    x0, 1\n\t"
        "msr    cntp_ctl_el0, x0\n\t" // ENABLE, bit [0], 0b1 Timer enabled. x0 -> cntp_ctl_el0
        "mrs    x0, cntfrq_el0\n\t"   // cntfrq_el0 -> x0
        "mov    x0, x0, LSL#1\n\t"    // Shift the value in register x0 left by one bit (equivalent to multiplying by 2)
        "msr    cntp_tval_el0, x0\n\t");  // TODO: cntfrq_el0 * 2 ??

    *(uint32_t*)CORE0_TIMER_IRQ_CTRL = 2;  // ENABLE, bit [1], nCNTPNSIRQ IRQ control
    printf("test");
}

void core_timer_handler() {
    asm volatile(
        "mrs    x0, cntfrq_el0\n\t"
        "mov    x0, x0, LSL#1\n\t"
        "msr    cntp_tval_el0, x0\n\t");

    uint64_t cntpct_el0, cntfrq_el0;
    asm volatile("mrs    %0, cntpct_el0\n\t"
                 : "=r"(cntpct_el0)); // The timer’s current count.
    asm volatile("mrs    %0, cntfrq_el0\n\t"
                 : "=r"(cntfrq_el0)); // The timer’s frequant.

    printf("Timer interrupt -> %02ds" ENDL, cntpct_el0 / cntfrq_el0);
    
}