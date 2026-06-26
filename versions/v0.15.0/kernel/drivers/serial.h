/* THUOS — COM1 serial port driver (host/QEMU debug channel). */
#ifndef THUOS_SERIAL_H
#define THUOS_SERIAL_H

#include "types.h"

#define SERIAL_COM1 0x3F8

int  serial_init(void);
void serial_write_char(char c);
void serial_write(const char *s);

#endif /* THUOS_SERIAL_H */
