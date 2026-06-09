# 05 — Shell Commands

**Status:** Implemented. The `thuos>` shell and all commands below are coded in
`kernel/shell/shell.c` and build-verified. The THU Desktop preview's terminal
simulates the same commands in a browser (see
[`06_THU_DESKTOP_PREVIEW.md`](06_THU_DESKTOP_PREVIEW.md)).

## The shell

`shell_run()` prints the prompt `thuos> `, reads a line via the blocking
`keyboard_getchar()`, supports backspace editing, and dispatches the first word
as the command with the remainder passed as arguments. Input that is empty is
ignored; unknown commands print `Unknown command: <x> (try 'help')`.

## Command reference

| Command | Description |
|---------|-------------|
| `help` | List all commands. |
| `about` | THUOS identity: kernel name, goal, direction. |
| `version` | Kernel name, version, codename, milestone, build date/time. |
| `status` | Honest per-subsystem status (done / wip / plan). |
| `sysinfo` | One-screen summary: arch, kernel, timer Hz, uptime, memory, build. |
| `uptime` | Uptime in PIT ticks and approximate seconds. |
| `ticks` | Raw PIT tick counter. |
| `mem` | Memory summary from the physical memory manager (hint + frame stats). |
| `memmap` | Dump the Multiboot memory map (base / length / type). |
| `pages` | Page-frame statistics (usable / reserved / free / used). |
| `allocpage` | Allocate one 4 KiB physical frame; prints its physical address. |
| `freepage <hex>` | Free a frame by physical address (refuses protected/unaligned). |
| `echo <text>` | Print the rest of the line. |
| `banner` | Print the THUOS ASCII banner. |
| `color <0-15>` | Set the VGA text foreground color. |
| `thupkg [list]` | Package-manager **design preview** (no real backend yet). |
| `clear` | Clear the screen. |
| `crash div0` | Intentionally trigger a divide-by-zero to test the exception handler. **Testing only.** |
| `reboot` | Reboot via the 8042 keyboard-controller reset pulse. |
| `halt` | Disable interrupts and halt the CPU. |

That is **21 commands** (including the four memory commands and `crash`,
`reboot`, `halt`). The list is printed by `help` and kept in sync with the
dispatcher in `shell.c`.

## Examples

```
thuos> version
THU Kernel 0.2.0 "Boot Seed"
Milestone 0.2 - Kernel Stability, Diagnostics, Timer, Preview
Built <date> <time>

thuos> uptime
Uptime: 1234 ticks, approx 12 seconds (PIT @ 100 Hz)

thuos> status
THUOS subsystem status (honest):
  [done]    Multiboot boot, VGA console, serial COM1
  [done]    GDT, IDT, CPU exceptions 0-31
  [done]    PIC remap, PIT timer, keyboard IRQ, shell
  [done]    Panic/assert system
  [wip]     Memory manager (Milestone 0.3)
  [plan]    VFS + initrd (Milestone 0.4)
  ...

thuos> thupkg list
thupkg - installed/known packages (design preview):
  thu-coreutils   0.2.0   [designed]
  thu-terminal    0.2.0   [designed]
  ...
Note: thupkg is a design preview; no real install backend yet.
```

## Honesty notes

- `mem` reports only what the Multiboot loader provides plus an explicit
  "full physical memory manager is planned for Milestone 0.3" line. It does not
  pretend to manage memory.
- `thupkg` is labeled a **design preview** in its own output; there is no install
  backend yet.
- `crash div0` is clearly marked as a testing-only fault injector for verifying
  the exception path.

## Future commands (planned)

Reserved for later milestones: `ls`, `cat`, `cd`, `pwd`, `mkdir`, `ps`, `kill`,
`mount`, `heap`, `dmesg`, `startx`. These appear in the roadmap but are **not**
in the kernel shell yet. (`memmap`, `pages`, `allocpage`, `freepage` graduated
from planned to implemented in Milestone 0.3.)
