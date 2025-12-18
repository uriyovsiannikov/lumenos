#ifndef PCI_H
#define PCI_H

#include <stdint.h>
#include <stdbool.h>

// Структура описания PCI-устройства
typedef struct pci_device {
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t class_code;
    uint8_t subclass;
    uint8_t prog_if;
    uint8_t revision_id;
    uint8_t bus;
    uint8_t device;
    uint8_t function;
    uint32_t bar[6];
    uint8_t interrupt_line;
    uint8_t interrupt_pin;
    struct pci_device *next;
} pci_device_t;

// Классы PCI-устройств
#define PCI_CLASS_UNCLASSIFIED      0x00
#define PCI_CLASS_MASS_STORAGE      0x01
#define PCI_CLASS_NETWORK           0x02
#define PCI_CLASS_DISPLAY           0x03
#define PCI_CLASS_MULTIMEDIA        0x04
#define PCI_CLASS_MEMORY            0x05
#define PCI_CLASS_BRIDGE            0x06
#define PCI_CLASS_SIMPLE_COMM       0x07
#define PCI_CLASS_BASE_PERIPHERAL   0x08
#define PCI_CLASS_INPUT             0x09
#define PCI_CLASS_DOCKING_STATION   0x0A
#define PCI_CLASS_PROCESSOR         0x0B
#define PCI_CLASS_SERIAL_BUS        0x0C
#define PCI_CLASS_WIRELESS          0x0D
#define PCI_CLASS_INTELLIGENT_IO    0x0E
#define PCI_CLASS_SATELLITE_COMM    0x0F
#define PCI_CLASS_ENCRYPTION        0x10
#define PCI_CLASS_SIGNAL_PROCESSING 0x11
#define PCI_CLASS_PROCESSING_ACCEL  0x12
#define PCI_CLASS_NON_ESSENTIAL     0xFF

// Подклассы для сетевых устройств
#define PCI_SUBCLASS_NETWORK_ETHERNET   0x00
#define PCI_SUBCLASS_NETWORK_TOKEN_RING 0x01
#define PCI_SUBCLASS_NETWORK_FDDI       0x02
#define PCI_SUBCLASS_NETWORK_ATM        0x03
#define PCI_SUBCLASS_NETWORK_OTHER      0x80

// Подклассы для Mass Storage
#define PCI_SUBCLASS_STORAGE_SCSI       0x00
#define PCI_SUBCLASS_STORAGE_IDE        0x01
#define PCI_SUBCLASS_STORAGE_FLOPPY     0x02
#define PCI_SUBCLASS_STORAGE_IPI        0x03
#define PCI_SUBCLASS_STORAGE_RAID       0x04
#define PCI_SUBCLASS_STORAGE_AHCI       0x06
#define PCI_SUBCLASS_STORAGE_SATA       0x06
#define PCI_SUBCLASS_STORAGE_OTHER      0x80

// Конфигурационные порты PCI
#define PCI_CONFIG_ADDRESS  0xCF8
#define PCI_CONFIG_DATA     0xCFC

// Макросы для работы с регистрами PCI
#define PCI_MAKE_ADDRESS(bus, dev, func, offset) \
    (0x80000000 | ((bus) << 16) | ((dev) << 11) | ((func) << 8) | ((offset) & 0xFC))

// Стандартные регистры PCI конфигурационного пространства
#define PCI_REGISTER_VENDOR_ID         0x00
#define PCI_REGISTER_DEVICE_ID         0x02
#define PCI_REGISTER_COMMAND           0x04
#define PCI_REGISTER_STATUS            0x06
#define PCI_REGISTER_REVISION_ID       0x08
#define PCI_REGISTER_PROG_IF           0x09
#define PCI_REGISTER_SUBCLASS          0x0A
#define PCI_REGISTER_CLASS_CODE        0x0B
#define PCI_REGISTER_CACHE_LINE_SIZE   0x0C
#define PCI_REGISTER_LATENCY_TIMER     0x0D
#define PCI_REGISTER_HEADER_TYPE       0x0E
#define PCI_REGISTER_BIST              0x0F
#define PCI_REGISTER_BAR0              0x10
#define PCI_REGISTER_BAR1              0x14
#define PCI_REGISTER_BAR2              0x18
#define PCI_REGISTER_BAR3              0x1C
#define PCI_REGISTER_BAR4              0x20
#define PCI_REGISTER_BAR5              0x24
#define PCI_REGISTER_INTERRUPT_LINE    0x3C
#define PCI_REGISTER_INTERRUPT_PIN     0x3D

// Прототипы функций
void pci_init(void);
void pci_scan_bus(uint8_t bus);
void pci_scan_all(void);
void pci_print_devices(void);
pci_device_t* pci_get_device_list(void);
pci_device_t* pci_find_device(uint16_t vendor_id, uint16_t device_id);
pci_device_t* pci_find_class(uint8_t class_code, uint8_t subclass);
uint32_t pci_read_config(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);
void pci_write_config(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value);
const char* pci_get_class_name(uint8_t class_code);
const char* pci_get_subclass_name(uint8_t class_code, uint8_t subclass);
const char* pci_get_vendor_name(uint16_t vendor_id);

#endif // PCI_H