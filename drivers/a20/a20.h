#ifndef A20_H
#define A20_H
#include <stdbool.h>
#include <stdint.h>
#define A20_DISABLED 0
#define A20_ENABLED  1
#define A20_UNKNOWN  -1
void a20_init(void);
bool a20_enable(void);
bool a20_disable(void);
bool a20_is_enabled(void);
int a20_get_status(void);
void a20_test(void);
bool a20_test_kbc_method(void);
bool a20_test_fast_method(void);
bool a20_test_bios_method(void);
#endif