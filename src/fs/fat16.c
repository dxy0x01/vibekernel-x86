#include "fat16.h"
#include "path_parser.h"
#include "../drivers/disk_stream.h"
#include "../memory/heap/kheap.h"
#include "../string/string.h"
#include <stddef.h>

static struct fat_private fs_private;

int fat16_get_total_items_for_directory(struct fat_directory_item* item) {
    // This is a simplified version, as FAT16 root directory has a fixed size
    // but subdirectories are cluster chains.
    return 0; // Not yet fully implemented for subdirs
}

static void fat16_to_proper_string(char** out, const char* in) {
    while(*in != 0x00 && *in != 0x20) {
        **out = *in;
        *out += 1;
        in += 1;
    }
}

void fat16_get_full_relative_filename(struct fat_directory_item* item, char* out, int max_len) {
    memset(out, 0x00, max_len);
    char* temp_out = out;
    fat16_to_proper_string(&temp_out, (const char*)item->filename);
    if (item->ext[0] != 0x00 && item->ext[0] != 0x20) {
        *temp_out++ = '.';
        fat16_to_proper_string(&temp_out, (const char*)item->ext);
    }
}

int fat16_init(int disk_id) {
    fs_private.stream = diskstream_new(disk_id);
    if (!fs_private.stream) {
        return -1;
    }

    if (diskstream_read(fs_private.stream, &fs_private.header, sizeof(fs_private.header)) != 0) {
        return -2;
    }

    if (diskstream_read(fs_private.stream, &fs_private.header_extended, sizeof(fs_private.header_extended)) != 0) {
        return -3;
    }

    if (fs_private.header_extended.signature != FAT16_SIGNATURE) {
        return -4;
    }

    return 0;
}

// Logic to convert cluster to absolute sector
uint32_t fat16_cluster_to_sector(uint32_t cluster) {
    uint32_t root_dir_sectors = ((fs_private.header.root_dir_entries * 32) + (fs_private.header.bytes_per_sector - 1)) / fs_private.header.bytes_per_sector;
    uint32_t first_data_sector = fs_private.header.reserved_sectors + (fs_private.header.fat_copies * fs_private.header.fat_sectors) + root_dir_sectors;
    return first_data_sector + (cluster - 2) * fs_private.header.sectors_per_cluster;
}

uint32_t fat16_get_root_directory_sector() {
    return fs_private.header.reserved_sectors + (fs_private.header.fat_copies * fs_private.header.fat_sectors);
}

uint32_t fat16_get_bytes_per_sector() {
    return fs_private.header.bytes_per_sector;
}

static int fat16_get_directory_entry(const char* name, struct fat_directory_item* out_item) {
    uint32_t root_dir_sector = fat16_get_root_directory_sector();
    uint32_t root_dir_entries = fs_private.header.root_dir_entries;
    
    diskstream_seek(fs_private.stream, root_dir_sector * fs_private.header.bytes_per_sector);
    
    for (int i = 0; i < root_dir_entries; i++) {
        struct fat_directory_item item;
        if (diskstream_read(fs_private.stream, &item, sizeof(item)) != 0) {
            return -1;
        }
        
        if (item.filename[0] == 0x00) {
            break; // No more items
        }
        
        if (item.filename[0] == 0xE5) {
            continue; // Deleted item
        }
        
        char out_name[13];
        fat16_get_full_relative_filename(&item, out_name, sizeof(out_name));
        
        // case insensitive compare usually?
        if (strncmp(out_name, name, sizeof(out_name)) == 0) {
            *out_item = item;
            return 0;
        }
    }
    
    return -2; // Not found
}

struct fat_file_descriptor* fat16_open(const char* filename) {
    struct fat_directory_item item;
    if (fat16_get_directory_entry(filename, &item) != 0) {
        return NULL;
    }
    
    struct fat_file_descriptor* desc = kmalloc(sizeof(struct fat_file_descriptor));
    desc->item = item;
    desc->pos = 0;
    return desc;
}

void fat16_close(struct fat_file_descriptor* desc) {
    kfree(desc);
}
static uint32_t fat16_get_fat_entry(uint32_t cluster) {
    uint32_t fat_table_sector = fs_private.header.reserved_sectors;
    uint32_t fat_table_offset = cluster * FAT16_ENTRY_SIZE;
    uint32_t fat_table_abs_pos = (fat_table_sector * fs_private.header.bytes_per_sector) + fat_table_offset;
    
    diskstream_seek(fs_private.stream, fat_table_abs_pos);
    
    uint16_t entry = 0;
    if (diskstream_read(fs_private.stream, &entry, sizeof(entry)) != 0) {
        return 0xFFFF; // Error
    }
    
    return entry;
}

static uint32_t fat16_get_cluster_for_offset(uint32_t start_cluster, uint32_t offset) {
    uint32_t cluster_size = fs_private.header.sectors_per_cluster * fs_private.header.bytes_per_sector;
    uint32_t clusters_to_skip = offset / cluster_size;
    uint32_t current_cluster = start_cluster;
    
    for (uint32_t i = 0; i < clusters_to_skip; i++) {
        current_cluster = fat16_get_fat_entry(current_cluster);
        if (current_cluster >= 0xFFF8) {
            return 0xFFFF; // End of chain
        }
    }
    
    return current_cluster;
}

int fat16_read(struct fat_file_descriptor* desc, uint32_t size, uint32_t nmemb, char* out) {
    uint32_t total_to_read = size * nmemb;
    if (desc->pos + total_to_read > desc->item.filesize) {
        total_to_read = desc->item.filesize - desc->pos;
    }
    
    uint32_t total_read = 0;
    uint32_t cluster_size = fs_private.header.sectors_per_cluster * fs_private.header.bytes_per_sector;
    
    while (total_read < total_to_read) {
        uint32_t offset_in_file = desc->pos + total_read;
        uint32_t current_cluster = fat16_get_cluster_for_offset(desc->item.low_16_bits_first_cluster, offset_in_file);
        
        if (current_cluster >= 0xFFF0) {
            break; // Something went wrong or end of file
        }
        
        uint32_t offset_in_cluster = offset_in_file % cluster_size;
        uint32_t abs_sector = fat16_cluster_to_sector(current_cluster);
        uint32_t abs_pos = (abs_sector * fs_private.header.bytes_per_sector) + offset_in_cluster;
        
        uint32_t to_read_this_cluster = cluster_size - offset_in_cluster;
        if (to_read_this_cluster > (total_to_read - total_read)) {
            to_read_this_cluster = total_to_read - total_read;
        }
        
        diskstream_seek(fs_private.stream, abs_pos);
        if (diskstream_read(fs_private.stream, out + total_read, to_read_this_cluster) != 0) {
            break;
        }
        
        total_read += to_read_this_cluster;
    }
    
    desc->pos += total_read;
    return total_read / size;
}

int fat16_seek(struct fat_file_descriptor* desc, uint32_t offset, int whence) {
    // Basic seek implementation
    desc->pos = offset;
    return 0;
}
