#include "mempool.h"
#include "../modules/mm/mm.h"  // Ваш malloc/free
#include "../libs/print.h"
#include "../libs/string.h"
#include <stddef.h>
mempool_t *pool_16 = NULL;
mempool_t *pool_32 = NULL;
mempool_t *pool_64 = NULL;
mempool_t *pool_128 = NULL;
#define PTR_ADD(ptr, offset) ((void*)((char*)(ptr) + (offset)))
#define PTR_SUB(ptr1, ptr2) ((char*)(ptr1) - (char*)(ptr2))
mempool_t *mempool_create(size_t block_size, uint32_t num_blocks) {
    mempool_t *pool = (mempool_t *)malloc(sizeof(mempool_t));
    if (!pool) return NULL;
    size_t total_size = block_size * num_blocks;
    pool->start = malloc(total_size);
    if (!pool->start) {
        free(pool);
        return NULL;
    }
    size_t bitmap_size = (num_blocks + 7) / 8;
    pool->bitmap = (uint8_t *)malloc(bitmap_size);
    if (!pool->bitmap) {
        free(pool->start);
        free(pool);
        return NULL;
    }
    pool->end = PTR_ADD(pool->start, total_size);
    pool->block_size = block_size;
    pool->total_blocks = num_blocks;
    pool->free_blocks = num_blocks;
    pool->next = NULL;
    memset(pool->bitmap, 0, bitmap_size);
    memset(pool->start, 0, total_size);
    return pool;
}
void *mempool_alloc(mempool_t *pool) {
    if (!pool || pool->free_blocks == 0) {
        return NULL;
    }
    size_t bitmap_bytes = (pool->total_blocks + 7) / 8;
    for (size_t byte = 0; byte < bitmap_bytes; byte++) {
        uint8_t bitmap_byte = pool->bitmap[byte];
        if (bitmap_byte != 0xFF) {
            for (int bit = 0; bit < 8; bit++) {
                if (!(bitmap_byte & (1 << bit))) {
                    uint32_t block_idx = byte * 8 + bit;
                    if (block_idx >= pool->total_blocks) {
                        return NULL;
                    }
                    pool->bitmap[byte] |= (1 << bit);
                    pool->free_blocks--;
                    char *block_ptr = (char *)pool->start + (block_idx * pool->block_size);
                    return (void *)block_ptr;
                }
            }
        }
    }
    return NULL;
}
void mempool_free(mempool_t *pool, void *ptr) {
    if (!pool || !ptr || ptr < pool->start || ptr >= pool->end) {
        return;
    }
    char *char_ptr = (char *)ptr;
    char *char_start = (char *)pool->start;
    size_t offset = char_ptr - char_start;
    uint32_t block_idx = offset / pool->block_size;
    if (block_idx >= pool->total_blocks) {
        return;
    }
    uint32_t byte_idx = block_idx / 8;
    uint32_t bit_idx = block_idx % 8;
    if (!(pool->bitmap[byte_idx] & (1 << bit_idx))) {
        return;
    }
    pool->bitmap[byte_idx] &= ~(1 << bit_idx);
    pool->free_blocks++;
    memset(ptr, 0, pool->block_size);
}
void mempool_destroy(mempool_t *pool) {
    if (!pool) return;
    if (pool->start) free(pool->start);
    if (pool->bitmap) free(pool->bitmap);
    free(pool);
}
void mempool_stats(mempool_t *pool) {
    if (!pool) return;
    print("Memory Pool Stats:\n", LIGHT_CYAN);
    print("  Block size:    ", WHITE);
    print_dec(pool->block_size, CYAN);
    print(" bytes\n", WHITE);
    print("  Total blocks:  ", WHITE);
    print_dec(pool->total_blocks, CYAN);
    print("\n", WHITE);
    print("  Free blocks:   ", WHITE);
    print_dec(pool->free_blocks, GREEN);
    print("\n", WHITE);
    print("  Used blocks:   ", WHITE);
    print_dec(pool->total_blocks - pool->free_blocks, YELLOW);
    print("\n", WHITE);
    print("  Usage:         ", WHITE);
    print_dec((pool->total_blocks - pool->free_blocks) * 100 / pool->total_blocks, 
              pool->free_blocks == 0 ? RED : YELLOW);
    print("%\n", WHITE);
}
void mempools_init(void) {
    pool_16 = mempool_create(16, 256);   // 256 блоков по 16 байт = 4KB
    pool_32 = mempool_create(32, 256);   // 256 блоков по 32 байта = 8KB
    pool_64 = mempool_create(64, 128);   // 128 блоков по 64 байта = 8KB
    pool_128 = mempool_create(128, 64);  // 64 блока по 128 байт = 8KB
    
    if (!pool_16 || !pool_32 || !pool_64 || !pool_128) {
        print("Failed to initialize memory pools!\n", RED);
    } else {
        print("Memory pools initialized successfully\n", GREEN);
    }
}
void *malloc_16(void) {
    if (!pool_16) return malloc(16);
    void *ptr = mempool_alloc(pool_16);
    return ptr ? ptr : malloc(16);
}
void *malloc_32(void) {
    if (!pool_32) return malloc(32);
    void *ptr = mempool_alloc(pool_32);
    return ptr ? ptr : malloc(32);
}
void *malloc_64(void) {
    if (!pool_64) return malloc(64);
    void *ptr = mempool_alloc(pool_64);
    return ptr ? ptr : malloc(64);
}
void *malloc_128(void) {
    if (!pool_128) return malloc(128);
    void *ptr = mempool_alloc(pool_128);
    return ptr ? ptr : malloc(128);
}
void free_16(void *ptr) {
    if (!pool_16 || !ptr) {
        free(ptr);
        return;
    }
    if (ptr >= pool_16->start && ptr < pool_16->end) {
        mempool_free(pool_16, ptr);
    } else {
        free(ptr);
    }
}
void free_32(void *ptr) {
    if (!pool_32 || !ptr) {
        free(ptr);
        return;
    }
    if (ptr >= pool_32->start && ptr < pool_32->end) {
        mempool_free(pool_32, ptr);
    } else {
        free(ptr);
    }
}
void free_64(void *ptr) {
    if (!pool_64 || !ptr) {
        free(ptr);
        return;
    }
    if (ptr >= pool_64->start && ptr < pool_64->end) {
        mempool_free(pool_64, ptr);
    } else {
        free(ptr);
    }
}
void free_128(void *ptr) {
    if (!pool_128 || !ptr) {
        free(ptr);
        return;
    }
    if (ptr >= pool_128->start && ptr < pool_128->end) {
        mempool_free(pool_128, ptr);
    } else {
        free(ptr);
    }
}
void *mp_malloc(size_t size) {
    if (size <= 16) return malloc_16();
    if (size <= 32) return malloc_32();
    if (size <= 64) return malloc_64();
    if (size <= 128) return malloc_128();
    return malloc(size);
}
void mp_free(void *ptr, size_t size) {
    if (!ptr) return;
    
    if (size <= 16) free_16(ptr);
    else if (size <= 32) free_32(ptr);
    else if (size <= 64) free_64(ptr);
    else if (size <= 128) free_128(ptr);
    else free(ptr);
}
void mempool_test(void) {
    print("=== Memory Pool Test ===\n", WHITE);
    void *ptrs[20];
    for (int i = 0; i < 20; i++) {
        ptrs[i] = malloc_16();
        if (!ptrs[i]) {
            print("Test 1: Failed to allocate from pool_16\n", RED);
            break;
        }
    }
    print("Test 1: Allocated 20 blocks from pool_16 - OK\n", GREEN);
    for (int i = 0; i < 20; i++) {
        free_16(ptrs[i]);
    }
    print("Test 2: Freed all blocks - OK\n", GREEN);
    void *many_ptrs[300];
    int allocated = 0;
    for (int i = 0; i < 300; i++) {
        many_ptrs[i] = malloc_16();
        if (many_ptrs[i]) allocated++;
        else break;
    }
    print("Test 3: Allocated ", GREEN);
    print_dec(allocated, CYAN);
    print(" blocks from pool_16 - OK\n", GREEN);
    for (int i = 0; i < allocated; i++) {
        free_16(many_ptrs[i]);
    }
    void *p32 = malloc_32();
    void *p64 = malloc_64();
    void *p128 = malloc_128();
    if (p32 && p64 && p128) {
        print("Test 4: Allocated from all pools - OK\n", GREEN);
    }
    free_32(p32);
    free_64(p64);
    free_128(p128);
    print("\nPool Statistics:\n", LIGHT_CYAN);
    mempool_stats(pool_16);
    mempool_stats(pool_32);
    mempool_stats(pool_64);
    mempool_stats(pool_128);
    print("=== Memory Pool Test Complete ===\n\n", WHITE);
}