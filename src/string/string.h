#ifndef STRING_H
#define STRING_H

#include <stddef.h>
#include <stdbool.h>

size_t strlen(const char* str);
char* strncpy(char* dest, const char* src, size_t n);
bool isdigit(char c);
char* strchr(const char* str, int c);

#endif
