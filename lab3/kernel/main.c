// #include "shell.h"
#include "dtb.h"
#include "stdint.h"
// #include "uart.h"

extern void *_dtb_ptr;
void kernel_main(char* x0) {
    dtb_init(_dtb_ptr);
    shell();
}