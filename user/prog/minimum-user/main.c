#include <stdint.h>
#include <stddef.h>
#include "minemu/user_abi.h"

#define MSH_LINE_SIZE 20U

static void uart_write(const char *data, uint32_t length) {
    (void)ioctl(MINEMU_FD_UART_OUT, MINEMU_IOCTL_UART_WRITE, (void *)data, length);
}

static void uart_putc(char c){
    uart_write(&c, 1U);
}

static void uart_puts(const char *s){
    size_t length = 0;

    while (s[length] != '\0') {
        length++;
    }
    uart_write(s, (uint32_t)length);
}

static void msh_process_line(char *line){
    char *command = line;
    while (*command == ' ') {
        command++;
    }
    if (*command == '\0') {
        return;
    }
    char *end = command;
    while (*end != '\0' && *end != ' ') {
        end++;
    }

    if ((size_t)(end - command) == 4U &&
        command[0] == 'e' &&
        command[1] == 'c' &&
        command[2] == 'h' &&
        command[3] == 'o') {
        while (*end == ' ') {
            end++;
        }
        uart_puts(end);
        uart_putc('\n');
        return;
    }
    uart_puts("command not found: ");
    uart_write(command, (uint32_t)(end - command));
    uart_putc('\n');
}


void minemu_user_main(void) {
    char line[MSH_LINE_SIZE + 1U];
    for (;;) {
        uart_puts("msh> ");
        size_t length = 0;
        size_t overflow_count = 0;
        for (;;) {
            char ch = '\0';
            int32_t result = ioctl(MINEMU_FD_UART_IN, MINEMU_IOCTL_UART_READ, (void *)&ch, (uint32_t)1);
            if (result <= 0) {
                continue;
            }
            if (ch == '\n') {
                break;
            }
            if ((uint8_t)ch == 0x08U || (uint8_t)ch == 0x7fU) {
                if (overflow_count > 0) {
                    --overflow_count;
                } else if(length > 0) {
                    --length;
                }
                continue;
            }
            if (length < MSH_LINE_SIZE && overflow_count == 0){
                line[length++] = (char)ch;
            }else{
                ++overflow_count;
            }
        }
        line[length] = '\0';
        msh_process_line(line);
    }
}
