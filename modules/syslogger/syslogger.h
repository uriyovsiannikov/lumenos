#ifndef SYSLOGGER_H
#define SYSLOGGER_H
#define MAX_LOG_ENTRIES 100
#define MAX_LOG_LENGTH 128
typedef enum {
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR
} log_level_t;
void log_message(const char* message, log_level_t level);
void show_logs();
void clear_logs();
void syslogger_command(const char* args);
#endif 
