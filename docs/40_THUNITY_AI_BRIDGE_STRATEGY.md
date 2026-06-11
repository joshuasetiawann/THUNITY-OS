# 40 — Thunity AI Bridge Strategy (honest)

> **Bottom line:** **THUOS cannot run the Thunity AI application natively today,
> and this document does not claim it can.** Thunity AI is a Linux container
> stack; THUOS is a from-scratch kernel that has no Linux userspace, no container
> runtime, and no networking yet. The realistic path is a **bridge**: run Thunity
> AI on a Linux host, and let THUOS *talk to it* once THUOS has networking. This
> file describes that path and the milestones in between.

## Why THUOS can’t run Thunity AI natively (yet)

Thunity AI currently requires a full Linux application platform:

| Thunity AI needs | THUOS has today |
|------------------|-----------------|
| Python 3.11 / FastAPI (backend) | no language runtime |
| Node / React / Vite (frontend) | no JS runtime |
| Docker + Compose (orchestration) | no container runtime, no Linux syscalls |
| PostgreSQL (database) | in-RAM toy filesystem only |
| Redis (cache) | none |
| n8n (automation) | none |
| Ollama + ROCm (local AI inference) | no GPU stack, no inference engine |
| TCP/IP networking | **none** (design-only) |

Each row is a large subsystem. None can be faked. So the honest position is: the
**Linux appliance** is how Thunity AI runs now; THUOS grows toward *connecting to*
it, then *hosting parts of it*, over a long horizon.

## Bridge architecture — three honest stages

### Now — separate hosts, no connection from THUOS
- Thunity AI runs as a **Linux appliance** on Joshua’s AMD/ROCm PC (its own repo).
- THUOS runs in a VM/emulator and models the bridge **design-only** (`ai bridge`):
  it records the request, applies the local-only policy, audits the decision, and
  **sends nothing** (no TCP/IP).
- Value today: the *contract* (services, tasks, permissions, audit) is defined and
  unit-tested, so later networking work has a clear target.

### Later — THUOS as an AI control plane (network bridge)
Once THUOS has the prerequisites below, `ai bridge` becomes real:
- THUOS opens a **local** TCP connection to the Thunity backend (e.g.
  `http://127.0.0.1:8000`) or directly to Ollama (`:11434`), **only** when the
  `net-bridge` permission is explicitly enabled.
- THUOS shell / apps send AI requests and render responses; the heavy lifting
  (Python, Postgres, the model) stays on the Linux server.
- Still local-only: the bridge targets a machine on the user’s own network; cloud
  remains blocked by doctrine.

### Far future — THUOS-native Thunity micro-runtime
- A minimal, capability-secured runtime on THUOS hosts small AI services directly
  (or a compatibility layer runs a subset of the workloads). This is **research**,
  not a commitment, and depends on every milestone below landing first.

## Prerequisite milestones (what THUOS must gain first)

In rough dependency order — each is its own milestone, none exist yet beyond the
foundation noted:

1. **Storage driver + persistent filesystem** (today: RAM only).
2. **Process model + ELF loader** so programs load from files (today: built-ins).
3. **Networking: NIC driver + TCP/IP stack** (today: none) — the gate for any bridge.
4. **A userspace runtime** (a small libc / program ABI) — and eventually a
   Python/JS interpreter or a compatibility layer (very large).
5. **Container-like isolation or an app runtime** for services.
6. **GPU / inference support _or_ a remote inference bridge** (the bridge is the
   realistic near-term answer; native GPU inference is far future).

The AI-native foundation in milestone 0.20 (registry, policy, tasks, audit) is the
**control-plane contract** these milestones plug into.

## What an operator should do today

- To **use Thunity AI now:** run the Linux appliance (its own repository) on the
  AMD/ROCm machine. THUOS is not part of that runtime path yet.
- To **follow THUOS:** use `features` and `ai status` in the THUOS shell to see
  exactly what is implemented vs design-only — the same source of truth this doc
  is written against.
