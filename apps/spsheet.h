#ifndef SPSHEET_H
#define SPSHEET_H
#include <stdint.h>
#define SPREADSHEET_ROWS 10
#define SPREADSHEET_COLS 5
#define CELL_VALUE_LENGTH 20
extern uint32_t current_dir_id;
void spsheet_init();
void spsheet_fill(const char* cell_ref, const char* value);
void spsheet_show();
void spsheet_clear();
void spsheet_save(const char* filename);
void spsheet_load(const char* filename);
#endif 