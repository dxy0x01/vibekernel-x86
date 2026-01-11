#ifndef SERIAL_H
#define SERIAL_H

#include "ports.h"

#define COM1 0x3F8

void serial_init();
void serial_putc(char c);
void serial_print(char* str);

#endif
