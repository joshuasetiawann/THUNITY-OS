# 04 — Driver Model

**Status:** Partially implemented. Four drivers are coded and build-verified;
the unified registration framework is designed; all other drivers are planned.

This is the canonical milestone summary. A deeper forward-looking specification
lives in [`design/DRIVER_MODEL.md`](design/DRIVER_MODEL.md).

## Implemented drivers (0.2.0)

| Driver | File | IRQ / port | Provides |
|--------|------|------------|----------|
| VGA text console | `kernel/drivers/vga.c` | MMIO `0xB8000`, CRTC `0x3D4/5` | `vga_init/clear/putc/write/set_color`, scrolling, hardware cursor |
| Serial COM1 | `kernel/drivers/serial.c` | port `0x3F8` | `serial_init` (loopback self-test), `serial_write_char/write` |
| PS/2 keyboard | `kernel/drivers/keyboard.c` | IRQ1, port `0x60` | `keyboard_init`, `keyboard_getchar`, scancode→ASCII, shift, ring buffer |
| PIT timer | `kernel/arch/x86/pit.c` | IRQ0, ports `0x40/0x43` | `pit_init`, `pit_ticks/seconds/frequency` |

Each driver today is initialized directly from `kernel_main()` in a fixed order
and exposes a small, explicit header API. None of them fails silently: serial
runs a loopback self-test, the keyboard drops on overflow rather than corrupting
state, and unexpected CPU faults route to the panic/exception path.

## Driver principles

- **Clear interface.** Every driver is a `.c`/`.h` pair with an explicit API.
- **Explicit init.** Drivers are brought up in a known order with logged status.
- **No silent failure.** Errors are reported (serial self-test, panic on faults).
- **Debuggable.** Drivers can log through `kprintf` (VGA + serial).
- **Freestanding.** No host libc, no Linux ioctl model.

## Designed — unified registration framework

A future `device`/`driver` registry will let drivers register by name and type
(character, block, input, timer, framebuffer, net) and be discovered rather than
hard-coded into `kernel_main`. Sketch:

```c
typedef enum { DEV_CHAR, DEV_BLOCK, DEV_INPUT, DEV_TIMER, DEV_FB, DEV_NET } dev_type_t;

typedef struct device {
    const char  *name;
    dev_type_t   type;
    int        (*init)(struct device *);
    /* read/write/ioctl-style ops added per class as needed */
} device_t;

int  driver_register(device_t *dev);   /* designed, not yet implemented */
```

This is **designed only**; 0.2.0 still initializes drivers explicitly.

## Planned drivers

| Driver | Milestone (target) |
|--------|--------------------|
| PS/2 mouse | 0.6 (graphics) |
| Framebuffer (VBE/GOP) | 0.6 |
| RTC / CMOS clock | 0.3–0.4 |
| ATA/IDE block storage | 0.4 |
| AHCI / NVMe storage | later |
| PCI enumeration | later |
| Network (e.g. e1000/virtio) | later |
| Audio | later |
| USB stack | later |

See [`08_ROADMAP.md`](08_ROADMAP.md) for sequencing and
[`design/DRIVER_MODEL.md`](design/DRIVER_MODEL.md) for the full model.

**Status summary:** VGA, serial, keyboard, PIT — Implemented. Registration
framework — Designed. Everything else — Planned.
