#include "floppy.h"
#include "../libs/print.h"
#include "../modules/io/idt.h"
#include "../modules/io/io.h"
#include <stddef.h>
volatile int floppy_irq_occurred = 0;
floppy_drive_t floppy_drives[4] = {0};
#define FLOPPY_DEBUG 1
static void floppy_irq_handler() {
  floppy_irq_occurred = 1;
#ifdef FLOPPY_DEBUG
  print_info("[FLOPPY IRQ]");
#endif
}
static void floppy_delay() {
  for (int i = 0; i < 10000; i++) {
    asm volatile("nop");
  }
}
static void floppy_delay_ms(int ms) {
  for (int i = 0; i < ms; i++) {
    for (int j = 0; j < 10000; j++) {
      asm volatile("nop");
    }
  }
}
static void floppy_reset_state() {
#ifdef FLOPPY_DEBUG
  print_info("[Reset state] ");
#endif
  for (int i = 0; i < 10; i++) {
    uint8_t msr = inb(FLOPPY_MSR);
    if (msr & 0x40) {
      inb(FLOPPY_FIFO);
#ifdef FLOPPY_DEBUG
      print_info(".");
#endif
    } else {
      break;
    }
    floppy_delay();
  }
  outb(FLOPPY_DOR, 0x00);
  floppy_delay_ms(10);
  outb(FLOPPY_DOR, 0x0C);
  floppy_delay_ms(10);
  outb(FLOPPY_DOR, 0x08);
  floppy_delay_ms(10);
  for (int i = 0; i < 4; i++) {
    outb(FLOPPY_FIFO, FDC_CMD_SENSE_INTERRUPT);
    floppy_delay_ms(1);
    if (inb(FLOPPY_MSR) & 0x40) {
      inb(FLOPPY_FIFO);
    }
    if (inb(FLOPPY_MSR) & 0x40) {
      inb(FLOPPY_FIFO);
    }
  }
}
static void floppy_send_command(uint8_t cmd) {
  int timeout = 10000;
  uint8_t msr = inb(FLOPPY_MSR);
  if ((msr & 0xF0) == 0xF0 || (msr & 0xC0) == 0xC0) {
    floppy_reset_state();
  }
  for (int i = 0; i < timeout; i++) {
    msr = inb(FLOPPY_MSR);
#ifdef FLOPPY_DEBUG_VERBOSE
    if (i % 1000 == 0) {
      print_info("MSR=0x");
      char hex[4];
      itoa(msr, hex, 16);
      if (msr < 0x10)
        print_info("0");
      print_info(hex);
      print_info(" ");
    }
#endif
    if ((msr & 0xC0) == 0x80) {
      outb(FLOPPY_FIFO, cmd);
#ifdef FLOPPY_DEBUG
      print_info("[0x");
      char hex[4];
      itoa(cmd, hex, 16);
      if (cmd < 0x10)
        print_info("0");
      print_info(hex);
      print_info("]");
#endif
      floppy_delay();
      return;
    }
    if (msr & 0x40) {
      inb(FLOPPY_FIFO);
#ifdef FLOPPY_DEBUG
      print_info("[FLUSH]");
#endif
    }
    floppy_delay();
  }
#ifdef FLOPPY_DEBUG
  print_error("\nCMD TIMEOUT: MSR=0x");
  char msr_hex[4];
  itoa(inb(FLOPPY_MSR), msr_hex, 16);
  if (inb(FLOPPY_MSR) < 0x10)
    print_error("0");
  print_error(msr_hex);
  print_error(" CMD=0x");
  itoa(cmd, msr_hex, 16);
  if (cmd < 0x10)
    print_error("0");
  print_error(msr_hex);
  print_error("\n");
#endif
}
static uint8_t floppy_read_data() {
  int timeout = 1000;
  for (int i = 0; i < timeout; i++) {
    uint8_t msr = inb(FLOPPY_MSR);
    if (msr & 0x40) {
      uint8_t data = inb(FLOPPY_FIFO);
#ifdef FLOPPY_DEBUG
      print_info("[DATA:0x");
      char hex[4];
      itoa(data, hex, 16);
      if (data < 0x10)
        print_info("0");
      print_info(hex);
      print_info("]");
#endif
      return data;
    }
    floppy_delay();
  }
#ifdef FLOPPY_DEBUG
  print_error("Floppy read data timeout\n");
#endif
  return 0;
}
void floppy_init() {
  print("Initializing floppy controller... ", WHITE);
  register_interrupt_handler(38, floppy_irq_handler);
  uint8_t mask = inb(0x21);
  outb(0x21, mask & ~(1 << 6));
  print_info("[IRQ6] ");
  outb(FLOPPY_DOR, 0x00);
  floppy_delay();
  outb(FLOPPY_DOR, 0x0C);
  floppy_delay();
  for (int i = 0; i < 100; i++) {
    floppy_delay();
  }
  outb(FLOPPY_DOR, 0x08);
  print_info("[RST] ");
  outb(FLOPPY_CCR, 0x00);
  floppy_send_command(FDC_CMD_SPECIFY);
  floppy_send_command(0xDF);
  floppy_send_command(0x02);
  print_info("[CFG] ");
  for (int drive = 0; drive < 4; drive++) {
    outb(FLOPPY_DOR, 0x10 << drive);
    for (int i = 0; i < 100; i++)
      floppy_delay();
    floppy_send_command(FDC_CMD_VERSION);
    uint8_t version = floppy_read_data();
    outb(FLOPPY_DOR, 0x00);
    if (version == 0x90) {
      floppy_drives[drive].type = FLOPPY_TYPE_1_44MB;
      floppy_drives[drive].initialized = true;
      floppy_drives[drive].drive_number = drive;
      floppy_drives[drive].current_track = 0;
      floppy_drives[drive].motor_on = false;
      print_info("[D");
      char buf[2];
      itoa(drive, buf, 10);
      print_info(buf);
      print_info("] ");
    }
  }
  print("[OK]\n", GREEN);
}
bool floppy_read_sector(uint8_t drive, uint8_t head, uint8_t track,
                        uint8_t sector, void *buffer) {
  if (drive > 3 || !floppy_drives[drive].initialized) {
#ifdef FLOPPY_DEBUG
    print_error("Invalid drive or not initialized\n");
#endif
    return false;
  }
  if (track >= FLOPPY_TRACKS_PER_SIDE || head >= FLOPPY_SIDES || sector == 0 ||
      sector > FLOPPY_SECTORS_PER_TRACK) {
#ifdef FLOPPY_DEBUG
    print_error("Invalid parameters\n");
#endif
    return false;
  }
  uint8_t dor = 0x10 << drive;
  dor |= 0x04;
  dor |= drive;
  outb(FLOPPY_DOR, dor);
  for (int i = 0; i < 300; i++)
    floppy_delay();
  if (floppy_drives[drive].current_track != track) {
    floppy_send_command(FDC_CMD_SEEK);
    floppy_send_command((head << 2) | drive);
    floppy_send_command(track);
    for (int i = 0; i < 1000; i++) {
      if (inb(FLOPPY_MSR) & 0x80)
        break;
      floppy_delay();
    }
    floppy_drives[drive].current_track = track;
  }
  floppy_send_command(FDC_CMD_READ_DATA | (head ? 0x04 : 0x00));
  floppy_send_command((head << 2) | drive);
  floppy_send_command(track);
  floppy_send_command(head);
  floppy_send_command(sector);
  floppy_send_command(2);
  floppy_send_command(FLOPPY_SECTORS_PER_TRACK);
  floppy_send_command(0x1B);
  floppy_send_command(0xFF);
  bool success = false;
  for (int i = 0; i < 20000; i++) {
    uint8_t msr = inb(FLOPPY_MSR);
    if (msr & 0x80) {
      for (int j = 0; j < 512; j++) {
        ((uint8_t *)buffer)[j] = inb(FLOPPY_FIFO);
      }
      for (int j = 0; j < 7; j++) {
        inb(FLOPPY_FIFO);
      }
      success = true;
      break;
    }
    floppy_delay();
  }
  outb(FLOPPY_DOR, 0x0C);
  floppy_drives[drive].motor_on = false;
  return success;
}
bool floppy_write_sector(uint8_t drive, uint8_t head, uint8_t track,
                         uint8_t sector, void *buffer) {
  if (drive > 3 || !floppy_drives[drive].initialized) {
    return false;
  }
  uint8_t dor = 0x10 << drive;
  dor |= 0x04;
  dor |= drive;
  outb(FLOPPY_DOR, dor);
  for (int i = 0; i < 300; i++)
    floppy_delay();
  floppy_send_command(FDC_CMD_WRITE_DATA | (head ? 0x04 : 0x00));
  floppy_send_command((head << 2) | drive);
  floppy_send_command(track);
  floppy_send_command(head);
  floppy_send_command(sector);
  floppy_send_command(2);
  floppy_send_command(FLOPPY_SECTORS_PER_TRACK);
  floppy_send_command(0x1B);
  floppy_send_command(0xFF);
  for (int i = 0; i < 10000; i++) {
    uint8_t msr = inb(FLOPPY_MSR);
    if (msr & 0x80) {
      for (int j = 0; j < 512; j++) {
        outb(FLOPPY_FIFO, ((uint8_t *)buffer)[j]);
      }
      for (int j = 0; j < 7; j++) {
        inb(FLOPPY_FIFO);
      }
      outb(FLOPPY_DOR, 0x0C);
      return true;
    }
    floppy_delay();
  }
  outb(FLOPPY_DOR, 0x0C);
  return false;
}
bool floppy_read_sectors(uint8_t drive, uint8_t head, uint8_t track,
                         uint8_t start_sector, uint8_t count, void *buffer) {
  uint8_t *buf = (uint8_t *)buffer;
  for (uint8_t i = 0; i < count; i++) {
    if (!floppy_read_sector(drive, head, track, start_sector + i,
                            buf + (i * 512))) {
      return false;
    }
  }
  return true;
}
void floppy_calibrate(uint8_t drive) {
  if (drive > 3 || !floppy_drives[drive].initialized)
    return;
#ifdef FLOPPY_DEBUG
  print("Calibrating drive ", WHITE);
  char buf[2];
  itoa(drive, buf, 10);
  print(buf, WHITE);
  print("... ", WHITE);
#endif
  uint8_t dor = 0x10 << drive;
  dor |= 0x04;
  dor |= drive;
  outb(FLOPPY_DOR, dor);
  for (int i = 0; i < 300; i++)
    floppy_delay();
  floppy_send_command(FDC_CMD_RECALIBRATE);
  floppy_send_command(drive);
  for (int i = 0; i < 5000; i++) {
    uint8_t msr = inb(FLOPPY_MSR);
    if (msr & 0x80)
      break;
    floppy_delay();
  }
  outb(FLOPPY_DOR, 0x0C);
  floppy_drives[drive].current_track = 0;
#ifdef FLOPPY_DEBUG
  print("[OK]\n", GREEN);
#endif
}
void floppy_get_info(uint8_t drive) {
  if (drive > 3) {
    print("Invalid drive number\n", RED);
    return;
  }
  print("Floppy Drive ", CYAN);
  char buf[4];
  itoa(drive, buf, 10);
  print(buf, CYAN);
  print(":\n", CYAN);
  if (floppy_drives[drive].initialized) {
    print("  Type: ", WHITE);
    switch (floppy_drives[drive].type) {
    case FLOPPY_TYPE_1_44MB:
      print("1.44MB 3.5\"\n", GREEN);
      break;
    case FLOPPY_TYPE_720KB:
      print("720KB 3.5\"\n", GREEN);
      break;
    case FLOPPY_TYPE_360KB:
      print("360KB 5.25\"\n", GREEN);
      break;
    case FLOPPY_TYPE_1_2MB:
      print("1.2MB 5.25\"\n", GREEN);
      break;
    default:
      print("Unknown\n", YELLOW);
      break;
    }
    print("  Current track: ", WHITE);
    itoa(floppy_drives[drive].current_track, buf, 10);
    print(buf, WHITE);
    print("\n", WHITE);
    print("  Motor: ", WHITE);
    print(floppy_drives[drive].motor_on ? "ON\n" : "OFF\n",
          floppy_drives[drive].motor_on ? GREEN : RED);
  } else {
    print("  Not detected\n", RED);
  }
}
void floppy_test() {
  uint8_t buffer[512];
  print("\n=== Floppy Test ===\n", CYAN);
  print("Test 1: Reading boot sector... ", WHITE);
  if (floppy_read_sector(0, 0, 0, 1, buffer)) {
    print("[SUCCESS]\n", GREEN);
    if (buffer[510] == 0x55 && buffer[511] == 0xAA) {
      print("  Boot signature: 0x55 0xAA [VALID]\n", GREEN);
    } else {
      print("  Boot signature: ", YELLOW);
      char hex[4];
      itoa(buffer[510], hex, 16);
      print("0x", YELLOW);
      print(hex, YELLOW);
      print(" 0x", YELLOW);
      itoa(buffer[511], hex, 16);
      print(hex, YELLOW);
      print(" [INVALID]\n", YELLOW);
    }
    print("  First 16 bytes: ", WHITE);
    for (int i = 0; i < 16; i++) {
      char hex[4];
      itoa(buffer[i], hex, 16);
      if (buffer[i] < 0x10)
        print("0", WHITE);
      print(hex, WHITE);
      print(" ", WHITE);
    }
    print("\n", WHITE);
  } else {
    print("[FAILED]\n", RED);
  }
  print("Test 2: Calibrating drive... ", WHITE);
  floppy_calibrate(0);
  print("[DONE]\n", GREEN);
  print("Test 3: Drive information:\n", WHITE);
  floppy_get_info(0);
  print("========================\n", CYAN);
}
void floppy_set_drive(uint8_t drive) {}
void floppy_motor_on(uint8_t drive) {
  if (drive < 4) {
    floppy_drives[drive].motor_on = true;
  }
}
void floppy_motor_off(uint8_t drive) {
  if (drive < 4) {
    floppy_drives[drive].motor_on = false;
  }
}
bool floppy_write_sectors(uint8_t drive, uint8_t head, uint8_t track,
                          uint8_t start_sector, uint8_t count, void *buffer) {
  return false;
}
void floppy_seek(uint8_t drive, uint8_t head, uint8_t track) {}
bool floppy_sense_interrupt(uint8_t *st0, uint8_t *cyl) { return false; }
void floppy_wait_irq() {}
bool floppy_poll() { return false; }
uint32_t floppy_get_size(uint8_t drive) {
  return floppy_drives[drive].initialized ? FLOPPY_TOTAL_SIZE : 0;
}
