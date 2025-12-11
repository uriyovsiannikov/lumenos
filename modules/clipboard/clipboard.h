#ifndef CLIPBOARD_H
#define CLIPBOARD_H
#define CLIPBOARD_SIZE 255
void copy_to_clipboard(const char* text);
void paste_from_clipboard(void);
void show_clipboard(void);
#endif 