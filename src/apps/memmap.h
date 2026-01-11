#ifndef MEMMAP_H
#define MEMMAP_H
#include <stdint.h>
typedef struct {
  uint64_t base_addr;
  uint64_t length;
  uint32_t type;
  uint32_t reserved;
} __attribute__((packed)) memory_map_entry_t;
extern uint32_t *mmap_addr;
extern uint32_t mmap_length;
extern uint32_t mmap_entries_count;
void init_memory_map(uint32_t magic, uint32_t *mb_info);
void show_memory_map(void);
uint64_t get_total_memory(void);
uint64_t get_available_memory(void);
uint32_t get_memory_map_entries_count(void);
const char *get_memory_type_string(uint32_t type);
#define MEMORY_AVAILABLE 1
#define MEMORY_RESERVED 2
#define MEMORY_ACPI_RECLAIM 3
#define MEMORY_ACPI_NVS 4
#define MEMORY_BAD 5
#endif
