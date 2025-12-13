#ifndef MM_H
#define MM_H
#include <stddef.h>
#define HEAP_SIZE (1024 * 1024)
void mm_init(void *start, size_t size);
void *malloc(size_t size);
void free(void *ptr);
void test_memory_manager();
#endif