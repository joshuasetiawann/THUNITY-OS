# 41 — General OS Roadmap

THUOS aims to feel like a **normal general-purpose OS** (with AI as a first-class
citizen), not a tech demo. This roadmap lists the standard OS surfaces, their
current honest status, and the prerequisite each one waits on. Status labels match
the in-kernel feature registry (`features` command) and the milestone report.

> Legend: **implemented** = boot-verified in QEMU · **host-tested** = logic unit-
> tested · **design-only** = documented, no runtime path yet · **not-verified** =
> present but unverified here.

## Core OS surfaces

| Surface | Today | Notes / prerequisite |
|---------|-------|----------------------|
| Boot + kernel shell (`thuos>`) | implemented | 30+ commands; line editing |
| System info / uptime / version / about | implemented | `sysinfo`, `uptime`, `version`, `about` |
| Memory commands | implemented | `mem`, `memmap`, `pages`, `heap`, `kmalloc/kfree` |
| Process commands | implemented (demo scope) | `ps`, `sched`, `tasks` (cooperative) |
| Filesystem commands | implemented (RAM) | `ls`, `cat`, `write`; needs a **storage driver** for persistence |
| Logs / dmesg | implemented | `dmesg`/`logs` replay the kernel log ring |
| Command history | implemented | `history` (ring of recent commands) |
| Settings | implemented (desktop) | Appearance/Display/Devices/About panes |
| AI namespace | implemented | `ai status/models/tasks/policy/bridge` |
| Users / sessions | design-only | single implicit operator today; needs auth + permissions |
| Package / app install | design-only | `thupkg` is a preview; needs ELF loader + storage |

## Standard apps (desktop)

| App | Today | Prerequisite to “real” version |
|-----|-------|-------------------------------|
| Terminal | implemented | — |
| Files | implemented (RAM browser) | storage driver for real disks |
| Notes | implemented (autosave to ramfs) | persistence |
| Calculator | implemented | — |
| Paint | implemented | — |
| File manager (copy/move/delete) | design-only | filesystem write semantics + storage |
| App launcher (load from files) | design-only | ELF loader + process isolation |
| Task manager / system monitor | design-only (data exists) | a `top`-style live view over scheduler/PMM stats |
| Network manager | design-only | NIC driver + TCP/IP stack |
| AI Center / Model Manager | design-only (control-plane host-tested) | networking bridge to a local AI server |
| Update / recovery mode | design-only | storage + signed images + a boot menu |

## Sequencing (honest dependency order)

1. **Storage driver + persistent FS** → real Files, Notes persistence, packages.
2. **Process model + ELF loader** → app launcher, real task manager, isolation.
3. **Networking (NIC + TCP/IP)** → network manager, AI bridge, updates.
4. **Permissions + users/sessions** → multi-user, capability-secured AI actions.
5. **Window manager (movable/resizable windows)** → a fuller desktop (see
   [42 — Desktop & App Model](42_DESKTOP_AND_APP_MODEL.md)).
6. **Update/recovery** → safe field updates.

Nothing here is claimed as working beyond the “implemented/host-tested” rows. The
roadmap is the plan; the `features` command is the truth.
