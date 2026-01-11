#include "string.h"

size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len]) len++;
    return len;
}

char* strncpy(char* dest, const char* src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i] != '\0'; i++)
        dest[i] = src[i];
    for ( ; i < n; i++)
        dest[i] = '\0';
    return dest;
}

bool isdigit(char c) {
    return (c >= '0' && c <= '9');
}

char* strchr(const char* str, int c) {
    while (*str != (char)c) {
        if (!*str++) return NULL;
    }
    return (char*)str;
}
