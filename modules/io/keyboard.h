#ifndef KEYBOARD_H
#define KEYBOARD_H
#include <stdbool.h>
#include <stdint.h>
extern bool shift_pressed;
extern bool caps_lock;
extern bool ctrl_pressed;
extern bool alt_pressed;
extern bool num_lock;
extern bool scroll_lock;
extern uint8_t last_keypress;
uint8_t get_last_keypress(void);
void (*get_keyboard_handler(void))(void);
void set_keyboard_handler(void (*handler)(void));
void keyboard_handler(void);
char get_char_from_scancode(uint8_t scancode);
void handle_function_key(uint8_t scancode);
void add_to_key_buffer(uint8_t scancode);
uint8_t read_from_key_buffer(void);
bool is_key_buffer_empty(void);
void clear_key_buffer(void);
uint8_t get_keyboard_modifiers(void);
void handle_alt_combinations(char c);
void handle_special_combinations(uint8_t scancode);
void handle_navigation_keys(uint8_t scancode);
void wait_for_key(void);
uint8_t get_key(void);
bool key_available(void);
void process_normal_key(uint8_t scancode);
void process_control_combinations(char c);
void process_extended_key(uint8_t scancode);
#endif
