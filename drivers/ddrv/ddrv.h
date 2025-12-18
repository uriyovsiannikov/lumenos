#ifndef DDRV_H
#define DDRV_H
#include <stdbool.h>
#include <stdint.h>
#define ATA_SR_BSY 0x80
#define ATA_SR_DRDY 0x40
#define ATA_SR_DF 0x20
#define ATA_SR_DSC 0x10
#define ATA_SR_DRQ 0x08
#define ATA_SR_CORR 0x04
#define ATA_SR_IDX 0x02
#define ATA_SR_ERR 0x01
#define ATA_CMD_READ_PIO 0x20
#define ATA_CMD_WRITE_PIO 0x30
#define ATA_CMD_CACHE_FLUSH 0xE7
#define ATA_CMD_IDENTIFY 0xEC
#define ATA_PRIMARY_BASE 0x1F0
#define ATA_SECONDARY_BASE 0x170
#define ATA_STATUS_PORT(bus) ((bus) == 0 ? 0x1F7 : 0x177)
#define DISK_TIMEOUT 100000
#define SECTOR_SIZE 512
#define FS_ROOT_DIR_SECTOR 1
#define FS_FAT_SECTOR 2
#define FS_DATA_START_SECTOR 3
typedef struct {
  char model[41];
  char serial[21];
  char firmware[9];
  uint64_t total_sectors;
  uint32_t capacity_mb;
  uint32_t capacity_gb;
  uint16_t sector_size;
  bool lba48_supported;
} disk_info_t;
typedef struct {
  uint8_t bus;
  uint8_t drive;
  bool present;
  disk_info_t info;
} disk_drive_t;
typedef struct {
  uint64_t total_sectors;
  uint64_t used_sectors;
  uint64_t free_sectors;
  uint32_t sector_size;
  uint64_t total_bytes;
  uint64_t free_bytes;
} disk_space_info_t;
bool disk_init(void);
bool disk_read_sector(uint32_t lba, uint8_t *buffer);
bool disk_write_sector(uint32_t lba, uint8_t *buffer);
bool disk_read_multiple_sectors(uint32_t lba, uint8_t num_sectors,
                                uint8_t *buffer);
bool disk_write_multiple_sectors(uint32_t lba, uint8_t num_sectors,
                                 uint8_t *buffer);
bool disk_flush_cache(void);
uint8_t disk_get_count(void);
disk_drive_t *disk_get_current(void);
disk_drive_t *disk_get_drive(uint8_t index);
bool disk_select_drive(uint8_t index);
void disk_print_info(void);
uint32_t disk_get_sector_count(void);
uint32_t disk_get_capacity_mb(void);
disk_space_info_t disk_get_space_info(void);
bool disk_format(void);
void disk_dump_sector(uint32_t lba);
static void io_delay(void);
static uint8_t disk_read_byte(uint16_t port);
static void disk_write_byte(uint16_t port, uint8_t value);
static uint16_t disk_read_word(uint16_t port);
static void disk_write_word(uint16_t port, uint16_t value);
static bool disk_wait_ready(uint8_t bus);
static bool disk_wait_data(uint8_t bus);
static bool disk_identify(uint8_t bus, uint8_t drive, disk_info_t *info);
static bool simple_disk_read(uint32_t lba, uint8_t *buffer);
static bool simple_disk_write(uint32_t lba, uint8_t *buffer);
bool disk_format_lowlevel(void);
#endif
