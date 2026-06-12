# 42 — Desktop & App Model

## Where the desktop is today (implemented, boot-verified)

THUOS already has a **real graphical desktop**, boot-verified in QEMU and via a
GRUB ISO, with screenshots in `preview/screenshots/`:

- 1024×768×32 truecolor framebuffer (bootloader-provided on real HW; Bochs-VBE
  fallback in QEMU).
- Top bar + clock, a dock of pictogram app icons, a mouse cursor (PS/2 **and**
  USB-HID), and a single focused app window at a time.
- Apps: Terminal, Files, Notes, Calculator, Paint, Settings — each drawn by the
  kernel’s `gui/` layer and driven by keyboard + mouse.

What it is **not** yet: there is **no movable/overlapping window manager**, no
per-app processes (apps are kernel-drawn views, not loaded programs), and no
compositor. The desktop is real; a *windowing system* is the next step.

## App model — now vs. next

### Now: built-in views
Each “app” is a function set in `kernel/gui/apps.c` that the desktop switches
between. There is one active app; the dock selects it. This is simple, fast, and
honest — but apps cannot be installed, isolated, or loaded from files.

### Next: a loadable app runtime (design-only)
To become a normal app platform, THUOS needs, in order:

1. **Storage + filesystem** so apps and their data live on disk.
2. **ELF loader + process isolation** so an app is a separate program in its own
   address space (ring 3 + paging already exist as the substrate).
3. **A program ABI / minimal libc** (syscalls already exist via `int 0x80`) so
   apps call the kernel for I/O, windows, and AI.
4. **A window manager** providing movable/resizable/overlapping windows, focus,
   and a compositor.
5. **A permission model** (extending the AI policy idea) so apps declare and are
   granted capabilities (files, network, AI, devices).

### AI-native angle
The AI permission/audit model from milestone 0.20 is designed to extend to apps:
an app requesting an AI action goes through the **same policy + audit path** as
the shell’s `ai` commands. So when the app runtime and networking land, AI access
is already governed and accountable by design — not bolted on.

## Window manager sketch (design-only)

- A `window_t` with position/size/z-order/title, owned by a process.
- Input routed to the focused window; a compositor blits windows + cursor.
- Decorations (title bar, close/min) and a taskbar/dock reflecting open windows.
- All of this waits on the process/app runtime above; today’s single-view desktop
  is the placeholder it will replace.

## Honest status

| Item | Status |
|------|--------|
| Truecolor desktop + dock + cursor + 6 apps | implemented (boot-verified) |
| Single focused app at a time | implemented |
| Movable/overlapping windows, compositor | design-only |
| Apps as isolated processes / loaded from files | design-only |
| App permission + AI-action governance | design-only (AI core host-tested) |
