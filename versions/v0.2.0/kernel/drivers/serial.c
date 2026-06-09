/* THUOS — COM1 serial port driver.
 * Used as an honest debug channel: when THUOS is booted under QEMU with
 * `-serial stdio`, boot milestones appear on the host terminal. */
#include "serial.h"
#include "io.h"

#define COM1 SERIAL_COM1

static int serial_transmit_empty(void) {
    return inb(COM1 + 5) & 0x20;
}

int serial_init(void) {
    outb(COM1 + 1, 0x00);  /* disable interrupts            */
    outb(COM1 + 3, 0x80);  /* enable DLAB (set baud divisor)*/
    outb(COM1 + 0, 0x03);  /* divisor low: 38400 baud       */
    outb(COM1 + 1, 0x00);  /* divisor high                  */
    outb(COM1 + 3, 0x03);  /* 8 bits, no parity, one stop   */
    outb(COM1 + 2, 0xC7);  /* enable FIFO, clear, 14-byte   */
    outb(COM1 + 4, 0x0B);  /* IRQs enabled, RTS/DSR set     */

    /* Loopback self-test: write a byte and confirm we read it back. */
    outb(COM1 + 4, 0x1E);  /* loopback mode */
    outb(COM1 + 0, 0xAE);
    if (inb(COM1 + 0) != 0xAE) {
        return -1;         /* serial port not present / faulty */
    }
    outb(COM1 + 4, 0x0F);  /* normal operation */
    return 0;
}

void serial_write_char(char c) {
    if (c == '\n') serial_write_char('\r');
    while (!serial_transmit_empty()) { }
    outb(COM1, (uint8_t)c);
}

void serial_write(const char *s) {
    for (size_t i = 0; s[i]; i++) serial_write_char(s[i]);
}
