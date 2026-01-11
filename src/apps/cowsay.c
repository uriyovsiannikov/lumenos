#include "../libs/print.h"
#include "../libs/string.h"
#include <stddef.h>
#define MAX_TEXT_LENGTH 256
#define BUBBLE_WIDTH 40
void cowsay_command(const char* message) {
    if (message == NULL || strlen(message) == 0) {
        print_info("Usage: cowsay <message>");
        return;
    }
    char text[MAX_TEXT_LENGTH] = "";
    strncpy(text, message, MAX_TEXT_LENGTH - 1);
    text[MAX_TEXT_LENGTH - 1] = '\0';
    if (strlen(text) > BUBBLE_WIDTH) {
        text[BUBBLE_WIDTH - 3] = '.';
        text[BUBBLE_WIDTH - 2] = '.';
        text[BUBBLE_WIDTH - 1] = '.';
        text[BUBBLE_WIDTH] = '\0';
    }
    int text_len = strlen(text);
    print(" ", WHITE);
    for (int i = 0; i < text_len + 2; i++) {
        print("_", WHITE);
    }
    print(" \n", WHITE);
    print("< ", WHITE);
    print(text, CYAN);
    print(" >\n", WHITE);
    print(" ", WHITE);
    for (int i = 0; i < text_len + 2; i++) {
        print("-", WHITE);
    }
    print(" \n", WHITE);
    print("        \\   ^__^\n", WHITE);
    print("         \\  (oo)\\_______\n", WHITE);
    print("            (__)\\       )\\/\\\n", WHITE);
    print("                ||----w |\n", WHITE);
    print("                ||     ||\n", WHITE);
}