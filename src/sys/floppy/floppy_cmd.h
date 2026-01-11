#ifndef FLOPPY_CMD_H
#define FLOPPY_CMD_H
void cmd_floppy_info(int argc, char **argv);
void cmd_floppy_read(int argc, char **argv);
void cmd_floppy_write(int argc, char **argv);
void cmd_floppy_calibrate(int argc, char **argv);
void cmd_floppy_test(int argc, char **argv);
void cmd_floppy_format(int argc, char **argv);
void cmd_floppy_dir(int argc, char **argv);
void cmd_floppy_copy(int argc, char **argv);
void cmd_floppy_erase(int argc, char **argv);
void register_floppy_commands(void);
#endif
