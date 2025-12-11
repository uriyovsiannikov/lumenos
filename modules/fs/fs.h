#ifndef FS_H
#define FS_H
#include <stdint.h>
#include <stdbool.h>
#include "../drivers/ddrv/ddrv.h"
#define FS_MAX_FILES         128
#define FS_FILENAME_LEN      32
#define FS_SECTOR_SIZE       512
#define FS_ROOT_DIR_SECTOR   1
#define FS_FAT_SECTOR        2
#define FS_DATA_START_SECTOR 3
#define FS_SUCCESS                0
#define FS_ERROR_FILE_NOT_FOUND  -1
#define FS_ERROR_FILE_EXISTS     -2
#define FS_ERROR_NO_SPACE        -3
#define FS_ERROR_ACCESS_DENIED   -4
#define FS_ERROR_INVALID_PARAM   -5
typedef struct {
    char name[FS_FILENAME_LEN];
    uint32_t size;
    uint32_t first_sector;
    uint32_t parent_dir;
    uint8_t is_used;
    uint8_t is_dir;
    uint8_t is_readonly;
} fs_file_entry_t;
void fs_init(void);
void fs_sync(void);
int fs_create(const char* filename, bool is_dir, uint32_t parent_dir, bool isSilent);
int fs_write(const char* filename, const void* data, uint32_t size, uint32_t parent_dir);
int fs_read(const char* filename, void* buffer, uint32_t max_size, uint32_t parent_dir);
int fs_delete(const char* name, uint32_t parent_dir);
int fs_list(uint32_t dir_id);
int fs_rename(const char* oldname, const char* newname, uint32_t parent_dir);
int fs_chmod(const char* name, bool readonly, uint32_t parent_dir);
int fs_find(const char* pattern, uint32_t dir_id, bool recursive);
void fs_cat_file(const char* filename, uint32_t parent_dir);
uint32_t fs_get_current_dir(void);
void fs_set_current_dir(uint32_t dir_id);
int fs_resolve_path(const char* path, uint32_t* parent_dir_id);
bool fs_is_dir(int index);
const char* fs_get_entry_name(int index);
int fs_find_directory(const char* path, uint32_t current_dir);
bool fs_format(void);
extern fs_file_entry_t fs_files[FS_MAX_FILES];
extern uint32_t fs_current_dir;
extern uint16_t fs_fat[1024];
#endif
