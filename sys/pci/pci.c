#include "pci.h"
#include "../sys/syslogger/syslogger.h"
#include "../sys/mm/mm.h"
#include "../libs/print.h"
#include "../libs/string.h"
#include "../sys/io/io.h"
#include <stddef.h>
static pci_device_t *pci_devices = NULL;
static void pci_add_device(pci_device_t *dev) {
    dev->next = pci_devices;
    pci_devices = dev;
}
uint32_t pci_read_config(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    uint32_t address = PCI_MAKE_ADDRESS(bus, device, function, offset);
    outl(PCI_CONFIG_ADDRESS, address);
    uint32_t result = inl(PCI_CONFIG_DATA);
    return result >> (8 * (offset & 3));
}
void pci_write_config(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value) {
    uint32_t address = PCI_MAKE_ADDRESS(bus, device, function, offset);
    outl(PCI_CONFIG_ADDRESS, address);
    outl(PCI_CONFIG_DATA, value);
}
static bool pci_device_exists(uint8_t bus, uint8_t device, uint8_t function) {
    uint32_t vendor_device = pci_read_config(bus, device, function, PCI_REGISTER_VENDOR_ID);
    return (vendor_device != 0xFFFFFFFF && (vendor_device & 0xFFFF) != 0xFFFF);
}
static bool pci_is_multifunction(uint8_t bus, uint8_t device, uint8_t function) {
    uint8_t header_type = pci_read_config(bus, device, function, PCI_REGISTER_HEADER_TYPE);
    return (header_type & 0x80) != 0;
}
static void pci_scan_device(uint8_t bus, uint8_t device) {
    if (!pci_device_exists(bus, device, 0)) {
        return;
    }
    uint8_t function = 0;
    uint8_t max_functions = 1;
    if (pci_is_multifunction(bus, device, 0)) {
        max_functions = 8;
    }
    for (function = 0; function < max_functions; function++) {
        if (!pci_device_exists(bus, device, function)) {
            continue;
        }
        uint32_t vendor_device = pci_read_config(bus, device, function, PCI_REGISTER_VENDOR_ID);
        uint16_t vendor_id = vendor_device & 0xFFFF;
        uint16_t device_id = vendor_device >> 16;
        if (vendor_id == 0xFFFF) {
            continue;
        }
        uint32_t class_reg = pci_read_config(bus, device, function, PCI_REGISTER_CLASS_CODE);
        uint8_t class_code = (class_reg >> 24) & 0xFF;
        uint8_t subclass = (class_reg >> 16) & 0xFF;
        uint8_t prog_if = (class_reg >> 8) & 0xFF;
        uint8_t revision_id = class_reg & 0xFF;
        pci_device_t *dev = (pci_device_t*)malloc(sizeof(pci_device_t));
        if (!dev) {
            log_message("Failed to allocate memory for PCI device", LOG_ERROR);
            continue;
        }
        for (int i = 0; i < sizeof(pci_device_t); i++) {
            ((uint8_t*)dev)[i] = 0;
        }
        dev->vendor_id = vendor_id;
        dev->device_id = device_id;
        dev->class_code = class_code;
        dev->subclass = subclass;
        dev->prog_if = prog_if;
        dev->revision_id = revision_id;
        dev->bus = bus;
        dev->device = device;
        dev->function = function;
        for (int i = 0; i < 6; i++) {
            dev->bar[i] = pci_read_config(bus, device, function, PCI_REGISTER_BAR0 + i * 4);
        }
        dev->interrupt_line = pci_read_config(bus, device, function, PCI_REGISTER_INTERRUPT_LINE) & 0xFF;
        dev->interrupt_pin = pci_read_config(bus, device, function, PCI_REGISTER_INTERRUPT_PIN) & 0xFF;
        dev->next = NULL;
        pci_add_device(dev);
        char log_msg[128];
        snprintf(log_msg, sizeof(log_msg),
                 "PCI: %02x:%02x.%01x %04x:%04x class %02x/%02x",
                 bus, device, function, vendor_id, device_id, class_code, subclass);
        log_message(log_msg, LOG_INFO);
        if (class_code == PCI_CLASS_BRIDGE && subclass == 0x04) { // PCI-to-PCI bridge
            uint8_t secondary_bus = pci_read_config(bus, device, function, 0x19) & 0xFF;
            pci_scan_bus(secondary_bus);
        }
    }
}
void pci_scan_bus(uint8_t bus) {
    for (uint8_t device = 0; device < 32; device++) {
        pci_scan_device(bus, device);
    }
}
void pci_scan_all(void) {
    log_message("Starting PCI bus scan...", LOG_INFO);
    if (pci_is_multifunction(0, 0, 0)) {
        for (uint8_t function = 0; function < 8; function++) {
            if (pci_device_exists(0, 0, function)) {
                uint8_t header_type = pci_read_config(0, 0, function, PCI_REGISTER_HEADER_TYPE) & 0x7F;
                if (header_type == 0x01) { // PCI-to-PCI bridge
                    uint8_t secondary_bus = pci_read_config(0, 0, function, 0x19) & 0xFF;
                    pci_scan_bus(secondary_bus);
                }
            }
        }
    } else {
        pci_scan_bus(0);
    }
    log_message("PCI bus scan completed", LOG_INFO);
}
pci_device_t* pci_get_device_list(void) {
    return pci_devices;
}
pci_device_t* pci_find_device(uint16_t vendor_id, uint16_t device_id) {
    pci_device_t *dev = pci_devices;
    while (dev) {
        if (dev->vendor_id == vendor_id && dev->device_id == device_id) {
            return dev;
        }
        dev = dev->next;
    }
    return NULL;
}
pci_device_t* pci_find_class(uint8_t class_code, uint8_t subclass) {
    pci_device_t *dev = pci_devices;
    while (dev) {
        if (dev->class_code == class_code && dev->subclass == subclass) {
            return dev;
        }
        dev = dev->next;
    }
    return NULL;
}
const char* pci_get_class_name(uint8_t class_code) {
    switch (class_code) {
        case PCI_CLASS_UNCLASSIFIED:      return "Unclassified";
        case PCI_CLASS_MASS_STORAGE:      return "Mass Storage";
        case PCI_CLASS_NETWORK:           return "Network";
        case PCI_CLASS_DISPLAY:           return "Display";
        case PCI_CLASS_MULTIMEDIA:        return "Multimedia";
        case PCI_CLASS_MEMORY:            return "Memory";
        case PCI_CLASS_BRIDGE:            return "Bridge";
        case PCI_CLASS_SIMPLE_COMM:       return "Simple Communication";
        case PCI_CLASS_BASE_PERIPHERAL:   return "Base Peripheral";
        case PCI_CLASS_INPUT:             return "Input";
        case PCI_CLASS_DOCKING_STATION:   return "Docking Station";
        case PCI_CLASS_PROCESSOR:         return "Processor";
        case PCI_CLASS_SERIAL_BUS:        return "Serial Bus";
        case PCI_CLASS_WIRELESS:          return "Wireless";
        case PCI_CLASS_INTELLIGENT_IO:    return "Intelligent I/O";
        case PCI_CLASS_SATELLITE_COMM:    return "Satellite Communication";
        case PCI_CLASS_ENCRYPTION:        return "Encryption";
        case PCI_CLASS_SIGNAL_PROCESSING: return "Signal Processing";
        case PCI_CLASS_PROCESSING_ACCEL:  return "Processing Accelerator";
        default:                          return "Unknown";
    }
}
const char* pci_get_subclass_name(uint8_t class_code, uint8_t subclass) {
    switch (class_code) {
        case PCI_CLASS_MASS_STORAGE:
            switch (subclass) {
                case PCI_SUBCLASS_STORAGE_SCSI:   return "SCSI";
                case PCI_SUBCLASS_STORAGE_IDE:    return "IDE";
                case PCI_SUBCLASS_STORAGE_FLOPPY: return "Floppy";
                case PCI_SUBCLASS_STORAGE_RAID:   return "RAID";
                case PCI_SUBCLASS_STORAGE_AHCI:   return "AHCI/SATA";
                case PCI_SUBCLASS_STORAGE_OTHER:  return "Other";
                default: return "Unknown";
            }
        case PCI_CLASS_NETWORK:
            switch (subclass) {
                case PCI_SUBCLASS_NETWORK_ETHERNET:   return "Ethernet";
                case PCI_SUBCLASS_NETWORK_TOKEN_RING: return "Token Ring";
                case PCI_SUBCLASS_NETWORK_FDDI:       return "FDDI";
                case PCI_SUBCLASS_NETWORK_ATM:        return "ATM";
                case PCI_SUBCLASS_NETWORK_OTHER:      return "Other";
                default: return "Unknown";
            }
        case PCI_CLASS_DISPLAY:
            switch (subclass) {
                case 0x00: return "VGA";
                case 0x01: return "XGA";
                case 0x02: return "3D";
                default: return "Unknown";
            }
        default:
            return "";
    }
}
const char* pci_get_vendor_name(uint16_t vendor_id) {
    switch (vendor_id) {
        case 0x8086: return "Intel";
        case 0x10DE: return "NVIDIA";
        case 0x1002: return "AMD/ATI";
        case 0x1022: return "AMD";
        case 0x10EC: return "Realtek";
        case 0x14E4: return "Broadcom";
        case 0x1969: return "Atheros";
        case 0x1AF4: return "Red Hat";
        case 0x1234: return "QEMU";
        case 0x80EE: return "VirtualBox";
        case 0x15AD: return "VMware";
        case 0x106B: return "Apple";
        case 0x5333: return "S3 Graphics";
        case 0x1274: return "Ensoniq";
        case 0x1106: return "VIA";
        case 0x1039: return "SiS";
        default: return "Unknown";
    }
}
void pci_print_devices(void) {
    pci_device_t *dev = pci_devices;
    int count = 0;
    print("=== PCI Devices ===\n", LIGHT_CYAN);
    if (!dev) {
        print("No PCI devices found\n", YELLOW);
        return;
    }
    print("Bus Dev Fnc Vendor Device  Class              Subclass  IRQ\n", WHITE);
    print("--- --- --- ------ ------  -----------------  --------  ---\n", WHITE);

    while (dev) {
        const char *vendor_name = pci_get_vendor_name(dev->vendor_id);
        const char *class_name = pci_get_class_name(dev->class_code);
        const char *subclass_name = pci_get_subclass_name(dev->class_code, dev->subclass);

        char buf[256];

        snprintf(buf, sizeof(buf),
                 "%02x   %02x   %01x   %04x   %04x   %-17s  %-8s  %3d",
                 dev->bus, dev->device, dev->function,
                 dev->vendor_id, dev->device_id,
                 class_name, subclass_name,
                 dev->interrupt_line);

        print(buf, WHITE);

        if (strcmp(vendor_name, "Unknown") != 0) {
            snprintf(buf, sizeof(buf), "  (%s)", vendor_name);
            print(buf, LIGHT_RED);
        }
        print("\n", WHITE);
        if (dev->bar[0] != 0) {
            char bar_buf[64];
            snprintf(bar_buf, sizeof(bar_buf), "  BAR0: 0x%08x", dev->bar[0]);
            print(bar_buf, LIGHT_RED);
            print("\n", WHITE);
        }

        dev = dev->next;
        count++;
    }
    char final_buf[64];
    snprintf(final_buf, sizeof(final_buf), "\nTotal PCI devices: %d\n", count);
    print(final_buf, LIGHT_CYAN);
}
void pci_init(void) {
    log_message("Initializing PCI subsystem...", LOG_INFO);
    pci_scan_all();
    char msg[64];
    pci_device_t *dev = pci_devices;
    int count = 0;
    while (dev) {
        count++;
        dev = dev->next;
    }
    snprintf(msg, sizeof(msg), "PCI init complete, found %d devices", count);
    log_message(msg, LOG_INFO);
}
