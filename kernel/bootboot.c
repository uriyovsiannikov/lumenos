// kernel/bootboot.c
#include "bootboot.h"
#include "../include/stdio.h"
#include "../libs/print.h"
#include "../libs/string.h"
/* BOOTBOOT structure pointer - загрузчик помещает его по адресу 0x1000 */
bootboot_t* bootboot = (bootboot_t*)0x00001000;

void bootboot_init(void) {
    /* Проверяем magic BOOTBOOT */
    if (bootboot->magic[0] != 'B' || bootboot->magic[1] != 'O' || 
        bootboot->magic[2] != 'O' || bootboot->magic[3] != 'T') {
        print("BOOTBOOT: Invalid magic!\n", LIGHT_RED);
        return;
    }
    
    print("BOOTBOOT v", WHITE);
    char buf[16];
    itoa(bootboot->protocol, buf, 10);
    print(buf, WHITE);
    print(" detected\n", WHITE);
    
    if (bootboot->framebuffer_type == 1) {
        print("Framebuffer: ", WHITE);
        itoa(bootboot->width, buf, 10);
        print(buf, WHITE);
        print("x", WHITE);
        itoa(bootboot->height, buf, 10);
        print(buf, WHITE);
        print(", ", WHITE);
        itoa(bootboot->bpp, buf, 10);
        print(buf, WHITE);
        print("bpp\n", WHITE);
    }
    
    if (bootboot->initrd_ptr && bootboot->initrd_size) {
        print("Initrd: ", WHITE);
        print_hex(bootboot->initrd_ptr,WHITE);
        print(" - ", WHITE);
        print_hex(bootboot->initrd_ptr + bootboot->initrd_size,WHITE);
        print(" (", WHITE);
        itoa(bootboot->initrd_size / 1024, buf, 10);
        print(buf, WHITE);
        print(" KB)\n", WHITE);
    }
}

void* bootboot_get_framebuffer(void) {
    if (bootboot->framebuffer_type == 1) {
        return (void*)bootboot->fb_ptr;
    }
    return NULL;
}

uint64_t bootboot_get_initrd_ptr(void) {
    return bootboot->initrd_ptr;
}

uint64_t bootboot_get_initrd_size(void) {
    return bootboot->initrd_size;
}