#include "../libs/print.h"
#include "../libs/string.h"
#include <stddef.h>

void lfetch_command() {
    print_reg("     /\\"    , RED); print("      : Console: /dev/tty1\n",WHITE);
    print_reg(" .--/--\\--,", RED); print("  : Uptime: 0d0h0s\n",WHITE);
    print_reg("  \\/    \\/", RED); print("   : Resolution: 80x25 VGA\n",WHITE);
    print_reg("  /\\    /\\", RED); print("   : CPU: Intel Core 2 Duo\n",WHITE);
    print_reg(" '--\\--/--`", RED); print("  : Memory: 10 / 100 Free\n",WHITE);
    print_reg("     \\/    ", RED); print("  : Kernel: LumenOS 1.2\n",WHITE);
}