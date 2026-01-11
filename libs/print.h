#ifndef PRINT_H
#define PRINT_H
#include <stdint.h>
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
enum COLORS {
  BLACK = 0x00,
  BLUE = 0x01,
  GREEN = 0x02,
  CYAN = 0x03,
  RED = 0x04,
  MAGENTA = 0x05,
  BROWN = 0x06,
  GRAY = 0x07,
  DARK_GRAY = 0x08,
  LIGHT_BLUE = 0x09,
  LIGHT_GREEN = 0x0A,
  LIGHT_CYAN = 0x0B,
  LIGHT_RED = 0x0C,
  LIGHT_MAGENTA = 0x0D,
  YELLOW = 0x0E,
  WHITE = 0x0F,
  ORANGE = 0x0E,
  PINK = 0x0D,
  PURPLE = 0x05,
  LAVENDER = 0x0D,
  TEAL = 0x0B,
  FOREST_GREEN = 0x02,
  OLIVE = 0x06,
  GOLD = 0x0E,
  SILVER = 0x07,
  BRONZE = 0x06
};
extern volatile uint16_t *vga_buffer;
extern uint8_t cursor_x, cursor_y;
extern uint8_t text_color;
void clear_screen(void);
void putchar(char c, uint8_t color);
void print(const char *str, uint8_t color);
void print_hex(uint32_t num, uint8_t color);
void print_dec(uint32_t num, uint8_t color);
void clear_current_line(void);
void scroll_screen(void);
void update_cursor(void);
void print_error(const char *msg);
void print_success(const char *msg);
void print_info(const char *msg);
void print_warning(const char *msg);
void putchar_at(int x, int y, char c, uint8_t color);
void print_at(int x, int y, const char *str, uint8_t color);
void hide_cursor();
void show_cursor();
int isprint(int c);
void print_uint(uint32_t num, uint8_t color);
int num_digits(uint32_t num);
void print_double(double value, int precision, uint8_t color);
#endif
