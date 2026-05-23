# THUOS Driver Model

**Status: Partially implemented.** Four drivers (VGA text console, COM1 serial,
PS/2 keyboard, PIT timer) are **Implemented** and build-verified in THU Kernel
0.2.0 "Boot Seed". The unified *driver registration framework* described in
sections 3–4 is **Designed**; today each driver is initialized directly from
`kernel_main()`. Every other driver listed here is **Planned**.

> Build-verified means the kernel compiles and links into a valid Multiboot ELF
> with these drivers present. It has **not** been booted under QEMU in this
> environment (QEMU is unavailable here), so "Implemented" refers to code that
> builds and is wired in, not to an observed boot.

---

## 1. Design principles

THUOS drivers follow five rules, inherited from the kernel's existing style:

1. **Clear interface.** Each driver exposes a small, explicit set of functions
   in its header (`kernel/drivers/*.h`, `kernel/arch/x86/*.h`). No globals leak.
2. **No silent failure.** Init that can fail returns a status (e.g.
   `serial_init()` returns `int`, `0` on success). Callers report the result;
   the boot log prints `[ OK ]` or `[WARN]` per device.
3. **Debug logging.** Drivers can log to COM1 via `serial_write()` so failures
   are visible even before any console is usable.
4. **Robust against bad input.** Device input is bounded — the keyboard uses a
   fixed ring buffer that *drops* on overflow rather than corrupting memory.
5. **Freestanding C.** No host libc; fixed-width types from `kernel/lib/types.h`;
   port I/O through the shared `io.h` helpers (`inb`/`outb`).

## 2. Implemented drivers (build-verified)

These are present and wired into boot today. Initialization order is fixed in
`kernel/core/kernel.c`.

| Driver        | Source                          | IRQ / Port        | Init signature                  | Status      |
|---------------|---------------------------------|-------------------|----------------------------------|-------------|
| VGA text      | `kernel/drivers/vga.c`          | MMIO `0xB8000`    | `void vga_init(void)`           | Implemented |
| Serial COM1   | `kernel/drivers/serial.c`       | Port `0x3F8`      | `int serial_init(void)`         | Implemented |
| PS/2 keyboard | `kernel/drivers/keyboard.c`     | IRQ1, port `0x60` | `void keyboard_init(void)`      | Implemented |
| PIT timer     | `kernel/arch/x86/pit.c`         | IRQ0              | `void pit_init(uint32_t hz)`    | Implemented |

Supporting low-level facilities that drivers build on — GDT, IDT, CPU exception
handlers (ISRs 0–31), and the PIC remap + IRQ routing — are also implemented in
`kernel/arch/x86/`. They are not "device drivers" but they are the substrate the
interrupt-driven drivers depend on.

### Representative interfaces (as built)

```c
/* VGA 80x25 text console — kernel/drivers/vga.h */
void    vga_init(void);
void    vga_clear(void);
void    vga_set_color(uint8_t fg, uint8_t bg);
void    vga_putc(char c);
void    vga_write(const char *s);

/* COM1 serial debug channel — kernel/drivers/serial.h */
int  serial_init(void);            /* returns 0 on success                  */
void serial_write_char(char c);
void serial_write(const char *s);

/* PIT timer (8253/8254) on IRQ0 — kernel/arch/x86/pit.h */
void     pit_init(uint32_t frequency_hz);   /* kernel boots at 100 Hz       */
uint32_t pit_ticks(void);
uint32_t pit_seconds(void);

/* PS/2 keyboard — kernel/drivers/keyboard.h */
void keyboard_init(void);          /* registers an IRQ1 handler             */
char keyboard_getchar(void);       /* blocks (sti; hlt) until a key arrives */
```

### How interrupt drivers register today

Interrupt-driven drivers attach a callback through the existing IRQ layer
(`kernel/arch/x86/irq.h`). For example, the keyboard does:

```c
/* from keyboard_init() — already in the tree */
irq_register_handler(1, keyboard_callback);   /* IRQ1 -> our callback        */

static void keyboard_callback(registers_t *r) {
    uint8_t scancode = inb(0x60);
    /* ... translate, push to ring buffer, drop on overflow ... */
}
```

This `irq_register_handler(line, fn)` hook is the seed of the future driver
model: it is the one place a driver binds itself to a hardware event.

## 3. Planned unified driver framework (Designed)

Today drivers are initialized by explicit calls in `kernel_main()`. The planned
framework introduces a uniform descriptor so drivers register themselves and the
kernel can enumerate them (and expose them via `devfs` / `/proc/drivers`).

```c
/* Designed — not yet in the tree. */
struct driver {
    const char *name;                 /* "vga", "serial", "keyboard", ...   */
    const char *kind;                 /* "console", "tty", "input", "timer" */
    int  (*probe)(void);              /* detect hardware; 0 if present       */
    int  (*init)(void);               /* bring up; 0 on success              */
    void (*shutdown)(void);           /* optional clean stop                 */
};

int  driver_register(const struct driver *drv);   /* called at boot          */
const struct driver *driver_find(const char *name);
void driver_for_each(void (*fn)(const struct driver *));  /* enumeration      */
```

Boot becomes: register all known drivers → for each, `probe()` then `init()` →
log `[ OK ]` / `[WARN]` per device (the convention already used in
`kernel_main()`). A failed `probe` is recorded, not hidden.

## 4. Driver lifecycle (Designed)

```text
  register ──► probe ──► init ──► running ──► shutdown
                 │         │
              absent     failed   ──►  logged to COM1 + console, marked DOWN
                 └─────────┴──►  device omitted, kernel continues (no panic
                                  unless the device is boot-critical)
```

## 5. Planned drivers

All **Planned**. Ordered roughly by how they unblock the rest of the roadmap.

| Driver            | Purpose                                  | Unblocks                         | Status  |
|-------------------|------------------------------------------|----------------------------------|---------|
| PS/2 mouse        | pointer input (IRQ12)                     | THU Desktop cursor               | Planned |
| Framebuffer       | linear graphics surface (VBE/Multiboot)   | GUI, bitmap font, compositor     | Planned |
| RTC               | wall-clock date/time (CMOS)               | timestamps, audit logs           | Planned |
| ATA/IDE           | PIO block storage                         | persistent FS, THUFS, installer  | Planned |
| PCI               | bus enumeration                           | AHCI, NIC, audio discovery       | Planned |
| AHCI / SATA       | modern block storage                      | faster persistent FS             | Planned |
| Network (NIC)     | Ethernet (e.g. virtio-net / e1000)        | online `thupkg` repo             | Planned |
| Audio             | basic PCM output                          | system sounds                    | Planned |
| USB (UHCI/EHCI)   | HID + mass storage                        | real-hardware input/storage      | Planned |

Storage and network drivers are intentionally gated behind the security model:
until they exist and are reviewed, THUOS does **no writes to real disks** and
makes **no autonomous network connections**. See `06_SECURITY_MODEL.md`.

## 6. Debugging drivers

- **Serial first.** Bring up COM1 early (already done — it is the first thing
  `kernel_main()` initializes) so later drivers can log before the console or
  GUI is trustworthy.
- **Per-device boot line.** Each device prints `[ OK ] <name>` or a `[WARN]`.
- **Fault visibility.** A bad device or a faulting handler surfaces through the
  panic/assert path rather than hanging silently.

---

## Status summary

VGA, serial, keyboard, and PIT drivers are **Implemented** and build-verified;
the unified driver-registration framework is **Designed**; mouse, framebuffer,
RTC, ATA/IDE, PCI, AHCI, network, audio, and USB are **Planned**.
