#ifndef SERIAL_H
#define SERIAL_H
#include "../libs/string.h"
#include <stddef.h>
#include <stdint.h>
#define COM1 0x3F8
#define COM2 0x2F8
#define COM3 0x3E8
#define COM4 0x2E8
#define SERIAL_DATA_PORT(base) (base)
#define SERIAL_FIFO_COMMAND_PORT(base) (base + 2)
#define SERIAL_LINE_COMMAND_PORT(base) (base + 3)
#define SERIAL_MODEM_COMMAND_PORT(base) (base + 4)
#define SERIAL_LINE_STATUS_PORT(base) (base + 5)
#define SERIAL_LINE_ENABLE_DLAB 0x80
#define SERIAL_LINE_8BIT 0x03
#define SERIAL_LINE_NO_PARITY 0x00
#define SERIAL_LINE_1_STOP_BIT 0x00
#define SERIAL_LINE_STATUS_DATA_READY 0x01
#define SERIAL_LINE_STATUS_EMPTY 0x20
#define SERIAL_MODEM_DTR_RTS 0x03
#define SERIAL_MODEM_OUT2 0x08
#define SERIAL_FIFO_ENABLE 0xC7
#define SERIAL_FIFO_SIZE_14 0xC0
void serial_init(uint16_t port);
void serial_putc(uint16_t port, char c);
char serial_getc(uint16_t port);
void serial_write(uint16_t port, const char *str);
void serial_write_hex(uint16_t port, uint32_t num);
void serial_write_dec(uint16_t port, uint32_t num);
void serial_printf(uint16_t port, const char *format, ...);
int serial_received(uint16_t port);
int serial_is_transmit_empty(uint16_t port);
void serial_flush(uint16_t port);
#define SERIAL_PUT(c) serial_putc(COM1, c)
#define SERIAL_GET() serial_getc(COM1)
#define SERIAL_PRINT(s) serial_write(COM1, s)
#define SERIAL_PRINTF(fmt, ...) serial_printf(COM1, fmt, ##__VA_ARGS__)
#define SERIAL_HEX(n) serial_write_hex(COM1, n)
#define SERIAL_DEC(n) serial_write_dec(COM1, n)
#endif