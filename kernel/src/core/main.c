#include "minemu/boot.h"
#include "minemu/trap.h"
#include "minemu/trace.h"
#include "minemu/platform.h"
#include "minemu/irq.h"

#define MSH_LINE_SIZE 20

static void uart_putc(char c)
{
    while (!(MINEMU_UART0->status & MINEMU_UART_STATUS_TX_READY)) {
    }
    MINEMU_UART0->tx_data = (uint8_t)c;
}

static void uart_puts(const char *s)
{
    while (*s != '\0') {
        uart_putc(*s++);
    }
}

static void msh_process_line(char *line)
{
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
    if ((size_t)(end - command) == 4 &&
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
    for (char *p = command; p < end; p++) {
        uart_putc(*p);
    }
    uart_putc('\n');
}

static void msh(void)
{
    char line[MSH_LINE_SIZE + 1];
    for (;;) {
        uart_puts("msh> ");
        size_t length = 0;
        for (;;) {
            int ch = minemu_uart_char();
            if (ch < 0) {
                continue;
            }
            if (ch == '\n') {
                break;
            }
            if (ch == 0x08 || ch == 0x7f) {
                if (length > 0) {
                    length--;
                }
                continue;
            }
            if (length < MSH_LINE_SIZE) {
                line[length++] = (char)ch;
            }
        }
        line[length] = '\0';
        msh_process_line(line);
    }
}

void minemu_kernel_main(const struct minemu_boot_info *boot_info) {
    if ((uintptr_t)boot_info != MINEMU_BOOT_INFO_VADDR ||
        boot_info->magic != MINEMU_BOOT_INFO_MAGIC ||
        boot_info->version != MINEMU_ABI_VERSION ||
        boot_info->size != sizeof(*boot_info) ||
        boot_info->system_rom_base != UINT32_C(0x08000000) ||
        boot_info->direct_map_vaddr != UINT32_C(0xc0000000) ||
        boot_info->direct_map_paddr != UINT32_C(0x40000000) ||
        boot_info->direct_map_size != UINT32_C(0x04000000)) {
        minemu_trace_event(UINT32_C(0xb007bad0));
        minemu_fail_stop();
    }
    MINEMU_UART0->control |= MINEMU_UART_CONTROL_RX_IRQ_ENABLE;
    MINEMU_INTERRUPT->enable |= UINT32_C(1) << MINEMU_IRQ_UART0;
    minemu_irq_enable();
    msh();
    minemu_fail_stop();
}
