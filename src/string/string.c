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

void* memset(void* ptr, int c, size_t size)
{
    char* c_ptr = (char*)ptr;
    for (size_t i = 0; i < size; i++)
    {
        c_ptr[i] = (char)c;
    }
    return ptr;
}

int strncmp(const char* str1, const char* str2, size_t n)
{
    unsigned char u1, u2;
    while (n > 0)
    {
        u1 = (unsigned char)*str1++;
        u2 = (unsigned char)*str2++;
        if (u1 != u2)
        {
            return u1 - u2;
        }
        if (u1 == '\0')
        {
            return 0;
        }
        n--;
    }
    return 0;
}

int tolower(int c)
{
    if (c >= 'A' && c <= 'Z')
    {
        return c + 32;
    }
    return c;
}

void* memcpy(void* dest, const void* src, size_t n)
{
    char* d = dest;
    const char* s = src;
    while (n--)
    {
        *d++ = *s++;
    }
    return dest;
}

char* strcpy(char* dest, const char* src) {
    char* d = dest;
    while ((*d++ = *src++));
    return dest;
}
