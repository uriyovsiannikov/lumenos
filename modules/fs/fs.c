#include "fs.h"
#include "../modules/syslogger/syslogger.h"
#include "../modules/power/power.h"
#include "../libs/print.h"
#include "../libs/string.h"
#include "../libs/ctype.h"
fs_file_entry_t fs_files[FS_MAX_FILES];
uint32_t fs_current_dir = 0;
uint16_t fs_fat[1024];
void fs_init(void) {
    print("Initializing filesystem...", WHITE);
    if (!disk_read_sector(FS_ROOT_DIR_SECTOR, (uint8_t*)fs_files) ||
        !disk_read_sector(FS_FAT_SECTOR, (uint8_t*)fs_fat)) {
        print(" [NEW]\n", YELLOW);
        memset(fs_files, 0, sizeof(fs_files));
        memset(fs_fat, 0, sizeof(fs_fat));
        strcpy(fs_files[0].name, "/");
        fs_files[0].is_used = 1;
        fs_files[0].is_dir = 1;
        fs_files[0].parent_dir = 0;
        fs_files[0].first_sector = 0xFFFF;
        fs_files[0].size = 0;
        fs_files[0].is_readonly = 0;
        fs_create("HOME", true, 0, true);
        fs_create("SYS", true, 0, true);
        fs_create("DATA", true, 0, true);
        fs_create("TEMP", true, 0, true);
        fs_sync();
    } else {
        print(" [OK]\n", GREEN);
    }
    fs_current_dir = 0;
}
void fs_sync(void) {
    disk_write_sector(FS_ROOT_DIR_SECTOR, (uint8_t*)fs_files);
    disk_write_sector(FS_FAT_SECTOR, (uint8_t*)fs_fat);
    disk_flush_cache();
}
int fs_create(const char* filename, bool is_dir, uint32_t parent_dir, bool isSilent) {
    if (!disk_read_sector(FS_ROOT_DIR_SECTOR, (uint8_t*)fs_files)) {
        if (!isSilent) print_error("Cannot read filesystem\n");
        return -1;
    }
    if (parent_dir >= FS_MAX_FILES || !fs_files[parent_dir].is_used || !fs_files[parent_dir].is_dir) {
        if (!isSilent) print_error("Parent directory not found\n");
        return -1;
    }
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (fs_files[i].is_used && fs_files[i].parent_dir == parent_dir && 
            strcasecmp(fs_files[i].name, filename) == 0) {
            if (!isSilent) print_error("File/directory already exists\n");
            return -1;
        }
    }
    for (int i = 1; i < FS_MAX_FILES; i++) {
        if (!fs_files[i].is_used) {
            strncpy(fs_files[i].name, filename, FS_FILENAME_LEN-1);
            fs_files[i].name[FS_FILENAME_LEN-1] = '\0';
            fs_files[i].is_used = 1;
            fs_files[i].is_dir = is_dir;
            fs_files[i].size = 0;
            fs_files[i].first_sector = 0xFFFF;
            fs_files[i].parent_dir = parent_dir;
            fs_files[i].is_readonly = 0;
            if (!disk_write_sector(FS_ROOT_DIR_SECTOR, (uint8_t*)fs_files)) {
                if (!isSilent) print_error("Failed to save filesystem\n");
                memset(&fs_files[i], 0, sizeof(fs_file_entry_t));
                return -1;
            }
            fs_sync();
            if (!isSilent) {
                print_success("Created ");
                print(filename, WHITE);
                print(is_dir ? " [DIR]\n" : " [FILE]\n", WHITE);
            }
            return i;
        }
    }
    if (!isSilent) print_error("Maximum files reached\n");
    return -1;
}
int fs_write(const char* filename, const void* data, uint32_t size, uint32_t parent_dir) {
    if (!disk_read_sector(FS_ROOT_DIR_SECTOR, (uint8_t*)fs_files) ||
        !disk_read_sector(FS_FAT_SECTOR, (uint8_t*)fs_fat)) {
        print_error("Cannot read filesystem\n");
        return -1;
    }
    fs_file_entry_t* file = NULL;
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (fs_files[i].is_used && !fs_files[i].is_dir && 
            fs_files[i].parent_dir == parent_dir && 
            strcasecmp(fs_files[i].name, filename) == 0) {
            file = &fs_files[i];
            break;
        }
    }
    if (!file) return -1;
    if (file->is_readonly) return -2;
    if (file->first_sector != 0xFFFF) {
        uint32_t current_sector = file->first_sector;
        while (current_sector != 0xFFFF) {
            uint32_t next_sector = fs_fat[current_sector];
            fs_fat[current_sector] = 0;
            current_sector = next_sector;
        }
        file->first_sector = 0xFFFF;
    }
    uint32_t sectors_needed = (size + FS_SECTOR_SIZE - 1) / FS_SECTOR_SIZE;
    uint32_t first_sector = 0xFFFF;
    uint32_t prev_sector = 0xFFFF;
    uint32_t sectors_found = 0;
    for (uint32_t sector = FS_DATA_START_SECTOR; sector < 1000 && sectors_found < sectors_needed; sector++) {
        if (fs_fat[sector] == 0) {
            if (first_sector == 0xFFFF) {
                first_sector = sector;
            } else {
                fs_fat[prev_sector] = sector;
            }
            prev_sector = sector;
            sectors_found++;
            fs_fat[sector] = 0xFFFF;
        }
    }
    if (sectors_found < sectors_needed) {
        if (first_sector != 0xFFFF) {
            uint32_t sector = first_sector;
            while (sector != 0xFFFF) {
                uint32_t next = fs_fat[sector];
                fs_fat[sector] = 0;
                sector = next;
            }
        }
        return -3;
    }
    uint32_t bytes_written = 0;
    uint32_t current_sector = first_sector;
    const uint8_t* data_ptr = (const uint8_t*)data;
    while (bytes_written < size && current_sector != 0xFFFF) {
        uint32_t bytes_to_write = (size - bytes_written > FS_SECTOR_SIZE) ? 
                                 FS_SECTOR_SIZE : (size - bytes_written);
        uint8_t sector_buffer[FS_SECTOR_SIZE] = {0};
        memcpy(sector_buffer, data_ptr + bytes_written, bytes_to_write);
        if (!disk_write_sector(current_sector, sector_buffer)) {
            uint32_t sector = first_sector;
            while (sector != 0xFFFF) {
                uint32_t next = fs_fat[sector];
                fs_fat[sector] = 0;
                sector = next;
            }
            return -4;
        }
        bytes_written += bytes_to_write;
        current_sector = fs_fat[current_sector];
    }
    file->first_sector = first_sector;
    file->size = size;
    fs_sync();
    print_success("Written ");
    print_dec(size, CYAN);
    print(" bytes to ", GREEN);
    print(filename, WHITE);
    print("\n", WHITE);
    return 0;
}
int fs_read(const char* filename, void* buffer, uint32_t max_size, uint32_t parent_dir) {
    if (!disk_read_sector(FS_ROOT_DIR_SECTOR, (uint8_t*)fs_files)) {
        return -1;
    }
    fs_file_entry_t* file = NULL;
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (fs_files[i].is_used && !fs_files[i].is_dir && 
            fs_files[i].parent_dir == parent_dir && 
            strcasecmp(fs_files[i].name, filename) == 0) {
            file = &fs_files[i];
            break;
        }
    }
    if (!file) {
        print_error("File not found: ");
        print(filename, WHITE);
        print("\n", WHITE);
        return -1;
    }
    if (file->size == 0 || file->first_sector == 0xFFFF) {
        return 0;
    }
    uint32_t bytes_to_read = (file->size < max_size) ? file->size : max_size;
    uint32_t bytes_read = 0;
    uint32_t current_sector = file->first_sector;
    uint8_t* buf_ptr = (uint8_t*)buffer;
    while (bytes_read < bytes_to_read && current_sector != 0xFFFF) {
        uint8_t sector_buffer[FS_SECTOR_SIZE];
        if (!disk_read_sector(current_sector, sector_buffer)) {
            print_error("Disk read error at sector ");
            print_dec(current_sector, WHITE);
            print("\n", WHITE);
            return -1;
        }
        uint32_t bytes_in_sector = bytes_to_read - bytes_read;
        if (bytes_in_sector > FS_SECTOR_SIZE) bytes_in_sector = FS_SECTOR_SIZE;
        memcpy(buf_ptr + bytes_read, sector_buffer, bytes_in_sector);
        bytes_read += bytes_in_sector;
        current_sector = fs_fat[current_sector];
    }
    return bytes_read;
}
int fs_delete(const char* name, uint32_t parent_dir) {
    if (!disk_read_sector(FS_ROOT_DIR_SECTOR, (uint8_t*)fs_files) ||
        !disk_read_sector(FS_FAT_SECTOR, (uint8_t*)fs_fat)) {
        print_error("Cannot read filesystem\n");
        return -1;
    }
    fs_file_entry_t* file = NULL;
    int file_index = -1;
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (fs_files[i].is_used && fs_files[i].parent_dir == parent_dir && 
            strcasecmp(fs_files[i].name, name) == 0) {
            file = &fs_files[i];
            file_index = i;
            break;
        }
    }
    if (!file) {
        print_error("File not found: ");
        print(name, WHITE);
        print("\n", WHITE);
        return -1;
    }
    if (file->is_dir) {
        for (int i = 0; i < FS_MAX_FILES; i++) {
            if (fs_files[i].is_used && fs_files[i].parent_dir == file_index) {
                print_error("Directory not empty: ");
                print(name, WHITE);
                print("\n", WHITE);
                return -2;
            }
        }
    }
    if (file->first_sector != 0xFFFF) {
        uint32_t current_sector = file->first_sector;
        while (current_sector != 0xFFFF) {
            uint32_t next_sector = fs_fat[current_sector];
            fs_fat[current_sector] = 0;
            current_sector = next_sector;
        }
    }
    memset(file, 0, sizeof(fs_file_entry_t));
    fs_sync();
    print_success("Deleted ");
    print(name, WHITE);
    print("\n", WHITE);
    return 0;
}
int fs_rename(const char* oldname, const char* newname, uint32_t parent_dir_id) {
    if (!disk_read_sector(FS_ROOT_DIR_SECTOR, (uint8_t*)fs_files)) {
        return -1;
    }
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (fs_files[i].is_used && fs_files[i].parent_dir == parent_dir_id && 
            strcasecmp(fs_files[i].name, newname) == 0) {
            print_error("File already exists: ");
            print(newname, WHITE);
            print("\n", WHITE);
            return -1;
        }
    }
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (fs_files[i].is_used && fs_files[i].parent_dir == parent_dir_id && 
            strcasecmp(fs_files[i].name, oldname) == 0) {
            char old_name_copy[FS_FILENAME_LEN];
            strcpy(old_name_copy, fs_files[i].name);
            strncpy(fs_files[i].name, newname, FS_FILENAME_LEN-1);
            fs_files[i].name[FS_FILENAME_LEN-1] = '\0';
            fs_sync();
            print_success("Renamed ");
            print(old_name_copy, WHITE);
            print(" -> ", GREEN);
            print(newname, WHITE);
            print("\n", WHITE);
            return 0;
        }
    }
    print_error("Source file not found: ");
    print(oldname, WHITE);
    print("\n", WHITE);
    return -1;
}
int fs_list(uint32_t dir_id) {
    if (!disk_read_sector(FS_ROOT_DIR_SECTOR, (uint8_t*)fs_files)) {
        print_error("Cannot read filesystem\n");
        return -1;
    }
    if (dir_id >= FS_MAX_FILES || !fs_files[dir_id].is_used || !fs_files[dir_id].is_dir) {
        print_error("Invalid directory\n");
        return -1;
    }
    print("Contents of '", WHITE);
    print(fs_files[dir_id].name, CYAN);
    print("':\n", WHITE);
    print("Name                   Type    Size     Perms\n", WHITE);
    print("---------------------------------------------\n", WHITE);
    int count = 0;
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (fs_files[i].is_used && fs_files[i].parent_dir == dir_id) {
            print(fs_files[i].name, fs_files[i].is_dir ? CYAN : WHITE);
            int name_len = strlen(fs_files[i].name);
            for (int s = name_len; s < 20; s++) print(" ", WHITE);
            print(fs_files[i].is_dir ? "  <DIR>  " : "  <FILE> ", 
                 fs_files[i].is_dir ? CYAN : WHITE);
            if (fs_files[i].is_dir) {
                print("          ", WHITE);
            } else {
                char size_buf[16];
                itoa(fs_files[i].size, size_buf, 10);
                int num_len = strlen(size_buf);
                int total_len = num_len + 6;
                int spaces = 10 - total_len - 1;
                print(" ", WHITE);
                print_dec(fs_files[i].size, GREEN);
                print(" bytes", WHITE);
                for (int s = 0; s < spaces; s++) print(" ", WHITE);
            }
            print(fs_files[i].is_readonly ? "  RO" : "  RW", 
                 fs_files[i].is_readonly ? RED : GREEN);
            print("\n", WHITE);
            count++;
        }
    }
    if (count == 0) {
        print("Directory is empty\n", GRAY);
    } else {
        print_dec(count, CYAN);
        print(" items total\n", WHITE);
    }
    return count;
}
int fs_chmod(const char* name, bool readonly, uint32_t parent_dir) {
    if (!disk_read_sector(FS_ROOT_DIR_SECTOR, (uint8_t*)fs_files)) {
        return -1;
    }
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (fs_files[i].is_used && fs_files[i].parent_dir == parent_dir && 
            strcasecmp(fs_files[i].name, name) == 0) {
            fs_files[i].is_readonly = readonly;
            fs_sync();
            print_success("Permissions changed: ");
            print(name, WHITE);
            print(" -> ", GREEN);
            print(readonly ? "READ-ONLY\n" : "READ-WRITE\n", WHITE);
            return 0;
        }
    }
    print_error("File not found: ");
    print(name, WHITE);
    print("\n", WHITE);
    return -1;
}
int fs_find(const char* pattern, uint32_t dir_id, bool recursive) {
    if (!disk_read_sector(FS_ROOT_DIR_SECTOR, (uint8_t*)fs_files)) {
        return -1;
    }
    int found = 0;
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (fs_files[i].is_used && fs_files[i].parent_dir == dir_id) {
            bool match = (strstr(fs_files[i].name, pattern) != NULL);
            if (match) {
                print(fs_files[i].name, WHITE);
                if (fs_files[i].is_dir) {
                    print(" [DIR]", CYAN);
                } else {
                    print(" [", WHITE);
                    print_dec(fs_files[i].size, GREEN);
                    print(" bytes]", WHITE);
                }
                print("\n", WHITE);
                found++;
            }
            if (recursive && fs_files[i].is_dir) {
                found += fs_find(pattern, i, true);
            }
        }
    }
    if (found == 0) {
        print("No files found matching: ", YELLOW);
        print(pattern, WHITE);
        print("\n", WHITE);
    } else {
        print_dec(found, CYAN);
        print(" files found\n", WHITE);
    }
    return found;
}
void fs_cat_file(const char* filename, uint32_t parent_dir) {
    char buffer[256];
    int bytes_read = fs_read(filename, buffer, sizeof(buffer)-1, parent_dir);
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        print("File content: ", CYAN);
        print(buffer, WHITE);
        print("\n", WHITE);
    } else if (bytes_read == 0) {
        print("File is empty\n", YELLOW);
    } else {
        print_error("Cannot read file: ");
        print(filename, WHITE);
        print("\n", WHITE);
    }
}
uint32_t fs_get_current_dir(void) {
    return fs_current_dir;
}
void fs_set_current_dir(uint32_t dir_id) {
    if (dir_id < FS_MAX_FILES && fs_files[dir_id].is_used && fs_files[dir_id].is_dir) {
        fs_current_dir = dir_id;
    }
}
int fs_resolve_path(const char* path, uint32_t* parent_dir_id) {
    if (!disk_read_sector(FS_ROOT_DIR_SECTOR, (uint8_t*)fs_files)) {
        return -1;
    }
    if (strcmp(path, "/") == 0) {
        *parent_dir_id = 0;
        return 0;
    }
    if (strcmp(path, ".") == 0) {
        *parent_dir_id = fs_current_dir;
        return 0;
    }
    if (strcmp(path, "..") == 0) {
        if (fs_current_dir == 0) {
            *parent_dir_id = 0; 
        } else {
            *parent_dir_id = fs_files[fs_current_dir].parent_dir;
        }
        return 0;
    }
    *parent_dir_id = fs_current_dir;
    return 0;
}
bool fs_is_dir(int index) {
    if (index < 0 || index >= FS_MAX_FILES) return false;
    return fs_files[index].is_dir;
}
const char* fs_get_entry_name(int index) {
    if (index < 0 || index >= FS_MAX_FILES) return "";
    return fs_files[index].name;
}
int fs_find_directory(const char* path, uint32_t current_dir) {
    if (strcmp(path, "/") == 0) return 0;
    if (strcmp(path, ".") == 0) return current_dir;
    char path_copy[FS_FILENAME_LEN];
    strcpy(path_copy, path);
    char* token = strtok(path_copy, "/");
    uint32_t dir_id = current_dir;
    while (token != NULL) {
        if (strcmp(token, "..") == 0) {
            if (dir_id != 0) {
                dir_id = fs_files[dir_id].parent_dir;
            }
        } else if (strcmp(token, ".") != 0 && strlen(token) > 0) {
            bool found = false;
            for (int i = 0; i < FS_MAX_FILES; i++) {
                if (fs_files[i].is_used && fs_files[i].is_dir && 
                    fs_files[i].parent_dir == dir_id && 
                    strcasecmp(fs_files[i].name, token) == 0) {
                    dir_id = i;
                    found = true;
                    break;
                }
            }
            if (!found) return -1;
        }
        token = strtok(NULL, "/");
    }
    return dir_id;
}
bool fs_format(void) {
    disk_format_lowlevel();
    reboot();
}
