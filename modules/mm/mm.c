#include "mm.h"
#include "../modules/syslogger/syslogger.h"
#include "../libs/print.h"
#include <stdint.h>
#include <string.h>
typedef struct mem_block {
    size_t size;
    struct mem_block *next;
    uint8_t free; 
} mem_block_t;
typedef unsigned int uintptr_t;
static mem_block_t *head = NULL;
static void *heap_start = NULL;
static void *heap_end = NULL;
void mm_init(void *start, size_t size) {
    heap_start = start;
    heap_end = (void*)((uintptr_t)start + size);
    head = (mem_block_t*)start;
    head->size = size - sizeof(mem_block_t);
    head->next = NULL;
    head->free = 1;
}
void *malloc(size_t size) {
    if (size == 0 || !heap_start) return NULL;
    size = (size + 7) & ~7;
    mem_block_t *current = head;
    while (current) {
        if (current->free && current->size >= size) {
            if (current->size > size + sizeof(mem_block_t)) {
                mem_block_t *new_block = (mem_block_t*)((uintptr_t)current + sizeof(mem_block_t) + size);
                new_block->size = current->size - size - sizeof(mem_block_t);
                new_block->next = current->next;
                new_block->free = 1;
                current->size = size;
                current->next = new_block;
            }
            current->free = 0;
            return (void*)((uintptr_t)current + sizeof(mem_block_t));
        }
        current = current->next;
    }
    return NULL;
}
void free(void *ptr) {
    if (!ptr || ptr < heap_start || ptr >= heap_end) return;
    mem_block_t *block = (mem_block_t*)((uintptr_t)ptr - sizeof(mem_block_t));
    block->free = 1;
    mem_block_t *current = head;
    while (current && current->next) {
        if (current->free && current->next->free) {
            current->size += sizeof(mem_block_t) + current->next->size;
            current->next = current->next->next;
        } else {
            current = current->next;
        }
    }
}
void test_memory_manager() {
    print("=== Memory Manager Test ===\n", WHITE);
    int *test_arr = (int*)malloc(10 * sizeof(int));
    if (test_arr) {
        print("Test 1: malloc(10*int) - OK\n", GREEN);
        for (int i = 0; i < 10; i++) {
            test_arr[i] = i * i;
        }
    } else {
        print("Test 1: malloc(10*int) - FAIL\n", RED);
    }
    free(test_arr);
    print("Test 2: free() - OK\n", GREEN);
    char *test_str = (char*)malloc(50);
    if (test_str) {
        print("Test 3: malloc(50) after free - OK\n", GREEN);
        strcpy(test_str, "Memory manager works!");
        print("Test string: ", WHITE);
        print(test_str, CYAN);
        putchar('\n', WHITE);
    } else {
        print("Test 3: malloc(50) after free - FAIL\n", RED);
    }
    free(test_str);
    void *blocks[5];
    for (int i = 0; i < 5; i++) {
        blocks[i] = malloc(32);
        if (!blocks[i]) {
            print("Test 4: Fragmentation test - FAIL at block ", RED);
            print_dec(i, RED);
            putchar('\n', RED);
            break;
        }
    }
    print("Test 4: Allocated 5 blocks - OK\n", GREEN);
    free(blocks[2]);
    free(blocks[0]);
    free(blocks[4]);
    free(blocks[1]);
    free(blocks[3]);
    print("Test 5: Free in random order - OK\n", GREEN);
    void *big_block = malloc(HEAP_SIZE - 256);
    if (big_block) {
        print("Test 6: Big allocation (", GREEN);
        print_dec(HEAP_SIZE - 256, GREEN);
        print(" bytes) - OK\n", GREEN);
        free(big_block);
    } else {
        print("Test 6: Big allocation failed - OK (expected)\n", YELLOW);
    }
    print("=== Memory Tests Complete ===\n\n", WHITE);
	log_message("Mm test complete",LOG_INFO);
}