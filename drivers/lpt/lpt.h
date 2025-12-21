#ifndef LPT_H
#define LPT_H
#include <stdint.h>
#include <stdbool.h>
#define LPT1_BASE 0x378
#define LPT2_BASE 0x278
#define LPT3_BASE 0x3BC
#define LPT_DATA_REG   0
#define LPT_STATUS_REG 1
#define LPT_CONTROL_REG 2
#define LPT_STATUS_ERROR   0x08
#define LPT_STATUS_SELECT  0x10
#define LPT_STATUS_PAPEROUT 0x20
#define LPT_STATUS_ACK     0x40
#define LPT_STATUS_BUSY    0x80
#define LPT_CTRL_STROBE    0x01
#define LPT_CTRL_AUTOFD    0x02
#define LPT_CTRL_INIT      0x04
#define LPT_CTRL_SELECTIN  0x08
#define LPT_CTRL_IRQENABLE 0x10
typedef struct {
    uint16_t base_port;
    char name[8];
    bool detected;
    bool supports_ecp;
    bool supports_epp;
} lpt_port_t;
void lpt_init(void);
bool lpt_detect_ports(void);
lpt_port_t* lpt_get_port(int port_num);
bool lpt_send_byte(uint16_t port, uint8_t data);
bool lpt_send_string(uint16_t port, const char* str);
void lpt_print_status(uint16_t port);
uint8_t lpt_read_status(uint16_t port);
void lpt_initialize_printer(uint16_t port);
bool lpt_test_port(uint16_t port);
void lpt_set_irq_enabled(uint16_t port, bool enabled);
#endif // LPT_H