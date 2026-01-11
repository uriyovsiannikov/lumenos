#include "../libs/print.h"
#include "../libs/string.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
// ===================== FOR INIT =========================
#include "../drivers/serial/serial.h"
#include "../drivers/a20/a20.h"
#include "../drivers/ddrv/ddrv.h"
#include "../drivers/floppy/floppy.h"
#include "../drivers/lpt/lpt.h"
#include "../drivers/serial/serial.h"
#include "../sys/syslogger/syslogger.h"
#include "../sys/panic/panic.h"
#include "../sys/fs/fs.h"
#include "../sys/mm/mm.h"
#include "../sys/mm/paging.h"
#include "../sys/event/event.h"
#include "../sys/timer/timer.h"
#include "../sys/io/idt.h"
#include "../sys/mempool/mempool.h"
#include "../apps/memmap.h"
#include "../sys/console/consoleutils.h"
// ======================= END ============================
__attribute__((section(".multiboot"), used))
static const uint32_t multiboot_header[4] = {
    0x1BADB002,
    (1 << 0) | (1 << 1),
    -(0x1BADB002 + ((1 << 0) | (1 << 1))),
    0, 0, 0, 0
};
#define HEAP_SIZE (1024 * 1024)
#define MULTIBOOT_BOOTLOADER_MAGIC 0x2BADB002
#define MULTIBOOT_INFO_MEMORY 0x00000001
#define MULTIBOOT_INFO_BOOTDEV 0x00000002
#define MULTIBOOT_INFO_CMDLINE 0x00000004
#define MULTIBOOT_INFO_MODS 0x00000008
#define MULTIBOOT_INFO_AOUT_SYMS 0x00000010
#define MULTIBOOT_INFO_ELF_SHDR 0x00000020
#define MULTIBOOT_INFO_MEM_MAP 0x00000040
#define MULTIBOOT_INFO_DRIVE_INFO 0x00000080
#define MULTIBOOT_INFO_CONFIG_TABLE 0x00000100
#define MULTIBOOT_INFO_BOOT_LOADER_NAME 0x00000200
#define MULTIBOOT_INFO_APM_TABLE 0x00000400
#define MULTIBOOT_INFO_VBE_INFO 0x00000800
#define MULTIBOOT_INFO_FRAMEBUFFER_INFO 0x00001000
#define FILENAME_LEN FS_FILENAME_LEN
uint32_t current_dir_id __attribute__((section(".data"))) = 0;
int a = 0;
static uint8_t heap_memory[HEAP_SIZE];
int recover_mode = 0;
typedef struct {
  uint32_t flags;
  uint32_t mem_lower;
  uint32_t mem_upper;
  uint32_t boot_device;
  uint32_t cmdline;
  uint32_t mods_count;
  uint32_t mods_addr;
  uint32_t num;
  uint32_t size;
  uint32_t addr;
  uint32_t shndx;
  uint32_t mmap_length;
  uint32_t mmap_addr;
  uint32_t drives_length;
  uint32_t drives_addr;
  uint32_t config_table;
  uint32_t boot_loader_name;
  uint32_t apm_table;
  uint32_t vbe_control_info;
  uint32_t vbe_mode_info;
  uint16_t vbe_mode;
  uint16_t vbe_interface_seg;
  uint16_t vbe_interface_off;
  uint16_t vbe_interface_len;
  uint64_t framebuffer_addr;
  uint32_t framebuffer_pitch;
  uint32_t framebuffer_width;
  uint32_t framebuffer_height;
  uint8_t framebuffer_bpp;
  uint8_t framebuffer_type;
  union {
    struct {
      uint32_t framebuffer_palette_addr;
      uint16_t framebuffer_palette_num_colors;
    };
    struct {
      uint32_t framebuffer_red_field_position;
      uint32_t framebuffer_red_mask_size;
      uint32_t framebuffer_green_field_position;
      uint32_t framebuffer_green_mask_size;
      uint32_t framebuffer_blue_field_position;
      uint32_t framebuffer_blue_mask_size;
    };
  };
} multiboot_info_t;
void check_boot_args(const char *args) {
  log_message("Trying to check boot args", LOG_WARNING);
  print("Checking boot args...", WHITE);
  print(" [OK]\n", GREEN);
  if (args && strstr(args, "secured")) {
    recover_mode = 1;
    log_message("Boot mode: recovery", LOG_INFO);
  } else {
    log_message("Boot mode: default", LOG_INFO);
  }
}
static void booticon() {
  print("|                        ,---.,---.\n", GREEN);
  print("|    .   .,-.-.,---.,---.|   |`---.\n", GREEN);
  print("|    |   || | ||---'|   ||   |    |\n", GREEN);
  print("`---'`---'` ' '`---'`   '`---'`---'\n", GREEN);
}
static void init_state() {
  //a20_init();
  mm_init(heap_memory, HEAP_SIZE);
  init_idt();
  fs_init();
  disk_init();
  timer_init(100);
  panic_init();
  paging_init();
  serial_init(COM1);
  mempools_init();
  event_system_init();
  lpt_init();
  floppy_init();
  asm volatile("sti");
}
static void create_syscfg() {
  current_dir_id = fs_get_current_dir();
  set_environment_var("ver", "1.2", true);
  log_message("Internal environment created", LOG_INFO);
  fs_create("HOME", 1, current_dir_id, true);
  fs_create("SYS", 1, current_dir_id, true);
  fs_create("DATA", 1, current_dir_id, true);
  fs_create("TEMP", 1, current_dir_id, true);
  fs_create("USRDATA.SYS", 0, current_dir_id, true);
}
void kernel_main(uint32_t magic, uint32_t *mb_info) {
  if (magic == MULTIBOOT_BOOTLOADER_MAGIC) {
    multiboot_info_t *mbi = (multiboot_info_t *)mb_info;
    if (mbi->flags & MULTIBOOT_INFO_CMDLINE) {
      char *cmdline = (char *)(mbi->cmdline);
      check_boot_args(cmdline);
    }
  }
  init_memory_map(magic, (uint32_t *)mb_info);
  init_state();
  create_syscfg();
  print("\nMultiboot magic: ", WHITE);
  char magic_buf[16];
  itoa(magic, magic_buf, 16);
  print_info("0x");
  print_info(magic_buf);
  print_info("\n");
  booticon();
  if (recover_mode) {
    print("LumenOS v1.2 (Recover mode)\n", LIGHT_CYAN);
  } else {
    print("LumenOS v1.2\n", LIGHT_CYAN);
  }
  print("Type 'help' for commands\n\n", WHITE);
  print_prompt();
  log_message("System init end", LOG_INFO);
  while (1) {
    asm volatile("hlt");
  }
}
