#include "cmdhandler.h"
#include "consoleutils.h"
#include "../libs/print.h"
#include "../libs/ctype.h"
#include "../libs/string.h"
#include "../include/stdio.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
// ================ APPS ENTRIES =====================
#include "../apps/calc.h"
#include "../apps/clock.h"
#include "../apps/spsheet.h"
#include "../apps/sysinfo.h"
#include "../apps/tedit.h"
#include "../apps/asciitb.h"
#include "../apps/memmap.h"
#include "../apps/cowsay.h"
// ================= SYS ENTRIES =====================
#include "../sys/basic/exec.h"
#include "../sys/console/aliases.h"
#include "../sys/floppy/floppy_cmd.h"
#include "../sys/clipboard/clipboard.h"
#include "../sys/fs/fs.h"
#include "../sys/io/io.h"
#include "../sys/mm/mm.h"
#include "../sys/mm/paging.h"
#include "../sys/event/event.h"
#include "../sys/mempool/mempool.h"
#include "../sys/power/cpufreq.h"
#include "../sys/power/power.h"
#include "../sys/syslogger/syslogger.h"
#include "../sys/pci/pci.h"
// =============== DRIVERS ENTRIES =====================
#include "../drivers/lpt/lpt.h"
#include "../drivers/speaker/speaker.h"
char username[32] = "user";
char input_buffer[MAX_COMMAND_LENGTH] = {0};
uint8_t input_pos = 0;
char command_history[MAX_HISTORY][MAX_COMMAND_LENGTH] = {0};
uint8_t history_pos = 0;
int8_t current_history = -1;
void change_username(const char *new_name) {
  if (strlen(new_name) > 0 && strlen(new_name) < sizeof(username)) {
    strcpy(username, new_name);
    print_success("Username changed successfully\n");
  } else {
    print_error("Invalid username length (1-31 chars)");
    log_message("Username change failed", LOG_ERROR);
  }
}
void show_help() {
  print_info("\nAvailable commands:");
  print("\n  help      - Show this message", WHITE);
  print("\n  clear     - Clear screen", WHITE);
  print("\n  about     - Show OS info", WHITE);
  print("\n  applist   - Show system apps list", WHITE);
  print("\n  shcts     - Show system shortcuts", WHITE);
  print("\n  color     - Change text color", WHITE);
  print("\n  echo      - Output text value", WHITE);
  print("\n  export    - Export system variable", WHITE);
  print("\n  beep      - Play beep sound", WHITE);
  print("\n  reboot    - Reboot system", WHITE);
  print("\n  shutdown  - Shutdown system", WHITE);
  print("\n  settime   - Change timezone", WHITE);
  print("\n  uptime    - Show system uptime", WHITE);
  print("\n  mkfile    - Create new file", WHITE);
  print("\n  wrtfile   - Write data to file", WHITE);
  print("\n  rdfile    - Read data from file", WHITE);
  print("\n  rnfile    - Rename file", WHITE);
  print("\n  fndfile   - Find file", WHITE);
  print("\n  rm        - Delete file or directory", WHITE);
  print("\n  ls        - Show files list (-s)", WHITE);
  print("\n  chmod     - Change file perms", WHITE);
  print("\n  cd        - Change directory", WHITE);
  print("\n  dskinfo  - Show disk information", WHITE);
  print("\n  dskformat - Erase all data on disk", WHITE);
  print("\n  pwd       - Current directory", WHITE);
  print("\n  mkdir     - Create directory", WHITE);
  print("\n  ioport    - Read / Write port data", WHITE);
  print("\n  syslogs   - Show system logs", WHITE);
  print("\n  hd        - Dump hex data", WHITE);
  print("\n  cpufreq   - Set CPU freq to [MIN,BALANCED,MAX]", WHITE);
  print("\n  cpufreq i - Show info about CPU frequencies", WHITE);
  print("\n  history   - Show command history", WHITE);
  print("\n  chusr     - Change local hostname", WHITE);
  print("\n  usrinfo   - Current user information", WHITE);
  print("\n  copy      - Copy text to clipboard", WHITE);
  print("\n  paste     - Paste from clipboard", WHITE);
  print("\n  clipboard - Show clipboard contents", WHITE);
  print("\n  mkbasic   - Compile basic code", WHITE);
  print("\n  run       - Run basic application", WHITE);
  print("\n  alias     - Create or manage command aliases", WHITE);
  print("\n  !num      - Execute command from history", WHITE);
}
void show_applist() {
  print_info("System apps: (RUN for launch custom application)");
  print("\n  calc      - Simple calculator", WHITE);
  print("\n  clock     - Show current time", WHITE);
  print("\n  spsheet   - Spreadsheets editor", WHITE);
  print("\n  tedit     - Text editor", WHITE);
  print("\n  memmap    - Show memory map", WHITE);
  print("\n  sysinfo   - Show system info", WHITE);
  print("\n  cowsay    - Simple ASCII art cow", WHITE);
}
void input_test() {
  char username_buffer[64];
  wait_for_input("Enter name", username_buffer, 64);
  print(username_buffer, CYAN);
}
void show_shortcuts() {
  print_info("Keyboard Shortcuts:");
  print("\n  Ctrl+A      - Move cursor to beginning of line", WHITE);
  print("\n  Ctrl+E      - Move cursor to end of line", WHITE);
  print("\n  Ctrl+U      - Clear entire line", WHITE);
  print("\n  Ctrl+K      - Delete from cursor to end of line", WHITE);
  print("\n  Ctrl+L      - Clear screen", WHITE);
  print("\n  Ctrl+C      - Cancel command (interrupt)", WHITE);
  print("\n  UP/DOWN     - Navigate command history", WHITE);
  print("\n  Home/End    - Jump to start/end of line", WHITE);
  print("\n  Tab         - Insert 4 spaces\n", WHITE);
  print_info("Other Shortcuts:");
  print("\n  F1          - Show help text", WHITE);
  print("\n  F2          - Show this text", WHITE);
  print("\n  F3          - Clear screen", WHITE);
  print("\n  F4          - Show current time", WHITE);
  print("\n  F5          - Show commands history", WHITE);
  print("\n  F6          - Show clipboard", WHITE);
}
void process_command(const char *cmd) {
  log_message("Trying to process command", LOG_INFO);
  const char *space = strchr(cmd, ' ');
  char first_word[MAX_COMMAND_LENGTH];
  if (space) {
    strncpy(first_word, cmd, space - cmd);
    first_word[space - cmd] = '\0';
  } else {
    strcpy(first_word, cmd);
  }
  const char *alias_value = resolve_alias(first_word);
  if (alias_value) {
    char expanded_cmd[MAX_COMMAND_LENGTH];
    if (space) {
      snprintf(expanded_cmd, sizeof(expanded_cmd), "%s%s", alias_value, space);
    } else {
      strcpy(expanded_cmd, alias_value);
    }
    process_command(expanded_cmd);
    return;
  }
  if (strlen(cmd) > 0 && strcmp(cmd, "up") != 0 && strcmp(cmd, "down") != 0) {
    if (current_history >= history_pos || current_history == -1) {
      add_to_history(cmd);
    }
    current_history = history_pos;
  }
  if (strcmp(cmd, "help") == 0) {
    show_help();
  } else if (strcmp(cmd, "applist") == 0) {
    show_applist();
  } else if (strcmp(cmd, "input") == 0) {
    input_test();
  } else if (strcmp(cmd, "echo") == 0)
    print_info("Usage: echo <text> OR echo $<(all) sysvar name>");
  else if (strncmp(cmd, "echo ", 5) == 0) {
    process_echo(cmd + 5);
  } else if (strcmp(cmd, "clear") == 0) {
    clear_screen();
    print_prompt();
    return;
  } else if (strcmp(cmd, "ioport") == 0)
    print_info("Usage: ioport <hex> [r|w <value>]");
  else if (strncmp(cmd, "ioport ", 7) == 0) {
    uint16_t port;
    char op;
    uint8_t value;
    if (sscanf(cmd + 7, "%hx %c %hhx", &port, &op, &value) >= 2) {
      if (op == 'r') {
        uint8_t res = inb(port);
        print("Port 0x", WHITE);
        print_hex(port, CYAN);
        print(" = 0x", WHITE);
        print_hex(res, GREEN);
        print("\n", WHITE);
      } else if (op == 'w') {
        outb(port, value);
        print("Wrote 0x", WHITE);
        print_hex(value, GREEN);
        print(" to 0x", WHITE);
        print_hex(port, CYAN);
        print("\n", WHITE);
      }
    } else {
      print_info("Usage: ioport <hex> [r|w <value>]");
    }
  } else if (strcmp(cmd, "hd") == 0) {
    print_info("Usage: hd [options] <address> [length]");
    print_info("\n  Format selection:");
    print("\n    -b          Byte format (8-bit, default)", WHITE);
    print("\n    -w          Word format (16-bit)", WHITE);
    print("\n    -d          Dword format (32-bit)", WHITE);
    print_info("\n  Display control:");
    print("\n    -n <count>  Number of units to display (default: 16)", WHITE);
    print("\n    -s          Enable ASCII view (default: on)", WHITE);
    print("\n    -nos        Disable ASCII view", WHITE);
    print("\n    -v          Show configuration without dumping", WHITE);
    print_info("\n  Memory access:");
    print("\n    -a          Use absolute physical addresses", WHITE);
    print_info("\n  Examples:");
    print("\n    hd 0x7C00             - Dump 16 bytes at bootloader address",
          WHITE);
    print("\n    hd -w -n 32 0xB8000   - Dump 32 words (64 bytes) of video "
          "memory",
          WHITE);
    print("\n    hd -d -nos 0x100000 4 - Dump 4 dwords without ASCII view",
          WHITE);
    print("\n    hd -v                 - Show current dump settings", WHITE);
    print_info("\n  Note:");
    print("\n    Address can be in decimal (1234) or hex (0xABCD)", WHITE);
    print("\n    Length specifies number of units (not bytes)", WHITE);
  } else if (strncmp(cmd, "hd ", 3) == 0 || strcmp(cmd, "hd") == 0) {
    uint32_t addr = 0;
    uint32_t count = 16;
    uint8_t unit_size = 1;
    uint8_t show_ascii = 1;
    uint8_t verbose = 0;
    uint8_t physical = 0;
    uint8_t parse_error = 0;
    char *args = (char *)((cmd[2] == ' ') ? cmd + 3 : cmd + 2);
    char *token = strtok(args, " ");
    while (token != NULL && !parse_error) {
      if (strcmp(token, "-b") == 0) {
        unit_size = 1;
      } else if (strcmp(token, "-w") == 0) {
        unit_size = 2;
      } else if (strcmp(token, "-d") == 0) {
        unit_size = 4;
      } else if (strcmp(token, "-s") == 0) {
        show_ascii = 1;
      } else if (strcmp(token, "-nos") == 0) {
        show_ascii = 0;
      } else if (strcmp(token, "-a") == 0) {
        physical = 1;
      } else if (strcmp(token, "-v") == 0) {
        verbose = 1;
      } else if (strcmp(token, "-n") == 0) {
        token = strtok(NULL, " ");
        if (token) {
          count = atoi(token);
          if (count == 0)
            parse_error = 1;
        } else {
          parse_error = 1;
        }
      } else if (token[0] == '0' && token[1] == 'x') {
        addr = strtol(token, NULL, 16);
      } else if (isdigit(token[0])) {
        uint32_t val = atoi(token);
        if (addr == 0)
          addr = val;
        else
          count = val;
      } else {
        parse_error = 1;
      }
      if (!parse_error)
        token = strtok(NULL, " ");
    }
    if (parse_error) {
      print_error("Invalid arguments");
      print("Usage: hd [options] <address> [length]\n", WHITE);
      print("Try 'hd --help' for more information\n", WHITE);
      return;
    }
    if (verbose) {
      print("Hex Dump Configuration:\n", WHITE);
      print("  Unit size: ", WHITE);
      print_dec(unit_size, CYAN);
      print(unit_size == 1 ? " byte\n" : " bytes\n", WHITE);
      print("  Count:    ", WHITE);
      print_dec(count, CYAN);
      print("\n", WHITE);
      print("  Address:  ", WHITE);
      print_hex(addr, CYAN);
      print("\n", WHITE);
      print("  Mode:     ", WHITE);
      print(physical ? "Physical" : "Virtual", CYAN);
      print("\n", WHITE);
      return;
    }
    if (addr == 0) {
      print_error("Address cannot be 0");
      return;
    }
    uint32_t total_bytes = count * unit_size;
    print("Dump ", WHITE);
    print_dec(count, CYAN);
    print(unit_size == 1   ? " bytes"
          : unit_size == 2 ? " words"
                           : " dwords",
          WHITE);
    print(" at 0x", WHITE);
    print_hex(addr, CYAN);
    print(" (", WHITE);
    print_dec(total_bytes, CYAN);
    print(" bytes):\n", WHITE);
    for (uint32_t offset = 0; offset < total_bytes; offset += 16) {
      uint32_t current_addr = addr + offset;
      print_hex(current_addr, GRAY);
      print(": ", WHITE);
      for (uint32_t i = 0; i < 16 && (offset + i) < total_bytes;
           i += unit_size) {
        switch (unit_size) {
        case 1: {
          uint8_t val = *((uint8_t *)(current_addr + i));
          print_hex(val, WHITE);
          break;
        }
        case 2: {
          uint16_t val = *((uint16_t *)(current_addr + i));
          print_hex(val, WHITE);
          break;
        }
        case 4: {
          uint32_t val = *((uint32_t *)(current_addr + i));
          print_hex(val, WHITE);
          break;
        }
        }
        print(" ", WHITE);
      }
      if (unit_size > 1) {
        uint32_t spaces =
            (16 - (total_bytes - offset)) * (unit_size == 2 ? 5 : 3);
        for (uint32_t i = 0; i < spaces; i++)
          putchar(' ', WHITE);
      }
      if (show_ascii) {
        print(" ", WHITE);
        for (uint32_t i = 0; i < 16 && (offset + i) < total_bytes; i++) {
          char c = *((char *)(current_addr + i));
          putchar((c >= 32 && c < 127) ? c : '.', CYAN);
        }
      }
      print("\n", WHITE);
    }
  } else if (strcmp(cmd, "alias") == 0) {
    print_info("Usage:");
    print("\n  alias -l              - List all aliases", WHITE);
    print("\n  alias name            - Show alias value", WHITE);
    print("\n  alias name=value      - Create new alias", WHITE);
    print("\n  alias -d name         - Delete alias", WHITE);
  } else if (strcmp(cmd, "alias -l") == 0) {
    list_aliases();
  } else if (strncmp(cmd, "alias ", 6) == 0) {
    const char *args = cmd + 6;
    if (strncmp(args, "-d ", 3) == 0) {
      remove_alias(args + 3);
    } else {
      char *equal_sign = strchr(args, '=');
      if (equal_sign) {
        char name[MAX_ALIAS_LENGTH];
        strncpy(name, args, equal_sign - args);
        name[equal_sign - args] = '\0';
        const char *value = equal_sign + 1;
        if (strlen(value) > 0) {
          add_alias(name, value);
        } else {
          print_error("Alias value cannot be empty");
        }
      } else {
        const char *value = resolve_alias(args);
        if (value) {
          print(args, CYAN);
          print(" = '", WHITE);
          print(value, WHITE);
          print("'\n", WHITE);
        } else {
          print_error("Alias not found");
        }
      }
    }
  } else if (strcmp(cmd, "about") == 0) {
    print(" ##### \n", WHITE);
    print("# @ @ #  LumenOS -- ", WHITE);
    print("Version 1.2\n", CYAN);
    print("#     #\n", WHITE);
    print("# $$$ #  Kernel Version: ", WHITE);
    print("1.2SELFCOMP\n", YELLOW);
    print(" ##### \n", WHITE);
  } else if (strcmp(cmd, "color") == 0)
    print_info("Usage: color <color>!");
  else if (strncmp(cmd, "color ", 6) == 0) {
    char *color_arg = cmd + 6;
    if (strcmp(color_arg, "black") == 0) {
      change_color(BLACK);
      print_success("Text color changed!\n");
    } else if (strcmp(color_arg, "blue") == 0) {
      change_color(BLUE);
      print_success("Text color changed!\n");
    } else if (strcmp(color_arg, "green") == 0) {
      change_color(GREEN);
      print_success("Text color changed!\n");
    } else if (strcmp(color_arg, "cyan") == 0) {
      change_color(CYAN);
      print_success("Text color changed!\n");
    } else if (strcmp(color_arg, "red") == 0) {
      change_color(RED);
      print_success("Text color changed!\n");
    } else if (strcmp(color_arg, "magenta") == 0) {
      change_color(MAGENTA);
      print_success("Text color changed!\n");
    } else if (strcmp(color_arg, "brown") == 0) {
      change_color(BROWN);
      print_success("Text color changed!\n");
    } else if (strcmp(color_arg, "gray") == 0) {
      change_color(GRAY);
      print_success("Text color changed!\n");
    } else if (strcmp(color_arg, "dark_gray") == 0 ||
               strcmp(color_arg, "dark gray") == 0) {
      change_color(DARK_GRAY);
      print_success("Text color changed!\n");
    } else if (strcmp(color_arg, "light_blue") == 0 ||
               strcmp(color_arg, "light blue") == 0) {
      change_color(LIGHT_BLUE);
      print_success("Text color changed!\n");
    } else if (strcmp(color_arg, "light_green") == 0 ||
               strcmp(color_arg, "light green") == 0) {
      change_color(LIGHT_GREEN);
      print_success("Text color changed!\n");
    } else if (strcmp(color_arg, "light_cyan") == 0 ||
               strcmp(color_arg, "light cyan") == 0) {
      change_color(LIGHT_CYAN);
      print_success("Text color changed!\n");
    } else if (strcmp(color_arg, "light_red") == 0 ||
               strcmp(color_arg, "light red") == 0) {
      change_color(LIGHT_RED);
      print_success("Text color changed!\n");
    } else if (strcmp(color_arg, "light_magenta") == 0 ||
               strcmp(color_arg, "light magenta") == 0) {
      change_color(LIGHT_MAGENTA);
      print_success("Text color changed!\n");
    } else if (strcmp(color_arg, "yellow") == 0) {
      change_color(YELLOW);
      print_success("Text color changed!\n");
    } else if (strcmp(color_arg, "white") == 0) {
      change_color(WHITE);
      print_success("Text color changed!\n");
    } else if (strcmp(color_arg, "orange") == 0) {
      change_color(ORANGE);
      print_success("Text color changed!\n");
    } else if (strcmp(color_arg, "pink") == 0) {
      change_color(PINK);
      print_success("Text color changed!\n");
    } else if (strcmp(color_arg, "purple") == 0) {
      change_color(PURPLE);
      print_success("Text color changed!\n");
    } else if (strcmp(color_arg, "lavender") == 0) {
      change_color(LAVENDER);
      print_success("Text color changed!\n");
    } else if (strcmp(color_arg, "teal") == 0) {
      change_color(TEAL);
      print_success("Text color changed!\n");
    } else if (strcmp(color_arg, "forest_green") == 0 ||
               strcmp(color_arg, "forest green") == 0) {
      change_color(FOREST_GREEN);
      print_success("Text color changed!\n");
    } else if (strcmp(color_arg, "olive") == 0) {
      change_color(OLIVE);
      print_success("Text color changed!\n");
    } else if (strcmp(color_arg, "gold") == 0) {
      change_color(GOLD);
      print_success("Text color changed!\n");
    } else if (strcmp(color_arg, "silver") == 0) {
      change_color(SILVER);
      print_success("Text color changed!\n");
    } else if (strcmp(color_arg, "bronze") == 0) {
      change_color(BRONZE);
      print_success("Text color changed!\n");
    } else
      print_error("Invalid color");
  } else if (strcmp(cmd, "test") == 0)
    print_info("Usage: test <test name>!");
  else if (strncmp(cmd, "test ", 5) == 0) {
    if (strcmp(cmd + 5, "mmtest") == 0)
      test_memory_manager();
    else if (strcmp(cmd + 5, "sndtest") == 0)
      speaker_test();
    else if (strcmp(cmd + 5, "pgtest 1") == 0)
      paging_run_tests();
    else if (strcmp(cmd + 5, "pgtest 2") == 0)
      paging_print_info();
    else if (strcmp(cmd + 5, "pgtest 3") == 0)
      paging_demo();
    else if (strcmp(cmd + 5, "evntest") == 0)
      event_test();
    else if (strcmp(cmd + 5, "memptest") == 0)
      mempool_test();
    else {
      print("Invalid test name. Available tests:\n", CYAN);
      print("* Gfxtest - graphics test\n", WHITE);
      print("* Mmtest  - memory manager test\n", WHITE);
      print("* Sndtest - Sound test\n", WHITE);
      print("* Pgtest  - paging test [1,2,3]\n", WHITE);
      print("* Evntest  - event test\n", WHITE);
      print("* Memptest  - mempool test\n", WHITE);
    }
  } else if (strcmp(cmd, "beep") == 0) {
    speaker_beep_nonblocking(1000, 200);
    print_success("Beep!\n");
  } else if (strcmp(cmd, "reboot") == 0) {
    reboot();
  } else if (strcmp(cmd, "lpt") == 0) {
    print_info("LPT Parallel Port Driver");
    print("\nUsage: lpt <command>", WHITE);
    print("\nCommands:", WHITE);
    print("\n  detect             - Detect parallel ports", WHITE);
    print("\n  status [1-3]       - Show port status", WHITE);
    print("\n  test [1-3]         - Test port", WHITE);
    print("\n  init [1-3]         - Initialize printer", WHITE);
    print("\n  write <port> <text> - Send text", WHITE);
    print("\n  hex <port> <value>  - Send hex byte", WHITE);
    print("\n  raw <port> <byte>   - Send decimal byte", WHITE);
    print("\n  irq <port> <on/off> - Control interrupts", WHITE);
    print("\n  help               - Show help", WHITE);
  } else if (strncmp(cmd, "lpt ", 4) == 0) {
    lpt_command(cmd+4);
  } else if (strcmp(cmd, "shutdown") == 0) {
    shutdown();
  } else if (strcmp(cmd, "sysinfo") == 0) {
    show_system_info();
  } else if (strcmp(cmd, "clock") == 0) {
    show_clock();
  } else if (strcmp(cmd, "settime") == 0)
    print_info("Usage: settime <timezone>!");
  else if (strncmp(cmd, "settime ", 8) == 0) {
    set_time(cmd + 8);
  } else if (strcmp(cmd, "uptime") == 0) {
    show_uptime();
  } else if (strcmp(cmd, "ps") == 0) {
  } else if (strcmp(cmd, "chmod") == 0) {
    print_info("Usage: chmod <filename> <mode>");
    print("\n  Modes:", WHITE);
    print("\n    +r - set read-only", WHITE);
    print("\n    -r - set read-write", WHITE);
  } else if (strncmp(cmd, "chmod ", 6) == 0) {
    char filename[FS_FILENAME_LEN];
    char mode[3];
    if (sscanf(cmd + 6, "%s %s", filename, mode) != 2) {
      print_error("Invalid syntax. Usage: chmod <filename> <mode>");
    } else {
      bool readonly;
      if (strcmp(mode, "+r") == 0) {
        readonly = true;
      } else if (strcmp(mode, "-r") == 0) {
        readonly = false;
      } else {
        print_error("Invalid mode. Use +r or -r");
        print_prompt();
        return;
      }
      int result = fs_chmod(filename, readonly, current_dir_id);
      if (result == FS_SUCCESS) {
        print("File permissions changed: ", GREEN);
        print(filename, WHITE);
        print(" is now ", GREEN);
        print(readonly ? "read-only" : "read-write", WHITE);
        print("\n", WHITE);
      } else {
        log_message("Chmod internal perms error", LOG_ERROR);
        print_error("Failed to change permissions");
      }
    }
  } else if (strcmp(cmd, "memmap") == 0) {
    show_memory_map();
  } else if (strcmp(cmd, "copy") == 0)
    print_info("Usage: copy <text>!");
  else if (strncmp(cmd, "copy ", 5) == 0) {
    copy_to_clipboard(cmd + 5);
  } else if (strcmp(cmd, "paste") == 0) {
    paste_from_clipboard();
  } else if (strcmp(cmd, "clipboard") == 0 || strcmp(cmd, "cb") == 0) {
    show_clipboard();
  } else if (strcmp(cmd, "calc") == 0)
    print_info("Usage: calc <expression>!");
  else if (strncmp(cmd, "calc", 4) == 0) {
    if (cmd[4] == ' ' && cmd[5] != '\0') {
      calc(cmd + 5);
    } else if (cmd[4] == '\0') {
      print("Usage: calc <expression>\n", RED);
      print("Example: calc 9-6\n", WHITE);
    } else {
      print("Invalid calc syntax\n", RED);
    }
  } else if (strcmp(cmd, "mkfile") == 0)
    print_info("Usage: mkfile <file>!");
  else if (strncmp(cmd, "mkfile ", 7) == 0) {
    const char *filename = cmd + 7;
    if (filename[0] == '\0') {
      print("File name cannot be empty\n", RED);
      log_message("File name parsing error", LOG_ERROR);
    } else {
      int invalid = 0;
      for (const char *p = filename; *p; p++) {
        if (*p == ' ' || *p == '/' || *p == '\\' || *p == ':' || *p == '*' ||
            *p == '?' || *p == '"' || *p == '<' || *p == '>' || *p == '|') {
          invalid = 1;
          break;
        }
      }
      if (invalid) {
        print("File name contains invalid characters (spaces or symbols)\n",
              RED);
        log_message("File name parsing error", LOG_ERROR);
      } else {
        if (fs_create((char *)filename, 0, current_dir_id, false) >= 0) {
          print("File created\n", GREEN);
        } else {
          print("Cannot create file\n", RED);
          log_message("File creation failed", LOG_ERROR);
        }
      }
    }
  } else if (strcmp(cmd, "wrtfile") == 0)
    print_info("Usage: wrtfile <file>!");
  else if (strncmp(cmd, "wrtfile ", 8) == 0) {
    char *space = strchr(cmd + 8, ' ');
    if (space) {
      *space = 0;
      const char *filename = cmd + 8;
      const char *content = space + 1;
      if (fs_write(filename, content, strlen(content), current_dir_id) == 0) {
        print("Write success\n", GREEN);
      } else {
        print("Write failed\n", RED);
      }
    }
  } else if (strcmp(cmd, "rdfile") == 0)
    print_info("Usage: rdfile <file>!");
  else if (strncmp(cmd, "rdfile ", 7) == 0) {
    fs_cat_file(cmd + 7, current_dir_id);
  } else if (strcmp(cmd, "finfo") == 0)
    print_info("Usage: finfo <file>!");
  else if (strncmp(cmd, "finfo ", 6) == 0) {
    fs_info(cmd + 6, current_dir_id);
  } else if (strcmp(cmd, "rm") == 0)
    print_info("Usage: rm <file / dir>!");
  else if (strncmp(cmd, "rm ", 3) == 0) {
    const char *filename = cmd + 3;
    if (strlen(filename) == 0) {
      print("Error: Please specify filename\n", RED);
      log_message("File name parsing error", LOG_ERROR);
    } else {
      int result = fs_delete(filename, current_dir_id);
      if (result == FS_SUCCESS) {
        print("File deleted successfully\n", GREEN);
      } else if (result == FS_ERROR_FILE_NOT_FOUND) {
      } else {
        print("Error: Failed to delete file\n", RED);
        log_message("File deletion error", LOG_ERROR);
      }
    }
  } else if (strcmp(cmd, "floppy_info") == 0) {
    char *args[] = {"floppy_info"};
    cmd_floppy_info(1, args);
  } else if (strncmp(cmd, "floppy_info ", 12) == 0) {
    char *token = strtok((char *)cmd + 12, " ");
    int argc = 1;
    char *argv[10];
    argv[0] = "floppy_info";
    while (token != NULL && argc < 10) {
      argv[argc++] = token;
      token = strtok(NULL, " ");
    }
    cmd_floppy_info(argc, argv);
  } else if (strcmp(cmd, "floppy_read") == 0 ||
             strncmp(cmd, "floppy_read ", 11) == 0) {
    char *args[6];
    args[0] = "floppy_read";
    int argc = 1;
    if (strlen(cmd) > 11) {
      char buffer[256];
      strcpy(buffer, cmd);
      char *token = strtok(buffer + 11, " ");
      while (token != NULL && argc < 6) {
        args[argc++] = token;
        token = strtok(NULL, " ");
      }
    }
    cmd_floppy_read(argc, args);
  } else if (strcmp(cmd, "floppy_write") == 0 ||
             strncmp(cmd, "floppy_write ", 13) == 0) {
    char *args[7];
    args[0] = "floppy_write";
    int argc = 1;
    if (strlen(cmd) > 13) {
      char buffer[256];
      strcpy(buffer, cmd);
      char *token = strtok(buffer + 13, " ");
      while (token != NULL && argc < 7) {
        args[argc++] = token;
        token = strtok(NULL, " ");
      }
    }
    cmd_floppy_write(argc, args);
  } else if (strcmp(cmd, "floppy_cal") == 0) {
    char *args[] = {"floppy_cal"};
    cmd_floppy_calibrate(1, args);
  } else if (strncmp(cmd, "floppy_cal ", 10) == 0) {
    char *args[2];
    args[0] = "floppy_cal";
    args[1] = (char *)cmd + 10;
    cmd_floppy_calibrate(2, args);
  } else if (strcmp(cmd, "floppy_test") == 0) {
    char *args[] = {"floppy_test"};
    cmd_floppy_test(1, args);
  } else if (strncmp(cmd, "floppy_test ", 11) == 0) {
    char *args[2];
    args[0] = "floppy_test";
    args[1] = (char *)cmd + 11;
    cmd_floppy_test(2, args);
  } else if (strcmp(cmd, "floppy_format") == 0) {
    char *args[] = {"floppy_format"};
    cmd_floppy_format(1, args);
  } else if (strncmp(cmd, "floppy_format ", 14) == 0) {
    char *args[2];
    args[0] = "floppy_format";
    args[1] = (char *)cmd + 14;
    cmd_floppy_format(2, args);
  } else if (strcmp(cmd, "floppy_dir") == 0) {
    char *args[] = {"floppy_dir"};
    cmd_floppy_dir(1, args);
  } else if (strcmp(cmd, "ls") == 0 || strncmp(cmd, "ls ", 3) == 0) {
    fs_list(current_dir_id);
  } else if (strcmp(cmd, "mkdir") == 0)
    print_info("Usage: mkdir <directory>!");
  else if (strncmp(cmd, "mkdir ", 6) == 0) {
    const char *dirname = cmd + 6;
    if (dirname[0] == '\0') {
      print("Directory name cannot be empty\n", RED);
      log_message("Dir name parsing error", LOG_ERROR);
    } else {
      int invalid = 0;
      for (const char *p = dirname; *p; p++) {
        if (*p == ' ' || *p == '/' || *p == '\\' || *p == ':' || *p == '*' ||
            *p == '?' || *p == '"' || *p == '<' || *p == '>' || *p == '|') {
          invalid = 1;
          break;
        }
      }
      if (invalid) {
        print(
            "Directory name contains invalid characters (spaces or symbols)\n",
            RED);
        log_message("Dir name parsing error", LOG_ERROR);
      } else {
        if (fs_create((char *)dirname, 1, current_dir_id, false) >= 0) {
          print("Directory created\n", GREEN);
        } else {
          print("Cannot create directory\n", RED);
          log_message("Dir creation error", LOG_ERROR);
        }
      }
    }
  } else if (strcmp(cmd, "cd") == 0)
    print_info("Usage: cd <directory>!");
  else if (strncmp(cmd, "cd ", 3) == 0) {
    uint32_t parent_id = fs_get_current_dir();
    int result = fs_resolve_path(cmd + 3, &parent_id);
    if (result >= 0 && fs_is_dir(parent_id)) {
      current_dir_id = parent_id;
      fs_set_current_dir(parent_id);
      print("Changed directory to ", GREEN);
      print(fs_get_entry_name(parent_id), WHITE);
      print("\n", WHITE);
    } else {
      print("Directory not found: ", RED);
      print(cmd + 3, WHITE);
      print("\n", WHITE);
      log_message("Directory not found", LOG_ERROR);
    }
  } else if (strcmp(cmd, "pwd") == 0) {
    print("Current directory: ", GREEN);
    print(fs_get_entry_name(current_dir_id), WHITE);
    print("\n", WHITE);
  } else if (strcmp(cmd, "rnfile") == 0)
    print_info("Usage: rnfile <oldname> <newname>");
  else if (strncmp(cmd, "rnfile ", 7) == 0) {
    const char *args = cmd + 7;
    char *space = strchr(args, ' ');
    if (space) {
      *space = '\0';
      const char *oldname = args;
      const char *newname = space + 1;
      if (strlen(oldname) == 0 || strlen(newname) == 0) {
        print("Usage: rnfile <oldname> <newname>\n", RED);
      } else {
        fs_rename(oldname, newname, current_dir_id);
      }
    } else {
      print("Usage: rnfile <oldname> <newname>\n", RED);
    }
  } else if (strcmp(cmd, "fndfile") == 0)
    print_info("Usage: fndfile <pattern> [-r]\n");
  else if (strncmp(cmd, "fndfile ", 8) == 0) {
    const char *args = cmd + 8;
    bool recursive = false;
    char *recursive_flag = strstr(args, " -r");
    if (recursive_flag) {
      *recursive_flag = '\0';
      recursive = true;
    }
    if (strlen(args) == 0) {
      print("Usage: fndfile <pattern> [-r]\n", RED);
      print("  -r: search recursively\n", WHITE);
    } else {
      fs_find(args, current_dir_id, recursive);
    }
  } else if (strcmp(cmd, "asciitb") == 0) {
    show_ascii_table();
  } else if (strcmp(cmd, "history") == 0) {
    show_history();
  } else if (strcmp(cmd, "pci") == 0) {
    print_info("PCI devices commands:\n");
    print("  * pci scan   -- scan devices\n", WHITE);
    print("  * pci print  -- print devices", WHITE);
  } else if (strcmp(cmd, "pci scan") == 0) {
    pci_scan_all();
  } else if (strcmp(cmd, "pci print") == 0) {
    pci_print_devices();
  } else if (strcmp(cmd, "chusr") == 0)
    print_info("Usage: chusr <new username>!");
  else if (strncmp(cmd, "chusr ", 6) == 0) {
    change_username(cmd + 6);
  } else if (strcmp(cmd, "export") == 0)
    print_info("Usage: export <var name> <var value>!");
  else if (strncmp(cmd, "export ", 7) == 0) {
    export_command(cmd + 7);
  } else if (strncmp(cmd, "usrinfo", 7) == 0) {
    print("Rights: ROOT (SYS-ONLY)", WHITE);
  } else if (strcmp(cmd, "cpufreq min") == 0) {
    cpufreq_set_pstate(CPU_PSTATE_MIN);
    print("CPU frequency set to MIN\n", GREEN);
  } else if (strcmp(cmd, "cpufreq balanced") == 0) {
    cpufreq_set_pstate(CPU_PSTATE_BALANCED);
    print("CPU frequency set to BALANCED\n", GREEN);
  } else if (strcmp(cmd, "cpufreq max") == 0) {
    cpufreq_set_pstate(CPU_PSTATE_MAX);
    print("CPU frequency set to MAX\n", GREEN);
  } else if (strcmp(cmd, "cpufreq i") == 0) {
    uint32_t current = cpufreq_get_current_freq();
    uint32_t max = cpufreq_get_max_freq();
    uint32_t min = cpufreq_get_min_freq();
    print("CPU Frequency:\n", WHITE);
    print("  Current: ", WHITE);
    print_dec(current, CYAN);
    print(" MHz\n", WHITE);
    print("  Max:     ", WHITE);
    print_dec(max, CYAN);
    print(" MHz\n", WHITE);
    print("  Min:     ", WHITE);
    print_dec(min, CYAN);
    print(" MHz\n", WHITE);
  } else if (strncmp(cmd, "spsheet", 8) == 0) {
    print_info("Spsheet commands:");
    print("\n  spsheet show                - Display spreadsheet", WHITE);
    print("\n  spsheet fill <cell> <value> - Fill cell with value", WHITE);
    print("\n  spsheet clear               - Clear all cells", WHITE);
    print("\n  spsheet save <filename>     - Save to file", WHITE);
    print("\n  spsheet load <filename>     - Load from file", WHITE);
  } else if (strncmp(cmd, "spsheet ", 8) == 0) {
    char command[20];
    char arg1[50];
    char arg2[50];
    const char *args = cmd + 8;
    char *space1 = strchr(args, ' ');
    if (!space1) {
      strcpy(command, args);
      arg1[0] = '\0';
      arg2[0] = '\0';
    } else {
      strncpy(command, args, space1 - args);
      command[space1 - args] = '\0';
      const char *rest = space1 + 1;
      char *space2 = strchr(rest, ' ');
      if (!space2) {
        strcpy(arg1, rest);
        arg2[0] = '\0';
      } else {
        strncpy(arg1, rest, space2 - rest);
        arg1[space2 - rest] = '\0';
        strcpy(arg2, space2 + 1);
      }
    }
    spsheet_init();
    if (strcmp(command, "show") == 0) {
      spsheet_show();
    } else if (strcmp(command, "fill") == 0) {
      if (strlen(arg1) == 0 || strlen(arg2) == 0) {
        print_error("Usage: spsheet fill <cell> <value>");
      } else {
        spsheet_fill(arg1, arg2);
      }
    } else if (strcmp(command, "clear") == 0) {
      spsheet_clear();
    } else if (strcmp(command, "save") == 0) {
      if (strlen(arg1) == 0) {
        spsheet_save("sheet.dat");
      } else {
        spsheet_save(arg1);
      }
    } else if (strcmp(command, "load") == 0) {
      if (strlen(arg1) == 0) {
        spsheet_load("sheet.dat");
      } else {
        spsheet_load(arg1);
      }
    } else {
      print_error("Unknown spsheet command");
    }
  } else if (strncmp(cmd, "tedit", 5) == 0 && strlen(cmd) == 5) {
    print_info("Text Editor commands:");
    print("\n  tedit show                    - Show document", WHITE);
    print("\n  tedit insert <line> <text>    - Insert line", WHITE);
    print("\n  tedit append <text>           - Append line", WHITE);
    print("\n  tedit delete <line>           - Delete line", WHITE);
    print("\n  tedit format <line> <color> <align> <B> <I> <U>", WHITE);
    print("\n  tedit clear                   - Clear document", WHITE);
    print("\n  tedit save <filename>         - Save document", WHITE);
    print("\n  tedit load <filename>         - Load document", WHITE);
    print("\n  tedit search <pattern>        - Search text", WHITE);
    print("\n  tedit replace <old> <new>     - Replace text", WHITE);
    print("\n  tedit help                    - Show help", WHITE);
  } else if (strncmp(cmd, "tedit ", 6) == 0) {
    char command[20];
    const char *args = cmd + 6;
    textedit_init();
    if (strncmp(args, "show", 4) == 0) {
      textedit_show();
    } else if (strncmp(args, "clear", 5) == 0) {
      textedit_clear();
    } else if (strncmp(args, "help", 4) == 0) {
      textedit_show_help();
    } else if (strncmp(args, "insert ", 7) == 0) {
      int line;
      char text[TEXT_EDIT_LINE_LENGTH];
      const char *insert_args = args + 7;
      char *space_pos = strchr(insert_args, ' ');
      if (space_pos != NULL) {
        *space_pos = '\0';
        line = atoi(insert_args);
        strncpy(text, space_pos + 1, sizeof(text) - 1);
        text[sizeof(text) - 1] = '\0';
        *space_pos = ' ';
        if (line > 0) {
          textedit_insert_line(line - 1, text);
        } else {
          print_error("Invalid line number");
        }
      } else {
        print_error("Usage: tedit insert <line> <text>");
      }
    } else if (strncmp(args, "append ", 7) == 0) {
      textedit_append_line(args + 7);
    } else if (strncmp(args, "delete ", 7) == 0) {
      int line = atoi(args + 7);
      if (line > 0) {
        textedit_delete_line(line - 1);
      } else {
        print_error("Usage: tedit delete <line>");
      }
    } else if (strncmp(args, "format ", 7) == 0) {
      int line, color, align, bold, italic, underline;
      if (sscanf(args + 7, "%d %d %d %d %d %d", &line, &color, &align, &bold,
                 &italic, &underline) == 6) {
        textedit_set_format(line - 1, color, align, bold, italic, underline);
      } else {
        print_error("Usage: tedit format <line> <color> <align> <B> <I> <U>");
      }
    } else if (strncmp(args, "save ", 5) == 0) {
      textedit_save(args + 5);
    } else if (strncmp(args, "load ", 5) == 0) {
      textedit_load(args + 5);
    } else if (strncmp(args, "search ", 7) == 0) {
      textedit_search(args + 7);
    } else if (strncmp(args, "replace ", 8) == 0) {
      char old_text[50], new_text[50];
      char *space = strchr(args + 8, ' ');
      if (space) {
        *space = '\0';
        strcpy(old_text, args + 8);
        strcpy(new_text, space + 1);
        textedit_replace(old_text, new_text);
      } else {
        print_error("Usage: tedit replace <old> <new>");
      }
    } else {
      print_error("Unknown tedit command");
    }
  } else if (strncmp(cmd, "syslogs ", 8) == 0) {
    syslogger_command(cmd + 8);
  } else if (strncmp(cmd, "shcts", 5) == 0) {
    show_shortcuts();
  } else if (strncmp(cmd, "lpt detect", 10) == 0) {
    lpt_detect_ports();
  } else if (strcmp(cmd, "syslogs") == 0) {
    show_logs();
  } else if (strcmp(cmd, "dskinfo") == 0) {
    disk_print_info();
  } else if (strcmp(cmd, "dskformat") == 0) {
    fs_format();
  } else if (strcmp(cmd, "cowsay") == 0) {
    print_info("Usage: cowsay <message>"); 
  } else if (strncmp(cmd, "cowsay ", 7) == 0) {
    cowsay_command(cmd + 7);
  } else if (strcmp(cmd, "run") == 0)
    print_info("Usage: run <filename>!");
  else if (strncmp(cmd, "run ", 4) == 0) {
    exec_run(cmd + 4);
  } else if (strcmp(cmd, "mkbasic") == 0)
    print_info("Usage: mkbasic <filename> <basic code>!");
  else if (strncmp(cmd, "mkbasic ", 8) == 0) {
    const char *args = cmd + 8;
    char *first_space = strchr(args, ' ');
    if (first_space) {
      *first_space = '\0';
      const char *filename = args;
      const char *basic_code = first_space + 1;
      char processed_code[512];
      int j = 0;
      for (int i = 0; basic_code[i] != '\0' && j < 510; i++) {
        if (basic_code[i] == '\\' && basic_code[i + 1] == 'n') {
          processed_code[j++] = '\n';
          i++;
        } else {
          processed_code[j++] = basic_code[i];
        }
      }
      processed_code[j] = '\0';
      create_basic_executable(filename, processed_code);
    } else {
      print_error("Usage: mkbasic <filename> <basic code>");
      print("Use \\n for new lines\n", WHITE);
    }
  } else if (cmd[0] == '!' && cmd[1] >= '1' && cmd[1] <= '9') {
    execute_from_history(cmd[1] - '0');
  } else if (cmd[0] != '\0') {
    if (!input_waiting_mode) {
      print_error("Unknown command. Type 'help' for help.");
      log_message("Unknown command", LOG_ERROR);
    }
  }
  print_prompt();
}
