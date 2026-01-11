#include "fat16.h"
#include "file.h"
#include "path_parser.h"
#include "../drivers/disk_stream.h"
#include "../memory/heap/kheap.h"
#include "../string/string.h"
#include "../drivers/screen.h"
#include <stddef.h>

void* fat16_open(struct disk* disk, struct path_part* path, FILE_MODE mode);

struct filesystem* fat16_init_vfs() {
    static struct filesystem fat16_fs = {
        .name = "FAT16",
        .resolve = fat16_resolve,
        .open = fat16_open,
        .read = (FS_READ_FUNCTION)fat16_read,
        .seek = (FS_SEEK_FUNCTION)fat16_seek,
        .tell = (FS_TELL_FUNCTION)fat16_tell,
        .close = (FS_CLOSE_FUNCTION)fat16_close,
        .stat = (FS_STAT_FUNCTION)fat16_stat,
        .list = (FS_LIST_FUNCTION)fat16_list
    };
    return &fat16_fs;
}

int fat16_get_total_items_for_directory(struct fat_directory_item* item) {
    // This is a simplified version, as FAT16 root directory has a fixed size
    // but subdirectories are cluster chains.
    return 0; // Not yet fully implemented for subdirs
}

static void fat16_to_proper_string(char** out, const char* in, int limit) {
    for (int i = 0; i < limit; i++) {
        if (in[i] == 0x00 || in[i] == 0x20) break;
        **out = in[i];
        *out += 1;
    }
}

void fat16_get_full_relative_filename(struct fat_directory_item* item, char* out, int max_len) {
    memset(out, 0x00, max_len);
    char* temp_out = out;
    fat16_to_proper_string(&temp_out, (const char*)item->filename, 8);
    if (item->ext[0] != 0x00 && item->ext[0] != 0x20) {
        *temp_out++ = '.';
        fat16_to_proper_string(&temp_out, (const char*)item->ext, 3);
    }
}

static uint32_t fat16_get_fat_entry(struct disk* disk, uint32_t cluster) {
    struct fat_private* private = disk->fs_private;
    uint32_t fat_table_sector = private->bpb.reserved_sectors;
    uint32_t fat_table_offset = cluster * FAT16_ENTRY_SIZE;
    uint32_t fat_table_abs_pos = (fat_table_sector * private->bpb.bytes_per_sector) + fat_table_offset;
    
    diskstream_seek(private->stream, fat_table_abs_pos);
    
    uint16_t entry = 0;
    if (diskstream_read(private->stream, &entry, sizeof(entry)) != 0) {
        return 0xFFFF; // Error
    }
    
    return entry;
}

static uint32_t fat16_get_cluster_for_offset(struct disk* disk, uint32_t start_cluster, uint32_t offset) {
    struct fat_private* private = disk->fs_private;
    uint32_t cluster_size = private->bpb.sectors_per_cluster * private->bpb.bytes_per_sector;
    uint32_t clusters_to_skip = offset / cluster_size;
    uint32_t current_cluster = start_cluster;
    
    for (uint32_t i = 0; i < clusters_to_skip; i++) {
        current_cluster = fat16_get_fat_entry(disk, current_cluster);
        if (current_cluster >= FAT16_CLUSTER_LAST_MIN) {
            return 0xFFFF; // End of chain
        }
    }
    
    return current_cluster;
}

// Logic to convert cluster to absolute sector
uint32_t fat16_cluster_to_sector(struct disk* disk, uint32_t cluster) {
    struct fat_private* private = disk->fs_private;
    uint32_t root_dir_sectors = ((private->bpb.root_dir_entries * 32) + (private->bpb.bytes_per_sector - 1)) / private->bpb.bytes_per_sector;
    uint32_t first_data_sector = private->bpb.reserved_sectors + (private->bpb.fat_copies * private->bpb.fat_sectors) + root_dir_sectors;
    return first_data_sector + (cluster - 2) * private->bpb.sectors_per_cluster;
}

