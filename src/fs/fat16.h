#ifndef FAT16_H
#define FAT16_H

#include <stdint.h>
#include "../drivers/disk_stream.h"

#define FAT16_SIGNATURE 0x29
#define FAT16_ENTRY_SIZE 2
#define FAT16_UNUSED 0x00

#define FAT16_CLUSTER_FREE 0x0000
#define FAT16_CLUSTER_RESERVED_MIN 0xFFF0
#define FAT16_CLUSTER_RESERVED_MAX 0xFFF6
#define FAT16_CLUSTER_BAD 0xFFF7
#define FAT16_CLUSTER_LAST_MIN 0xFFF8
#define FAT16_CLUSTER_LAST_MAX 0xFFFF

typedef uint8_t FAT_DIRECTORY_ITEM_ATTRIB;
#define FAT_FILE_READ_ONLY 0x01
#define FAT_FILE_HIDDEN 0x02
#define FAT_FILE_SYSTEM 0x04
#define FAT_FILE_VOLUME_LABEL 0x08
#define FAT_FILE_SUBDIRECTORY 0x10
#define FAT_FILE_ARCHIVE 0x20
#define FAT_FILE_DEVICE 0x40
#define FAT_FILE_RESERVED 0x80

struct fat_boot_sector {
    uint8_t short_jmp_ins[3];
    uint8_t oem_identifier[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t fat_copies;
    uint16_t root_dir_entries;
    uint16_t total_sectors;
    uint8_t media_type;
    uint16_t fat_sectors;
    uint16_t sectors_per_track;
    uint16_t number_of_heads;
    uint32_t hidden_sectors;
    uint32_t sectors_big;

    // Extended BPB (FAT12/FAT16)
    uint8_t drive_number;
    uint8_t reserved;
    uint8_t signature;
    uint32_t volume_id;
    uint8_t volume_label[11];
    uint8_t system_id[8];

    uint8_t boot_code[448];
    uint16_t boot_signature;
} __attribute__((packed));

struct fat_directory_item {
    uint8_t filename[8];
    uint8_t ext[3];
    uint8_t attribute;
    uint8_t reserved;
    uint8_t creation_time_tenth;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t last_access_date;
    uint16_t high_16_bits_first_cluster;
    uint16_t last_mod_time;
    uint16_t last_mod_date;
    uint16_t low_16_bits_first_cluster;
    uint32_t filesize;
} __attribute__((packed));

struct fat_private {
    struct fat_boot_sector bpb;
    struct disk_stream* stream;
};

// Generic Filesystem Interface (Simplified for now)
struct fat_file_descriptor {
    struct fat_directory_item item;
    uint32_t pos;
    uint32_t last_cluster;
    uint32_t last_cluster_pos;
};

#include "file.h"

int fat16_resolve(struct disk* disk);
void* fat16_open(struct disk* disk, struct path_part* path, FILE_MODE mode);
struct filesystem* fat16_init_vfs();

int fat16_read(struct disk* disk, void* private, uint32_t size, uint32_t nmemb, char* out);
int fat16_seek(void* private, int offset, FILE_SEEK_MODE whence);
int fat16_tell(void* private);
int fat16_close(void* private);
int fat16_stat(struct disk* disk, void* private, struct file_stat* stat);
int fat16_list(struct disk* disk, struct path_part* path);

// For Testing
uint32_t fat16_cluster_to_sector(struct disk* disk, uint32_t cluster);
uint32_t fat16_get_root_directory_sector(struct disk* disk);
uint32_t fat16_get_bytes_per_sector(struct disk* disk);

#endif
