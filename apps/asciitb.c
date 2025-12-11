#include "../libs/print.h"
void show_ascii_table() {
    print_info("\nASCII table (32-127):");
    for (int i = 32; i < 128; i++) {
        if ((i-32) % 16 == 0) print("\n  ", WHITE);
        print_dec(i, CYAN);
        print(": '", WHITE);
        putchar(i, WHITE);
        print("'  ", WHITE);
    }
}