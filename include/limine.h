#ifndef _LIMINE_H
#define _LIMINE_H

#include <stdint.h>

// Структуры ответов
struct limine_framebuffer_response {
    uint64_t revision;
    uint64_t framebuffer_count;
    void **framebuffers;
};

struct limine_memmap_response {
    uint64_t revision;
    uint64_t entry_count;
    void **entries;
};

struct limine_kernel_file_response {
    uint64_t revision;
    void *kernel_file;
};

// Структура файла
struct limine_file {
    uint64_t revision;
    uint64_t address;
    uint64_t size;
    char *path;
    char *cmdline;
    uint64_t partition_index;
    uint64_t mbr_disk_id;
    uint64_t gpt_disk_uuid[2];
    uint64_t part_uuid[2];
};

#endif