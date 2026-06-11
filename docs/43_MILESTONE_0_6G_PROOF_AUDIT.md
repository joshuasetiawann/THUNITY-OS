# 43 — Milestone 0.6G Proof Audit (THUOS 0.20.0)

A transcript-level record of **what was verified in this environment, how, and
what could not be**. The point is that every claim in the milestone report maps to
a command anyone can re-run. Nothing here is asserted without a check.

## How to reproduce

```
make clean && make kernel     # build the kernel
make verify                   # multiboot / ELF checks
make test                     # 10 host unit tests (native gcc, no QEMU)
make stress                   # re-run the host tests 50x
make scan                     # honesty / overclaim guard
make deep-verify              # build + verify + test + scan + shell syntax
make export ; make package    # artifacts + release archive
make boottest                 # boot in QEMU, check serial boot markers
```

## Results captured (2026-06-11, this container)

| # | Check | Command | Result |
|---|-------|---------|--------|
| 1 | Clean build | `make clean && make kernel` | **OK** — `build/kernel.elf`, strings show `THUOS 0.20.0 "AI-Native"` |
| 2 | Multiboot/ELF | `make verify` | **PASS** — ELF 32-bit, Intel 80386, statically linked |
| 3 | Host tests | `make test` | **10/10 OK** — incl. `test_ai`, `test_features` |
| 4 | Stress | `make stress` | **OK** — 50 iterations × 10 tests, no failures |
| 5 | Overclaim scan | `make scan` | **CLEAN** — no positive claims of Thunity/Docker/Python/Node/Ollama/networking/AI-runtime |
| 6 | Deep verify | `make deep-verify` | **OK** — build + verify + test + scan + shell syntax |
| 7 | Export | `make export` | **OK** — `build/export/` (kernel.elf, verification, screenshots, manifest) |
| 8 | Package | `make package` | **OK** — `thuos_milestone_0_20.tar.gz` (~280K) |
| 9 | Shell syntax | `bash -n scripts/*.sh` | **OK** — all 5 scripts parse |
| 10 | Boot smoke | `make boottest` | **BOOT-VERIFIED** — boot line `[ OK ] AI-native layer …` + all markers, reaches `thuos>` |
| 11 | AI functional | QMP send-key `ai status` | **OK** — prints services `[design-only]`, `Policy: … net-bridge=OFF` |
| 12 | Features functional | QMP send-key `features` | **OK** — prints `feature map`, `[design-only]`, `implemented=…` |

### Host test detail (`make test`)
`test_pmm, test_kheap, test_vmm, test_sched, test_task, test_fs, test_syscall,
test_usermode, test_ai, test_features` — all print `OK`.

- `test_ai`: default policy is local-only (`net-bridge`/`file-write`/`tool-exec`/
  `cloud` OFF), service+model registries, task FSM (valid path + rejected illegal
  transitions + terminal states), audit ring wraps at capacity, subcommand parse.
- `test_features`: add/count/count-by-status, stable non-empty labels, capacity
  guard at `FEAT_MAX`.

### Boot-time evidence (`make boottest`, COM1 serial)
```
[ OK ] AI-native layer: service/model/task/policy/audit core (host-tested)
[ ok ] saw: THUOS ... Kernel heap ... Paging ENABLED ... Context switch OK
[ ok ] saw: all tasks finished ... RAM filesystem ... Syscall interface
[ ok ] saw: User mode ... THU Desktop
```

### Runtime evidence (`ai status` over QMP)
```
THUOS AI-native layer — foundation (host-tested core; no inference here)
Services (2):
  - thunity-backend (bridge) http://127.0.0.1:8000 [design-only]
  - ollama (bridge) http://127.0.0.1:11434 [design-only]
Policy: local-infer=on net-bridge=OFF file-write=OFF tool-exec=OFF cloud=OFF
Note: no request leaves this machine; a bridge needs TCP/IP (not in THUOS yet).
```

## What was NOT verified (and cannot be, honestly)

| Not verified | Why |
|--------------|-----|
| AI inference / model output | THUOS has no inference runtime; nothing to run |
| Network bridge to Thunity/Ollama | no TCP/IP stack — `ai bridge` deliberately sends nothing |
| Docker / Python / Node / Ollama inside THUOS | no Linux userspace or container runtime |
| Boot on physical hardware | only QEMU `-kernel` + GRUB ISO available here |
| Real package install (`thupkg`) | no storage driver / ELF loader yet |
| Movable-window desktop | single focused app today (see doc 42) |

## Honesty controls in place
- **`make scan`** fails the build text if forbidden positive claims appear; it is
  part of `make deep-verify`.
- The **`features`** command and the in-kernel **feature registry** are the single
  source of truth the docs and this audit are written against.
- `ai bridge` is engineered to demonstrate the pipeline *and* refuse to fake a
  connection — it ends in `denied`/`failed` with “no request sent”.
