#include "file.h"
#include "../memory/heap/kheap.h"
#include "../string/string.h"
#include "path_parser.h"
#include <stddef.h>

#define MAX_FILESYSTEMS 12
#define MAX_FILE_DESCRIPTORS 512

static struct filesystem* filesystems[MAX_FILESYSTEMS];
static struct file_descriptor* file_descriptors[MAX_FILE_DESCRIPTORS];

static struct disk disks[2];

static struct filesystem** fs_get_free_filesystem() {
    for (int i = 0; i < MAX_FILESYSTEMS; i++) {
        if (filesystems[i] == NULL) {
            return &filesystems[i];
        }
    }
    return NULL;
}

void fs_init() {
    memset(filesystems, 0, sizeof(filesystems));
    memset(file_descriptors, 0, sizeof(file_descriptors));
    memset(disks, 0, sizeof(disks));
    for (int i = 0; i < 2; i++) {
        disks[i].id = i;
    }
}

int fs_insert_filesystem(struct filesystem* fs) {
    struct filesystem** free_fs = fs_get_free_filesystem();
    if (!free_fs) return -1;
    *free_fs = fs;
    return 0;
}

static struct file_descriptor* fs_new_descriptor() {
    for (int i = 0; i < MAX_FILE_DESCRIPTORS; i++) {
        if (file_descriptors[i] == NULL) {
            struct file_descriptor* desc = kmalloc(sizeof(struct file_descriptor));
            desc->index = i + 1;
            file_descriptors[i] = desc;
            return desc;
        }
    }
    return NULL;
}

static struct file_descriptor* fs_get_descriptor(int fd) {
    if (fd <= 0 || fd > MAX_FILE_DESCRIPTORS) return NULL;
    return file_descriptors[fd - 1];
}

struct filesystem* fs_resolve(struct disk* disk) {
    for (int i = 0; i < MAX_FILESYSTEMS; i++) {
        if (filesystems[i] != NULL && filesystems[i]->resolve(disk) == 0) {
            return filesystems[i];
        }
    }
    return NULL;
}

static FILE_MODE fs_parse_mode(const char* str) {
    if (strncmp(str, "r", 1) == 0) return FILE_MODE_READ;
    if (strncmp(str, "w", 1) == 0) return FILE_MODE_WRITE;
    if (strncmp(str, "a", 1) == 0) return FILE_MODE_APPEND;
    return FILE_MODE_INVALID;
}

int fopen(const char* filename, const char* mode_str) {
    int res = 0;
    struct path_root* root_path = path_parser_parse(filename, NULL);
    if (!root_path) {
        res = -1;
        goto out;
    }

    if (!root_path->first) {
        res = -2;
        goto out;
    }

    int drive_no = root_path->drive_no;
    if (drive_no < 0 || drive_no > 1) {
        res = -3;
        goto out;
    }
    
    struct disk* disk = &disks[drive_no];
    struct filesystem* fs = fs_resolve(disk);
    if (!fs) {
        res = -4;
        goto out;
    }

    FILE_MODE mode = fs_parse_mode(mode_str);
    if (mode == FILE_MODE_INVALID) {
        res = -5;
        goto out;
    }

    void* fs_private = fs->open(disk, root_path->first, mode);
    if (!fs_private) {
        res = -6;
        goto out;
    }

    struct file_descriptor* desc = fs_new_descriptor();
    if (!desc) {
        res = -7;
        goto out;
    }

    desc->filesystem = fs;
    desc->private = fs_private;
    desc->disk = disk;
    res = desc->index;

out:
    if (root_path) {
        path_parser_free(root_path);
    }
    return res;
}

int fread(void* ptr, uint32_t size, uint32_t nmemb, int fd) {
    struct file_descriptor* desc = fs_get_descriptor(fd);
    if (!desc || !desc->filesystem->read) return -1;
    return desc->filesystem->read(desc->disk, desc->private, size, nmemb, (char*)ptr);
}

int fseek(int fd, int offset, FILE_SEEK_MODE whence) {
    struct file_descriptor* desc = fs_get_descriptor(fd);
    if (!desc || !desc->filesystem->seek) return -1;
    return desc->filesystem->seek(desc->private, offset, whence);
}

int ftell(int fd) {
    struct file_descriptor* desc = fs_get_descriptor(fd);
    if (!desc || !desc->filesystem->tell) return -1;
    return desc->filesystem->tell(desc->private);
}

int fstat(int fd, struct file_stat* stat) {
    struct file_descriptor* desc = fs_get_descriptor(fd);
    if (!desc || !desc->filesystem->stat) return -1;
    return desc->filesystem->stat(desc->disk, desc->private, stat);
}

int fclose(int fd) {
    struct file_descriptor* desc = fs_get_descriptor(fd);
    if (!desc || !desc->filesystem->close) return -1;
    int res = desc->filesystem->close(desc->private);
    if (res == 0) {
        file_descriptors[desc->index - 1] = NULL;
        kfree(desc);
    }
    return res;
}
