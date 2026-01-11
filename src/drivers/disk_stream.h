#ifndef DISK_STREAM_H
#define DISK_STREAM_H

#include <stdint.h>
#include <stddef.h>

struct disk_stream {
    uint32_t pos;
    int disk_id;
};

struct disk_stream* diskstream_new(int disk_id);
int diskstream_seek(struct disk_stream* stream, uint32_t pos);
int diskstream_read(struct disk_stream* stream, void* out, uint32_t total);
void diskstream_close(struct disk_stream* stream);

#endif