uint32_t fat16_get_root_directory_sector(struct disk* disk) {
    struct fat_private* private = disk->fs_private;
    return private->bpb.reserved_sectors + (private->bpb.fat_copies * private->bpb.fat_sectors);
}

uint32_t fat16_get_bytes_per_sector(struct disk* disk) {
    struct fat_private* private = disk->fs_private;
    return private->bpb.bytes_per_sector;
}

static int fat16_get_directory_entry(struct disk* disk, uint32_t cluster, const char* name, struct fat_directory_item* out_item) {
    struct fat_private* private = disk->fs_private;
    uint32_t bytes_per_sector = private->bpb.bytes_per_sector;
    uint32_t root_dir_entries = private->bpb.root_dir_entries;
    uint32_t items_processed = 0;
    
    if (cluster == 0) { // Root Directory
        uint32_t root_dir_sector = fat16_get_root_directory_sector(disk);
        diskstream_seek(private->stream, root_dir_sector * bytes_per_sector);
        
        for (int i = 0; i < root_dir_entries; i++) {
            struct fat_directory_item item;
            if (diskstream_read(private->stream, &item, sizeof(item)) != 0) return -1;
            if (item.filename[0] == 0x00) break;
            if (item.filename[0] == 0xE5) continue;
            
            char out_name[13];
            fat16_get_full_relative_filename(&item, out_name, sizeof(out_name));
            if (strncasecmp(out_name, name, sizeof(out_name)) == 0) {
                *out_item = item;
                return 0;
            }
        }
    } else { // Subdirectory (Cluster Chain)
        uint32_t cluster_size = private->bpb.sectors_per_cluster * bytes_per_sector;
        uint32_t current_cluster = cluster;
        
        while (current_cluster < FAT16_CLUSTER_RESERVED_MIN) {
            uint32_t abs_sector = fat16_cluster_to_sector(disk, current_cluster);
            diskstream_seek(private->stream, abs_sector * bytes_per_sector);
            
            for (uint32_t i = 0; i < cluster_size / sizeof(struct fat_directory_item); i++) {
                struct fat_directory_item item;
                if (diskstream_read(private->stream, &item, sizeof(item)) != 0) return -2;
                if (item.filename[0] == 0x00) return -3;
                if (item.filename[0] == 0xE5) continue;
                
                char out_name[13];
                fat16_get_full_relative_filename(&item, out_name, sizeof(out_name));
                if (strncasecmp(out_name, name, sizeof(out_name)) == 0) {
                    *out_item = item;
                    return 0;
                }
            }
            current_cluster = fat16_get_fat_entry(disk, current_cluster);
        }
    }
    
    return -4; // Not found
}

int fat16_list(struct disk* disk, struct path_part* path) {
    struct fat_private* private = disk->fs_private;
    uint32_t bytes_per_sector = private->bpb.bytes_per_sector;
    uint32_t root_dir_entries = private->bpb.root_dir_entries;
    uint32_t current_cluster = 0;

    struct path_part* current_part = path;
    while (current_part) {
        struct fat_directory_item item;
        if (fat16_get_directory_entry(disk, current_cluster, current_part->part, &item) != 0) {
            return -1;
        }
        if (!(item.attribute & FAT_FILE_SUBDIRECTORY)) return -1;
        current_cluster = item.low_16_bits_first_cluster;
        current_part = current_part->next;
    }

    if (current_cluster == 0) {
         uint32_t root_dir_sector = fat16_get_root_directory_sector(disk);
         diskstream_seek(private->stream, root_dir_sector * bytes_per_sector);
         for (int i = 0; i < root_dir_entries; i++) {
             struct fat_directory_item item;
             if (diskstream_read(private->stream, &item, sizeof(item)) != 0) break;
             if (item.filename[0] == 0x00) break;
             if (item.filename[0] == 0xE5) continue;
             
             char out_name[13];
             fat16_get_full_relative_filename(&item, out_name, sizeof(out_name));
             print_string(out_name);
             if (item.attribute & FAT_FILE_SUBDIRECTORY) print_string("/");
             print_string("\n");
         }
    } else {
        uint32_t cluster_size = private->bpb.sectors_per_cluster * bytes_per_sector;
        while (current_cluster < FAT16_CLUSTER_RESERVED_MIN) {
            uint32_t abs_sector = fat16_cluster_to_sector(disk, current_cluster);
            diskstream_seek(private->stream, abs_sector * bytes_per_sector);
            for (uint32_t i = 0; i < cluster_size / sizeof(struct fat_directory_item); i++) {
                struct fat_directory_item item;
                if (diskstream_read(private->stream, &item, sizeof(item)) != 0) break;
                if (item.filename[0] == 0x00) break;
                if (item.filename[0] == 0xE5) continue;
                
                char out_name[13];
                fat16_get_full_relative_filename(&item, out_name, sizeof(out_name));
                print_string(out_name);
                if (item.attribute & FAT_FILE_SUBDIRECTORY) print_string("/");
                print_string("\n");
            }
            current_cluster = fat16_get_fat_entry(disk, current_cluster);
        }
    }
    return 0;
}

