#include "ps2.h"
#include "ports.h"

static void ps2_wait_write() {
    while (port_byte_in(PS2_STATUS_PORT) & PS2_STATUS_INPUT_FULL);
}

static void ps2_wait_read() {
    while (!(port_byte_in(PS2_STATUS_PORT) & PS2_STATUS_OUTPUT_FULL));
}

void ps2_init() {
    // Basic PS/2 controller initialization could go here
    // For now, we assume the BIOS has set it up or it's in a usable state
}

uint8_t ps2_read_data() {
    ps2_wait_read();
    return port_byte_in(PS2_DATA_PORT);
}

void ps2_write_data(uint8_t data) {
    ps2_wait_write();
    port_byte_out(PS2_DATA_PORT, data);
}

void ps2_write_command(uint8_t command) {
    ps2_wait_write();
    port_byte_out(PS2_COMMAND_PORT, command);
}
