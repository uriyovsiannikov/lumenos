#include "print.h"
#include "string.h"
#include "../modules/io/io.h"
volatile uint16_t *vga_buffer = (volatile uint16_t*)0xB8000;
uint8_t cursor_x = 0, cursor_y = 0;
uint8_t text_color = WHITE;
void clear_screen() {
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_buffer[i] = (text_color << 8) | ' ';
    }
    cursor_x = 0;
    cursor_y = 0;
}
void scroll_screen() {
    uint16_t blank = (text_color << 8) | ' ';
    if (cursor_y >= VGA_HEIGHT) {
        cursor_y = VGA_HEIGHT - 1;
    }
    memmove((void*)vga_buffer, 
           (void*)(vga_buffer + VGA_WIDTH), 
           VGA_WIDTH * (VGA_HEIGHT-1) * sizeof(uint16_t));
    for (int x = 0; x < VGA_WIDTH; x++) {
        vga_buffer[(VGA_HEIGHT-1)*VGA_WIDTH + x] = blank;
    }
    cursor_y = VGA_HEIGHT - 1;
    cursor_x = 0;
    update_cursor();
}
void putchar(char c, uint8_t color) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } 
    else if (c == '\r') {
        cursor_x = 0;
    }
    else if (c == '\b') {
        if (cursor_x > 0 || cursor_y > 0) {
            if (cursor_x == 0) {
                cursor_y--;
                cursor_x = VGA_WIDTH - 1;
            } else {
                cursor_x--;
            }
            vga_buffer[cursor_y * VGA_WIDTH + cursor_x] = (color << 8) | ' ';
        }
        return;
    }
    else if (c == '\t') {
        do { putchar(' ', color); } while (cursor_x % 4 != 0 && cursor_x < VGA_WIDTH);
        return;
    }
    else if (c >= ' ') {
        vga_buffer[cursor_y * VGA_WIDTH + cursor_x] = (color << 8) | c;
        cursor_x++;
    }
    if (cursor_x >= VGA_WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }
    if (cursor_y >= VGA_HEIGHT) {
        scroll_screen();
    } else {
        update_cursor();
    }
}
void print(const char *str, uint8_t color) {
    while (*str) {
        putchar(*str++, color);
    }
}
void print_hex(uint32_t num, uint8_t color) {
    char hex_chars[] = "0123456789ABCDEF";
    print("0x", color);
    for (int i = 7; i >= 0; i--) {
        uint8_t nibble = (num >> (i * 4)) & 0xF;
        putchar(hex_chars[nibble], color);
    }
}
void clear_current_line() {
    uint8_t saved_y = cursor_y;
    putchar('\r', text_color);
    for (int i = 0; i < VGA_WIDTH; i++) {
        putchar(' ', text_color);
    }
    cursor_x = 0;
    cursor_y = saved_y;
    update_cursor();
}
void print_dec(uint32_t num, uint8_t color) {
    if (num == 0) {
        putchar('0', color);
        return;
    }
    char buffer[10];
    uint8_t i = 0;
    while (num > 0) {
        buffer[i++] = '0' + (num % 10);
        num /= 10;
    }
    for (int j = i - 1; j >= 0; j--) {
        putchar(buffer[j], color);
    }
}
void update_cursor(void) {
    uint16_t pos = cursor_y * VGA_WIDTH + cursor_x;
    outb(0x3D4, 14);
    outb(0x3D5, (pos >> 8) & 0xFF);
    outb(0x3D4, 15);
    outb(0x3D5, pos & 0xFF);
}
void print_error(const char *msg) {
    print(msg, RED);
    putchar('\n', RED);
}
void print_success(const char *msg) {
    print(msg, LIGHT_GREEN);
}
void print_info(const char *msg) {
    print(msg, LIGHT_CYAN);
}
void print_warning(const char *msg) {
    print(msg, YELLOW);
}
void putchar_at(int x, int y, char c, uint8_t color) {
    if (x >= 0 && x < VGA_WIDTH && y >= 0 && y < VGA_HEIGHT) {
        vga_buffer[y * VGA_WIDTH + x] = (color << 8) | c;
    }
}
void print_at(int x, int y, const char* str, uint8_t color) {
    while (*str) {
        putchar_at(x++, y, *str++, color);
    }
}
void hide_cursor(void) {
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x20);
}
void show_cursor(void) {
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x0F);
    outb(0x3D4, 0x0B);
    outb(0x3D5, 0x0F);
}
int isprint(int c) {
    return (c >= 0x20 && c <= 0x7E);
}
void print_uint(uint32_t num, uint8_t color) {
    char buffer[12]; 
    size_t i = 0;
    if (num == 0) {
        print("0", color);
        return;
    }
    while (num > 0 && i < sizeof(buffer)-1) {
        buffer[i++] = '0' + (num % 10);
        num /= 10;
    }
    for (int j = i-1; j >= 0; j--) {
        char c[2] = {buffer[j], '\0'};
        print(c, color);
    }
}
int num_digits(uint32_t num) {
    if (num == 0) return 1;
    int count = 0;
    while (num != 0) {
        num /= 10;
        count++;
    }
    return count;
}
void print_double(double value, int precision, uint8_t color) {
    int int_part = (int)value;
    print_dec(int_part, color);
    if (precision > 0) {
        putchar('.', color);
        double fractional = value - int_part;
        if (fractional < 0) fractional = -fractional;
        for (int i = 0; i < precision; i++) {
            fractional *= 10;
            int digit = (int)fractional % 10;
            putchar('0' + digit, color);
        }
    }
}
