#include "shell.h"

char buf[0x100];

void welcome_msg() {
    printf(
        ENDL
        "   ____    _____   _____   ___    ___   ___   _  _" ENDL
        "  / __ \\  / ____| / ____| |__ \\  / _ \\ |__ \\ | || |" ENDL
        " | |  | || (___  | |         ) || | | |   ) || || |_" ENDL
        " | |  | | \\___ \\ | |        / / | | | |  / / |__   _|" ENDL
        " | |__| | ____) || |____   / /_ | |_| | / /_    | | "ENDL
        "  \\____/ |_____/  \\_____| |____| \\___/ |____|   |_|" ENDL);
}

void read_cmd() {
    char tmp;
    printf("# ");
    for (uint32_t i = 0; uart_read(&tmp, 1);) {
        _putchar(tmp);
        switch (tmp) {
            case '\r':
            case '\n':
                buf[i++] = '\0';
                return;
            case 127:  // Backspace
                if (i > 0) {
                    i--;
                    buf[i] = '\0';
                    printf("\b \b");
                }
                break;
            default:
                buf[i++] = tmp;
                break;
        }
    }
}

void exec_cmd() {
    if (!strlen(buf)) return;
    for (uint32_t i = 0; i < sizeof(func_list) / sizeof(struct func); i++) {
        if (!strncmp(buf, func_list[i].name, strlen(func_list[i].name) - 1)) {
            char* param = buf + strlen(func_list[i].name);
            while (*param != '\n' && *param == ' ') param++;
            func_list[i].ptr(param);
            return;
        }
    }
    cmd_unknown();
}

void cmd_help(char* param) {
    for (uint32_t i = 0; i < sizeof(func_list) / sizeof(struct func); i++) {
        printf(func_list[i].name);
        for (uint32_t j = 0; j < (10 - strlen(func_list[i].name)); j++) _putchar(' ');
        printf(": %s" ENDL, func_list[i].desc);
    }
}

void cmd_hello(char* param) {
    printf("Hello World!" ENDL);
}



void cmd_sysinfo(char* param) {
    uint32_t *board_revision;
    uint32_t *board_serial_msb, *board_serial_lsb;
    uint32_t *mem_base, *mem_size;
    const int padding = 20;

    // Board Revision
    get_board_revision(board_revision);
    printf("Board Revision      : 0x%08lX" ENDL, *board_revision);

    
    // Memory Info
    get_memory_info(mem_base, mem_size);
    printf("Memroy Base Address : 0x%08lX" ENDL, *mem_base);
    printf("Memory Size         : 0x%08lX" ENDL, *mem_size);
}

void cmd_reboot(char* param) {
    uart_write_string("Rebooting...."ENDL);
    reset(10);
}

void cmd_ls(char* param) {
    cpio_newc_parser(cpio_ls_callback, param);
}

void cmd_cat(char* param) {
    cpio_newc_parser(cpio_cat_callback, param);
}

void cmd_dtb(char* param) {
    dtb_parser(dtb_show_callback);
}

void cmd_exec(char* param) {
    cpio_newc_parser(cpio_exec_callback, param);
}

void cmd_unknown() {
    printf("Unknown command: %s" ENDL, buf);
}

void shell() {
    cpio_init();
    welcome_msg();
    do {
        read_cmd();

        printf("# %s" ENDL, buf);

        exec_cmd();
    } while (1);
}