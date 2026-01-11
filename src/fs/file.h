#ifndef FILE_H
#define FILE_H

#include <stdint.h>
#include <stddef.h>
#include "path_parser.h"

typedef enum {
    FILE_MODE_READ,
    FILE_MODE_WRITE,
    FILE_MODE_APPEND,
    FILE_MODE_INVALID
} FILE_MODE;

typedef enum {
    SEEK_SET,
    SEEK_CUR,
    SEEK_END
} FILE_SEEK_MODE;

struct disk {
    int id;
    void* fs_private;
};

struct filesystem;

struct file_descriptor {
    int index;
    struct filesystem* filesystem;
    void* private;
    struct disk* disk;
};

typedef void* (*FS_OPEN_FUNCTION)(struct disk* disk, struct path_part* path, FILE_MODE mode);
typedef int (*FS_READ_FUNCTION)(struct disk* disk, void* private, uint32_t size, uint32_t nmemb, char* out);
typedef int (*FS_SEEK_FUNCTION)(void* private, uint32_t offset, FILE_SEEK_MODE whence);
typedef int (*FS_CLOSE_FUNCTION)(void* private);
typedef int (*FS_RESOLVE_FUNCTION)(struct disk* disk);

struct filesystem {
    char name[20];
    FS_RESOLVE_FUNCTION resolve;
    FS_OPEN_FUNCTION open;
    FS_READ_FUNCTION read;
    FS_SEEK_FUNCTION seek;
    FS_CLOSE_FUNCTION close;
};

void fs_init();
int fs_insert_filesystem(struct filesystem* fs);
struct filesystem* fs_resolve(struct disk* disk);

int fopen(const char* filename, const char* mode_str);
int fread(void* ptr, uint32_t size, uint32_t nmemb, int fd);
int fseek(int fd, uint32_t offset, FILE_SEEK_MODE whence);
int fclose(int fd);

#endif
