#include "lpt.h"
#include "../../sys/io/io.h"
#include "../../libs/print.h"
#include "../../libs/string.h"
#include "../../libs/ctype.h"
#include "../../sys/syslogger/syslogger.h"
static lpt_port_t lpt_ports[3] = {
    {LPT1_BASE, "LPT1", false, false, false},
    {LPT2_BASE, "LPT2", false, false, false},
    {LPT3_BASE, "LPT3", false, false, false}
};
static void print_hex_padded(uint32_t num, uint8_t digits, uint8_t color) {
    char hex_chars[] = "0123456789ABCDEF";
    char buffer[9];
    int i;
    for (i = 7; i >= 0; i--) {
        uint8_t nibble = (num >> (i * 4)) & 0xF;
        buffer[7 - i] = hex_chars[nibble];
    }
    buffer[8] = '\0';
    print("0x", color);
    print(&buffer[8 - digits], color);
}
void lpt_init(void) {
    print("Initializing LPT (Parallel Port)...", WHITE);
    for (int i = 0; i < 3; i++) {
        uint16_t base = lpt_ports[i].base_port;
        outb(base + LPT_DATA_REG, 0xAA);
        uint8_t read_back = inb(base + LPT_DATA_REG);
        if (read_back != 0xFF) {
            lpt_ports[i].detected = true;
            outb(base + LPT_DATA_REG, 0x00);
            uint8_t status = inb(base + LPT_STATUS_REG);
            uint8_t control = inb(base + LPT_CONTROL_REG);
            if ((status != 0xFF) && (control != 0xFF)) {
                print(" [", GREEN);
                print(lpt_ports[i].name, GREEN);
                print(" FOUND]", GREEN);
            }
        }
    }
    print("\n", WHITE);
    log_message("LPT initialization complete", LOG_INFO);
}
bool lpt_detect_ports(void) {
    bool any_detected = false;
    print("Detecting parallel ports:\n", WHITE);
    for (int i = 0; i < 3; i++) {
        lpt_port_t* port = &lpt_ports[i];
        outb(port->base_port + LPT_DATA_REG, 0x55);
        uint8_t val1 = inb(port->base_port + LPT_DATA_REG);
        outb(port->base_port + LPT_DATA_REG, 0xAA);
        uint8_t val2 = inb(port->base_port + LPT_DATA_REG);
        uint8_t status = inb(port->base_port + LPT_STATUS_REG);
        if ((val1 != 0xFF && val1 != 0x00) ||
            (val2 != 0xFF && val2 != 0x00) ||
            (status != 0xFF && status != 0x00)) {
            port->detected = true;
            any_detected = true;
            print("  ", WHITE);
            print(port->name, GREEN);
            print(" at ", WHITE);
            print_hex_padded(port->base_port, 4, LIGHT_CYAN);
            print(" [DETECTED]\n", GREEN);
        } else {
            print("  ", WHITE);
            print(port->name, WHITE);
            print("  ", WHITE);
            print_hex_padded(port->base_port, 4, WHITE);
            print(" [NOT FOUND]\n", RED);
        }
        outb(port->base_port + LPT_DATA_REG, 0x00);
    }
    return any_detected;
}
lpt_port_t* lpt_get_port(int port_num) {
    if (port_num >= 1 && port_num <= 3) {
        return &lpt_ports[port_num - 1];
    }
    return NULL;
}
bool lpt_send_byte(uint16_t port, uint8_t data) {
    int timeout = 100000;
    while (timeout-- > 0) {
        uint8_t status = inb(port + LPT_STATUS_REG);
        if (!(status & LPT_STATUS_BUSY) && (status & LPT_STATUS_ACK)) {
            break;
        }
        if (timeout == 0) {
            log_message("LPT send timeout - printer busy", LOG_WARNING);
            return false;
        }
    }
    outb(port + LPT_DATA_REG, data);
    uint8_t control = inb(port + LPT_CONTROL_REG);
    outb(port + LPT_CONTROL_REG, control | LPT_CTRL_STROBE);
    for (volatile int i = 0; i < 100; i++);
    outb(port + LPT_CONTROL_REG, control & ~LPT_CTRL_STROBE);
    timeout = 100000;
    while (timeout-- > 0) {
        uint8_t status = inb(port + LPT_STATUS_REG);
        if (status & LPT_STATUS_ACK) {
            break;
        }
        if (timeout == 0) {
            log_message("LPT no ACK received", LOG_WARNING);
            return false;
        }
    }
    return true;
}
bool lpt_send_string(uint16_t port, const char* str) {
    if (!str) return false;
    bool success = true;
    const char* ptr = str;
    while (*ptr) {
        if (!lpt_send_byte(port, *ptr)) {
            success = false;
            break;
        }
        ptr++;
    }
    if (success) {
        lpt_send_byte(port, '\r');
        lpt_send_byte(port, '\n');
    }
    return success;
}
uint8_t lpt_read_status(uint16_t port) {
    return inb(port + LPT_STATUS_REG);
}
void lpt_print_status(uint16_t port) {
    uint8_t status = lpt_read_status(port);
    print("LPT Status at ", WHITE);
    print_hex_padded(port, 4, LIGHT_CYAN);
    print(": ", WHITE);
    print_hex_padded(status, 2, WHITE);
    print("\n", WHITE);
    print("  Busy:      ", WHITE);
    print(status & LPT_STATUS_BUSY ? "YES" : "NO",
          status & LPT_STATUS_BUSY ? YELLOW : GREEN);
    print("\n", WHITE);
    print("  Ack:       ", WHITE);
    print(status & LPT_STATUS_ACK ? "YES" : "NO",
          status & LPT_STATUS_ACK ? GREEN : YELLOW);
    print("\n", WHITE);
    print("  Paper Out: ", WHITE);
    print(status & LPT_STATUS_PAPEROUT ? "YES" : "NO",
          status & LPT_STATUS_PAPEROUT ? RED : GREEN);
    print("\n", WHITE);
    print("  Selected:  ", WHITE);
    print(status & LPT_STATUS_SELECT ? "YES" : "NO",
          status & LPT_STATUS_SELECT ? GREEN : YELLOW);
    print("\n", WHITE);
    print("  Error:     ", WHITE);
    print(status & LPT_STATUS_ERROR ? "YES" : "NO",
          status & LPT_STATUS_ERROR ? RED : GREEN);
    print("\n", WHITE);
}
void lpt_initialize_printer(uint16_t port) {
    uint8_t control = inb(port + LPT_CONTROL_REG);
    outb(port + LPT_CONTROL_REG, control & ~LPT_CTRL_INIT);
    for (volatile int i = 0; i < 1000; i++);
    outb(port + LPT_CONTROL_REG, control | LPT_CTRL_INIT);
    outb(port + LPT_CONTROL_REG, (control | LPT_CTRL_INIT | LPT_CTRL_SELECTIN) & ~LPT_CTRL_AUTOFD);
    log_message("LPT printer initialized", LOG_INFO);
}
bool lpt_test_port(uint16_t port) {
    print("Testing LPT port ", WHITE);
    print_hex_padded(port, 4, LIGHT_CYAN);
    print("...\n", WHITE);
    lpt_print_status(port);
    lpt_initialize_printer(port);
    print("  Sending test pattern... ", WHITE);
    bool success = true;
    uint8_t test_pattern[] = {0x55, 0xAA, 0x00, 0xFF};
    for (int i = 0; i < 4; i++) {
        if (!lpt_send_byte(port, test_pattern[i])) {
            success = false;
            break;
        }
    }
    if (success) {
        print("[OK]\n", GREEN);
        print("  Port is working correctly\n", GREEN);
    } else {
        print("[FAILED]\n", RED);
        print("  Port test failed\n", RED);
    }
    return success;
}
void lpt_set_irq_enabled(uint16_t port, bool enabled) {
    uint8_t control = inb(port + LPT_CONTROL_REG);
    if (enabled) {
        outb(port + LPT_CONTROL_REG, control | LPT_CTRL_IRQENABLE);
        log_message("LPT interrupts enabled", LOG_INFO);
    } else {
        outb(port + LPT_CONTROL_REG, control & ~LPT_CTRL_IRQENABLE);
        log_message("LPT interrupts disabled", LOG_INFO);
    }
}
void lpt_command(const char* args) {
    while (*args == ' ') args++;
    if (*args == '\0') {
        print_error("LPT command expected");
        print("Try: lpt detect, lpt status, lpt help\n", WHITE);
        return;
    }
    char subcmd[32] = {0};
    int i = 0;
    while (*args != ' ' && *args != '\0' && i < 31) {
        subcmd[i++] = *args++;
    }
    subcmd[i] = '\0';
    while (*args == ' ') args++;
    if (strcmp(subcmd, "help") == 0 || strcmp(subcmd, "?") == 0) {
        print_info("LPT Commands Help:");
        print("\n  detect    - Scan for parallel ports", WHITE);
        print("\n  status    - Show port status", WHITE);
        print("\n  test      - Run port self-test", WHITE);
        print("\n  init      - Initialize printer", WHITE);
        print("\n  write     - Send text string", WHITE);
        print("\n  hex       - Send hex byte", WHITE);
        print("\n  raw       - Send decimal byte", WHITE);
        print("\n  irq       - Control interrupts", WHITE);
    }
    else if (strcmp(subcmd, "detect") == 0) {
        lpt_detect_ports();
    }
    else if (strcmp(subcmd, "status") == 0) {
        int port_num = 1; // по умолчанию LPT1
        if (*args != '\0') {
            port_num = atoi(args);
        }
        lpt_port_t* port = lpt_get_port(port_num);
        if (port && port->detected) {
            print("Status for ", WHITE);
            print(port->name, LIGHT_CYAN);
            print(":\n", WHITE);
            lpt_print_status(port->base_port);
        } else {
            print_error("Port not detected");
        }
    }
    else if (strcmp(subcmd, "test") == 0) {
        int port_num = 1;
        if (*args != '\0') {
            port_num = atoi(args);
        }
        lpt_port_t* port = lpt_get_port(port_num);
        if (port && port->detected) {
            lpt_test_port(port->base_port);
        } else {
            print_error("Port not detected");
        }
    }
    else if (strcmp(subcmd, "init") == 0) {
        int port_num = 1;
        if (*args != '\0') {
            port_num = atoi(args);
        }
        lpt_port_t* port = lpt_get_port(port_num);
        if (port && port->detected) {
            lpt_initialize_printer(port->base_port);
            print_success("Printer initialized");
        } else {
            print_error("Port not detected");
        }
    }
    else if (strcmp(subcmd, "write") == 0) {
        char port_str[10] = {0};
        char text[256] = {0};
        i = 0;
        while (*args != ' ' && *args != '\0' && i < 9) {
            port_str[i++] = *args++;
        }
        port_str[i] = '\0';
        while (*args == ' ') args++;
        i = 0;
        while (*args != '\0' && i < 255) {
            text[i++] = *args++;
        }
        text[i] = '\0';
        if (strlen(port_str) > 0 && strlen(text) > 0) {
            int port_num = atoi(port_str);
            lpt_port_t* port = lpt_get_port(port_num);
            if (port && port->detected) {
                print("Sending to ", WHITE);
                print(port->name, LIGHT_CYAN);
                print(": \"", WHITE);
                print(text, LIGHT_GREEN);
                print("\"\n", WHITE);
                if (lpt_send_string(port->base_port, text)) {
                    print_success("Text sent");
                } else {
                    print_error("Failed");
                }
            } else {
                print_error("Port not detected");
            }
        } else {
            print_error("Usage: lpt write <port> <text>");
        }
    }
    else if (strcmp(subcmd, "hex") == 0) {
        char port_str[10] = {0};
        char hex_str[10] = {0};
        i = 0;
        while (*args != ' ' && *args != '\0' && i < 9) {
            port_str[i++] = *args++;
        }
        port_str[i] = '\0';
        while (*args == ' ') args++;
        i = 0;
        while (*args != ' ' && *args != '\0' && i < 9) {
            hex_str[i++] = *args++;
        }
        hex_str[i] = '\0';
        if (strlen(port_str) > 0 && strlen(hex_str) > 0) {
            int port_num = atoi(port_str);
            uint8_t data = (uint8_t)strtol(hex_str, NULL, 16);
            lpt_port_t* port = lpt_get_port(port_num);
            if (port && port->detected) {
                print("Sending ", WHITE);
                print_hex(data, WHITE);
                print(" to ", WHITE);
                print(port->name, LIGHT_CYAN);
                print("... ", WHITE);
                if (lpt_send_byte(port->base_port, data)) {
                    print_success("OK");
                } else {
                    print_error("FAILED");
                }
            } else {
                print_error("Port not detected");
            }
        } else {
            print_error("Usage: lpt hex <port> <hex_value>");
        }
    }
    else if (strcmp(subcmd, "raw") == 0) {
        char port_str[10] = {0};
        char dec_str[10] = {0};
        i = 0;
        while (*args != ' ' && *args != '\0' && i < 9) {
            port_str[i++] = *args++;
        }
        port_str[i] = '\0';
        while (*args == ' ') args++;
        i = 0;
        while (*args != ' ' && *args != '\0' && i < 9) {
            dec_str[i++] = *args++;
        }
        dec_str[i] = '\0';
        if (strlen(port_str) > 0 && strlen(dec_str) > 0) {
            int port_num = atoi(port_str);
            uint8_t data = (uint8_t)atoi(dec_str);
            lpt_port_t* port = lpt_get_port(port_num);
            if (port && port->detected) {
                print("Sending byte ", WHITE);
                print_dec(data, WHITE);
                print(" (", WHITE);
                print_hex(data, WHITE);
                print(") to ", WHITE);
                print(port->name, LIGHT_CYAN);
                print("... ", WHITE);
                if (lpt_send_byte(port->base_port, data)) {
                    print_success("OK");
                } else {
                    print_error("FAILED");
                }
            } else {
                print_error("Port not detected");
            }
        } else {
            print_error("Usage: lpt raw <port> <decimal>");
        }
    }
    else if (strcmp(subcmd, "irq") == 0) {
        char port_str[10] = {0};
        char state_str[10] = {0};
        i = 0;
        while (*args != ' ' && *args != '\0' && i < 9) {
            port_str[i++] = *args++;
        }
        port_str[i] = '\0';
        while (*args == ' ') args++;
        i = 0;
        while (*args != ' ' && *args != '\0' && i < 9) {
            state_str[i++] = *args++;
        }
        state_str[i] = '\0';
        if (strlen(port_str) > 0 && strlen(state_str) > 0) {
            int port_num = atoi(port_str);
            lpt_port_t* port = lpt_get_port(port_num);
            if (port && port->detected) {
                if (strcmp(state_str, "on") == 0 || strcmp(state_str, "enable") == 0) {
                    lpt_set_irq_enabled(port->base_port, true);
                    print_success("Interrupts enabled");
                }
                else if (strcmp(state_str, "off") == 0 || strcmp(state_str, "disable") == 0) {
                    lpt_set_irq_enabled(port->base_port, false);
                    print_success("Interrupts disabled");
                }
                else {
                    print_error("Invalid state. Use 'on' or 'off'");
                }
            } else {
                print_error("Port not detected");
            }
        } else {
            print_error("Usage: lpt irq <port> <on|off>");
        }
    }
    else {
        print_error("Unknown LPT command: '");
        print(subcmd, RED);
        print("'", RED);
        print("\nType 'lpt help' for commands\n", WHITE);
    }
}