#include "multitasking.h"
#include "../../libs/print.h"
#include <string.h>
typedef struct {
  uint32_t esp;
  uint32_t stack[STACK_SIZE];
  task_func_t func;
  uint8_t active;
  uint8_t first_run;
} task_t;
static task_t tasks[MAX_TASKS];
int current_task = 0;
int task_count = 0;
static uint8_t scheduler_running = 0;
static void task_exit(void) {
  tasks[current_task].active = 0;
  print("Task ", RED);
  print_dec(current_task, RED);
  print(" exited\n", RED);
  mt_stop();
  while (1)
    asm volatile("hlt");
}
void mt_init(void) {
  for (int i = 0; i < MAX_TASKS; i++) {
    tasks[i].active = 0;
    tasks[i].esp = 0;
    tasks[i].func = 0;
    tasks[i].first_run = 1;
  }
  tasks[0].active = 1;
  tasks[0].func = 0;
  tasks[0].first_run = 0;
  current_task = 0;
  task_count = 1;
  scheduler_running = 0;
  print("Multitasking: initialized\n", GREEN);
}
int task_create(task_func_t function) {
  if (task_count >= MAX_TASKS) {
    print("Cannot create task: max reached\n", RED);
    return -1;
  }
  if (!function) {
    print("Cannot create task: null function\n", RED);
    return -1;
  }
  int id = task_count;
  uint32_t *stack_top = &tasks[id].stack[STACK_SIZE - 1];
  *(--stack_top) = 0x202;
  *(--stack_top) = 0x08;
  *(--stack_top) = (uint32_t)function;
  *(--stack_top) = 0;
  *(--stack_top) = 0;
  *(--stack_top) = 0;
  *(--stack_top) = 0;
  *(--stack_top) = 0;
  *(--stack_top) = 0;
  *(--stack_top) = 0;
  *(--stack_top) = (uint32_t)task_exit;
  tasks[id].esp = (uint32_t)stack_top;
  tasks[id].func = function;
  tasks[id].active = 1;
  tasks[id].first_run = 1;
  task_count++;
  print("Task created: ID=", GREEN);
  print_dec(id, GREEN);
  print("\n", GREEN);
  return id;
}
void task_yield(void) {
  if (!scheduler_running || task_count <= 1)
    return;
  int old_task = current_task;
  int new_task = -1;
  for (int i = 1; i <= task_count; i++) {
    int candidate = (old_task + i) % task_count;
    if (candidate != old_task && tasks[candidate].active) {
      new_task = candidate;
      break;
    }
  }
  if (new_task == -1)
    return;
  asm volatile("pusha\n"
               "pushf\n"
               "mov %%esp, %0\n"
               : "=m"(tasks[old_task].esp)
               :
               : "memory");
  current_task = new_task;
  asm volatile("mov %0, %%esp\n"
               "popf\n"
               "popa\n"
               :
               : "r"(tasks[new_task].esp)
               : "memory");
  if (tasks[new_task].first_run) {
    tasks[new_task].first_run = 0;
    asm volatile("iret\n" ::: "memory");
  }
}
void mt_tick(void) {
  static int tick_counter = 0;
  tick_counter++;
  if (tick_counter >= 10) {
    tick_counter = 0;
    task_yield();
  }
}
void mt_start(void) {
  if (task_count > 1) {
    scheduler_running = 1;
    print("Multitasking: STARTED\n", GREEN);
  } else {
    print("Multitasking: no tasks to schedule\n", YELLOW);
  }
}
void mt_stop(void) {
  scheduler_running = 0;
  current_task = 0;
  print("Multitasking: STOPPED\n", YELLOW);
}
static void simple_task1(void) {
  int counter = 0;
  while (1) {
    print("Task1: ", GREEN);
    print_dec(counter, WHITE);
    print("\n", WHITE);
    counter++;
    task_yield();
  }
}
static void simple_task2(void) {
  int counter = 0;
  while (1) {
    print("Task2: ", CYAN);
    print_dec(counter, WHITE);
    print("\n", WHITE);
    counter++;
    task_yield();
  }
}
void test_multitasking(void) {
  print("\n=== Testing Basic Multitasking ===\n", LIGHT_CYAN);
  task_create(simple_task1);
  task_create(simple_task2);
  mt_start();
  print("Multitasking test started!\n", GREEN);
  print("You should see alternating messages from tasks.\n", WHITE);
}
