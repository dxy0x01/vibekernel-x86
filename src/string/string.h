#ifndef STRING_H
#define STRING_H

#include <stddef.h>
#include <stdbool.h>

size_t strlen(const char* str);
char* strncpy(char* dest, const char* src, size_t n);
bool isdigit(char c);
char* strchr(const char* str, int c);
void* memset(void* ptr, int c, size_t size);
int strncmp(const char* str1, const char* str2, size_t n);
int tolower(int c);
int toupper(int c);
int strncasecmp(const char *s1, const char *s2, size_t n);
void* memcpy(void* dest, const void* src, size_t n);

char* strcpy(char* dest, const char* src);
int strcmp(const char* str1, const char* str2);

#endif
