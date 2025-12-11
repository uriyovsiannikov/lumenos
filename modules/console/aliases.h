#ifndef ALIASES_H
#define ALIASES_H
#include <stdint.h>
#define MAX_ALIASES 20
#define MAX_ALIAS_LENGTH 50
#define MAX_ALIAS_VALUE_LENGTH 100
struct alias {
    char name[MAX_ALIAS_LENGTH];
    char value[MAX_ALIAS_VALUE_LENGTH];
};
void add_alias(const char* name, const char* value);
void remove_alias(const char* name);
void list_aliases();
const char* resolve_alias(const char* name);
#endif 