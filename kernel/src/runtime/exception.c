#include "minemu/irq.h"
#include "minemu/syscall.h"
#include "minemu/platform.h"

#define UART_BUFFER_SIZE 128
static volatile char uart_buffer[UART_BUFFER_SIZE];
static volatile size_t uart_buffer_count;

void minemu_fail_stop(void) {
    for (;;) {
        __asm__ volatile("nop");
    }
}

void minemu_panic(const char *message) {
    (void)message;
    minemu_fail_stop();
}

__attribute__((weak, noreturn)) void minemu_svc_trampoline(void) {
    minemu_fail_stop();
}

static void minemu_uart0_irq_handler(void) {
    while(MINEMU_UART0->status & MINEMU_UART_STATUS_RX_READY) {
        uint8_t byte = (uint8_t)MINEMU_UART0->rx_data;
        if (uart_buffer_count < UART_BUFFER_SIZE) {
            uart_buffer[uart_buffer_count++] = byte;
        }
    }
}

int minemu_uart_getchar(void) {
    int result = -1;
    minemu_irq_disable();
    if (uart_buffer_count > 0) {
        result = (unsigned char)uart_buffer[0];
        for (size_t i = 1; i < uart_buffer_count; ++i) {
            uart_buffer[i - 1] = uart_buffer[i];
        }
        --uart_buffer_count;
    }
    minemu_irq_enable();
    return result;
}


__attribute__((weak)) struct minemu_trap_frame *minemu_svc_dispatch(
    struct minemu_trap_frame *frame) {
    (void)frame;
    minemu_fail_stop();
}

__attribute__((weak)) struct minemu_trap_frame *minemu_irq_dispatch(
    struct minemu_trap_frame *frame) {
    switch((uint32_t)frame->exception_id) {
        case MINEMU_IRQ_UART0:
            minemu_uart0_irq_handler();
            break;
        default:
            minemu_fail_stop();
    }
    MINEMU_INTERRUPT->eoi = (uint32_t)frame->exception_id;
    return frame;
}

__attribute__((weak)) void minemu_undefined_dispatch(struct minemu_trap_frame *frame) {
    (void)frame;
    minemu_fail_stop();
}

__attribute__((weak)) void minemu_abort_dispatch(struct minemu_trap_frame *frame) {
    (void)frame;
    minemu_fail_stop();
}
