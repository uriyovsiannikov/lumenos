#include "../libs/print.h"
#include "../modules/syslogger/syslogger.h"
#include <stddef.h>
#include <string.h>
#include "clipboard.h"
static uint8_t clipboard_length = 0;
static char clipboard[CLIPBOARD_SIZE + 1] = {0};
void copy_to_clipboard(const char* text) {
    size_t len = strlen(text);
    if (len > CLIPBOARD_SIZE) {
        print("Error: Text too long (max ", RED);
        print_dec(CLIPBOARD_SIZE, RED);
        print(" chars)\n", RED);
		log_message("Clipboard limit reached",LOG_ERROR);
        return;
    }
    strncpy(clipboard, text, CLIPBOARD_SIZE);
    clipboard[CLIPBOARD_SIZE] = '\0';
    clipboard_length = (uint8_t)len;
    print("Copied ", GREEN);
    print_dec((uint8_t)len, GREEN);
    print(" chars to clipboard\n", GREEN);
}
void paste_from_clipboard(void) {
    if (clipboard_length == 0) {
        print("Clipboard is empty\n", RED);
        return;
    }
    print("Pasted: ", LIGHT_GREEN);
    print(clipboard, WHITE);
    putchar('\n', WHITE);
}
void show_clipboard(void) {
    if (clipboard_length == 0) {
        print("Clipboard is empty\n", WHITE);
        return;
    }
    print("Clipboard (", WHITE);
    print_dec(clipboard_length, WHITE);
    print(" chars): \"", WHITE);
    print(clipboard, CYAN);
    print("\"\n", WHITE);
}