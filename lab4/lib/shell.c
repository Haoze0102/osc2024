#include "shell.h"

char buf[0x100];
uint32_t buf_idx;

void welcome_msg() {
    async_printf(ENDL);
    async_printf("   ____    _____   _____   ___    ___   ___   _  _" ENDL);
    async_printf("  / __ \\  / ____| / ____| |__ \\  / _ \\ |__ \\ | || |" ENDL);
    async_printf(" | |  | || (___  | |         ) || | | |   ) || || |_" ENDL);
    async_printf(" | |  | | \\___ \\ | |        / / | | | |  / / |__   _|" ENDL);
    async_printf(" | |__| | ____) || |____   / /_ | |_| | / /_    | | "ENDL);
    async_printf("  \\____/ |_____/  \\_____| |____| \\___/ |____|   |_|" ENDL);
}


void read_cmd() {
    char tmp;
    async_printf("# ");
    for (buf_idx = 0;;) {
        async_uart_read(&tmp, 1);
        // _async_putchar(tmp);
        switch (tmp) {
            case '\r':
            case '\n':
                buf[buf_idx++] = '\0';
                printf(ENDL);
                return;
            case 127:  // Backspace
                if (buf_idx > 0) {
                    buf_idx--;
                    buf[buf_idx] = '\0';
                    printf("\b \b");
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
    parse_arg();
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

void parse_arg() {
    // TODO: remove extra spaces
}

uint64_t get_arg(uint64_t i) {
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

void cmd_reboot(char* param) {
    reset(10);
}

void cmd_sysinfo(char* param) {
    uint32_t* board_revision;
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

void cmd_timer(char* param) {
    add_timer(show_time_callback, param, 2);
}

void cmd_setTimeout(char* param) {
    // TODO: better parameter handling
    char *arg1 = param,
         *arg2 = strchr(param, ' ') + 1;
    *strchr(param, ' ') = '\0';
    add_timer(show_msg_callback, arg1, atoi(arg2));
}

void cmd_unknown() {
    async_printf("Unknown command: %s" ENDL, buf);
}

void shell() {
    cpio_init();
    enable_core_timer();
    welcome_msg();
    do {
        read_cmd();
        exec_cmd();
    } while (1);
}