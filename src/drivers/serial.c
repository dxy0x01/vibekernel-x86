#include "serial.h"

void serial_init() {
   port_byte_out(COM1 + 1, 0x00);    // Disable all interrupts
   port_byte_out(COM1 + 3, 0x80);    // Enable DLAB (set baud rate divisor)
   port_byte_out(COM1 + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
   port_byte_out(COM1 + 1, 0x00);    //                  (hi byte)
   port_byte_out(COM1 + 3, 0x03);    // 8 bits, no parity, one stop bit
   port_byte_out(COM1 + 4, 0x0B);    // IRQs enabled, RTS/DSR set
}

int is_transmit_empty() {
   return port_byte_in(COM1 + 5) & 0x20;
}

void serial_putc(char c) {
   while (is_transmit_empty() == 0);
   port_byte_out(COM1, c);
}

void serial_print(const char* str) {
   for (int i = 0; str[i] != '\0'; i++) {
      serial_putc(str[i]);
   }
}
