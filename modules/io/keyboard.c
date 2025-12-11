#include <stdint.h>
#include <stdbool.h>
#include "../../libs/print.h"
#include "io.h"
#include <string.h>
#include "../modules/console/consoleutils.h"
bool shift_pressed = false;
bool caps_lock = false;
bool ctrl_pressed = false;
bool alt_pressed = false;
bool num_lock = false;
bool scroll_lock = false;
#define MAX_COMMAND_LENGTH 256
#define KEYBOARD_BUFFER_SIZE 32
extern char input_buffer[];
extern uint8_t input_pos;
extern char command_history[][MAX_COMMAND_LENGTH];
extern uint8_t history_pos;
extern int8_t current_history;
static void (*current_handler)(void) = NULL;
uint8_t last_keypress = 0;
static uint8_t key_buffer[KEYBOARD_BUFFER_SIZE];
static uint8_t key_buffer_start = 0;
static uint8_t key_buffer_end = 0;
extern void process_command(const char *cmd);
extern void print_prompt(void);
extern void restore_from_history(void);
extern void update_cursor(void);
extern void clear_screen(void);
char get_char_from_scancode(uint8_t scancode) {
    char c = 0;
    switch(scancode) {
        case 0x02: c = shift_pressed ^ caps_lock ? '!' : '1'; break;
        case 0x03: c = shift_pressed ^ caps_lock ? '@' : '2'; break;
        case 0x04: c = shift_pressed ^ caps_lock ? '#' : '3'; break;
        case 0x05: c = shift_pressed ^ caps_lock ? '$' : '4'; break;
        case 0x06: c = shift_pressed ^ caps_lock ? '%' : '5'; break;
        case 0x07: c = shift_pressed ^ caps_lock ? '^' : '6'; break;
        case 0x08: c = shift_pressed ^ caps_lock ? '&' : '7'; break;
        case 0x09: c = shift_pressed ^ caps_lock ? '*' : '8'; break;
        case 0x0A: c = shift_pressed ^ caps_lock ? '(' : '9'; break;
        case 0x0B: c = shift_pressed ^ caps_lock ? ')' : '0'; break;
        case 0x0C: c = shift_pressed ^ caps_lock ? '_' : '-'; break;
        case 0x0D: c = shift_pressed ^ caps_lock ? '+' : '='; break;
        case 0x10: c = (shift_pressed ^ caps_lock) ? 'Q' : 'q'; break;
        case 0x11: c = (shift_pressed ^ caps_lock) ? 'W' : 'w'; break;
        case 0x12: c = (shift_pressed ^ caps_lock) ? 'E' : 'e'; break;
        case 0x13: c = (shift_pressed ^ caps_lock) ? 'R' : 'r'; break;
        case 0x14: c = (shift_pressed ^ caps_lock) ? 'T' : 't'; break;
        case 0x15: c = (shift_pressed ^ caps_lock) ? 'Y' : 'y'; break;
        case 0x16: c = (shift_pressed ^ caps_lock) ? 'U' : 'u'; break;
        case 0x17: c = (shift_pressed ^ caps_lock) ? 'I' : 'i'; break;
        case 0x18: c = (shift_pressed ^ caps_lock) ? 'O' : 'o'; break;
        case 0x19: c = (shift_pressed ^ caps_lock) ? 'P' : 'p'; break;
        case 0x1A: c = shift_pressed ^ caps_lock ? '{' : '['; break;
        case 0x1B: c = shift_pressed ^ caps_lock ? '}' : ']'; break;
        case 0x1E: c = (shift_pressed ^ caps_lock) ? 'A' : 'a'; break;
        case 0x1F: c = (shift_pressed ^ caps_lock) ? 'S' : 's'; break;
        case 0x20: c = (shift_pressed ^ caps_lock) ? 'D' : 'd'; break;
        case 0x21: c = (shift_pressed ^ caps_lock) ? 'F' : 'f'; break;
        case 0x22: c = (shift_pressed ^ caps_lock) ? 'G' : 'g'; break;
        case 0x23: c = (shift_pressed ^ caps_lock) ? 'H' : 'h'; break;
        case 0x24: c = (shift_pressed ^ caps_lock) ? 'J' : 'j'; break;
        case 0x25: c = (shift_pressed ^ caps_lock) ? 'K' : 'k'; break;
        case 0x26: c = (shift_pressed ^ caps_lock) ? 'L' : 'l'; break;
        case 0x27: c = shift_pressed ^ caps_lock ? ':' : ';'; break;
        case 0x28: c = shift_pressed ^ caps_lock ? '"' : '\''; break;
        case 0x29: c = shift_pressed ^ caps_lock ? '~' : '`'; break;
        case 0x2B: c = shift_pressed ^ caps_lock ? '|' : '\\'; break;
        case 0x2C: c = (shift_pressed ^ caps_lock) ? 'Z' : 'z'; break;
        case 0x2D: c = (shift_pressed ^ caps_lock) ? 'X' : 'x'; break;
        case 0x2E: c = (shift_pressed ^ caps_lock) ? 'C' : 'c'; break;
        case 0x2F: c = (shift_pressed ^ caps_lock) ? 'V' : 'v'; break;
        case 0x30: c = (shift_pressed ^ caps_lock) ? 'B' : 'b'; break;
        case 0x31: c = (shift_pressed ^ caps_lock) ? 'N' : 'n'; break;
        case 0x32: c = (shift_pressed ^ caps_lock) ? 'M' : 'm'; break;
        case 0x33: c = shift_pressed ^ caps_lock ? '<' : ','; break;
        case 0x34: c = shift_pressed ^ caps_lock ? '>' : '.'; break;
        case 0x35: c = shift_pressed ^ caps_lock ? '?' : '/'; break;
        case 0x39: c = ' '; break;
    }
    return c;
}
void handle_function_key(uint8_t scancode) {
    switch(scancode) {
        case 0x3B: 
            process_command("help");
            break;
        case 0x3C: 
            process_command("shcts");
            break;
        case 0x3D: 
            process_command("clear");
            break;
        case 0x3E: 
            process_command("clock");
            break;
        case 0x3F: 
            process_command("history");
            break;
        case 0x40: 
            process_command("clipboard");
            break;
        case 0x41: 
            break;
        case 0x42: 
            break;
        case 0x43: 
            break;
        case 0x44: 
            break;
        case 0x57: 
            break;
        case 0x58: 
            break;
    }
}
void add_to_key_buffer(uint8_t scancode) {
    key_buffer[key_buffer_end] = scancode;
    key_buffer_end = (key_buffer_end + 1) % KEYBOARD_BUFFER_SIZE;
    if (key_buffer_end == key_buffer_start) {
        key_buffer_start = (key_buffer_start + 1) % KEYBOARD_BUFFER_SIZE;
    }
}
uint8_t read_from_key_buffer(void) {
    if (key_buffer_start == key_buffer_end) {
        return 0; 
    }
    uint8_t scancode = key_buffer[key_buffer_start];
    key_buffer_start = (key_buffer_start + 1) % KEYBOARD_BUFFER_SIZE;
    return scancode;
}
bool is_key_buffer_empty(void) {
    return key_buffer_start == key_buffer_end;
}
void clear_key_buffer(void) {
    key_buffer_start = 0;
    key_buffer_end = 0;
}
uint8_t get_keyboard_modifiers(void) {
    uint8_t modifiers = 0;
    if (shift_pressed) modifiers |= 0x01;
    if (ctrl_pressed) modifiers |= 0x02;
    if (alt_pressed) modifiers |= 0x04;
    if (caps_lock) modifiers |= 0x08;
    if (num_lock) modifiers |= 0x10;
    if (scroll_lock) modifiers |= 0x20;
    return modifiers;
}
void handle_alt_combinations(char c) {
    if (!alt_pressed) return;
    switch(c) {
        case '1': case '2': case '3': case '4': case '5':
        case '6': case '7': case '8': case '9': case '0':
            print("\nSwitching to console ", text_color);
            putchar(c, text_color);
            break;
    }
}
void handle_special_combinations(uint8_t scancode) {
    if (ctrl_pressed && alt_pressed) {
        switch(scancode) {
            case 0x17: 
                print("\nSystem info", text_color);
                break;
            case 0x20: 
                print("\nDebug info", text_color);
                break;
            case 0x31: 
                num_lock = !num_lock;
                print(num_lock ? "\nNum Lock ON" : "\nNum Lock OFF", text_color);
                break;
            case 0x39: 
                print("\nSpecial function", text_color);
                break;
        }
    }
}
void handle_navigation_keys(uint8_t scancode) {
    if (scancode == 0x47) { 
        input_pos = 0;
        update_cursor();
    }
    else if (scancode == 0x4F) { 
        input_pos = strlen(input_buffer);
        update_cursor();
    }
}
void keyboard_handler() {
    uint8_t scancode = inb(0x60);
    static uint8_t extended = 0;
    add_to_key_buffer(scancode);
    if (current_handler != NULL) {
        last_keypress = scancode;
        return;
    }
    switch(scancode) {
        case 0x2A: case 0x36: shift_pressed = true; return;
        case 0xAA: case 0xB6: shift_pressed = false; return;
        case 0x3A: caps_lock = !caps_lock; return;
        case 0x1D: ctrl_pressed = true; return;
        case 0x9D: ctrl_pressed = false; return;
        case 0x38: alt_pressed = true; return;
        case 0xB8: alt_pressed = false; return;
        case 0x45: num_lock = !num_lock; return;
        case 0x46: scroll_lock = !scroll_lock; return;
        case 0xE0: extended = 1; return;
    }
    if (extended) {
        extended = 0;
        handle_navigation_keys(scancode);
        if (scancode == 0x1C) { 
            input_buffer[input_pos] = '\0';
            putchar('\n', text_color);
            process_command(input_buffer);
            input_pos = 0;
            return;
        }
        if (scancode == 0x48) { 
            if (history_pos == 0) return;
            if (current_history == -1) {
                input_buffer[input_pos] = '\0';
                strcpy(command_history[history_pos], input_buffer);
                current_history = history_pos;
            }
            if (current_history > 0) {
                current_history--;
                restore_from_history();
            }
            return;
        }
        else if (scancode == 0x50) { 
            if (current_history < history_pos - 1) {
                current_history++;
                restore_from_history();
            } 
            else if (current_history == history_pos - 1) {
                current_history = -1;
                restore_from_history();
            }
            return;
        }
        return;
    }
    if (scancode < 0x80) {
        if (scancode >= 0x3B && scancode <= 0x44) { 
            handle_function_key(scancode);
            return;
        }
        if (scancode == 0x57 || scancode == 0x58) { 
            handle_function_key(scancode);
            return;
        }
        if (scancode == 0x1C) { 
            input_buffer[input_pos] = '\0';
            putchar('\n', text_color);
            process_command(input_buffer);
            input_pos = 0;
            return;
        }
        else if (scancode == 0x0E) { 
            if (input_pos > 0) {
                input_buffer[--input_pos] = '\0';
                cursor_x--;
                vga_buffer[cursor_y * VGA_WIDTH + cursor_x] = (text_color << 8) | ' ';
                update_cursor();
            }
            return;
        }
        else if (scancode == 0x0F) { 
            for (int i = 0; i < 4 && input_pos < MAX_COMMAND_LENGTH-1; i++) {
                putchar(' ', text_color);
                input_buffer[input_pos++] = ' ';
            }
            return;
        }
        char c = get_char_from_scancode(scancode);
        handle_special_combinations(scancode);
        handle_alt_combinations(c);
        if (ctrl_pressed && c) {
            switch(c) {
                case 'l': case 'L': 
                    clear_screen();
                    print_prompt();
                    return;
                case 'c': case 'C': 
                    print("\n^C", text_color);
                    input_pos = 0;
                    print_prompt();
                    return;
                case 'a': case 'A': 
                    input_pos = 0;
                    update_cursor();
                    return;
                case 'e': case 'E': 
                    input_pos = strlen(input_buffer);
                    update_cursor();
                    return;
                case 'u': case 'U': 
                    input_pos = 0;
                    input_buffer[0] = '\0';
                    print_prompt();
                    return;
                case 'k': case 'K': 
                    input_buffer[input_pos] = '\0';
                    print_prompt();
                    return;
            }
        }
        if (c && input_pos < MAX_COMMAND_LENGTH-1) {
            putchar(c, text_color);
            input_buffer[input_pos++] = c;
            update_cursor();
        }
    }
}
void wait_for_key(void) {
    while (is_key_buffer_empty()) {
    }
}
uint8_t get_key(void) {
    while (is_key_buffer_empty()) {
    }
    return read_from_key_buffer();
}
bool key_available(void) {
    return !is_key_buffer_empty();
}
void (*get_keyboard_handler(void))(void) {
    return current_handler;
}
void set_keyboard_handler(void (*handler)(void)) {
    current_handler = handler;
}
uint8_t get_last_keypress(void) {
    uint8_t key = last_keypress;
    last_keypress = 0;
    return key;
}
