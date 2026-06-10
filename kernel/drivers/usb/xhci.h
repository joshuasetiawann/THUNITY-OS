#ifndef THUOS_XHCI_H
#define THUOS_XHCI_H

#include "types.h"

/* Bring up the xHCI USB host controller (the controller class real Intel/AMD
 * machines expose for USB 1/2/3). Stage 1: discover via PCI, take ownership
 * from firmware, reset, set up the command + event rings, start it, prove the
 * rings with a No-Op command, and report port connection status. Safe to call
 * when no controller is present (it just reports none). */
void xhci_init(void);

/* 1 once a controller has been found and started. */
int  xhci_present(void);

/* Drain the event ring: deliver HID keyboard/mouse reports into the keyboard and
 * mouse input buffers, and re-arm the interrupt transfers. Called from the
 * desktop/shell input loops (we poll; USB interrupts are not wired to the IDT). */
void usb_poll(void);

#endif /* THUOS_XHCI_H */
