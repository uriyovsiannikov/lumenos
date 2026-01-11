
#include "a20.h"
#include "../../libs/print.h"
#include "../../libs/string.h"
#include "../../sys/syslogger/syslogger.h"
#include "../../sys/io/io.h"
#include <stddef.h>
#define KBC_CMD_PORT  0x64
#define KBC_DATA_PORT 0x60
#define KBC_READ_CMD  0xD0
#define KBC_WRITE_CMD 0xD1
#define KBC_DISABLE   0xAD
#define KBC_ENABLE    0xAE
#define A20_BIT       0x02
static bool wait_kbc_ready(void) {
    uint32_t timeout = 100000;
    while (timeout--) {
        if ((inb(KBC_CMD_PORT) & 0x02) == 0) {
            return true;
        }
        io_wait();
    }
    log_message("A20: KBC timeout waiting for ready", LOG_ERROR);
    return false;
}
static bool wait_kbc_data(void) {
    uint32_t timeout = 100000;
    while (timeout--) {
        if ((inb(KBC_CMD_PORT) & 0x01) != 0) {
            return true;
        }
        io_wait();
    }
    log_message("A20: KBC timeout waiting for data", LOG_ERROR);
    return false;
}
static bool test_a20_simple(void) {
    volatile uint32_t *low_mem = (uint32_t*)0x00000400;
    volatile uint32_t *high_mem = (uint32_t*)0x00100400;
    uint32_t orig_low = *low_mem;
    uint32_t orig_high = *high_mem;
    *low_mem = 0x12345678;
    *high_mem = 0x87654321;
    uint32_t val_low = *low_mem;
    uint32_t val_high = *high_mem;
    *low_mem = orig_low;
    *high_mem = orig_high;
    return val_low != val_high;
}
static bool test_a20_thorough(void) {
    volatile uint32_t *addrs[] = {
        (uint32_t*)0x00050000,
        (uint32_t*)0x00150000,
        (uint32_t*)0x00070000,
        (uint32_t*)0x00170000
    };
    uint32_t saved_values[4];
    for (int i = 0; i < 4; i++) {
        saved_values[i] = *addrs[i];
    }
    *addrs[0] = 0xAA55AA55;
    *addrs[1] = 0x55AA55AA;
    *addrs[2] = 0xDEADBEEF;
    *addrs[3] = 0xCAFEBABE;
    bool wrap_detected = false;
    if (*addrs[0] == *addrs[1]) {
        wrap_detected = true;
    }
    if (*addrs[2] == *addrs[3]) {
        wrap_detected = true;
    }
    for (int i = 0; i < 4; i++) {
        *addrs[i] = saved_values[i];
    }
    return !wrap_detected;
}
static bool enable_a20_kbc(void) {
    log_message("A20: Trying keyboard controller method", LOG_INFO);
    if (!wait_kbc_ready()) return false;
    outb(KBC_CMD_PORT, KBC_DISABLE);
    io_wait();
    if (!wait_kbc_ready()) return false;
    outb(KBC_CMD_PORT, KBC_READ_CMD);
    io_wait();
    if (!wait_kbc_data()) return false;
    uint8_t status = inb(KBC_DATA_PORT);
    io_wait();
    status |= A20_BIT;
    if (!wait_kbc_ready()) return false;
    outb(KBC_CMD_PORT, KBC_WRITE_CMD);
    io_wait();
    if (!wait_kbc_ready()) return false;
    outb(KBC_DATA_PORT, status);
    io_wait();
    if (!wait_kbc_ready()) return false;
    outb(KBC_CMD_PORT, KBC_ENABLE);
    io_wait();
    for (volatile int i = 0; i < 10000; i++);
    log_message("A20: KBC method completed", LOG_INFO);
    return true;
}
static bool enable_a20_fast(void) {
    log_message("A20: Trying fast method (port 0x92)", LOG_INFO);
    uint8_t status = inb(0x92);
    status |= A20_BIT;
    status &= ~0x01;
    outb(0x92, status);
    io_wait();
    for (volatile int i = 0; i < 5000; i++);
    log_message("A20: Fast method completed", LOG_INFO);
    return true;
}
static bool enable_a20_bios(void) {
    log_message("A20: Trying BIOS method", LOG_INFO);
    #ifdef __i386__
    uint32_t result;
    bool success;
    asm volatile (
        "mov $0x2401, %%ax\n\t"
        "int $0x15\n\t"
        "mov $1, %%eax\n\t"
        "jc 1f\n\t"
        "mov $0, %%eax\n\t"
        "1:\n\t"
        : "=a" (result)
        :
        : "cc"
    );
    success = (result == 0);

    if (success) {
        log_message("A20: BIOS method succeeded", LOG_INFO);
        return true;
    } else {
        log_message("A20: BIOS method failed", LOG_WARNING);
        return false;
    }
    #else
    log_message("A20: BIOS method not available on this architecture", LOG_WARNING);
    return false;
    #endif
}
void a20_init(void) {
    log_message("A20: Initializing A20 gate driver", LOG_INFO);
    bool enabled = test_a20_simple();
    if (enabled) {
        log_message("A20: Already enabled", LOG_INFO);
        print_info("A20: Already enabled\n");
    } else {
        log_message("A20: Currently disabled, will enable", LOG_WARNING);
        print_warning("A20: Disabled, enabling...\n");
        if (enable_a20_kbc()) {
            if (test_a20_simple()) {
                log_message("A20: Enabled via KBC method", LOG_INFO);
                print_success("A20: Enabled via KBC\n");
                return;
            }
        }
        if (enable_a20_fast()) {
            if (test_a20_simple()) {
                log_message("A20: Enabled via fast method", LOG_INFO);
                print_success("A20: Enabled via fast method\n");
                return;
            }
        }
        if (enable_a20_bios()) {
            if (test_a20_simple()) {
                log_message("A20: Enabled via BIOS", LOG_INFO);
                print_success("A20: Enabled via BIOS\n");
                return;
            }
        }
        log_message("A20: CRITICAL - Failed to enable A20 gate", LOG_ERROR);
        print_error("A20: FAILED to enable! System may not boot properly.\n");
    }
}
bool a20_enable(void) {
    log_message("A20: Manual enable requested", LOG_INFO);
    if (enable_a20_kbc() && test_a20_thorough()) {
        return true;
    }
    if (enable_a20_fast() && test_a20_thorough()) {
        return true;
    }
    if (enable_a20_bios() && test_a20_thorough()) {
        return true;
    }

    return false;
}
bool a20_disable(void) {
    log_message("A20: Manual disable requested (for testing)", LOG_WARNING);
    uint8_t status = inb(0x92);
    status &= ~A20_BIT;
    outb(0x92, status);
    io_wait();
    return !test_a20_simple();
}
bool a20_is_enabled(void) {
    return test_a20_simple();
}
int a20_get_status(void) {
    if (test_a20_simple()) {
        return A20_ENABLED;
    } else {
        return A20_DISABLED;
    }
}
void a20_test(void) {
    print("\n=== A20 Gate Test ===\n", WHITE);
    print("Simple test: ", WHITE);
    if (test_a20_simple()) {
        print("[ENABLED]\n", GREEN);
    } else {
        print("[DISABLED]\n", RED);
    }
    print("Thorough test: ", WHITE);
    if (test_a20_thorough()) {
        print("[ENABLED]\n", GREEN);
    } else {
        print("[DISABLED]\n", RED);
    }
    print("\nWrap-around demonstration:\n", WHITE);
    volatile uint32_t *addr_1mb = (uint32_t*)0x00100000;  // Ровно 1 МБ
    volatile uint32_t *addr_0mb = (uint32_t*)0x00000000;  // Начало памяти
    uint32_t saved_1mb = *addr_1mb;
    uint32_t saved_0mb = *addr_0mb;
    *addr_1mb = 0xDEADBEEF;
    if (*addr_0mb == 0xDEADBEEF) {
        print("  Write to 1MB, read from 0MB: MATCH (A20 disabled)\n", YELLOW);
    } else {
        print("  Write to 1MB, read from 0MB: DIFFERENT (A20 enabled)\n", CYAN);
    }
    *addr_1mb = saved_1mb;
    *addr_0mb = saved_0mb;
    print("=== Test Complete ===\n\n", WHITE);
}
bool a20_test_kbc_method(void) {
    log_message("A20: Testing KBC method only", LOG_INFO);
    bool was_enabled = a20_is_enabled();
    if (was_enabled) {
        a20_disable();
    }
    bool result = enable_a20_kbc();
    bool now_enabled = a20_is_enabled();

    if (!was_enabled) {
        a20_disable();
    }
    return result && now_enabled;
}
bool a20_test_fast_method(void) {
    log_message("A20: Testing fast method only", LOG_INFO);
    bool was_enabled = a20_is_enabled();
    if (was_enabled) {
        a20_disable();
    }
    bool result = enable_a20_fast();
    bool now_enabled = a20_is_enabled();

    if (!was_enabled) {
        a20_disable();
    }
    return result && now_enabled;
}
bool a20_test_bios_method(void) {
    log_message("A20: Testing BIOS method only", LOG_INFO);
    bool was_enabled = a20_is_enabled();
    if (was_enabled) {
        a20_disable();
    }
    bool result = enable_a20_bios();
    bool now_enabled = a20_is_enabled();

    if (!was_enabled) {
        a20_disable();
    }
    return result && now_enabled;
}
