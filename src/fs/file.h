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
    FILE_SEEK_SET,
    FILE_SEEK_CUR,
    FILE_SEEK_END
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
typedef int (*FS_SEEK_FUNCTION)(void* private, int offset, FILE_SEEK_MODE whence);
typedef int (*FS_CLOSE_FUNCTION)(void* private);
typedef int (*FS_TELL_FUNCTION)(void* private);
typedef int (*FS_LIST_FUNCTION)(struct disk* disk, struct path_part* path);

typedef uint32_t FILE_STAT_FLAGS;
#define FILE_STAT_READ_ONLY 0x01
#define FILE_STAT_HIDDEN 0x02
#define FILE_STAT_SYSTEM 0x04
#define FILE_STAT_VOLUME_LABEL 0x08
#define FILE_STAT_DIRECTORY 0x10
#define FILE_STAT_ARCHIVE 0x20

struct file_stat {
    FILE_STAT_FLAGS flags;
    uint32_t filesize;
};

typedef int (*FS_STAT_FUNCTION)(struct disk* disk, void* private, struct file_stat* stat);

typedef int (*FS_RESOLVE_FUNCTION)(struct disk* disk);

struct filesystem {
    char name[20];
    FS_RESOLVE_FUNCTION resolve;
    FS_OPEN_FUNCTION open;
    FS_READ_FUNCTION read;
    FS_SEEK_FUNCTION seek;
    FS_TELL_FUNCTION tell;
    FS_CLOSE_FUNCTION close;
    FS_STAT_FUNCTION stat;
    FS_LIST_FUNCTION list;
};

void fs_init();
int fs_insert_filesystem(struct filesystem* fs);
struct filesystem* fs_resolve(struct disk* disk);

int fopen(const char* filename, const char* mode_str);
int fread(void* ptr, uint32_t size, uint32_t nmemb, int fd);
int fseek(int fd, int offset, FILE_SEEK_MODE whence);
int ftell(int fd);
int fstat(int fd, struct file_stat* stat);
int fclose(int fd);
int fs_list(const char* path);

#endif
