#ifndef CONSOLEUTILS_H
#define CONSOLEUTILS_H
#include <stdint.h>
#include <stdbool.h>
#define MAX_VARIABLES 50
#define MAX_VAR_NAME_LENGTH 32
#define MAX_VAR_VALUE_LENGTH 64
#define MAX_COMMAND_LENGTH 256
#define MAX_HISTORY 10
extern char username[32];
extern char input_buffer[MAX_COMMAND_LENGTH];
extern uint8_t input_pos;
extern char command_history[MAX_HISTORY][MAX_COMMAND_LENGTH];
extern uint8_t history_pos;
extern int8_t current_history;
void set_environment_var(const char *name, const char *value, bool isSilent);
const char *get_environment_var(const char *name);
void print_environment_vars();
void export_command(const char *args);
void process_echo(const char *text);
void restore_from_history();
void add_to_history(const char *cmd);
void show_history();
void execute_from_history(uint8_t index);
void print_prompt();
void change_color(uint8_t new_color);
void wait_for_input(const char *prompt, char *buffer, uint16_t max_len);
#endif
