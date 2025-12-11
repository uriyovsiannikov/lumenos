#ifndef TEXTEDIT_H
#define TEXTEDIT_H
#include <stdint.h>
#define TEXT_EDIT_MAX_LINES 100
#define TEXT_EDIT_LINE_LENGTH 80
#define TEXT_EDIT_MAX_FILENAME 32
typedef enum {
    TEXT_COLOR_BLACK = 0,
    TEXT_COLOR_BLUE,
    TEXT_COLOR_GREEN,
    TEXT_COLOR_CYAN,
    TEXT_COLOR_RED,
    TEXT_COLOR_MAGENTA,
    TEXT_COLOR_BROWN,
    TEXT_COLOR_WHITE,
    TEXT_COLOR_GRAY,
    TEXT_COLOR_LIGHT_BLUE,
    TEXT_COLOR_LIGHT_GREEN,
    TEXT_COLOR_LIGHT_CYAN,
    TEXT_COLOR_LIGHT_RED,
    TEXT_COLOR_LIGHT_MAGENTA,
    TEXT_COLOR_YELLOW,
    TEXT_COLOR_BRIGHT_WHITE
} text_color_t;
typedef enum {
    ALIGN_LEFT = 0,
    ALIGN_CENTER,
    ALIGN_RIGHT,
    ALIGN_JUSTIFY
} text_align_t;
struct text_format {
    text_color_t color;
    text_align_t alignment;
    uint8_t bold : 1;
    uint8_t italic : 1;
    uint8_t underline : 1;
};
struct text_line {
    char text[TEXT_EDIT_LINE_LENGTH];
    struct text_format format;
};
extern uint32_t current_dir_id;
void textedit_init();
void textedit_insert_line(int line_num, const char* text);
void textedit_delete_line(int line_num);
void textedit_append_line(const char* text);
void textedit_show();
void textedit_clear();
void textedit_save(const char* filename);
void textedit_load(const char* filename);
void textedit_set_format(int line_num, text_color_t color, text_align_t alignment, uint8_t bold, uint8_t italic, uint8_t underline);
void textedit_search(const char* pattern);
void textedit_replace(const char* old_text, const char* new_text);
void textedit_show_help();
#endif 