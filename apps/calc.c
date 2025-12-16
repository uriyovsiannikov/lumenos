#include <stdint.h>
#include "../libs/print.h"
#include "../libs/string.h"
#include "../modules/syslogger/syslogger.h"
#include "../include/stdio.h"
void calc(const char* expr) {
    int a, b;
    char op;
    if (sscanf(expr, "%d %c %d", &a, &op, &b) == 3) {
    } else if (sscanf(expr, "%d%c%d", &a, &op, &b) == 3) {
    } else {
        print("Invalid expression! Usage: calc NUMBER OPERATOR NUMBER\n", RED);
        print("Examples:\n", WHITE);
        print("  calc 9-6\n", WHITE);
        print("  calc 5 + 3\n", WHITE);
        return;
    }
    int result;
    switch(op) {
        case '+': result = a + b; break;
        case '-': result = a - b; break;
        case '*': result = a * b; break;
        case '/':
            if (b == 0) {
                print("Error: Division by zero!\n", RED);
                log_message("Calc DBZ error", LOG_ERROR);
                return;
            }
            result = a / b;
            break;
        default:
            print("Unknown operator: '", RED);
            putchar(op, WHITE);
            print("'\nSupported: + - * /\n", WHITE);
            return;
    }
    print_dec(a, CYAN);
    putchar(' ', WHITE);
    putchar(op, WHITE);
    putchar(' ', WHITE);
    print_dec(b, CYAN);
    print(" = ", WHITE);
    print_dec(result, GREEN);
    print("\n", WHITE);
}