// legacy fat16_open removed

int fat16_close(void* private) {
    kfree(private);
    return 0;
}

int fat16_read(struct disk* disk, void* private_data, uint32_t size, uint32_t nmemb, char* out) {
    struct fat_file_descriptor* desc = (struct fat_file_descriptor*)private_data;
    struct fat_private* private = disk->fs_private;
    uint32_t total_to_read = size * nmemb;
    if (desc->pos + total_to_read > desc->item.filesize) {
        total_to_read = desc->item.filesize - desc->pos;
    }
    
    if (total_to_read == 0) return 0;

    uint32_t total_read = 0;
    uint32_t cluster_size = private->bpb.sectors_per_cluster * private->bpb.bytes_per_sector;
    
    // Efficient cluster lookup: Use cache if possible
    uint32_t current_cluster;
    uint32_t start_cluster;
    uint32_t start_pos;

    if (desc->pos >= desc->last_cluster_pos) {
        start_cluster = desc->last_cluster;
        start_pos = desc->last_cluster_pos;
    } else {
        start_cluster = desc->item.low_16_bits_first_cluster;
        start_pos = 0;
    }

    current_cluster = fat16_get_cluster_for_offset(disk, start_cluster, desc->pos - start_pos);
    if (current_cluster >= FAT16_CLUSTER_RESERVED_MIN) return 0;

    // Update cache
    desc->last_cluster = current_cluster;
    desc->last_cluster_pos = (desc->pos / cluster_size) * cluster_size;

    while (total_read < total_to_read) {
        uint32_t offset_in_file = desc->pos + total_read;
        uint32_t offset_in_cluster = offset_in_file % cluster_size;
        
        // Check if we need to advance to the next cluster
        if (total_read > 0 && offset_in_cluster == 0) {
            current_cluster = fat16_get_fat_entry(disk, current_cluster);
            // Update cache as we go
            desc->last_cluster = current_cluster;
            desc->last_cluster_pos = offset_in_file;
        }

        if (current_cluster >= FAT16_CLUSTER_RESERVED_MIN) {
            break; // End of chain or error
        }
        
        uint32_t abs_sector = fat16_cluster_to_sector(disk, current_cluster);
        uint32_t abs_pos = (abs_sector * private->bpb.bytes_per_sector) + offset_in_cluster;
        
        uint32_t to_read_this_cluster = cluster_size - offset_in_cluster;
        if (to_read_this_cluster > (total_to_read - total_read)) {
            to_read_this_cluster = total_to_read - total_read;
        }
        
        diskstream_seek(private->stream, abs_pos);
        if (diskstream_read(private->stream, out + total_read, to_read_this_cluster) != 0) {
            break;
        }
        
        total_read += to_read_this_cluster;
    }
    
    desc->pos += total_read;
    return total_read / size;
}

