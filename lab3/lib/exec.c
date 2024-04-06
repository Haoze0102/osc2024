#include "exec.h"

void exec(char* file_data, uint32_t data_size) {
    char* ustack = malloc(USTACK_SIZE);

    asm volatile(
        "msr spsr_el1, %0\n\t"  // M[4:0] = 0 -> User, 0 -> to enable interrupt in EL0
        "msr elr_el1, %1\n\t"   // return to start of file
        "msr sp_el0, %2\n\t"    // stack for EL0
        "eret\n\t" ::"r"(0),
        "r"(file_data),
        "r"(ustack + USTACK_SIZE));

    free();
}