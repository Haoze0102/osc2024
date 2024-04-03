#ifndef __SHELL_H__
#define __SHELL_H__
#include <stdint.h>

#include "cpio.h"
#include "dtb.h"
#include "mbox.h"
#include "printf.h"
#include "reset.h"
#include "string.h"
#include "timer.h"
// #include "uart.h"

struct func {
    char* name;
    void (*ptr)();
    char* desc;
};

void welcome_msg();
void shell();
void read_cmd();
void exec_cmd();

void cmd_help(char* param);
void cmd_hello(char* param);
void cmd_reboot(char* param);
void cmd_sysinfo(char* param);
void cmd_ls(char* param);
void cmd_cat(char* param);
void cmd_dtb(char* param);
void cmd_exec(char* param);
void cmd_timer(char* param);
void cmd_unknown();

struct func func_list[] = {
    {.name = "help", .ptr = cmd_help, .desc = "print this help menu"},
    {.name = "hello", .ptr = cmd_hello, .desc = "print Hello World!"},
    {.name = "reboot", .ptr = cmd_reboot, .desc = "reboot the device"},
    {.name = "sysinfo", .ptr = cmd_sysinfo, .desc = "get system info"},
    {.name = "ls", .ptr = cmd_ls, .desc = "list directory contents"},
    {.name = "cat", .ptr = cmd_cat, .desc = "concatenate and print files"},
    {.name = "dtb", .ptr = cmd_dtb, .desc = "show device tree"},
    {.name = "exec", .ptr = cmd_exec, .desc = "execute a file"},
    {.name = "timer", .ptr = cmd_timer, .desc = "start timer notification"}};

#endif