int fat16_seek(void* private, int offset, FILE_SEEK_MODE whence) {
    struct fat_file_descriptor* desc = (struct fat_file_descriptor*)private;
    uint32_t new_pos = desc->pos;

    switch (whence) {
        case FILE_SEEK_SET:
            new_pos = offset;
            break;
        case FILE_SEEK_CUR:
            new_pos += offset;
            break;
        case FILE_SEEK_END:
            new_pos = desc->item.filesize + offset;
            break;
    }

    if (new_pos > desc->item.filesize) {
        return -1;
    }

    desc->pos = new_pos;
    return 0;
}

int fat16_tell(void* private) {
    struct fat_file_descriptor* desc = (struct fat_file_descriptor*)private;
    return desc->pos;
}

int fat16_stat(struct disk* disk, void* private, struct file_stat* stat) {
    struct fat_file_descriptor* desc = (struct fat_file_descriptor*)private;
    stat->filesize = desc->item.filesize;
    stat->flags = 0;
    if (desc->item.attribute & FAT_FILE_READ_ONLY) stat->flags |= FILE_STAT_READ_ONLY;
    if (desc->item.attribute & FAT_FILE_HIDDEN) stat->flags |= FILE_STAT_HIDDEN;
    if (desc->item.attribute & FAT_FILE_SYSTEM) stat->flags |= FILE_STAT_SYSTEM;
    if (desc->item.attribute & FAT_FILE_VOLUME_LABEL) stat->flags |= FILE_STAT_VOLUME_LABEL;
    if (desc->item.attribute & FAT_FILE_SUBDIRECTORY) stat->flags |= FILE_STAT_DIRECTORY;
    if (desc->item.attribute & FAT_FILE_ARCHIVE) stat->flags |= FILE_STAT_ARCHIVE;
    return 0;
}
void* fat16_open(struct disk* disk, struct path_part* path, FILE_MODE mode) {
    if (mode != FILE_MODE_READ) return NULL;
    
    struct fat_directory_item item;
    uint32_t current_cluster = 0; // Start at root
    struct path_part* current_part = path;
    
    while (current_part) {
        if (fat16_get_directory_entry(disk, current_cluster, current_part->part, &item) != 0) {
            return NULL;
        }
        
        if (current_part->next) {
            // Must be a directory
            if (!(item.attribute & FAT_FILE_SUBDIRECTORY)) return NULL;
            current_cluster = item.low_16_bits_first_cluster;
        }
        current_part = current_part->next;
    }
    
    // Last part found. Ensure it's not a directory if we are opening a file.
    if (item.attribute & FAT_FILE_SUBDIRECTORY) return NULL;
    
    // Create descriptor
    struct fat_file_descriptor* desc = kmalloc(sizeof(struct fat_file_descriptor));
    desc->item = item;
    desc->pos = 0;
    desc->last_cluster = item.low_16_bits_first_cluster;
    desc->last_cluster_pos = 0;
    return desc;
}

int fat16_resolve(struct disk* disk) {
    struct disk_stream* stream = diskstream_new(disk->id);
    if (!stream) return -1;
    
    struct fat_boot_sector bpb;
    if (diskstream_read(stream, &bpb, sizeof(bpb)) != 0) {
        diskstream_close(stream);
        return -2;
    }
    
    // Check boot signature 0xAA55 at offset 510
    if (bpb.boot_signature != 0xAA55) {
        diskstream_close(stream);
        return -3;
    }

    // Check Extended BPB signature (0x28 or 0x29)
    if (bpb.signature != 0x29 && bpb.signature != 0x28) {
        diskstream_close(stream);
        return -4;
    }

    // Basic consistency checks
    if (bpb.bytes_per_sector != 512 || bpb.sectors_per_cluster == 0) {
        diskstream_close(stream);
        return -5;
    }
    
    struct fat_private* private = kmalloc(sizeof(struct fat_private));
    private->stream = stream;
    private->bpb = bpb;
    disk->fs_private = private;
    
    return 0;
}
