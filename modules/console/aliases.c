#include "../libs/print.h"
#include "../modules/syslogger/syslogger.h"
#include <stddef.h>
#include <string.h>
#include "aliases.h"
static struct alias aliases[MAX_ALIASES];
const char* resolve_alias(const char* name);
static uint8_t alias_count = 0;
void add_alias(const char* name, const char* value) {
    if (alias_count >= MAX_ALIASES) {
        print_error("Maximum number of aliases reached");
		log_message("Aliases limit reached",LOG_ERROR);
        return;
    }
    if (strlen(name) >= MAX_ALIAS_LENGTH || strlen(value) >= MAX_ALIAS_VALUE_LENGTH) {
        print_error("Alias name or value too long");
		log_message("Aliases internal error",LOG_ERROR);
        return;
    }
    for (int i = 0; i < alias_count; i++) {
        if (strcmp(aliases[i].name, name) == 0) {
            strcpy(aliases[i].value, value);
            print_success("Alias updated");
            return;
        }
    }
    strcpy(aliases[alias_count].name, name);
    strcpy(aliases[alias_count].value, value);
    alias_count++;
    print_success("Alias added");
}
void remove_alias(const char* name) {
    for (int i = 0; i < alias_count; i++) {
        if (strcmp(aliases[i].name, name) == 0) {
            for (int j = i; j < alias_count - 1; j++) {
                strcpy(aliases[j].name, aliases[j+1].name);
                strcpy(aliases[j].value, aliases[j+1].value);
            }
            alias_count--;
            print_success("Alias removed");
            return;
        }
    }
    print_error("Alias not found");
	log_message("Aliases internal error",LOG_ERROR);
}
void list_aliases() {
    if (alias_count == 0) {
        print_info("No aliases defined");
        return;
    }
    print_info("Defined aliases:");
    for (int i = 0; i < alias_count; i++) {
        print("\n  ", WHITE);
        print(aliases[i].name, CYAN);
        print(" = '", WHITE);
        print(aliases[i].value, WHITE);
        print("'", WHITE);
    }
    print("\n", WHITE);
}
const char* resolve_alias(const char* name) {
    for (int i = 0; i < alias_count; i++) {
        if (strcmp(aliases[i].name, name) == 0) {
            return aliases[i].value;
        }
    }
    return NULL;
}