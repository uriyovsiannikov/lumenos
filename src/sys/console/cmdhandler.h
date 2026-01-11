#ifndef CMDHANDLER_H
#define CMDHANDLER_H
#include <stdbool.h>
#include <stdint.h>
#define MAX_COMMAND_LENGTH 256
#define MAX_HISTORY 10
#define PROMPT_FORMAT "%s@os # "
extern char username[32];
extern char input_buffer[MAX_COMMAND_LENGTH];
extern uint8_t input_pos;
extern char command_history[MAX_HISTORY][MAX_COMMAND_LENGTH];
extern uint8_t history_pos;
extern int8_t current_history;
void process_command(const char *cmd);
void show_help(void);
void show_applist(void);
void show_shortcuts(void);
void change_username(const char *new_name);
extern bool input_waiting_mode;
#endif
