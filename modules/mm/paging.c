#include "paging.h"
#include "../../libs/print.h"
#include "../../libs/string.h"
#include "../syslogger/syslogger.h"
#include <stddef.h>
static uint32_t page_directory[1024] __attribute__((aligned(4096)));
static uint32_t page_tables[1024 * 16] __attribute__((aligned(4096))); 
static physical_memory_manager_t phys_mem;
static uint32_t initialized_tables = 4; 
static void bitmap_set(uint32_t page_index) {
    phys_mem.bitmap[page_index / 8] |= (1 << (page_index % 8));
    phys_mem.used_pages++;
}
static void bitmap_clear(uint32_t page_index) {
    phys_mem.bitmap[page_index / 8] &= ~(1 << (page_index % 8));
    phys_mem.used_pages--;
}
static bool bitmap_test(uint32_t page_index) {
    return phys_mem.bitmap[page_index / 8] & (1 << (page_index % 8));
}
static uint32_t bitmap_find_free(void) {
    for (uint32_t i = 1024; i < phys_mem.total_pages; i++) {
        if (!bitmap_test(i)) {
            phys_mem.last_allocated = i;
            return i;
        }
    }
    return 0;
}
static bool ensure_page_table(uint32_t pd_index) {
    if (pd_index < initialized_tables) {
        return true; 
    }
    for (uint32_t i = initialized_tables; i <= pd_index; i++) {
        if (i >= 16) { 
            print_error("Cannot create page table beyond 64MB");
            return false;
        }
        for (int j = 0; j < 1024; j++) {
            page_tables[i * 1024 + j] = 0x00000002; 
        }
        page_directory[i] = ((uint32_t)&page_tables[i * 1024]) | 0x03;
        initialized_tables++;
        print("Created page table for PD index: ", WHITE);
        print_dec(i, CYAN);
        print(" (", WHITE);
        print_hex(i * 4 * 1024 * 1024, CYAN);
        print(")\n", WHITE);
    }
    return true;
}
void paging_init(void) {
    print("Initializing paging...", WHITE);
	print(" [OK]\n",GREEN);
    phys_mem.total_pages = (128 * 1024 * 1024) / 4096;
    phys_mem.used_pages = 0;
    phys_mem.last_allocated = 1024; 
    memset(phys_mem.bitmap, 0, PAGE_BITMAP_SIZE);
    for (uint32_t i = 0; i < 1024; i++) {
        bitmap_set(i);
    }
    for (int table = 0; table < 4; table++) {
        for (int i = 0; i < 1024; i++) {
            uint32_t physical_addr = (table * 1024 + i) * 0x1000;
            page_tables[table * 1024 + i] = physical_addr | 0x03;
        }
    }
    for (int i = 0; i < 1024; i++) {
        if (i < 4) {
            page_directory[i] = ((uint32_t)&page_tables[i * 1024]) | 0x03;
        } else if (i < 16) {
            page_directory[i] = 0x00000002;
        } else {
            page_directory[i] = 0x00000002;
        }
    }
    asm volatile("mov %0, %%cr3" :: "r"(page_directory));
    uint32_t cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;
    asm volatile("mov %0, %%cr0" :: "r"(cr0));
    print_success("Extended paging enabled (64MB addressable)\n");
}
bool paging_map_page(uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags) {
    uint32_t pd_index = virtual_addr >> 22;
    uint32_t pt_index = (virtual_addr >> 12) & 0x3FF;
    if (!ensure_page_table(pd_index)) {
        return false;
    }
    page_tables[pd_index * 1024 + pt_index] = physical_addr | flags | 0x03;
    asm volatile("invlpg (%0)" :: "r"(virtual_addr) : "memory");
    return true;
}
bool paging_unmap_page(uint32_t virtual_addr) {
    uint32_t pd_index = virtual_addr >> 22;
    uint32_t pt_index = (virtual_addr >> 12) & 0x3FF;
    if (pd_index >= initialized_tables) return false;
    page_tables[pd_index * 1024 + pt_index] = 0x00000002;
    asm volatile("invlpg (%0)" :: "r"(virtual_addr) : "memory");
    return true;
}
uint32_t paging_get_physical_address(uint32_t virtual_addr) {
    uint32_t pd_index = virtual_addr >> 22;
    uint32_t pt_index = (virtual_addr >> 12) & 0x3FF;
    if (pd_index >= initialized_tables) return 0;
    uint32_t entry = page_tables[pd_index * 1024 + pt_index];
    if (!(entry & 0x01)) return 0;
    return (entry & 0xFFFFF000) | (virtual_addr & 0xFFF);
}
void* paging_alloc_page(void) {
    uint32_t free_page = bitmap_find_free();
    if (!free_page) {
        print_error("No free physical pages");
        return NULL;
    }
    uint32_t physical_addr = free_page * 4096;
    bitmap_set(free_page);
    static uint32_t next_virtual = 0x1000000;
    uint32_t virtual_addr = next_virtual;
    next_virtual += 4096;
    if (next_virtual >= 0x2000000) {
        next_virtual = 0x1000000; 
    }
    if (paging_map_page(virtual_addr, physical_addr, 0x03)) {
        return (void*)virtual_addr;
    }
    bitmap_clear(free_page);
    return NULL;
}
void paging_free_page(void* page) {
    uint32_t virtual_addr = (uint32_t)page;
    uint32_t physical_addr = paging_get_physical_address(virtual_addr);
    if (physical_addr) {
        uint32_t page_index = physical_addr / 4096;
        bitmap_clear(page_index);
        paging_unmap_page(virtual_addr);
    }
}
void paging_print_info(void) {
    uint32_t cr0, cr3, cr4;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    asm volatile("mov %%cr3, %0" : "=r"(cr3));
    asm volatile("mov %%cr4, %0" : "=r"(cr4));
    print("=== Paging Information ===\n", WHITE);
    print("Status: ", WHITE); print("ACTIVE\n", GREEN);
    print("CR0.PG: ", WHITE); print((cr0 & 0x80000000) ? "ENABLED\n" : "DISABLED\n", CYAN);
    print("CR3:    ", WHITE); print_hex(cr3, CYAN); print("\n", WHITE);
    print("Physical Memory: ", WHITE);
    print_dec(phys_mem.used_pages * 4, CYAN); print(" KB / ", WHITE);
    print_dec(phys_mem.total_pages * 4, CYAN); print(" KB used\n", WHITE);
    print("Virtual Space:   ", WHITE);
    print_dec(initialized_tables * 4, CYAN); print(" MB / 64 MB mapped\n", WHITE);
    print("Page Tables:     ", WHITE);
    print_dec(initialized_tables, CYAN); print(" / 16 initialized\n", WHITE);
    print("==========================\n", WHITE);
}
void paging_run_tests(void) {
    print("=== Paging Tests ===\n", WHITE);
    uint32_t initial_tables = initialized_tables;
    void* page = paging_alloc_page();
    if (page) {
        print("Test 1: Page allocation - OK\n", GREEN);
        print("  Virtual: ", WHITE); print_hex((uint32_t)page, CYAN);
        print(" -> Physical: ", WHITE); print_hex(paging_get_physical_address((uint32_t)page), CYAN);
        print("\n", WHITE);
        paging_free_page(page);
    } else {
        print("Test 1: Page allocation - FAIL\n", RED);
    }
    if (paging_map_page(0x01000000, 0x200000, 0x03)) {
        uint32_t phys = paging_get_physical_address(0x01000000);
        if (phys == 0x200000) {
            print("Test 2: Memory mapping - OK\n", GREEN);
            print("  Mapped: 0x01000000 -> 0x200000\n", WHITE);
        } else {
            print("Test 2: Memory mapping - FAIL\n", RED);
            print("  Expected: 0x200000, Got: ", WHITE); 
            print_hex(phys, RED); print("\n", WHITE);
        }
        paging_unmap_page(0x01000000);
    } else {
        print("Test 2: Memory mapping - FAIL\n", RED);
    }
    void* pages[3];
    bool success = true;
    for (int i = 0; i < 3; i++) {
        pages[i] = paging_alloc_page();
        if (!pages[i]) {
            success = false;
            break;
        }
    }
    if (success) {
        print("Test 3: Multiple allocations - OK\n", GREEN);
        for (int i = 0; i < 3; i++) {
            print("  Page ", WHITE); print_dec(i, CYAN); 
            print(": ", WHITE); print_hex((uint32_t)pages[i], CYAN);
            print(" -> ", WHITE); print_hex(paging_get_physical_address((uint32_t)pages[i]), CYAN);
            print("\n", WHITE);
            paging_free_page(pages[i]);
        }
    } else {
        print("Test 3: Multiple allocations - FAIL\n", RED);
        for (int i = 0; i < 3; i++) {
            if (pages[i]) paging_free_page(pages[i]);
        }
    }
    void* test_page = paging_alloc_page();
    if (test_page) {
        uint32_t virt = (uint32_t)test_page;
        uint32_t phys = paging_get_physical_address(virt);
        if (paging_unmap_page(virt)) {
            uint32_t after_unmap = paging_get_physical_address(virt);
            if (after_unmap == 0) {
                print("Test 4: Page unmapping - OK\n", GREEN);
            } else {
                print("Test 4: Page unmapping - FAIL\n", RED);
            }
        } else {
            print("Test 4: Page unmapping - FAIL\n", RED);
        }
        paging_free_page(test_page);
    } else {
        print("Test 4: Page unmapping - SKIP (no free pages)\n", YELLOW);
    }
    uint32_t kernel_addr = 0x100000; 
    uint32_t phys_addr = paging_get_physical_address(kernel_addr);
    if (phys_addr == kernel_addr) {
        print("Test 5: Identity mapping - OK\n", GREEN);
    } else {
        print("Test 5: Identity mapping - FAIL\n", RED);
        print("  Expected: ", WHITE); print_hex(kernel_addr, RED);
        print(" Got: ", WHITE); print_hex(phys_addr, RED); print("\n", WHITE);
    }
    if (initialized_tables <= initial_tables + 2) { 
        print("Test 6: Table management - OK\n", GREEN);
        print("  Tables created: ", WHITE); 
        print_dec(initialized_tables - initial_tables, CYAN); print("\n", WHITE);
    } else {
        print("Test 6: Table management - FAIL\n", RED);
        print("  Too many tables created: ", WHITE);
        print_dec(initialized_tables - initial_tables, RED); print("\n", WHITE);
    }
    print("=== Paging Tests Complete ===\n\n", WHITE);
    paging_print_info();
}
void paging_enable(void) {}
void paging_disable(void) {}
void paging_load_directory(uint32_t addr) {}
void paging_invalidate_tlb(void) { asm volatile("mov %%cr3, %%eax; mov %%eax, %%cr3" ::: "eax"); }
void paging_invalidate_page(uint32_t addr) { asm volatile("invlpg (%0)" :: "r"(addr) : "memory"); }
void paging_map_range(uint32_t vstart, uint32_t pstart, size_t pages, uint32_t flags) {
    for (size_t i = 0; i < pages; i++) {
        paging_map_page(vstart + i * 4096, pstart + i * 4096, flags);
    }
}
void paging_unmap_range(uint32_t vstart, size_t pages) {
    for (size_t i = 0; i < pages; i++) {
        paging_unmap_page(vstart + i * 4096);
    }
}
void* paging_alloc_pages(size_t count) {
    void* first = paging_alloc_page();
    if (!first) return NULL;
    for (size_t i = 1; i < count; i++) {
        if (!paging_alloc_page()) {
            for (size_t j = 0; j < i; j++) {
                paging_free_page((void*)((uint32_t)first + j * 4096));
            }
            return NULL;
        }
    }
    return first;
}
void paging_free_pages(void* pages, size_t count) {
    for (size_t i = 0; i < count; i++) {
        paging_free_page((void*)((uint32_t)pages + i * 4096));
    }
}
size_t paging_get_used_pages(void) { return phys_mem.used_pages; }
size_t paging_get_free_pages(void) { return phys_mem.total_pages - phys_mem.used_pages; }
uint32_t paging_virtual_to_physical(uint32_t virt) { return paging_get_physical_address(virt); }
bool paging_is_mapped(uint32_t virt) { return paging_get_physical_address(virt) != 0; }
void paging_set_flags(uint32_t virt, uint32_t flags) {}
uint32_t paging_get_flags(uint32_t virt) { 
    uint32_t phys = paging_get_physical_address(virt);
    return phys ? 0x03 : 0; 
}
void paging_demo(void) {
    print("=== Paging Features Demo ===\n", WHITE);
    print("\n1. Page Allocation Demo:\n", CYAN);
    void* demo_pages[5];
    for (int i = 0; i < 5; i++) {
        demo_pages[i] = paging_alloc_page();
        if (demo_pages[i]) {
            print("  Allocated page ", WHITE); print_dec(i, CYAN);
            print(": VA=", WHITE); print_hex((uint32_t)demo_pages[i], GREEN);
            print(" PA=", WHITE); print_hex(paging_get_physical_address((uint32_t)demo_pages[i]), GREEN);
            print("\n", WHITE);
        }
    }
    print("\n2. Memory Access Demo:\n", CYAN);
    for (int i = 0; i < 5; i++) {
        if (demo_pages[i]) {
            uint32_t* test_ptr = (uint32_t*)demo_pages[i];
            *test_ptr = 0xDEADBEEF; 
            uint32_t read_back = *test_ptr;
            print("  Page ", WHITE); print_dec(i, CYAN);
            print(": Write 0xDEADBEEF -> Read ", WHITE);
            print_hex(read_back, read_back == 0xDEADBEEF ? GREEN : RED);
            print("\n", WHITE);
        }
    }
    print("\n3. Custom Mapping Demo:\n", CYAN);
    uint32_t custom_va = 0x3000000; 
    uint32_t custom_pa = 0x500000;  
    if (paging_map_page(custom_va, custom_pa, PAGE_PRESENT | PAGE_READ_WRITE)) {
        print("  Mapped: VA=", WHITE); print_hex(custom_va, GREEN);
        print(" -> PA=", WHITE); print_hex(custom_pa, GREEN);
        print("\n", WHITE);
        uint32_t* custom_ptr = (uint32_t*)custom_va;
        *custom_ptr = 0xCAFEBABE;
        uint32_t custom_read = *custom_ptr;
        print("  Write 0xCAFEBABE -> Read ", WHITE);
        print_hex(custom_read, custom_read == 0xCAFEBABE ? GREEN : RED);
        print("\n", WHITE);
        paging_unmap_page(custom_va);
    }
    print("\n4. Unmapping Demo:\n", CYAN);
    for (int i = 0; i < 5; i++) {
        if (demo_pages[i]) {
            uint32_t va = (uint32_t)demo_pages[i];
            paging_unmap_page(va);
            bool still_mapped = paging_is_mapped(va);
            print("  Page ", WHITE); print_dec(i, CYAN);
            print(" unmapped: ", WHITE);
            print(still_mapped ? "FAIL (still mapped)" : "OK", still_mapped ? RED : GREEN);
            print("\n", WHITE);
        }
    }
    print("\n5. Multi-page Allocation:\n", CYAN);
    void* multi_page = paging_alloc_pages(4); 
    if (multi_page) {
        print("  Allocated 4 pages: VA=", WHITE); print_hex((uint32_t)multi_page, GREEN);
        print(" Size=16KB\n", WHITE);
        uint8_t* data = (uint8_t*)multi_page;
        for (int i = 0; i < 4 * 4096; i++) {
            data[i] = i & 0xFF; 
        }
        bool data_ok = true;
        for (int i = 0; i < 100; i++) { 
            if (data[i] != (i & 0xFF)) {
                data_ok = false;
                break;
            }
        }
        print("  Data integrity: ", WHITE);
        print(data_ok ? "OK" : "CORRUPTED", data_ok ? GREEN : RED);
        print("\n", WHITE);
        paging_free_pages(multi_page, 4);
    }
    print("\n6. Final Statistics:\n", CYAN);
    print("  Physical pages used: ", WHITE); 
    print_dec(paging_get_used_pages(), CYAN); print(" / ", WHITE);
    print_dec(phys_mem.total_pages, CYAN); print("\n", WHITE);
    print("  Memory used: ", WHITE);
    print_dec(paging_get_used_pages() * 4, CYAN); print(" KB\n", WHITE);
    for (int i = 0; i < 5; i++) {
        if (demo_pages[i]) {
            paging_free_page(demo_pages[i]);
        }
    }
    print("\n=== Paging Demo Complete ===\n\n", WHITE);
}