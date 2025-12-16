#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include "../libs/print.h"
#include "../libs/string.h"
#include "../modules/fs/fs.h"
#include "../libs/ctype.h"
#include "tedit.h"
static struct text_line text_buffer[TEXT_EDIT_MAX_LINES];
static int current_line_count = 0;
static uint8_t textedit_initialized = 0;
#ifndef BRIGHT_WHITE
#define BRIGHT_WHITE 15
#endif
void textedit_init() {
    if (!textedit_initialized) {
        for (int i = 0; i < TEXT_EDIT_MAX_LINES; i++) {
            text_buffer[i].text[0] = '\0';
            text_buffer[i].format.color = TEXT_COLOR_WHITE;
            text_buffer[i].format.alignment = ALIGN_LEFT;
            text_buffer[i].format.bold = 0;
            text_buffer[i].format.italic = 0;
            text_buffer[i].format.underline = 0;
        }
        current_line_count = 0;
        textedit_initialized = 1;
    }
}
void textedit_insert_line(int line_num, const char* text) {
    if (line_num < 0 || line_num > current_line_count || current_line_count >= TEXT_EDIT_MAX_LINES) {
        print_error("Invalid line number or buffer full");
        return;
    }
    for (int i = current_line_count; i > line_num; i--) {
        strcpy(text_buffer[i].text, text_buffer[i-1].text);
        text_buffer[i].format = text_buffer[i-1].format;
    }
    int copy_len = strlen(text);
    if (copy_len >= TEXT_EDIT_LINE_LENGTH) {
        copy_len = TEXT_EDIT_LINE_LENGTH - 1;
    }
    strncpy(text_buffer[line_num].text, text, copy_len);
    text_buffer[line_num].text[copy_len] = '\0';
    text_buffer[line_num].format.color = TEXT_COLOR_WHITE;
    text_buffer[line_num].format.alignment = ALIGN_LEFT;
    text_buffer[line_num].format.bold = 0;
    text_buffer[line_num].format.italic = 0;
    text_buffer[line_num].format.underline = 0;
    current_line_count++;
    print_success("Line inserted");
}
void textedit_delete_line(int line_num) {
    if (line_num < 0 || line_num >= current_line_count) {
        print_error("Invalid line number");
        return;
    }
    for (int i = line_num; i < current_line_count - 1; i++) {
        strcpy(text_buffer[i].text, text_buffer[i+1].text);
        text_buffer[i].format = text_buffer[i+1].format;
    }
    text_buffer[current_line_count - 1].text[0] = '\0';
    current_line_count--;
    print_success("Line deleted");
}
void textedit_append_line(const char* text) {
    textedit_insert_line(current_line_count, text);
}
void textedit_set_format(int line_num, text_color_t color, text_align_t alignment, 
                        uint8_t bold, uint8_t italic, uint8_t underline) {
    if (line_num < 0 || line_num >= current_line_count) {
        print_error("Invalid line number");
        return;
    }
    text_buffer[line_num].format.color = color;
    text_buffer[line_num].format.alignment = alignment;
    text_buffer[line_num].format.bold = bold;
    text_buffer[line_num].format.italic = italic;
    text_buffer[line_num].format.underline = underline;
    print_success("Format applied");
}
int get_terminal_color(text_color_t color) {
    switch (color) {
        case TEXT_COLOR_BLACK: return BLACK;
        case TEXT_COLOR_BLUE: return BLUE;
        case TEXT_COLOR_GREEN: return GREEN;
        case TEXT_COLOR_CYAN: return CYAN;
        case TEXT_COLOR_RED: return RED;
        case TEXT_COLOR_MAGENTA: return MAGENTA;
        case TEXT_COLOR_BROWN: return BROWN;
        case TEXT_COLOR_WHITE: return WHITE;
        case TEXT_COLOR_GRAY: return GRAY;
        case TEXT_COLOR_LIGHT_BLUE: return LIGHT_BLUE;
        case TEXT_COLOR_LIGHT_GREEN: return LIGHT_GREEN;
        case TEXT_COLOR_LIGHT_CYAN: return LIGHT_CYAN;
        case TEXT_COLOR_LIGHT_RED: return LIGHT_RED;
        case TEXT_COLOR_LIGHT_MAGENTA: return LIGHT_MAGENTA;
        case TEXT_COLOR_YELLOW: return YELLOW;
        case TEXT_COLOR_BRIGHT_WHITE: return BRIGHT_WHITE;
        default: return WHITE;
    }
}
void textedit_show() {
    print_info("Text Editor Contents:");
    print("\n", WHITE);
    if (current_line_count == 0) {
        print("Document is empty\n", GRAY);
        return;
    }
    for (int i = 0; i < current_line_count; i++) {
        if (i < 9) {
            print("  ", WHITE);
            print_dec(i + 1, CYAN);
        } else if (i < 99) {
            print(" ", WHITE);
            print_dec(i + 1, CYAN);
        } else {
            print_dec(i + 1, CYAN);
        }
        print(" | ", WHITE);
        int term_color = get_terminal_color(text_buffer[i].format.color);
        const char* text = text_buffer[i].text;
        int len = strlen(text);
        if (text_buffer[i].format.bold) {
            print("[B]", term_color);
        }
        if (text_buffer[i].format.italic) {
            print("[I]", term_color);
        }
        if (text_buffer[i].format.underline) {
            print("[U]", term_color);
        }
        switch (text_buffer[i].format.alignment) {
            case ALIGN_LEFT:
                print(text, term_color);
                break;
            case ALIGN_CENTER:
                {
                    int spaces = (TEXT_EDIT_LINE_LENGTH - len) / 2;
                    for (int s = 0; s < spaces; s++) print(" ", term_color);
                    print(text, term_color);
                }
                break;
            case ALIGN_RIGHT:
                {
                    int spaces = TEXT_EDIT_LINE_LENGTH - len;
                    for (int s = 0; s < spaces; s++) print(" ", term_color);
                    print(text, term_color);
                }
                break;
            case ALIGN_JUSTIFY:
                print(text, term_color);
                for (int s = len; s < TEXT_EDIT_LINE_LENGTH; s++) print(" ", term_color);
                break;
        }
        print("\n", WHITE);
    }
}
void textedit_clear() {
    for (int i = 0; i < TEXT_EDIT_MAX_LINES; i++) {
        text_buffer[i].text[0] = '\0';
        text_buffer[i].format.color = TEXT_COLOR_WHITE;
        text_buffer[i].format.alignment = ALIGN_LEFT;
        text_buffer[i].format.bold = 0;
        text_buffer[i].format.italic = 0;
        text_buffer[i].format.underline = 0;
    }
    current_line_count = 0;
    print_success("Document cleared");
}
void textedit_save(const char* filename) {
    fs_delete((char*)filename, current_dir_id);
    if (fs_create((char*)filename, 0, current_dir_id, false) < 0) {
        print_error("Cannot create text file");
        return;
    }
    char big_buffer[4096] = {0};
    char* ptr = big_buffer;
    for (int i = 0; i < current_line_count; i++) {
        ptr += sprintf(ptr, "%d,%d,%d,%d,%d|", 
                      text_buffer[i].format.color,
                      text_buffer[i].format.alignment,
                      text_buffer[i].format.bold,
                      text_buffer[i].format.italic,
                      text_buffer[i].format.underline);
        const char* text = text_buffer[i].text;
        while (*text) {
            *ptr++ = *text++;
        }
        *ptr++ = '\n';
    }
    *ptr = '\0';
    fs_write((char*)filename, big_buffer, strlen(big_buffer), current_dir_id);
    print_success("Document saved to ");
    print(filename, GREEN);
    print("\n", WHITE);
}
void textedit_load(const char* filename) {
    char buffer[4096];
    int bytes_read = fs_read((char*)filename, buffer, sizeof(buffer)-1, current_dir_id);
    if (bytes_read <= 0) {
        print_error("Cannot read text file");
        return;
    }
    buffer[bytes_read] = '\0';
    textedit_clear();
    char* line_start = buffer;
    int line_num = 0;
    for (int i = 0; i <= bytes_read && line_num < TEXT_EDIT_MAX_LINES; i++) {
        if (buffer[i] == '\n' || buffer[i] == '\0') {
            if (line_start < &buffer[i]) {
                char* format_end = strchr(line_start, '|');
                if (format_end) {
                    int color, alignment, bold, italic, underline;
                    if (sscanf(line_start, "%d,%d,%d,%d,%d", 
                              &color, &alignment, &bold, &italic, &underline) == 5) {
                        text_buffer[line_num].format.color = (text_color_t)color;
                        text_buffer[line_num].format.alignment = (text_align_t)alignment;
                        text_buffer[line_num].format.bold = bold;
                        text_buffer[line_num].format.italic = italic;
                        text_buffer[line_num].format.underline = underline;
                    }
                    char* text_start = format_end + 1;
                    int text_len = &buffer[i] - text_start;
                    if (text_len > 0) {
                        int copy_len = (text_len > TEXT_EDIT_LINE_LENGTH-1) ? TEXT_EDIT_LINE_LENGTH-1 : text_len;
                        strncpy(text_buffer[line_num].text, text_start, copy_len);
                        text_buffer[line_num].text[copy_len] = '\0';
                    }
                    line_num++;
                    current_line_count++;
                }
            }
            line_start = &buffer[i] + 1;
            if (buffer[i] == '\0') break;
        }
    }
    print_success(", Document loaded from ");
    print(filename, GREEN);
    print("\n", WHITE);
}
void textedit_search(const char* pattern) {
    if (current_line_count == 0) {
        print_info("Document is empty");
        return;
    }
    print_info("Search results: ");
    int found = 0;
    for (int i = 0; i < current_line_count; i++) {
        char* pos = strstr(text_buffer[i].text, pattern);
        if (pos) {
            found = 1;
            print("Line ", WHITE);
            print_dec(i + 1, CYAN);
            print(": ", WHITE);
            print(text_buffer[i].text, get_terminal_color(text_buffer[i].format.color));
            print("\n", WHITE);
        }
    }
    if (!found) {
        print("Pattern not found\n", YELLOW);
    }
}
void textedit_replace(const char* old_text, const char* new_text) {
    if (current_line_count == 0) {
        print_error("Document is empty");
        return;
    }
    int replaced = 0;
    for (int i = 0; i < current_line_count; i++) {
        char* pos = strstr(text_buffer[i].text, old_text);
        if (pos) {
            char temp[TEXT_EDIT_LINE_LENGTH];
            strcpy(temp, text_buffer[i].text);
            char result[TEXT_EDIT_LINE_LENGTH] = {0};
            char* current = temp;
            while ((pos = strstr(current, old_text))) {
                strncat(result, current, pos - current);
                strcat(result, new_text);
                current = pos + strlen(old_text);
            }
            strcat(result, current);
            if (strlen(result) < TEXT_EDIT_LINE_LENGTH) {
                strcpy(text_buffer[i].text, result);
                replaced++;
            }
        }
    }
    if (replaced > 0) {
        print_success("Replacements made: ");
        print_dec(replaced, GREEN);
        print("\n", WHITE);
    } else {
        print_info("No replacements made");
    }
}
void textedit_show_help() {
    print_info("Text Editor Commands:");
    print("  insert <line> <text>    - Insert text at line number\n", WHITE);
    print("  append <text>           - Append text to end\n", WHITE);
    print("  delete <line>           - Delete line\n", WHITE);
    print("  format <line> <color> <align> <B> <I> <U> - Set formatting\n", WHITE);
    print("  show                    - Show document\n", WHITE);
    print("  clear                   - Clear document\n", WHITE);
    print("  save <filename>         - Save to file\n", WHITE);
    print("  load <filename>         - Load from file\n", WHITE);
    print("  search <pattern>        - Search text\n", WHITE);
    print("  replace <old> <new>     - Replace text\n", WHITE);
    print("  help                    - Show this help\n", WHITE);
    print("\nColors: 0=Black,1=Blue,2=Green,3=Cyan,4=Red,5=Magenta,6=Brown,7=White\n", GRAY);
    print("        8=Gray,9=LightBlue,10=LightGreen,11=LightCyan,12=LightRed\n", GRAY);
    print("        13=LightMagenta,14=Yellow,15=BrightWhite\n", GRAY);
    print("Align: 0=Left,1=Center,2=Right,3=Justify\n", GRAY);
    print("Format: 0/1 for Bold, Italic, Underline\n", GRAY);
}
