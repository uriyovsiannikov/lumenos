#ifndef MEMPOOL_H
#define MEMPOOL_H
#include <stddef.h>
#include <stdint.h>
typedef struct mempool {
    void *start;
    void *end;
    size_t block_size;
    uint32_t total_blocks;
    uint32_t free_blocks;
    uint8_t *bitmap;
    struct mempool *next;
} mempool_t;
mempool_t *mempool_create(size_t block_size, uint32_t num_blocks);
void *mempool_alloc(mempool_t *pool);
void mempool_free(mempool_t *pool, void *ptr);
void mempool_destroy(mempool_t *pool);
void mempool_stats(mempool_t *pool);
extern mempool_t *pool_16;
extern mempool_t *pool_32;
extern mempool_t *pool_64;
extern mempool_t *pool_128;
void *malloc_16(void);
void *malloc_32(void);
void *malloc_64(void);
void *malloc_128(void);
void free_16(void *ptr);
void free_32(void *ptr);
void free_64(void *ptr);
void free_128(void *ptr);
void mempools_init(void);
void mempool_test(void);
#endif // MEMPOOL_H