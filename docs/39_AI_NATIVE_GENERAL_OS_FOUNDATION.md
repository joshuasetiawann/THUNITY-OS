# 39 — AI-Native General OS Foundation (THUOS 0.20 · founder tag “0.6G”)

> **Status in one line:** THUOS gains an **AI-native design foundation** — an
> in-kernel model of AI services, models, tasks, a permission policy, and an
> audit log, plus an `ai` shell namespace. It is **host-tested logic**, not a
> running AI. **THUOS does not perform inference, open sockets, or run Docker /
> Python / Node / Ollama.** Those remain on a separate Linux host (see
> [40 — Thunity AI Bridge Strategy](40_THUNITY_AI_BRIDGE_STRATEGY.md)).

## Why “AI-native”

The goal of THUOS is not only to run one app — it is to be a **general operating
system with AI as a first-class concern**: AI requests, the models they target,
the permissions they need, and an audit trail are modeled **in the kernel**, the
same way a normal OS models processes, files, and users. This milestone lays that
foundation in a way that is **honest and verifiable today**: pure logic that the
host test suite exercises, with no pretend runtime behind it.

## What was added (this milestone)

| Piece | Where | Status |
|-------|-------|--------|
| AI service registry (name, endpoint, kind, status) | `kernel/ai/ai_core.c` | host-tested |
| Model registry (name, role, declared/present) | `kernel/ai/ai_core.c` | host-tested |
| AI task model + state machine (created→queued→dispatched→done / denied / failed) | `kernel/ai/ai_core.c` | host-tested |
| Permission policy (local-only defaults; cloud + net-bridge OFF) | `kernel/ai/ai_core.c` | host-tested |
| Audit ring (records every AI decision) | `kernel/ai/ai_core.c` | host-tested |
| `ai status / models / tasks / policy / bridge` shell namespace | `kernel/ai/ai.c` | boot-verified (QEMU) |
| OS feature/status registry (honesty labels) | `kernel/os/feature_registry.c`, `osreport.c` | host-tested + boot-verified |
| `features`, `dmesg`/`logs`, `history` shell commands | `kernel/os/`, `kernel/core/kprintf.c`, `kernel/shell/shell.c` | boot-verified |

Host tests: `tests/test_ai.c`, `tests/test_features.c` (run by `make test`).

## The AI core (pure, no kernel deps)

`kernel/ai/ai_core.{c,h}` is policy/bookkeeping only — it compiles into the
kernel **and** into the host test (`THUOS_HOSTED_TEST`). It never calls hardware,
never allocates, never blocks. It models:

- **Services** — e.g. `thunity-backend` and `ollama`, each with an endpoint and a
  status. Default status is `design-only`: THUOS cannot reach them (no network).
- **Models** — example names (`llama3.1`, `qwen2.5-coder`, …) marked `declared`,
  never `present`, because THUOS has no inference runtime to load them.
- **Tasks** — an AI request with a validated state machine. Terminal states
  (`done`/`denied`/`failed`) accept no further transitions.
- **Policy** — a permission bitmask. **Defaults encode the local-only doctrine:**
  `local-inference` + `file-read` are ON; `net-bridge`, `file-write`,
  `tool-exec`, and `cloud` are **OFF** until explicitly enabled. Nothing is ever
  silently turned on.
- **Audit** — a fixed ring buffer recording each decision (allow/deny) so AI
  actions are accountable.

## The `ai` shell namespace

```
ai status   services, policy summary, recent AI audit
ai models   registered model names (declared; no runtime here)
ai tasks    AI task list + state machine
ai policy   permission policy (local-only by default)
ai bridge   Thunity/Ollama bridge demo — DESIGN-ONLY (no networking)
```

`ai bridge` is deliberately honest: it runs the **create → policy-check → audit**
pipeline and then **denies/fails the task**, printing that no bytes are sent
because THUOS has no TCP/IP stack. It demonstrates the design without faking a
connection.

## What this is **not**

- Not inference. THUOS runs no model and produces no AI output.
- Not networking. There is no TCP/IP stack; the “bridge” sends nothing.
- Not Docker/Python/Node/Ollama. None of these run inside THUOS.
- Not verified on real hardware. Verified in QEMU and by host unit tests only.

These limits are tracked in code by the **feature registry** (`features` command)
and enforced in text by the **overclaim scanner** (`make scan`).
