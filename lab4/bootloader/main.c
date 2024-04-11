#include "stdint.h"
#include "string.h"

extern char _kernel_start[];  // 0x80000
char buf[0x100];
uint32_t buf_idx;
char* _dtb;

void load_kernel() {
    uint32_t size;
    uart_read(&size, 4);  // read 4 byte magic code

    uart_write_string("[+] Loading kernel.img from UART\r\n");  //

    // read `kernel.img` size
    uart_read(&size, 4);
    uart_write_string("[+] Get kernel.img size: 0x");
    uart_puth(size);
    uart_write_string("\r\n");

    // read `kernel.img`
    uart_read(_kernel_start, size);

    // finish!
    uart_write_string("[+] Load kernel.img successfully!\r\n");
}

void read_cmd() {
    char tmp;
    uart_write_string("# ");
    for (buf_idx = 0; uart_read(&tmp, 1);) {
        switch (tmp) {
            case '\r':
            case '\n':
                buf[buf_idx++] = '\0';
                uart_write_string(ENDL);
                return;
            case 127:  // Backspace
                if (buf_idx > 0) {
                    buf_idx--;
                    buf[buf_idx] = '\0';
                    uart_write_string("\b \b");
                }
                break;
            default:
                buf[buf_idx++] = tmp;
                _putchar(tmp);
                break;
        }
    }
}

void exec_cmd() {
    if (!strlen(buf)) return;
    if (!strncmp(buf, "help", 4)) {
        uart_write_string("load" ENDL);
    } else if (!strncmp(buf, "load", 4)) {
        ((void (*)(char*))_kernel_start)(_dtb);
        return;
    }
}

void bootloader_main(char* x0) {
    _dtb = x0;  // store x0
    uart_init();
    load_kernel();
    ((void (*)())_kernel_start)();
    ((void (*)(char*))_kernel_start)(_dtb);

    while (1) {
        read_cmd();
        exec_cmd();
    }
}