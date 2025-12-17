#include "serial.h"
#include "../libs/print.h"
#include "../modules/io/io.h"
#include <stdarg.h>
void serial_init(uint16_t port) {
    outb(port + 1, 0x00);
    outb(port + 3, SERIAL_LINE_ENABLE_DLAB);
    outb(port + 0, 0x03);
    outb(port + 1, 0x00);
    outb(port + 3, SERIAL_LINE_8BIT | SERIAL_LINE_NO_PARITY | SERIAL_LINE_1_STOP_BIT);
    outb(port + 2, SERIAL_FIFO_ENABLE | SERIAL_FIFO_SIZE_14);
    outb(port + 4, SERIAL_MODEM_DTR_RTS | SERIAL_MODEM_OUT2);
    serial_flush(port);
}
void serial_putc(uint16_t port, char c) {
    while (!serial_is_transmit_empty(port))
        ;
    outb(port, c);
}
char serial_getc(uint16_t port) {
    while (!serial_received(port))
        ;
    return inb(port);
}
int serial_received(uint16_t port) {
    return inb(port + 5) & SERIAL_LINE_STATUS_DATA_READY;
}
int serial_is_transmit_empty(uint16_t port) {
    return inb(port + 5) & SERIAL_LINE_STATUS_EMPTY;
}
void serial_write(uint16_t port, const char* str) {
    if (!str) return;
    
    for (size_t i = 0; str[i] != '\0'; i++) {
        if (str[i] == '\n') {
            serial_putc(port, '\r');
        }
        serial_putc(port, str[i]);
    }
}
void serial_write_hex(uint16_t port, uint32_t num) {
    char buf[11];
    buf[0] = '0';
    buf[1] = 'x';
    buf[10] = '\0';
    for (int i = 9; i >= 2; i--) {
        uint8_t nibble = num & 0xF;
        buf[i] = (nibble < 10) ? ('0' + nibble) : ('A' + (nibble - 10));
        num >>= 4;
    }
    serial_write(port, buf);
}
void serial_write_dec(uint16_t port, uint32_t num) {
    if (num == 0) {
        serial_putc(port, '0');
        return;
    }
    char buf[12];
    int i = 0;
    while (num > 0) {
        buf[i++] = '0' + (num % 10);
        num /= 10;
    }
    for (int j = i - 1; j >= 0; j--) {
        serial_putc(port, buf[j]);
    }
}
void serial_printf(uint16_t port, const char* format, ...) {
    va_list args;
    va_start(args, format);
    char buffer[256];
    char* ptr = buffer;
    while (*format) {
        if (*format == '%') {
            format++;
            switch (*format) {
                case 's': {
                    char* str = va_arg(args, char*);
                    while (*str) {
                        *ptr++ = *str++;
                    }
                    break;
                }
                case 'c': {
                    char c = (char)va_arg(args, int);
                    *ptr++ = c;
                    break;
                }
                case 'd': 
                case 'i': {
                    int num = va_arg(args, int);
                    if (num < 0) {
                        *ptr++ = '-';
                        num = -num;
                    }
                    char num_buf[12];
                    int i = 0;
                    if (num == 0) {
                        num_buf[i++] = '0';
                    }
                    while (num > 0) {
                        num_buf[i++] = '0' + (num % 10);
                        num /= 10;
                    }
                    for (int j = i - 1; j >= 0; j--) {
                        *ptr++ = num_buf[j];
                    }
                    break;
                }
                case 'x': 
                case 'X': {
                    unsigned int num = va_arg(args, unsigned int);
                    char hex_buf[9];
                    int i = 0;
                    if (num == 0) {
                        hex_buf[i++] = '0';
                    }
                    while (num > 0) {
                        uint8_t nibble = num & 0xF;
                        hex_buf[i++] = (nibble < 10) ? ('0' + nibble) : 
                                      ((*format == 'X') ? ('A' + (nibble - 10)) : ('a' + (nibble - 10)));
                        num >>= 4;
                    }
                    for (int j = i - 1; j >= 0; j--) {
                        *ptr++ = hex_buf[j];
                    }
                    break;
                }
                case 'p': {
                    void* addr = va_arg(args, void*);
                    *ptr++ = '0';
                    *ptr++ = 'x';
                    uint32_t num = (uint32_t)addr;
                    char hex_buf[9];
                    int i = 0;
                    if (num == 0) {
                        hex_buf[i++] = '0';
                    }
                    while (num > 0) {
                        uint8_t nibble = num & 0xF;
                        hex_buf[i++] = (nibble < 10) ? ('0' + nibble) : ('A' + (nibble - 10));
                        num >>= 4;
                    }
                    while (i < 8) {
                        hex_buf[i++] = '0';
                    }
                    for (int j = i - 1; j >= 0; j--) {
                        *ptr++ = hex_buf[j];
                    }
                    break;
                }
                case '%': {
                    *ptr++ = '%';
                    break;
                }
                default: {
                    *ptr++ = '%';
                    *ptr++ = *format;
                    break;
                }
            }
        } else {
            *ptr++ = *format;
        }
        format++;
    }
    *ptr = '\0';
    serial_write(port, buffer);
    
    va_end(args);
}
void serial_flush(uint16_t port) {
    while (serial_received(port)) {
        inb(port);
    }
}
void serial_init_default() {
    serial_init(COM1);
    SERIAL_PRINT("\nSerial port COM1 initialized\n");
}