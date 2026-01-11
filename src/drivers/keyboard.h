#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "../cpu/types.h"
#include "../cpu/isr.h"

#define KEY_DELETE 0x7F
#define LSHIFT 0x2A
#define RSHIFT 0x36

void keyboard_init();
void keyboard_handler(registers_t *regs);
char keyboard_getc();

#endif
