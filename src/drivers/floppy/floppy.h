#ifndef FLOPPY_H
#define FLOPPY_H
#include "../libs/string.h"
#include <stdbool.h>
#include <stdint.h>
#define FLOPPY_DOR 0x3F2
#define FLOPPY_MSR 0x3F4
#define FLOPPY_FIFO 0x3F5
#define FLOPPY_CCR 0x3F7
#define FLOPPY_DMA 0x08
#define FDC_CMD_SPECIFY 0x03
#define FDC_CMD_SENSE_DRIVE 0x04
#define FDC_CMD_WRITE_DATA 0x05
#define FDC_CMD_READ_DATA 0x06
#define FDC_CMD_RECALIBRATE 0x07
#define FDC_CMD_SENSE_INTERRUPT 0x08
#define FDC_CMD_SEEK 0x0F
#define FDC_CMD_VERSION 0x10
#define FDC_CMD_CONFIGURE 0x13
#define FDC_CMD_LOCK 0x14
#define FDC_CMD_SEEK_TRACK 0x0F
#define FLOPPY_ST_BUSY 0x10
#define FLOPPY_ST_DMA 0x20
#define FLOPPY_ST_DATAIO 0x40
#define FLOPPY_ST_READY 0x80
#define FLOPPY_TYPE_NONE 0
#define FLOPPY_TYPE_360KB 1
#define FLOPPY_TYPE_1_2MB 2
#define FLOPPY_TYPE_720KB 3
#define FLOPPY_TYPE_1_44MB 4
#define FLOPPY_TYPE_2_88MB 5
#define FLOPPY_SECTORS_PER_TRACK 18
#define FLOPPY_TRACKS_PER_SIDE 80
#define FLOPPY_SIDES 2
#define FLOPPY_SECTOR_SIZE 512
#define FLOPPY_MAX_SECTORS                                                     \
  (FLOPPY_TRACKS_PER_SIDE * FLOPPY_SIDES * FLOPPY_SECTORS_PER_TRACK)
#define FLOPPY_TOTAL_SIZE (FLOPPY_MAX_SECTORS * FLOPPY_SECTOR_SIZE)
typedef struct {
  uint8_t type;
  uint8_t drive_number;
  bool motor_on;
  uint8_t current_track;
  bool initialized;
  uint8_t step_rate;
  uint8_t head_unload_time;
  uint8_t head_load_time;
  uint8_t motor_off_time;
} floppy_drive_t;
typedef struct {
  uint32_t address;
  uint16_t count;
  bool write;
} dma_transfer_t;
void floppy_init();
void floppy_reset();
bool floppy_detect_drive(uint8_t drive);
void floppy_set_drive(uint8_t drive);
void floppy_motor_on(uint8_t drive);
void floppy_motor_off(uint8_t drive);
bool floppy_read_sector(uint8_t drive, uint8_t head, uint8_t track,
                        uint8_t sector, void *buffer);
bool floppy_write_sector(uint8_t drive, uint8_t head, uint8_t track,
                         uint8_t sector, void *buffer);
bool floppy_read_sectors(uint8_t drive, uint8_t head, uint8_t track,
                         uint8_t start_sector, uint8_t count, void *buffer);
bool floppy_write_sectors(uint8_t drive, uint8_t head, uint8_t track,
                          uint8_t start_sector, uint8_t count, void *buffer);
uint32_t floppy_get_size(uint8_t drive);
void floppy_seek(uint8_t drive, uint8_t head, uint8_t track);
void floppy_calibrate(uint8_t drive);
bool floppy_sense_interrupt(uint8_t *st0, uint8_t *cyl);
void floppy_wait_irq();
bool floppy_poll();
static void floppy_send_command(uint8_t cmd);
static uint8_t floppy_read_data();
static void floppy_write_ccr(uint8_t data_rate);
static void setup_dma(uint32_t phys_addr, uint16_t count, bool write);
static void floppy_delay();
extern volatile int floppy_irq_occurred;
extern floppy_drive_t floppy_drives[4];
#endif
