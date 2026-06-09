# 06 — THU Desktop Preview

**Status:** Implemented as a **host-side concept preview only.** This is a web
page, not the kernel GUI. THU Kernel 0.2.0 runs in VGA text mode and has no
graphics, mouse, windows, or compositor. The preview communicates the intended
look and feel of THU Desktop while the real framebuffer GUI is built in later
milestones. A deeper concept spec is in [`design/GUI_THU_DESKTOP.md`](design/GUI_THU_DESKTOP.md).

## Files

| File | Role |
|------|------|
| `preview/thuos_preview.html` | The interactive concept preview (self-contained, no CDN). |
| `preview/thuos_desktop_preview.html` | Thin page that forwards to the interactive preview. |

Both are pure HTML/CSS/JS with **no external resources**, so they run offline and
when served over a restricted network.

## How to open

```bash
# simplest: open the file directly in a browser
preview/thuos_preview.html

# or serve it locally
make demo          # http://localhost:8080/preview/thuos_preview.html
# or
python3 -m http.server 8080
```

## What the preview contains

1. **Boot screen** — THUOS logo, an animated progress bar, and a boot log that
   mirrors the real kernel's `[ OK ]` bring-up lines (serial, GDT, IDT,
   exceptions, PIC/IRQ, PIT, keyboard). Click to skip.
2. **Login screen** — a calm sign-in for user `thu`; any input (or the Sign in
   button / Enter) continues to the desktop.
3. **Desktop** — deep-space background, a top status bar (logo, milestone,
   Local-first / Secure / Concept Preview chips, a live clock), and a persistent
   footer that states this is a concept preview.
4. **Dock / sidebar** — a vertical app launcher on the left (the "menu sidebar"):
   Terminal, Files, System Monitor, Settings, thupkg, Text Editor, Kernel Status,
   Build Verification, About. Active apps show an indicator.
5. **Draggable windows** — glass-style windows you can drag by the title bar,
   focus by clicking, and close with the red traffic-light dot.
6. **Terminal app** — simulates the `thuos>` shell. Commands include `help`,
   `about`, `clear`, `uptime`, `mem`, `version`, `echo`, `panic`, `reboot`,
   `halt`, plus `status`, `sysinfo`, `ls`, `cat`, `thupkg`, `roadmap`,
   `neofetch`, `banner`. Supports command history (↑/↓). The `panic` command
   shows a simulated red panic overlay, and `reboot`/`halt` replay the kernel's
   shutdown messaging — clearly as a browser simulation.
7. **Kernel Status panel** — states plainly:
   *"Preview UI is web-based; kernel boot not verified here because QEMU is
   unavailable."* and lists which subsystems are implemented vs planned.
8. **Build Verification window** — shows the real verification facts: ELF class
   (32-bit Intel 80386), static/freestanding link, Multiboot header at file
   offset 4096 with a valid checksum, and the required-symbols list.
9. **System Monitor** — honest *implementation-progress* bars (not fake CPU/RAM
   gauges) plus a live demo-session uptime.
10. **Settings** — privacy-first toggles and an accent-theme picker that
    re-themes the preview live.

## Honesty rules for the preview

- The preview is labeled **Concept Preview** in the top bar and the footer.
- Nothing in it implies the kernel can render a GUI. The Kernel Status panel
  says so explicitly.
- Where the preview shows a terminal, the **real** terminal is the in-kernel text
  shell (`kernel/shell/shell.c`); the web terminal is a simulation for design.
- No screenshot of a kernel-rendered desktop exists or is implied.

## Path to a real GUI

The preview is a design target for the future framebuffer implementation:

```
HTML concept (now)
  -> framebuffer mode + drawing primitives (0.6)
  -> bitmap font rendering (0.6)
  -> mouse cursor (0.6)
  -> window rectangles + event loop (0.7)
  -> compositor + window manager (0.7)
  -> in-kernel Terminal / Files / Settings apps (0.7+)
```

Verification of the preview itself (it is a real artifact we can check):
`node --check` on its embedded script passes, and it serves over local HTTP with
`200 OK`. Those checks are recorded in [`../BUILD_VERIFICATION.txt`](../BUILD_VERIFICATION.txt).
