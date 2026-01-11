#include "keyboard.h"
#include "ports.h"
#include "screen.h"
#include "../string/string.h"

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64

#define BUFFER_SIZE 256
static char keyboard_buffer[BUFFER_SIZE];
static int buffer_head = 0;
static int buffer_tail = 0;

static const char scancode_ascii[] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' ',
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x7F
};

void keyboard_init() {
    // Basic initialization if needed
}

void keyboard_handler(registers_t *regs) {
    uint8_t scancode = port_byte_in(KEYBOARD_DATA_PORT);
    
    if (scancode & 0x80) {
        // Key release - ignore for now
    } else {
        if (scancode < sizeof(scancode_ascii)) {
            char c = scancode_ascii[scancode];
            if (c != 0) {
                // Add to buffer
                int next = (buffer_head + 1) % BUFFER_SIZE;
                if (next != buffer_tail) {
                    keyboard_buffer[buffer_head] = c;
                    buffer_head = next;
                }
                
                // For direct shell feedback, we might want to print here or let the shell handle it
            }
        }
    }
}

char keyboard_getc() {
    while (buffer_head == buffer_tail) {
        // Wait for key press (Busy wait for now in this simple kernel)
        __asm__ __volatile__("hlt");
    }
    
    char c = keyboard_buffer[buffer_tail];
    buffer_tail = (buffer_tail + 1) % BUFFER_SIZE;
    return c;
}
