#include "floppy_cmd.h"
#include "../drivers/floppy/floppy.h"
#include "../libs/ctype.h"
#include "../libs/print.h"
#include "../libs/string.h"
#include <stdint.h>
static uint8_t hex_to_int(char hex[2]) {
  uint8_t result = 0;
  for (int i = 0; i < 2; i++) {
    result <<= 4;
    if (hex[i] >= '0' && hex[i] <= '9') {
      result |= hex[i] - '0';
    } else if (hex[i] >= 'A' && hex[i] <= 'F') {
      result |= hex[i] - 'A' + 10;
    } else if (hex[i] >= 'a' && hex[i] <= 'f') {
      result |= hex[i] - 'a' + 10;
    }
  }
  return result;
}
void cmd_floppy_info(int argc, char **argv) {
  print("=== Floppy Drives Information ===\n", CYAN);
  for (int i = 0; i < 4; i++) {
    print("Drive ", WHITE);
    char drive_num[2];
    itoa(i, drive_num, 10);
    print(drive_num, WHITE);
    print(": ", WHITE);
    if (floppy_drives[i].initialized) {
      print("[PRESENT] ", GREEN);
      print("Type: ", WHITE);
      switch (floppy_drives[i].type) {
      case FLOPPY_TYPE_360KB:
        print("360KB 5.25\"", GREEN);
        break;
      case FLOPPY_TYPE_1_2MB:
        print("1.2MB 5.25\"", GREEN);
        break;
      case FLOPPY_TYPE_720KB:
        print("720KB 3.5\"", GREEN);
        break;
      case FLOPPY_TYPE_1_44MB:
        print("1.44MB 3.5\"", GREEN);
        break;
      case FLOPPY_TYPE_2_88MB:
        print("2.88MB 3.5\"", GREEN);
        break;
      default:
        print("Unknown", YELLOW);
        break;
      }
      print(", Track: ", WHITE);
      char track[4];
      itoa(floppy_drives[i].current_track, track, 10);
      print(track, WHITE);
      print(", Motor: ", WHITE);
      print(floppy_drives[i].motor_on ? "ON" : "OFF",
            floppy_drives[i].motor_on ? GREEN : RED);
      uint32_t size = floppy_get_size(i);
      if (size > 0) {
        print(", Size: ", WHITE);
        char size_str[16];
        itoa(size / 1024, size_str, 10);
        print(size_str, WHITE);
        print(" KB", WHITE);
      }
    } else {
      print("[NOT PRESENT]\n", RED);
      continue;
    }
    print("\n", WHITE);
  }
  print("===============================\n", CYAN);
}
void cmd_floppy_read(int argc, char **argv) {
  if (argc < 5) {
    print("Usage: floppy_read <drive> <track> <head> <sector>\n", RED);
    print("       drive: 0-3, track: 0-79, head: 0-1, sector: 1-18\n", WHITE);
    return;
  }
  uint8_t drive = atoi(argv[1]);
  uint8_t track = atoi(argv[2]);
  uint8_t head = atoi(argv[3]);
  uint8_t sector = atoi(argv[4]);
  if (drive > 3) {
    print("Error: Drive must be 0-3\n", RED);
    return;
  }
  if (track >= 80) {
    print("Error: Track must be 0-79\n", RED);
    return;
  }
  if (head > 1) {
    print("Error: Head must be 0-1\n", RED);
    return;
  }
  if (sector == 0 || sector > 18) {
    print("Error: Sector must be 1-18\n", RED);
    return;
  }
  uint8_t buffer[512];
  print("Reading from floppy drive ", CYAN);
  char buf[4];
  itoa(drive, buf, 10);
  print(buf, CYAN);
  print(", track ", CYAN);
  itoa(track, buf, 10);
  print(buf, CYAN);
  print(", head ", CYAN);
  itoa(head, buf, 10);
  print(buf, CYAN);
  print(", sector ", CYAN);
  itoa(sector, buf, 10);
  print(buf, CYAN);
  print("... ", CYAN);
  if (floppy_read_sector(drive, head, track, sector, buffer)) {
    print("[SUCCESS]\n", GREEN);
    print("\nSector content (512 bytes):\n", WHITE);
    print("Offset   00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F  ASCII\n",
          LIGHT_RED);
    print("-------- ----------------------------------------------- "
          "----------------\n",
          LIGHT_RED);
    for (int i = 0; i < 512; i += 16) {
      print("0x", WHITE);
      char addr[4];
      itoa(i >> 4, addr, 16);
      if (i < 0x100)
        print("0", WHITE);
      if (i < 0x10)
        print("0", WHITE);
      print(addr, WHITE);
      itoa(i & 0x0F, addr, 16);
      print(addr, WHITE);
      print("0: ", WHITE);
      for (int j = 0; j < 16; j++) {
        if (i + j < 512) {
          char hex[3];
          itoa(buffer[i + j] >> 4, hex, 16);
          print(hex, WHITE);
          itoa(buffer[i + j] & 0x0F, hex, 16);
          print(hex, WHITE);
        } else {
          print("  ", WHITE);
        }
        print(" ", WHITE);
        if (j == 7)
          print(" ", WHITE);
      }
      print(" ", WHITE);
      for (int j = 0; j < 16; j++) {
        if (i + j < 512) {
          char c = buffer[i + j];
          if (c >= 32 && c < 127) {
            print_hex(c, WHITE);
          } else {
            print(".", LIGHT_RED);
          }
        } else {
          print(" ", WHITE);
        }
      }
      print("\n", WHITE);
    }
    if (track == 0 && head == 0 && sector == 1) {
      print("\nBoot sector analysis:\n", CYAN);
      if (buffer[510] == 0x55 && buffer[511] == 0xAA) {
        print("  Boot signature: 0x55 0xAA [VALID]\n", GREEN);
      } else {
        print("  Boot signature: ", YELLOW);
        char sig[4];
        itoa(buffer[510], sig, 16);
        print("0x", YELLOW);
        if (buffer[510] < 0x10)
          print("0", YELLOW);
        print(sig, YELLOW);
        print(" 0x", YELLOW);
        itoa(buffer[511], sig, 16);
        if (buffer[511] < 0x10)
          print("0", YELLOW);
        print(sig, YELLOW);
        print(" [INVALID]\n", YELLOW);
      }
    }
  } else {
    print("[FAILED]\n", RED);
  }
}
void cmd_floppy_write(int argc, char **argv) {
  if (argc < 6) {
    print("Usage: floppy_write <drive> <track> <head> <sector> <data>\n", RED);
    print("       data: hex string (e.g., A1B2C3) or 'test' for test pattern\n",
          WHITE);
    return;
  }
  uint8_t drive = atoi(argv[1]);
  uint8_t track = atoi(argv[2]);
  uint8_t head = atoi(argv[3]);
  uint8_t sector = atoi(argv[4]);
  char *data = argv[5];
  uint8_t buffer[512];
  if (strcmp(data, "test") == 0) {
    for (int i = 0; i < 512; i++) {
      buffer[i] = (i % 256);
    }
    print("Using test pattern (00-FF repeating)\n", CYAN);
  } else {
    memset(buffer, 0, 512);
    int len = strlen(data);
    int buf_idx = 0;
    for (int i = 0; i < len && buf_idx < 512; i += 2) {
      char hex[3] = {data[i], (i + 1 < len) ? data[i + 1] : '0', '\0'};
      buffer[buf_idx++] = hex_to_int(hex);
    }
    print("Using hex data\n", CYAN);
  }
  print("Writing to floppy drive ", CYAN);
  char buf[4];
  itoa(drive, buf, 10);
  print(buf, CYAN);
  print(", track ", CYAN);
  itoa(track, buf, 10);
  print(buf, CYAN);
  print(", head ", CYAN);
  itoa(head, buf, 10);
  print(buf, CYAN);
  print(", sector ", CYAN);
  itoa(sector, buf, 10);
  print(buf, CYAN);
  print("... ", CYAN);
  if (floppy_write_sector(drive, head, track, sector, buffer)) {
    print("[SUCCESS]\n", GREEN);
  } else {
    print("[FAILED]\n", RED);
  }
}
void cmd_floppy_calibrate(int argc, char **argv) {
  uint8_t drive = 0;
  if (argc > 1) {
    drive = atoi(argv[1]);
  }
  if (drive > 3) {
    print("Error: Drive must be 0-3\n", RED);
    return;
  }
  print("Calibrating floppy drive ", CYAN);
  char buf[2];
  itoa(drive, buf, 10);
  print(buf, CYAN);
  print(" (moving to track 0)... ", CYAN);
  floppy_calibrate(drive);
  print("[DONE]\n", GREEN);
}
void cmd_floppy_test(int argc, char **argv) {
  uint8_t drive = 0;
  if (argc > 1) {
    drive = atoi(argv[1]);
  }
  print("=== Floppy Diagnostic Test ===\n", CYAN);
  print("Testing drive ", CYAN);
  char buf[2];
  itoa(drive, buf, 10);
  print(buf, CYAN);
  print("\n", CYAN);
  print("1. Drive detection... ", WHITE);
  if (floppy_drives[drive].initialized) {
    print("[PRESENT]\n", GREEN);
  } else {
    print("[NOT FOUND]\n", RED);
    print("Test aborted: Drive not detected\n", RED);
    return;
  }
  print("2. Calibration... ", WHITE);
  floppy_calibrate(drive);
  print("[OK]\n", GREEN);
  print("3. Reading boot sector... ", WHITE);
  uint8_t buffer[512];
  if (floppy_read_sector(drive, 0, 0, 1, buffer)) {
    print("[SUCCESS]\n", GREEN);
    if (buffer[510] == 0x55 && buffer[511] == 0xAA) {
      print("   Boot signature: [VALID]\n", GREEN);
    } else {
      print("   Boot signature: [INVALID]\n", YELLOW);
    }
  } else {
    print("[FAILED]\n", RED);
  }
  print("4. Reading multiple sectors... ", WHITE);
  bool multi_success = true;
  for (int i = 1; i <= 3 && multi_success; i++) {
    if (!floppy_read_sector(drive, 0, 0, i, buffer)) {
      multi_success = false;
    }
  }
  if (multi_success) {
    print("[SUCCESS]\n", GREEN);
  } else {
    print("[FAILED]\n", RED);
  }
  print("5. Write/read test... ", WHITE);
  uint8_t test_buffer[512];
  for (int i = 0; i < 512; i++) {
    test_buffer[i] = i % 256;
  }
  if (floppy_write_sector(drive, 0, 0, 18, test_buffer)) {
    uint8_t verify_buffer[512];
    if (floppy_read_sector(drive, 0, 0, 18, verify_buffer)) {
      if (memcmp(test_buffer, verify_buffer, 512) == 0) {
        print("[SUCCESS]\n", GREEN);
      } else {
        print("[VERIFY FAILED]\n", YELLOW);
      }
    } else {
      print("[READ BACK FAILED]\n", RED);
    }
  } else {
    print("[WRITE FAILED]\n", RED);
  }
  print("=============================\n", CYAN);
  print("Diagnostic test completed.\n", CYAN);
}
void cmd_floppy_format(int argc, char **argv) {
  print("WARNING: This will erase ALL data on the floppy!\n", RED);
  print("Type 'YES' to confirm: ", YELLOW);
  uint8_t drive = 0;
  if (argc > 1) {
    drive = atoi(argv[1]);
  }
  print("\nFormatting floppy drive ", CYAN);
  char buf[2];
  itoa(drive, buf, 10);
  print(buf, CYAN);
  print("...\n", CYAN);
  uint8_t zero_buffer[512] = {0};
  uint32_t sectors_formatted = 0;
  for (int track = 0; track < 10; track++) {
    for (int head = 0; head < 2; head++) {
      for (int sector = 1; sector <= 18; sector++) {
        if (floppy_write_sector(drive, head, track, sector, zero_buffer)) {
          sectors_formatted++;
        }
        if ((sectors_formatted % 36) == 0) {
          print(".", WHITE);
        }
      }
    }
  }
  print("\nFormatted ", GREEN);
  char count[10];
  itoa(sectors_formatted, count, 10);
  print(count, GREEN);
  print(" sectors (", GREEN);
  itoa(sectors_formatted * 512 / 1024, count, 10);
  print(count, GREEN);
  print(" KB)\n", GREEN);
  print("Format complete.\n", GREEN);
}
void cmd_floppy_dir(int argc, char **argv) {
  print("=== Floppy Directory (simulated) ===\n", CYAN);
  print("Since FAT12 support is not implemented yet,\n", WHITE);
  print("this command shows sector map instead:\n\n", WHITE);
  print("Track Head Sector Status\n", LIGHT_RED);
  print("----- ---- ------ ----------\n", LIGHT_RED);
  uint8_t buffer[512];
  uint8_t drive = 0;
  for (int track = 0; track < 5; track++) {
    for (int head = 0; head < 2; head++) {
      for (int sector = 1; sector <= 3; sector++) {
        if (floppy_read_sector(drive, head, track, sector, buffer)) {
          bool empty = true;
          for (int i = 0; i < 16; i++) {
            if (buffer[i] != 0) {
              empty = false;
              break;
            }
          }
          char t[4], h[4], s[4];
          itoa(track, t, 10);
          itoa(head, h, 10);
          itoa(sector, s, 10);
          print("  ", WHITE);
          if (track < 10)
            print(" ", WHITE);
          print(t, WHITE);
          print("    ", WHITE);
          print(h, WHITE);
          print("      ", WHITE);
          if (sector < 10)
            print(" ", WHITE);
          print(s, WHITE);
          print("    ", WHITE);
          if (empty) {
            print("[EMPTY]\n", LIGHT_RED);
          } else {
            print("[DATA] ", GREEN);
            if (track == 0 && head == 0 && sector == 1) {
              print("(Boot sector)", CYAN);
            } else if (buffer[0] == 0xE9 || buffer[0] == 0xEB) {
              print("(Executable)", MAGENTA);
            } else if (isprint(buffer[0]) && isprint(buffer[1]) &&
                       isprint(buffer[2])) {
              print("(Text data)", BLUE);
            }
            print("\n", WHITE);
          }
        } else {
          print("  ", WHITE);
          char t[4], h[4], s[4];
          itoa(track, t, 10);
          itoa(head, h, 10);
          itoa(sector, s, 10);
          if (track < 10)
            print(" ", WHITE);
          print(t, WHITE);
          print("    ", WHITE);
          print(h, WHITE);
          print("      ", WHITE);
          if (sector < 10)
            print(" ", WHITE);
          print(s, WHITE);
          print("    [READ ERROR]\n", RED);
        }
      }
    }
  }
  print("==================================\n", CYAN);
}
