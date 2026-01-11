#include "keyboard.h"
#include "ps2.h"
#include "ports.h"
#include "screen.h"
#include "../string/string.h"

#define BUFFER_SIZE 256
static char keyboard_buffer[BUFFER_SIZE];
static int buffer_head = 0;
static int buffer_tail = 0;
static bool shift_active = false;

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

static const char scancode_ascii_shift[] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '\"', '~', 0,
    '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*', 0, ' ',
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x7F
};

void keyboard_init() {
    ps2_init();
}

void keyboard_handler(registers_t *regs) {
    uint8_t scancode = ps2_read_data();
    
    if (scancode & 0x80) {
        // Key release
        uint8_t released_scancode = scancode & 0x7F;
        if (released_scancode == LSHIFT || released_scancode == RSHIFT) {
            shift_active = false;
        }
    } else {
        // Key press
        if (scancode == LSHIFT || scancode == RSHIFT) {
            shift_active = true;
            return;
        }

        if (scancode < sizeof(scancode_ascii)) {
            char c = shift_active ? scancode_ascii_shift[scancode] : scancode_ascii[scancode];
            if (c != 0) {
                // Add to buffer
                int next = (buffer_head + 1) % BUFFER_SIZE;
                if (next != buffer_tail) {
                    keyboard_buffer[buffer_head] = c;
                    buffer_head = next;
                }
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

void keyboard_push(char c) {
    if (c == 0) return;
    int next = (buffer_head + 1) % BUFFER_SIZE;
    if (next != buffer_tail) {
        keyboard_buffer[buffer_head] = c;
        buffer_head = next;
    }
}
