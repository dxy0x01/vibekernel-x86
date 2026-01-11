#ifndef PS2_H
#define PS2_H

#include <stdint.h>

#define PS2_DATA_PORT    0x60
#define PS2_STATUS_PORT  0x64
#define PS2_COMMAND_PORT 0x64

// Status register bits
#define PS2_STATUS_OUTPUT_FULL 0x01
#define PS2_STATUS_INPUT_FULL  0x02

void ps2_init();
uint8_t ps2_read_data();
void ps2_write_data(uint8_t data);
void ps2_write_command(uint8_t command);

#endif
