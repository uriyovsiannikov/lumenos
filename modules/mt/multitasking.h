#ifndef MULTITASKING_H
#define MULTITASKING_H
#include <stdint.h>
#define MAX_TASKS 4
#define STACK_SIZE 1024
typedef void (*task_func_t)(void);
void mt_init(void);
int task_create(task_func_t function);
void task_yield(void);
void mt_tick(void);
void mt_start(void);
void mt_stop(void);
void test_multitasking(void);
extern int current_task;
extern int task_count;
#endif
