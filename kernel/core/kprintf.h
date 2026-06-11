/* THUOS — minimal freestanding formatted output.
 * Every character is mirrored to both the VGA console and COM1 serial. */
#ifndef THUOS_KPRINTF_H
#define THUOS_KPRINTF_H

#include "types.h"

void kputc(char c);
void kputs(const char *s);
void kprintf(const char *fmt, ...);

/* Toggle whether kputc also mirrors to the serial port (on by default). */
void k_serial_mirror(bool enabled);

/* Replay the captured kernel log (boot messages + recent output) — used by `dmesg`. */
void klog_dump(void);

#endif /* THUOS_KPRINTF_H */
