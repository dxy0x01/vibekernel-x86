#include "path_parser.h"
#include "../string/string.h"
#include "../memory/heap/kheap.h"
#include "../memory/paging/paging.h" // For any alignment/paging needs if any, though kheap is enough
#include "../drivers/serial.h"

static int pathparser_get_drive_no(const char** path) {
    if (!isdigit((*path)[0]) || (*path)[1] != ':' || (*path)[2] != '/') {
        return -1;
    }

    int drive_no = (*path)[0] - '0';
    *path += 3; // Skip "0:/"
    return drive_no;
}

static struct path_root* pathparser_create_root(int drive_no) {
    struct path_root* root = kmalloc(sizeof(struct path_root));
    root->drive_no = drive_no;
    root->first = NULL;
    return root;
}

static const char* pathparser_get_next_part(const char** path) {
    char* result = kmalloc(256); // Max path part length
    int i = 0;
    while (**path && **path != '/') {
        result[i++] = **path;
        (*path)++;
    }

    if (**path == '/') {
        (*path)++; // Skip '/'
    }

    if (i == 0) {
        kfree(result);
        return NULL;
    }

    result[i] = '\0';
    return result;
}

struct path_root* pathparser_parse(const char* path, const char* current_directory_path) {
    const char* tmp_path = path;
    int drive_no = pathparser_get_drive_no(&tmp_path);
    if (drive_no < 0) return NULL;

    struct path_root* root = pathparser_create_root(drive_no);
    struct path_part* first_part = NULL;
    struct path_part* last_part = NULL;

    while (1) {
        const char* part_str = pathparser_get_next_part(&tmp_path);
        if (!part_str) break;

        struct path_part* part = kmalloc(sizeof(struct path_part));
        part->part = part_str;
        part->next = NULL;

        if (!first_part) {
            first_part = part;
            root->first = part;
        } else {
            last_part->next = part;
        }
        last_part = part;
    }

    return root;
}

void pathparser_free(struct path_root* root) {
    struct path_part* part = root->first;
    while (part) {
        struct path_part* next = part->next;
        kfree((void*)part->part);
        kfree(part);
        part = next;
    }
    kfree(root);
}
