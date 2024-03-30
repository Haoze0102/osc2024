#include "exec.h"

void exec(char* file_data, uint32_t data_size) {
    asm volatile(
        "msr spsr_el1, %0\n\t"
        "msr elr_el1, %1\n\t"
        "eret\n\t" ::"r"(0x3c0),
        "r"(file_data));

    /*
    mov     x1, (1 << 31) // EL1 uses aarch64
    msr     hcr_el2, x1
    mov     x1, 0x3c5
    msr     spsr_el2, x1
    ldr     x1, =el1_start // return to `el1_start` when goto EL1
    msr     elr_el2, x1
    eret
*/
}