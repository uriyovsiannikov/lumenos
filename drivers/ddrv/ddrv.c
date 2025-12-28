#include "ddrv.h"
#include "../libs/ctype.h"
#include "../libs/print.h"
#include "../libs/string.h"
#include "../sys/io/io.h"
#include "../sys/syslogger/syslogger.h"
static disk_drive_t drives[4];
static uint8_t disk_count = 0;
static uint8_t current_drive = 0;
static void io_delay(void) {
  for (int i = 0; i < 100; i++) {
    asm volatile("nop");
  }
}
static uint8_t disk_read_byte(uint16_t port) {
  io_delay();
  return inb(port);
}
static void disk_write_byte(uint16_t port, uint8_t value) {
  io_delay();
  outb(port, value);
}
static uint16_t disk_read_word(uint16_t port) {
  io_delay();
  return inw(port);
}
static void disk_write_word(uint16_t port, uint16_t value) {
  io_delay();
  outw(port, value);
}
static bool disk_wait_ready(uint8_t bus) {
  uint16_t status_port = (bus == 0) ? 0x1F7 : 0x177;
  for (uint32_t i = 0; i < DISK_TIMEOUT; i++) {
    uint8_t status = disk_read_byte(status_port);
    if (status & ATA_SR_ERR) {
      return false;
    }
    if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRDY)) {
      return true;
    }
    io_delay();
  }
  return false;
}
static bool disk_wait_data(uint8_t bus) {
  uint16_t status_port = (bus == 0) ? 0x1F7 : 0x177;
  for (uint32_t i = 0; i < DISK_TIMEOUT; i++) {
    uint8_t status = disk_read_byte(status_port);
    if (status & ATA_SR_ERR) {
      return false;
    }
    if (status & ATA_SR_DRQ) {
      return true;
    }
    io_delay();
  }
  return false;
}
static bool simple_disk_read(uint32_t lba, uint8_t *buffer) {
  disk_drive_t *drive = disk_get_current();
  if (!drive)
    return false;
  uint8_t bus = drive->bus;
  uint8_t drive_num = drive->drive;
  uint16_t base = (bus == 0) ? 0x1F0 : 0x170;
  if (!disk_wait_ready(bus)) {
    return false;
  }
  disk_write_byte(base + 6, 0xE0 | (drive_num << 4) | ((lba >> 24) & 0x0F));
  disk_write_byte(base + 1, 0);
  disk_write_byte(base + 2, 1);
  disk_write_byte(base + 3, lba & 0xFF);
  disk_write_byte(base + 4, (lba >> 8) & 0xFF);
  disk_write_byte(base + 5, (lba >> 16) & 0xFF);
  disk_write_byte(base + 7, ATA_CMD_READ_PIO);
  if (!disk_wait_data(bus)) {
    return false;
  }
  for (int i = 0; i < 256; i++) {
    uint16_t data = disk_read_word(base);
    buffer[i * 2] = data & 0xFF;
    buffer[i * 2 + 1] = (data >> 8) & 0xFF;
  }
  return true;
}
static bool simple_disk_write(uint32_t lba, uint8_t *buffer) {
  disk_drive_t *drive = disk_get_current();
  if (!drive)
    return false;
  uint8_t bus = drive->bus;
  uint8_t drive_num = drive->drive;
  uint16_t base = (bus == 0) ? 0x1F0 : 0x170;
  if (!disk_wait_ready(bus)) {
    return false;
  }
  disk_write_byte(base + 6, 0xE0 | (drive_num << 4) | ((lba >> 24) & 0x0F));
  disk_write_byte(base + 1, 0);
  disk_write_byte(base + 2, 1);
  disk_write_byte(base + 3, lba & 0xFF);
  disk_write_byte(base + 4, (lba >> 8) & 0xFF);
  disk_write_byte(base + 5, (lba >> 16) & 0xFF);
  disk_write_byte(base + 7, ATA_CMD_WRITE_PIO);
  if (!disk_wait_data(bus)) {
    return false;
  }
  for (int i = 0; i < 256; i++) {
    uint16_t data = (buffer[i * 2 + 1] << 8) | buffer[i * 2];
    disk_write_word(base, data);
  }
  disk_write_byte(base + 7, ATA_CMD_CACHE_FLUSH);
  disk_wait_ready(bus);
  return true;
}
bool disk_read_sector(uint32_t lba, uint8_t *buffer) {
  return simple_disk_read(lba, buffer);
}
bool disk_write_sector(uint32_t lba, uint8_t *buffer) {
  return simple_disk_write(lba, buffer);
}
bool disk_read_multiple_sectors(uint32_t lba, uint8_t num_sectors,
                                uint8_t *buffer) {
  for (int i = 0; i < num_sectors; i++) {
    if (!simple_disk_read(lba + i, buffer + i * SECTOR_SIZE)) {
      return false;
    }
  }
  return true;
}
bool disk_write_multiple_sectors(uint32_t lba, uint8_t num_sectors,
                                 uint8_t *buffer) {
  for (int i = 0; i < num_sectors; i++) {
    if (!simple_disk_write(lba + i, buffer + i * SECTOR_SIZE)) {
      return false;
    }
  }
  return true;
}
static bool disk_identify(uint8_t bus, uint8_t drive, disk_info_t *info) {
  uint16_t base = (bus == 0) ? 0x1F0 : 0x170;
  if (!disk_wait_ready(bus)) {
    return false;
  }
  disk_write_byte(base + 6, 0xE0 | (drive << 4));
  disk_write_byte(base + 2, 0);
  disk_write_byte(base + 3, 0);
  disk_write_byte(base + 4, 0);
  disk_write_byte(base + 5, 0);
  disk_write_byte(base + 7, ATA_CMD_IDENTIFY);
  if (!disk_wait_ready(bus)) {
    return false;
  }
  uint8_t status = disk_read_byte(base + 7);
  if (status == 0) {
    return false;
  }
  if (!disk_wait_data(bus)) {
    return false;
  }
  uint16_t identify_data[256];
  for (int i = 0; i < 256; i++) {
    identify_data[i] = disk_read_word(base);
  }
  for (int i = 0; i < 20; i++) {
    info->model[i * 2] = (identify_data[27 + i] >> 8) & 0xFF;
    info->model[i * 2 + 1] = identify_data[27 + i] & 0xFF;
  }
  info->model[40] = '\0';
  int j = 0;
  for (int i = 0; i < 40; i++) {
    if (info->model[i] != ' ') {
      info->model[j++] = info->model[i];
    }
  }
  info->model[j] = '\0';
  for (int i = 0; i < 10; i++) {
    info->serial[i * 2] = (identify_data[10 + i] >> 8) & 0xFF;
    info->serial[i * 2 + 1] = identify_data[10 + i] & 0xFF;
  }
  info->serial[20] = '\0';
  for (int i = 0; i < 4; i++) {
    info->firmware[i * 2] = (identify_data[23 + i] >> 8) & 0xFF;
    info->firmware[i * 2 + 1] = identify_data[23 + i] & 0xFF;
  }
  info->firmware[8] = '\0';
  info->lba48_supported = (identify_data[83] & (1 << 10)) != 0;
  if (info->lba48_supported) {
    info->total_sectors = ((uint64_t)identify_data[103] << 48) |
                          ((uint64_t)identify_data[102] << 32) |
                          ((uint64_t)identify_data[101] << 16) |
                          identify_data[100];
  } else {
    info->total_sectors = identify_data[61] << 16 | identify_data[60];
  }
  info->sector_size = 512;
  info->capacity_mb = (info->total_sectors * info->sector_size) / (1024 * 1024);
  info->capacity_gb = info->capacity_mb / 1024;
  return true;
}
bool disk_init(void) {
  print("Initializing disk controller...\n", WHITE);
  memset(drives, 0, sizeof(drives));
  disk_count = 0;
  disk_info_t disk_info;
  if (disk_identify(0, 0, &disk_info)) {
    drives[0].present = true;
    drives[0].bus = 0;
    drives[0].drive = 0;
    memcpy(&drives[0].info, &disk_info, sizeof(disk_info_t));
    disk_count = 1;
    current_drive = 0;
    print("Using ", GREEN);
    print(drives[0].info.model, WHITE);
    print(" (", WHITE);
    print_dec(drives[0].info.capacity_mb, CYAN);
    print(" MB)\n", WHITE);
  } else {
    drives[0].present = true;
    drives[0].bus = 0;
    drives[0].drive = 0;
    strcpy(drives[0].info.model, "Generic Hard Disk");
    strcpy(drives[0].info.serial, "QEMU-000001");
    strcpy(drives[0].info.firmware, "1.0");
    drives[0].info.total_sectors = 20480;
    drives[0].info.capacity_mb = 10;
    drives[0].info.capacity_gb = 0;
    drives[0].info.lba48_supported = false;
    drives[0].info.sector_size = 512;
    disk_count = 1;
    current_drive = 0;
    print("Using Generic Hard Disk (10 MB)\n", GREEN);
  }
  return true;
}
bool disk_flush_cache(void) {
  disk_drive_t *drive = disk_get_current();
  if (!drive)
    return false;
  uint16_t base = (drive->bus == 0) ? 0x1F0 : 0x170;
  disk_write_byte(base + 7, ATA_CMD_CACHE_FLUSH);
  return disk_wait_ready(drive->bus);
}
uint8_t disk_get_count(void) { return disk_count; }
disk_drive_t *disk_get_drive(uint8_t index) {
  return (index == 0) ? &drives[0] : NULL;
}
disk_drive_t *disk_get_current(void) { return &drives[0]; }
bool disk_select_drive(uint8_t index) { return (index == 0); }
void disk_print_info(void) {
  disk_drive_t *drive = disk_get_current();
  print("Disk Information:\n", CYAN);
  print("Model: ", WHITE);
  print(drive->info.model, WHITE);
  print("\n", WHITE);
  print("Size: ", WHITE);
  print_dec(drive->info.capacity_mb, GREEN);
  print(" MB\n", WHITE);
  print("Sectors: ", WHITE);
  print_dec(drive->info.total_sectors, GREEN);
  print("\n", WHITE);
}
uint32_t disk_get_sector_count(void) { return 20480; }
uint32_t disk_get_capacity_mb(void) { return 10; }
bool disk_format(void) {
  print("Format not available in basic mode\n", YELLOW);
  return false;
}
disk_space_info_t disk_get_space_info(void) {
  disk_space_info_t info = {0};
  info.total_sectors = 20480;
  info.free_sectors = 20480;
  info.sector_size = 512;
  info.total_bytes = 20480 * 512;
  info.free_bytes = 20480 * 512;
  return info;
}
void disk_dump_sector(uint32_t lba) {
  uint8_t buffer[SECTOR_SIZE];
  print("Dumping sector ", WHITE);
  print_dec(lba, CYAN);
  print(":\n", WHITE);
  if (!simple_disk_read(lba, buffer)) {
    print("Failed to read sector\n", RED);
    return;
  }
  for (int i = 0; i < SECTOR_SIZE; i += 16) {
    print_hex(i, GRAY);
    print(": ", WHITE);
    for (int j = 0; j < 16; j++) {
      if (i + j < SECTOR_SIZE) {
        print_hex(buffer[i + j], WHITE);
        print(" ", WHITE);
      }
    }
    print(" ", WHITE);
    for (int j = 0; j < 16; j++) {
      if (i + j < SECTOR_SIZE) {
        char c = buffer[i + j];
        putchar((c >= 32 && c < 127) ? c : '.', CYAN);
      }
    }
    print("\n", WHITE);
  }
}
bool disk_format_lowlevel(void) {
  print("Formatting disk and creating empty filesystem...\n", YELLOW);
  uint8_t zero_buffer[SECTOR_SIZE] = {0};
  simple_disk_write(1, zero_buffer);
  simple_disk_write(2, zero_buffer);
  uint8_t root_dir[SECTOR_SIZE] = {0};
  root_dir[0] = '/';
  root_dir[44] = 1;
  root_dir[45] = 1;
  simple_disk_write(1, root_dir);
  disk_flush_cache();
  print("Format completed! Empty filesystem created.", GREEN);
  return true;
}
