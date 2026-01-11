#ifndef EXEC_H
#define EXEC_H
#include <stdbool.h>
#include <stdint.h>
#define LUMEN_MAGIC 0x4C554D45
#define MAX_PROGRAM_SIZE 4096
#define MAX_VARIABLES 26
typedef struct {
  uint32_t magic;
  uint32_t entry_point;
  uint32_t text_size;
  uint32_t data_size;
  uint32_t bss_size;
  uint32_t stack_size;
  char name[32];
} lumen_header_t;
int exec_load(const char *filename);
int exec_run(const char *filename);
void create_basic_executable(const char *filename, const char *basic_code);
void basic_run(const char *program);
int execute_basic_line(const char *line);
void clear_basic_memory(void);
#endif
