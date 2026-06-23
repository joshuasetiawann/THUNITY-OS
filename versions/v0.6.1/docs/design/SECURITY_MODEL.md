# THUOS Security Model

**Status: Designed / Planned.** THUOS is privacy-first and honesty-first by
charter. The principles below describe the security posture the system is being
built toward. Most mechanisms (sandboxing, package signing, secure update,
recovery mode, audit logs) are **Designed** or **Planned** and are not enforced
by the 0.2.0 "Boot Seed" kernel. The two postures that are **in force today** are
the development-safety rules in section 8: emulator-first, and no destructive
disk writes.

> The kernel today runs entirely in ring 0 with no memory protection, no users,
> and no privilege boundaries. There is nothing to defend against yet — and,
> just as importantly, nothing that can quietly do harm. This document is the
> contract for keeping it that way as capabilities grow.

---

## 1. Security goals

| Goal                       | Meaning for THUOS                                          | Status   |
|----------------------------|-----------------------------------------------------------|----------|
| Least privilege            | Code and apps get only the capabilities they declare      | Designed |
| User consent               | Sensitive actions require an explicit human "yes"         | Designed |
| No hidden telemetry        | Zero background data collection; nothing phones home      | Designed |
| Sandboxed apps             | Apps run confined; permissions are enforced, not advisory | Planned  |
| Signed packages            | `.thupkg` / `.thuapp` verified before install             | Designed |
| Secure update              | Updates are signed, atomic, and reversible                | Planned  |
| Recovery mode              | A known-good path to repair a broken system               | Planned  |
| Audit logs                 | Security-relevant events are recorded and readable        | Designed |

## 2. Least privilege (Designed)

THUOS treats privilege as something you *request and justify*, not something you
get by default.

- **Kernel vs. user separation (Planned, milestone 0.5).** Userspace will run in
  ring 3 behind the syscall ABI; today everything is ring 0, which is noted
  plainly rather than dressed up as a security boundary.
- **Capability-scoped apps (Designed).** An app declares exactly what it needs in
  its manifest (filesystem paths, devices, network). Anything not declared is
  denied. See `09_APP_PLATFORM.md`.
- **No ambient authority.** There is no "run as everything" mode for normal apps.

## 3. User consent & no hidden telemetry (Designed)

- THUOS ships with **no telemetry, analytics, or beacons**. There is no opt-out
  to perform because there is nothing collecting data in the first place.
- Any future diagnostics are **opt-in**, local-by-default, and inspectable as
  plain text (consistent with the planned `/proc`-style introspection in
  `04_FILESYSTEM.md`).
- Actions that touch real hardware, real disks, or the network will require an
  explicit confirmation step and will say exactly what they are about to do.

## 4. Sandboxed apps (Planned)

The intended model for `.thuapp` bundles:

```text
  app requests  ──►  manifest declares permissions  ──►  kernel/runtime enforces
  (e.g. read /home, no network)        |                         |
                                        v                         v
                              user reviews on install    denied calls fail loud
                                                          (no silent escalation)
```

Enforcement depends on userspace + syscalls (milestone 0.5) and is therefore
**Planned**, not yet real. Until then, no untrusted code runs at all.

## 5. Signed packages & secure update

### Package signing (Designed)

Every `.thupkg` and `.thuapp` carries a detached signature over a manifest that
includes content hashes. `thupkg verify` checks the signature **and** that the
payload hashes match the manifest before anything is installed.

```text
package = manifest + payload + signature
verify():
  1. recompute SHA-256 of payload
  2. check it equals the hash recorded in the manifest   -> else REJECT
  3. check signature over the manifest with a trusted key -> else REJECT
  4. only then is install permitted
```

See `08_PACKAGE_MANAGER.md` for the full format. The signing/verification
*backend is not implemented*; the `thupkg` shell command is a design preview.

### Secure update (Planned)

- Updates are **signed** and verified with the same path as packages.
- Updates aim to be **atomic and reversible** (write-new-then-switch), so a
  failed or interrupted update never leaves a half-written system.
- Updates never silently change security settings or permissions.

## 6. Recovery mode (Planned)

A minimal, trusted boot path for repairing a broken install:

- Boots a known-good kernel + initrd (in-RAM, see `04_FILESYSTEM.md`).
- Offers filesystem check/repair and package rollback.
- Is **read-only by default**; any write requires explicit confirmation.
- Recovery integrates with THUFS's recovery-friendly design (`THUFS_SPEC.md`).

## 7. Audit logs (Designed)

Security-relevant events should be recorded in a readable, append-style log:

| Event                         | Example entry                                  |
|-------------------------------|------------------------------------------------|
| Package install / removal     | `pkg install thu-files (sig OK) by user`       |
| Permission grant / denial     | `deny net for thu-textedit`                    |
| Update applied / rolled back  | `update 0.3.0 applied (atomic)`                |
| Recovery-mode entry           | `recovery: started, read-only`                 |

Logs are plain text, local, and never transmitted off-device. (Wall-clock
timestamps require the RTC driver, which is **Planned** — see
`05_DRIVER_MODEL.md`; until then events can be stamped with PIT uptime.)

## 8. Development safety posture (IN FORCE TODAY)

These rules govern how THUOS is built right now, and they are honored in the
current repo and Makefile:

- **Emulator-first.** Development and testing target QEMU
  (`qemu-system-i386 -kernel build/kernel.elf`). The Makefile's `run` target
  *skips with a message* if QEMU is absent rather than pretending to boot.
- **No destructive disk writes.** THUOS has no storage driver yet and writes to
  **no real disk, partition, boot sector, or EFI partition**. All filesystem
  work is in-RAM (ramfs) or against emulator disk images only.
- **No hidden behavior.** No malware, spyware, backdoors, credential theft,
  stealth persistence, or destructive payloads — by charter, full stop.
- **No aggressive networking.** The kernel makes no network connections; there is
  no NIC driver, and one will not be added without explicit review.
- **Honest status.** Capabilities are labeled Implemented / Partially implemented
  / Designed / Planned, and never overstated.

## 9. Threat model (current vs. target)

| Aspect            | Today (0.2.0)                    | Target                                  |
|-------------------|----------------------------------|-----------------------------------------|
| Privilege levels  | Ring 0 only, no protection       | Ring 0 kernel / ring 3 userspace        |
| Untrusted code    | None runs                        | Sandboxed apps with declared perms      |
| Disk exposure     | None (no storage driver)         | Confirmed, signed, reversible writes    |
| Network exposure  | None (no NIC)                    | Opt-in, reviewed, signed repo access    |
| Data collection   | None                             | None (opt-in local diagnostics only)    |

---

## Status summary

The development-safety posture (emulator-first, no destructive disk writes, no
hidden behavior) is **in force today**; least privilege, signed packages, and
audit logging are **Designed**; sandboxing, secure update, and recovery mode are
**Planned**.
