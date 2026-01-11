#include "disk_stream.h"
#include "ata.h"
#include "../memory/heap/kheap.h"
#include <stddef.h>

struct disk_stream* diskstream_new(int disk_id)
{
    struct disk_stream* stream = kmalloc(sizeof(struct disk_stream));
    stream->pos = 0;
    stream->disk_id = disk_id;
    return stream;
}

int diskstream_seek(struct disk_stream* stream, uint32_t pos)
{
    stream->pos = pos;
    return 0;
}

int diskstream_read(struct disk_stream* stream, void* out, uint32_t total)
{
    uint32_t sector = stream->pos / 512;
    uint32_t offset = stream->pos % 512;
    char* out_ptr = (char*)out;
    uint32_t total_to_read = total;

    while(total_to_read > 0)
    {
        uint16_t buffer[256];
        if (ata_read_sector(sector, buffer) != 0)
        {
            return -1;
        }

        uint32_t total_read_this_time = 512 - offset;
        if (total_read_this_time > total_to_read)
        {
            total_read_this_time = total_to_read;
        }

        for (uint32_t i = 0; i < total_read_this_time; i++)
        {
            *out_ptr++ = ((char*)buffer)[offset + i];
        }

        total_to_read -= total_read_this_time;
        sector++;
        offset = 0;
    }

    stream->pos += total;
    return 0;
}

void diskstream_close(struct disk_stream* stream)
{
    kfree(stream);
}
