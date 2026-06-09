# THU Desktop — Graphical Environment Concept

**Status: Concept / Planned.** THU Desktop is the planned graphical environment
for THUOS. Today it exists **only as a host-side HTML concept preview** intended
to live at [`preview/thuos_preview.html`](../../preview/thuos_preview.html) and open
in an ordinary web browser. It is **not** a kernel framebuffer GUI: THU Kernel
0.2.0 "Boot Seed" runs in **VGA 80x25 text mode** and has no graphics, no mouse,
no windows, and no compositor.

> The HTML preview is a *design artifact* — it communicates the intended look and
> feel so the visual direction can be reviewed before any pixel is drawn by the
> kernel. Nothing in the preview implies the kernel can render it. Where the
> preview shows a terminal, the *real* terminal today is the in-kernel text shell
> with the `thuos>` prompt (see `kernel/shell/shell.c`).

---

## 1. What exists today vs. what is planned

| Item                                  | Status            |
|---------------------------------------|-------------------|
| HTML concept preview (browser)        | Concept (artifact)|
| In-kernel text shell (`thuos>`)       | Implemented       |
| Framebuffer graphics in the kernel    | Planned           |
| Bitmap font renderer                  | Planned           |
| Mouse cursor                          | Planned           |
| Window manager / window rects         | Planned           |
| GUI event loop                        | Planned           |
| Compositor                            | Planned           |
| Any GUI app running *in the kernel*   | Planned           |

## 2. Concept overview

THU Desktop aims to feel calm, modern, and distinctly THUOS — not a clone of
Windows, macOS, or any Linux desktop. The intended composition:

```text
 +-----------------------------------------------------------------------+
 |  THUOS            top bar: clock · status · power            ◑  ⚙  ⏻   |
 +------+----------------------------------------------------------------+
 |      |                                                                |
 |  D   |        +--------------------------+                            |
 |  O   |        |  Terminal   thuos>       |     desktop workspace      |
 |  C   |        |  > status                |                            |
 |  K   |        +--------------------------+                            |
 |      |                                                                |
 | apps |   +-----------------+     +--------------------+               |
 |      |   | Files           |     | System Monitor     |              |
 |  ▣   |   +-----------------+     +--------------------+               |
 +------+----------------------------------------------------------------+
```

- **Boot → login → desktop.** A THUOS boot screen, a simple login, then the
  workspace. (All three are mocked in the HTML preview only.)
- **Top bar.** Clock, system status, and power controls.
- **Dock / sidebar.** Launches and switches between core apps.
- **Window manager.** Movable, focusable windows on the workspace.

## 3. Core apps (planned)

These mirror the app set in `09_APP_PLATFORM.md`. In the preview they are static
mockups; as real apps they depend on userspace (milestone 0.5) and a framebuffer.

| App            | Purpose                                  | Notes                                   |
|----------------|------------------------------------------|-----------------------------------------|
| Terminal       | Command line with `thuos>` prompt        | Mirrors the existing in-kernel shell    |
| Files          | Browse the VFS tree                      | Needs VFS (`04_FILESYSTEM.md`)          |
| Settings       | System configuration                     | Theme, time, packages                   |
| System Monitor | Uptime, memory, drivers                  | Mirrors `sysinfo` / `uptime` / `mem`    |
| Text Editor    | Edit plain-text files                    | Needs VFS write path                    |
| thupkg         | Browse/install packages                  | UI over the `thupkg` design (`08_...`)  |
| About          | THUOS identity, version, license         | Mirrors the `about` shell command       |

## 4. Theme engine (Designed)

A small, declarative theme layer so the desktop's palette and density are data,
not hard-coded values. The same tokens drive both the HTML preview and the future
framebuffer renderer, keeping them visually in sync.

```text
theme "THUOS Default" {
  bg.base      = #0E1116    # deep, calm background
  bg.panel     = #161B22    # bars, docks, window chrome
  fg.primary   = #E6EDF3    # main text
  accent       = #4FD1C5    # THUOS signature teal
  radius       = 8px
  density      = comfortable
}
```

## 5. Implementation path: HTML mock → compositor

The kernel GUI is built strictly bottom-up. Each stage is small, testable, and
must work before the next begins. **Only the first item (the HTML mock) exists.**

```text
[ done ]  1. HTML concept mock           preview/thuos_preview.html (browser)
[ plan ]  2. Framebuffer primitives      put_pixel, fill_rect, blit (linear FB)
[ plan ]  3. Bitmap font                 8x16 glyphs -> draw_text(x,y,str)
[ plan ]  4. Mouse driver + cursor       PS/2 mouse (IRQ12), draw a cursor
[ plan ]  5. Window rectangles           track windows as rects + z-order
[ plan ]  6. Event loop                  route keyboard/mouse to focused window
[ plan ]  7. Compositor                  combine windows + cursor into one frame
```

Stage details:

1. **HTML mock (Concept).** Establishes layout, palette, and app set in the
   browser. No kernel involvement.
2. **Framebuffer primitives (Planned).** Acquire a linear framebuffer (VBE via
   the bootloader / Multiboot framebuffer tags) and implement pixel/rectangle
   fills. This requires the **framebuffer driver** in `05_DRIVER_MODEL.md`.
3. **Bitmap font (Planned).** A fixed 8x16 font so text can be drawn at pixel
   coordinates — the graphical analogue of today's VGA text output.
4. **Mouse + cursor (Planned).** Bring up the **PS/2 mouse driver** (IRQ12) and
   draw a movable cursor sprite.
5. **Window rectangles (Planned).** Represent windows as rectangles with a
   z-order and a focused window.
6. **Event loop (Planned).** Dispatch keyboard and mouse events to the focused
   window; the existing IRQ-driven input feeds this.
7. **Compositor (Planned).** Composite all window surfaces plus the cursor into a
   single frame and present it, ideally double-buffered to avoid tearing.

Each stage depends on the one above it, which is why the kernel cannot "just"
render the HTML preview — the entire stack from framebuffer up is still to be
built.

## 6. Honesty notes

- The browser preview is labeled a **Concept Preview** and is not a claim that
  the kernel has a GUI.
- No screenshots of a *kernel-rendered* desktop exist or are implied; THUOS has
  not been booted graphically (and QEMU is unavailable in the current build
  environment).
- The text shell is the only interactive surface that actually runs in THUOS
  today.

---

## Status summary

THU Desktop is **Concept** today, realized solely as a browser HTML preview; the
entire kernel graphics stack — framebuffer, font, mouse, window manager, event
loop, and compositor — is **Planned**.
