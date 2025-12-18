#include "spsheet.h"
#include "../libs/ctype.h"
#include "../libs/print.h"
#include "../libs/string.h"
#include "../modules/fs/fs.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>
struct spreadsheet_cell {
  char value[CELL_VALUE_LENGTH];
};
static struct spreadsheet_cell spsheet[SPREADSHEET_ROWS][SPREADSHEET_COLS];
static uint8_t spsheet_initialized = 0;
static int s_toupper(int c) { return (c >= 'a' && c <= 'z') ? c - 32 : c; }
void spsheet_init() {
  if (!spsheet_initialized) {
    for (int i = 0; i < SPREADSHEET_ROWS; i++) {
      for (int j = 0; j < SPREADSHEET_COLS; j++) {
        spsheet[i][j].value[0] = '\0';
      }
    }
    spsheet_initialized = 1;
  }
}
int spsheet_parse_cell(const char *cell_ref, int *row, int *col) {
  if (strlen(cell_ref) < 2)
    return 0;
  char col_char = s_toupper(cell_ref[0]);
  if (col_char < 'A' || col_char > 'A' + SPREADSHEET_COLS - 1)
    return 0;
  *col = col_char - 'A';
  *row = atoi(cell_ref + 1) - 1;
  if (*row < 0 || *row >= SPREADSHEET_ROWS)
    return 0;
  return 1;
}
void spsheet_fill(const char *cell_ref, const char *value) {
  int row, col;
  if (!spsheet_parse_cell(cell_ref, &row, &col)) {
    print_error("Invalid cell reference");
    return;
  }
  if (strlen(value) >= CELL_VALUE_LENGTH) {
    print_error("Value too long");
    return;
  }
  strcpy(spsheet[row][col].value, value);
  print_success("Cell filled");
}
void spsheet_show() {
  print_info("Spreadsheet:");
  print("\n          ", WHITE);
  for (int j = 0; j < SPREADSHEET_COLS; j++) {
    print("    ", WHITE);
    putchar('A' + j, CYAN);
    print("     ", WHITE);
  }
  print("\n", WHITE);
  print("    +", WHITE);
  for (int j = 0; j < SPREADSHEET_COLS; j++) {
    print("---------+", WHITE);
  }
  print("\n", WHITE);
  for (int i = 0; i < SPREADSHEET_ROWS; i++) {
    if (i < 9) {
      print("  ", WHITE);
      print_dec(i + 1, CYAN);
    } else {
      print(" ", WHITE);
      print_dec(i + 1, CYAN);
    }
    print(" |", WHITE);
    for (int j = 0; j < SPREADSHEET_COLS; j++) {
      char display[10] = "         ";
      int len = strlen(spsheet[i][j].value);
      if (len > 0) {
        int copy_len = (len > 9) ? 9 : len;
        strncpy(display, spsheet[i][j].value, copy_len);
        for (int k = copy_len; k < 9; k++) {
          display[k] = ' ';
        }
      }
      print(display, WHITE);
      print("|", WHITE);
    }
    print("\n", WHITE);
    print("    +", WHITE);
    for (int j = 0; j < SPREADSHEET_COLS; j++) {
      print("---------+", WHITE);
    }
    print("\n", WHITE);
  }
}
void spsheet_clear() {
  for (int i = 0; i < SPREADSHEET_ROWS; i++) {
    for (int j = 0; j < SPREADSHEET_COLS; j++) {
      spsheet[i][j].value[0] = '\0';
    }
  }
  print_success("Spreadsheet cleared");
}
void spsheet_save(const char *filename) {
  fs_delete((char *)filename, current_dir_id);
  if (fs_create((char *)filename, 0, current_dir_id, false) < 0) {
    print_error("Cannot create sheet file");
    return;
  }
  char big_buffer[1024] = {0};
  char *ptr = big_buffer;
  for (int i = 0; i < SPREADSHEET_ROWS; i++) {
    for (int j = 0; j < SPREADSHEET_COLS; j++) {
      const char *value = spsheet[i][j].value;
      while (*value) {
        *ptr++ = *value++;
      }
      *ptr++ = '\n';
    }
  }
  *ptr = '\0';
  fs_write((char *)filename, big_buffer, strlen(big_buffer), current_dir_id);
  print_success("Spreadsheet saved to ");
  print(filename, GREEN);
  print("\n", WHITE);
}
void spsheet_load(const char *filename) {
  char buffer[1024];
  int bytes_read =
      fs_read((char *)filename, buffer, sizeof(buffer) - 1, current_dir_id);
  if (bytes_read <= 0) {
    print_error("Cannot read sheet file");
    return;
  }
  buffer[bytes_read] = '\0';
  spsheet_clear();
  int row = 0;
  int col = 0;
  char *line_start = buffer;
  for (int i = 0; i <= bytes_read; i++) {
    if (buffer[i] == '\n' || buffer[i] == '\0') {
      if (row < SPREADSHEET_ROWS && col < SPREADSHEET_COLS) {
        int line_len = &buffer[i] - line_start;
        if (line_len > 0) {
          int copy_len = (line_len > CELL_VALUE_LENGTH - 1)
                             ? CELL_VALUE_LENGTH - 1
                             : line_len;
          strncpy(spsheet[row][col].value, line_start, copy_len);
          spsheet[row][col].value[copy_len] = '\0';
        }
      }
      col++;
      if (col >= SPREADSHEET_COLS) {
        col = 0;
        row++;
      }
      line_start = &buffer[i] + 1;
      if (buffer[i] == '\0' || row >= SPREADSHEET_ROWS)
        break;
    }
  }
  print_success("Spreadsheet loaded from ");
  print(filename, GREEN);
  print("\n", WHITE);
}
