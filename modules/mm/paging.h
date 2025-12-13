#ifndef PAGING_H
#define PAGING_H
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#define PAGE_SIZE 4096
#define PAGE_BITMAP_SIZE 32768  
#define PAGE_PRESENT        0x01
#define PAGE_READ_WRITE     0x02
#define PAGE_USER_SUPERVISOR 0x04
typedef struct {
    uint8_t bitmap[PAGE_BITMAP_SIZE];
    uint32_t total_pages;
    uint32_t used_pages;
    uint32_t last_allocated;
} physical_memory_manager_t;
void paging_init(void);
void paging_demo(void);
void paging_enable(void);
void paging_disable(void);
bool paging_map_page(uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags);
bool paging_unmap_page(uint32_t virtual_addr);
uint32_t paging_get_physical_address(uint32_t virtual_addr);
void paging_map_range(uint32_t virtual_start, uint32_t physical_start, size_t pages, uint32_t flags);
void paging_unmap_range(uint32_t virtual_start, size_t pages);
void* paging_alloc_page(void);
void paging_free_page(void* page);
void* paging_alloc_pages(size_t count);
void paging_free_pages(void* pages, size_t count);
void paging_print_info(void);
size_t paging_get_used_pages(void);
size_t paging_get_free_pages(void);
bool paging_is_mapped(uint32_t virtual_addr);
void paging_run_tests(void);
#endif 