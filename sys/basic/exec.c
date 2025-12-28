#include "exec.h"
#include "../../libs/print.h"
#include "../../libs/string.h"
#include "../mm/mm.h"
#include "../sys/fs/fs.h"
#include "../sys/io/keyboard.h"
#include "../sys/syslogger/syslogger.h"
#include <ctype.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
extern uint32_t current_dir_id;
static int variables[MAX_VARIABLES];
static char program_buffer[MAX_PROGRAM_SIZE];
static int program_lines = 0;
extern char get_char_from_scancode(uint8_t scancode);
extern uint8_t get_key(void);
extern bool key_available(void);
typedef struct {
  int line_number;
  char code[100];
} program_line_t;
static program_line_t program[100];
void clear_basic_memory(void) {
  log_message("Trying to clear BASIC memory", LOG_WARNING);
  memset(variables, 0, sizeof(variables));
  memset(program_buffer, 0, sizeof(program_buffer));
  memset(program, 0, sizeof(program));
  program_lines = 0;
  change_color(WHITE);
  log_message("BASIC memory cleared", LOG_INFO);
}
int exec_load(const char *filename) {
  log_message("Trying exec BASIC code", LOG_WARNING);
  char buffer[512];
  int bytes_read =
      fs_read((char *)filename, buffer, sizeof(buffer), current_dir_id);
  if (bytes_read < (int)sizeof(lumen_header_t)) {
    print_error("Cannot read file or file too small");
    log_message("BASIC file reading error", LOG_ERROR);
    return -1;
  }
  lumen_header_t *header = (lumen_header_t *)buffer;
  if (header->magic != LUMEN_MAGIC) {
    print_error("Invalid executable format");
    log_message("BASIC file format error", LOG_ERROR);
    return -1;
  }
  print("Loading: ", WHITE);
  print(header->name, CYAN);
  print("\n", WHITE);
  return 0;
}
int exec_run(const char *filename) {
  log_message("Trying run BASIC code", LOG_WARNING);
  char buffer[4096];
  int bytes_read =
      fs_read((char *)filename, buffer, sizeof(buffer), current_dir_id);
  if (bytes_read <= 0) {
    print_error("Cannot read executable file");
    log_message("BASIC file reading error", LOG_ERROR);
    return -1;
  }
  lumen_header_t *header = (lumen_header_t *)buffer;
  if (header->magic != LUMEN_MAGIC) {
    print_error("Not a valid LumenOS executable");
    log_message("BASIC file running error", LOG_ERROR);
    return -1;
  }
  print_success("Running: ");
  print(header->name, CYAN);
  print("\n", WHITE);
  if (header->text_size > 0) {
    char *basic_code = (char *)(buffer + sizeof(lumen_header_t));
    basic_run(basic_code);
  }
  clear_basic_memory();
  return 0;
}
int execute_basic_line(const char *line) {
  char command[20];
  char arg1[50], arg2[50], arg3[50], arg4[50];
  const char *code = line;
  if (isdigit(*code)) {
    while (isdigit(*code))
      code++;
    while (*code == ' ')
      code++;
  }
  if (strlen(code) == 0)
    return 0;
  int parsed = sscanf(code, "%s %s %s %s %s", command, arg1, arg2, arg3, arg4);
  if (strcmp(command, "PRINT") == 0) {
    if (arg1[0] == '"') {
      const char *start_quote = strchr(code, '"');
      if (start_quote) {
        const char *end_quote = strchr(start_quote + 1, '"');
        if (end_quote) {
          int len = end_quote - (start_quote + 1);
          char text[100];
          strncpy(text, start_quote + 1, len);
          text[len] = '\0';
          print(text, WHITE);
          print("\n", WHITE);
        } else {
          print(start_quote + 1, WHITE);
          print("\n", WHITE);
        }
      }
    } else if (isalpha(arg1[0])) {
      int var_index = toupper(arg1[0]) - 'A';
      if (var_index >= 0 && var_index < MAX_VARIABLES) {
        print_dec(variables[var_index], WHITE);
        print("\n", WHITE);
      }
    } else {
      print(arg1, WHITE);
      print("\n", WHITE);
    }
  } else if (strcmp(command, "LET") == 0 && parsed >= 3) {
    if (isalpha(arg1[0]) && strcmp(arg2, "=") == 0) {
      int var_index = toupper(arg1[0]) - 'A';
      if (var_index >= 0 && var_index < MAX_VARIABLES) {
        if (strcmp(arg3, "RND") == 0) {
          int max = atoi(arg4);
          variables[var_index] = (int)((uint32_t)&var_index % (max + 1));
        } else {
          variables[var_index] = atoi(arg3);
        }
      }
    }
  } else if (strcmp(command, "INPUT") == 0 && isalpha(arg1[0])) {
    print("? ", CYAN);
    int var_index = toupper(arg1[0]) - 'A';
    if (var_index >= 0 && var_index < MAX_VARIABLES) {
      char input_buffer[20];
      int input_pos = 0;
      uint8_t scancode;
      char c;
      while (1) {
        scancode = get_key();
        c = get_char_from_scancode(scancode);
        if (scancode == 0x1C) {
          break;
        } else if (scancode == 0x0E && input_pos > 0) {
          input_pos--;
          print("\b \b", WHITE);
        } else if (isdigit(c) && input_pos < sizeof(input_buffer) - 1) {
          input_buffer[input_pos++] = c;
          putchar(c, WHITE);
        }
      }
      input_buffer[input_pos] = '\0';
      print("\n", WHITE);
      if (input_pos > 0) {
        variables[var_index] = atoi(input_buffer);
      } else {
        variables[var_index] = 0;
      }
      print("Variable ", WHITE);
      putchar(toupper(arg1[0]), CYAN);
      print(" = ", WHITE);
      print_dec(variables[var_index], CYAN);
      print("\n", WHITE);
    }
  } else if (strcmp(command, "CLS") == 0) {
    clear_screen();
  } else if (strcmp(command, "REM") == 0) {
  } else if (strcmp(command, "END") == 0 || strcmp(command, "EXIT") == 0) {
    return -1;
  } else if (strcmp(command, "POKE") == 0 && parsed >= 3) {
    uint32_t addr = strtol(arg1, NULL, 16);
    uint8_t value = atoi(arg2);
    *((uint8_t *)addr) = value;
    print("POKE ", WHITE);
    print_hex(addr, CYAN);
    print(" = ", WHITE);
    print_dec(value, CYAN);
    print("\n", WHITE);
  } else if (strcmp(command, "PEEK") == 0 && parsed >= 2) {
    uint32_t addr = strtol(arg1, NULL, 16);
    uint8_t value = *((uint8_t *)addr);
    print("PEEK(", WHITE);
    print_hex(addr, CYAN);
    print(") = ", WHITE);
    print_dec(value, CYAN);
    print("\n", WHITE);
  } else if (strcmp(command, "WAIT") == 0 && parsed >= 2) {
    int delay_ms = atoi(arg1);
    print("Waiting ", WHITE);
    print_dec(delay_ms, CYAN);
    print(" ms...\n", WHITE);
    for (volatile int i = 0; i < delay_ms * 1000; i++)
      ;
  } else if (strcmp(command, "BEEP") == 0) {
    speaker_beep_nonblocking(1000, 200);
    print("BEEP!\n", CYAN);
  } else if (strcmp(command, "COLOR") == 0 && parsed >= 2) {
    if (strcmp(arg1, "red") == 0)
      change_color(RED);
    else if (strcmp(arg1, "green") == 0)
      change_color(GREEN);
    else if (strcmp(arg1, "blue") == 0)
      change_color(BLUE);
    else if (strcmp(arg1, "cyan") == 0)
      change_color(CYAN);
    else if (strcmp(arg1, "magenta") == 0)
      change_color(MAGENTA);
    else if (strcmp(arg1, "yellow") == 0)
      change_color(YELLOW);
    else if (strcmp(arg1, "white") == 0)
      change_color(WHITE);
    else
      print_error("Invalid color");
  } else if (strcmp(command, "LIST") == 0) {
    print("Current program:\n", CYAN);
    for (int i = 0; i < program_lines && i < 10; i++) {
      print_dec(program[i].line_number, WHITE);
      print(" ", WHITE);
      print(program[i].code, WHITE);
      print("\n", WHITE);
    }
    if (program_lines == 0) {
      print("No program in memory\n", YELLOW);
    }
  } else if (strcmp(command, "NEW") == 0) {
    clear_basic_memory();
    print("Program memory cleared\n", GREEN);
  } else if (strcmp(command, "RUN") == 0) {
    if (program_lines > 0) {
      print("Running program from memory...\n", GREEN);
      for (int i = 0; i < program_lines; i++) {
        if (execute_basic_line(program[i].code) == -1)
          break;
      }
      clear_basic_memory();
    } else {
      print("No program in memory\n", YELLOW);
    }
  } else if (strcmp(command, "SAVE") == 0 && parsed >= 2) {
    create_basic_executable(arg1, program_buffer);
    print("Program saved to ", GREEN);
    print(arg1, CYAN);
    print("\n", WHITE);
  } else if (strcmp(command, "LOAD") == 0 && parsed >= 2) {
    char buffer[4096];
    int bytes_read = fs_read(arg1, buffer, sizeof(buffer), current_dir_id);
    if (bytes_read > 0) {
      lumen_header_t *header = (lumen_header_t *)buffer;
      if (header->magic == LUMEN_MAGIC && header->text_size > 0) {
        char *basic_code = (char *)(buffer + sizeof(lumen_header_t));
        strncpy(program_buffer, basic_code, sizeof(program_buffer) - 1);
        program_buffer[sizeof(program_buffer) - 1] = '\0';
        print("Program loaded from ", GREEN);
        print(arg1, CYAN);
        print("\n", WHITE);
      }
    } else {
      print_error("Cannot load program");
    }
  } else {
    print("Unknown command: ", RED);
    print(command, WHITE);
    print("\n", WHITE);
  }
  return 0;
}
void basic_run(const char *program_text) {
  clear_basic_memory();
  char line[100];
  const char *ptr = program_text;
  int pos = 0;
  int line_count = 0;
  char *lines[50];
  char program_copy[1024];
  strncpy(program_copy, program_text, sizeof(program_copy) - 1);
  program_copy[sizeof(program_copy) - 1] = '\0';
  char *token = strtok(program_copy, "\n");
  while (token != NULL && line_count < 50) {
    lines[line_count] = token;
    line_count++;
    token = strtok(NULL, "\n");
  }
  for (int i = 0; i < line_count; i++) {
    if (strlen(lines[i]) > 0) {
      if (execute_basic_line(lines[i]) == -1)
        break;
    }
  }
  clear_basic_memory();
}
void create_basic_executable(const char *filename, const char *basic_code) {
  log_message("Trying to create BASIC executable", LOG_WARNING);
  char buffer[512];
  lumen_header_t *header = (lumen_header_t *)buffer;
  header->magic = LUMEN_MAGIC;
  header->entry_point = 0;
  header->text_size = strlen(basic_code);
  header->data_size = 0;
  header->bss_size = 0;
  header->stack_size = 1024;
  strcpy(header->name, filename);
  memcpy(buffer + sizeof(lumen_header_t), basic_code, strlen(basic_code));
  extern fs_file_entry_t fs_files[FS_MAX_FILES];
  extern uint32_t fs_current_dir;
  bool file_exists = false;
  for (int i = 0; i < FS_MAX_FILES; i++) {
    if (fs_files[i].is_used && !fs_files[i].is_dir &&
        strcmp(fs_files[i].name, filename) == 0 &&
        fs_files[i].parent_dir == fs_current_dir) {
      file_exists = true;
      break;
    }
  }
  if (file_exists) {
    fs_delete((char *)filename, fs_current_dir);
    log_message("File already exists", LOG_WARNING);
  }
  fs_create((char *)filename, 0, fs_current_dir, false);
  fs_write((char *)filename, buffer,
           sizeof(lumen_header_t) + strlen(basic_code), fs_current_dir);
  print_success("Created BASIC executable: ");
  print(filename, CYAN);
  print("\n", WHITE);
  log_message("BASIC executable created", LOG_INFO);
}
