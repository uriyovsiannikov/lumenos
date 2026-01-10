#include "consoleutils.h"
#include "cmdhandler.h"
#include "../libs/print.h"
#include "../sys/io/keyboard.h"
#include "../sys/syslogger/syslogger.h"
#include "../sys/io/io.h"
#include <stddef.h>
#include <string.h>
struct environment_var {
  char name[MAX_VAR_NAME_LENGTH];
  char value[MAX_VAR_VALUE_LENGTH];
};
static struct environment_var env_vars[MAX_VARIABLES];
static uint8_t env_var_count = 0;
void set_environment_var(const char *name, const char *value, bool isSilent) {
  if (strlen(name) >= MAX_VAR_NAME_LENGTH ||
      strlen(value) >= MAX_VAR_VALUE_LENGTH) {
    if (!isSilent) {
      print_error("Variable name or value too long");
    }
    log_message("Env var error: name size reached", LOG_ERROR);
    return;
  }
  for (int i = 0; i < env_var_count; i++) {
    if (strcmp(env_vars[i].name, name) == 0) {
      strcpy(env_vars[i].value, value);
      if (!isSilent) {
        print_success("Variable updated");
      }
      return;
    }
  }
  if (env_var_count >= MAX_VARIABLES) {
    if (!isSilent) {
      print_error("Maximum number of variables reached");
    }
    log_message("Env var error: limit reached", LOG_ERROR);
    return;
  }
  strcpy(env_vars[env_var_count].name, name);
  strcpy(env_vars[env_var_count].value, value);
  env_var_count++;
  if (!isSilent) {
    print_success("Variable set");
  }
  log_message("Env var set", LOG_INFO);
}
const char *get_environment_var(const char *name) {
  for (int i = 0; i < env_var_count; i++) {
    if (strcmp(env_vars[i].name, name) == 0) {
      return env_vars[i].value;
    }
  }
  return NULL;
}
void print_environment_vars() {
  if (env_var_count == 0) {
    print_info("No environment variables set");
    return;
  }
  print_info("Environment variables:");
  for (int i = 0; i < env_var_count; i++) {
    print("\n  ", WHITE);
    print(env_vars[i].name, CYAN);
    print("=", WHITE);
    print(env_vars[i].value, GREEN);
  }
  print("\n", WHITE);
}
void export_command(const char *args) {
  char *equals = strchr(args, '=');
  if (!equals) {
    print_error("Syntax: export NAME=value");
    return;
  }
  char name[MAX_VAR_NAME_LENGTH];
  char value[MAX_VAR_VALUE_LENGTH];
  strncpy(name, args, equals - args);
  name[equals - args] = '\0';
  strcpy(value, equals + 1);
  set_environment_var(name, value, false);
}
void process_echo(const char *text) {
  if (strcmp(text, "$(all)") == 0) {
    if (env_var_count == 0) {
      print_info("No environment variables set");
    } else {
      print_info("Environment variables:");
      for (int i = 0; i < env_var_count; i++) {
        print("\n  ", WHITE);
        print(env_vars[i].name, CYAN);
        print("=", WHITE);
        print(env_vars[i].value, GREEN);
      }
      print("\n", WHITE);
    }
    return;
  }
  char output[MAX_COMMAND_LENGTH] = {0};
  char temp[MAX_VAR_VALUE_LENGTH] = {0};
  int output_pos = 0;
  int text_pos = 0;
  while (text[text_pos] != '\0' && output_pos < MAX_COMMAND_LENGTH - 1) {
    if (text[text_pos] == '$' && text[text_pos + 1] != '\0') {
      if (strncmp(text + text_pos, "$(all)", 6) == 0) {
        text_pos += 6;
        if (env_var_count == 0) {
          const char *msg = "No variables";
          strcpy(output + output_pos, msg);
          output_pos += strlen(msg);
        } else {
          for (int i = 0;
               i < env_var_count && output_pos < MAX_COMMAND_LENGTH - 1; i++) {
            if (i > 0) {
              output[output_pos++] = ' ';
            }
            strcpy(output + output_pos, env_vars[i].name);
            output_pos += strlen(env_vars[i].name);
            output[output_pos++] = '=';
            strcpy(output + output_pos, env_vars[i].value);
            output_pos += strlen(env_vars[i].value);
          }
        }
        continue;
      }
      text_pos++;
      int var_pos = 0;
      while (text[text_pos] != '\0' && text[text_pos] != ' ' &&
             var_pos < MAX_VAR_NAME_LENGTH - 1) {
        temp[var_pos++] = text[text_pos++];
      }
      temp[var_pos] = '\0';
      const char *var_value = get_environment_var(temp);
      if (var_value) {
        int val_len = strlen(var_value);
        if (output_pos + val_len < MAX_COMMAND_LENGTH - 1) {
          strcpy(output + output_pos, var_value);
          output_pos += val_len;
        }
      } else {
        if (output_pos + var_pos + 1 < MAX_COMMAND_LENGTH - 1) {
          output[output_pos++] = '$';
          strcpy(output + output_pos, temp);
          output_pos += var_pos;
        }
      }
    } else {
      output[output_pos++] = text[text_pos++];
    }
  }
  output[output_pos] = '\0';
  print(output, WHITE);
  print("\n", WHITE);
}
void restore_from_history() {
  uint8_t original_y = cursor_y;
  putchar('\r', text_color);
  for (int i = 0; i < VGA_WIDTH; i++) {
    vga_buffer[original_y * VGA_WIDTH + i] = (text_color << 8) | ' ';
  }
  cursor_x = 0;
  cursor_y = original_y;
  vga_buffer[cursor_y * VGA_WIDTH + cursor_x] = (text_color << 8) | ' ';
  print(username, LIGHT_BLUE);
  print("@os $ ", LIGHT_GREEN);
  if (current_history >= 0 && current_history < history_pos) {
    print(command_history[current_history], WHITE);
    strcpy(input_buffer, command_history[current_history]);
    input_pos = strlen(input_buffer);
  } else {
    input_buffer[0] = '\0';
    input_pos = 0;
  }
  update_cursor();
}
void add_to_history(const char *cmd) {
  if (strlen(cmd) == 0)
    return;
  if (history_pos > 0 && strcmp(command_history[history_pos - 1], cmd) == 0) {
    return;
  }
  if (history_pos >= MAX_HISTORY) {
    for (int i = 0; i < MAX_HISTORY - 1; i++) {
      strcpy(command_history[i], command_history[i + 1]);
    }
    history_pos = MAX_HISTORY - 1;
  }
  strcpy(command_history[history_pos], cmd);
  history_pos++;
  current_history = history_pos;
}
void show_history() {
  print_info("Command history:");
  for (int i = 0; i < history_pos; i++) {
    print("\n  ", WHITE);
    print_dec(i + 1, WHITE);
    print(": ", WHITE);
    print(command_history[i], WHITE);
  }
}
void execute_from_history(uint8_t index) {
  if (index < 1 || index > history_pos) {
    print_error("\nInvalid history index");
    log_message("Internal CMDEXR error", LOG_ERROR);
    return;
  }
  strcpy(input_buffer, command_history[index - 1]);
  input_pos = strlen(input_buffer);
  print("\n> ", LIGHT_GREEN);
  print(input_buffer, WHITE);
  process_command(input_buffer);
}
void print_prompt() {
  if (cursor_x != 0) {
    putchar('\n', text_color);
  }
  uint16_t current_cell = vga_buffer[cursor_y * VGA_WIDTH];
  char current_char = current_cell & 0xFF;
  if (current_char != 'u' && current_char != '>') {
    print(username, LIGHT_BLUE);
    print("@os $ ", LIGHT_GREEN);
  }
  input_pos = 0;
  input_buffer[0] = '\0';
  update_cursor();
}
void change_color(uint8_t new_color) { text_color = new_color; }
bool input_waiting_mode = false;
void set_input_waiting_mode(bool enabled) {
    input_waiting_mode = enabled;
}
void wait_for_input(const char *prompt, char *buffer, uint16_t max_len) {
    set_input_waiting_mode(true);
    if (cursor_x != 0) {
        putchar('\n', text_color);
    }
    print(prompt, text_color);
    print(": ", LIGHT_GREEN);
    buffer[0] = '\0';
    uint8_t buffer_pos = 0;
    bool input_complete = false;
    while (!input_complete) {
        uint8_t status = inb(0x64);
        if (status & 0x01) {
            uint8_t scancode = inb(0x60);
            if (scancode & 0x80) {
                uint8_t keycode = scancode & 0x7F;
                if (keycode == 0x1D) ctrl_pressed = false;
                if (keycode == 0x2A || keycode == 0x36) shift_pressed = false;
                if (keycode == 0x38) alt_pressed = false;
                continue;
            }
            if (scancode == 0x1D) ctrl_pressed = true;
            if (scancode == 0x2A || scancode == 0x36) shift_pressed = true;
            if (scancode == 0x38) alt_pressed = true;
            if (scancode == 0x1C) {
                putchar('\n', text_color);
                buffer[buffer_pos] = '\0';
                input_complete = true;
                break;
            }
            else if (scancode == 0x0E) {
                if (buffer_pos > 0) {
                    buffer_pos--;
                    buffer[buffer_pos] = '\0';

                    if (cursor_x > 0) {
                        cursor_x--;
                        vga_buffer[cursor_y * VGA_WIDTH + cursor_x] = (text_color << 8) | ' ';
                        update_cursor();
                    }
                }
            }
            else {
                char c = get_char_from_scancode(scancode);
                if (c != 0 && buffer_pos < max_len - 1) {
                    putchar(c, text_color);
                    buffer[buffer_pos++] = c;
                    buffer[buffer_pos] = '\0';
                }
            }
        }
        asm volatile("pause");
    }
    set_input_waiting_mode(false);
}
