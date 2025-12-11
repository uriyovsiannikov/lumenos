// include/bootboot.h
#ifndef BOOTBOOT_H
#define BOOTBOOT_H

#include <stdint.h>

/* BOOTBOOT структура (загрузчик помещает её по адресу 0x00001000) */
typedef struct {
    uint8_t magic[4];           /* 'B','O','O','T' */
    uint32_t size;              /* размер этой структуры */
    uint8_t protocol;           /* 0 - old, 1 - new */
    uint8_t framebuffer_type;   /* 0 - текстовый, 1 - графический */
    uint16_t width;             /* ширина в пикселях */
    uint16_t height;            /* высота в пикселях */
    uint16_t bpp;               /* бит на пиксель */
    uint16_t pitch;             /* байт на строку */
    uint64_t fb_ptr;            /* указатель на framebuffer */
    uint64_t fb_size;           /* размер framebuffer */
    uint64_t initrd_ptr;        /* указатель на initrd */
    uint64_t initrd_size;       /* размер initrd */
    uint64_t acpi_ptr;          /* указатель на ACPI RSDP */
    uint64_t smbi_ptr;          /* указатель на SMBIOS */
    uint64_t efi_ptr;           /* указатель на EFI system table */
    uint64_t mp_ptr;            /* указатель на MP table */
    uint64_t unused0;
    uint64_t unused1;
    uint64_t unused2;
    uint64_t unused3;
} __attribute__((packed)) bootboot_t;

/* Memory map entry */
typedef struct {
    uint64_t ptr;
    uint64_t size;
    uint32_t type;
    uint32_t unused;
} __attribute__((packed)) mmap_entry_t;

/* Глобальный указатель на BOOTBOOT структуру */
extern bootboot_t* bootboot;

/* Функции инициализации */
void bootboot_init(void);
void* bootboot_get_framebuffer(void);
uint64_t bootboot_get_initrd_ptr(void);
uint64_t bootboot_get_initrd_size(void);

#endif