# thupkg — THUOS Package Manager

**Status: Designed.** `thupkg` is the planned package manager for THUOS. This
document specifies its format and commands. There is **no real backend yet**:
the `thupkg` command in the kernel shell (`kernel/shell/shell.c`) is a
**design preview** that prints example output and explicitly says so — it does
not download, verify, install, or remove anything.

> What runs today: typing `thupkg list` at the `thuos>` prompt prints a small
> hardcoded list of *designed* packages and the note "thupkg is a design
> preview; no real install backend yet." That is the full extent of the
> implementation. Everything below describes the intended system.

---

## 1. Concepts

| Term            | Meaning                                                        |
|-----------------|----------------------------------------------------------------|
| `.thupkg`       | A distributable package archive (manifest + payload + signature)|
| Manifest        | Declarative metadata: name, version, deps, files, permissions   |
| Repository      | A collection of packages + an index; offline first, online later|
| Dependency graph| The resolved set of packages required to satisfy an install     |
| Signature       | Cryptographic proof of authorship over the manifest             |

Packages install *applications* (`.thuapp`, see `09_APP_PLATFORM.md`), command
utilities, and system components. THUOS deliberately keeps one packaging story
for all of them.

## 2. `.thupkg` format (Designed)

A `.thupkg` is a simple, self-describing archive. Layout:

```text
example.thupkg
├── manifest.toml        # required: identity, deps, permissions, file list
├── payload/             # the actual files to install
│   ├── bin/...
│   └── share/...
├── hashes.txt           # SHA-256 of every file under payload/
└── signature.sig        # detached signature over manifest.toml + hashes.txt
```

The format is intentionally plain (no opaque blobs) so a package can be audited
by hand — consistent with THUOS's "auditable, no hidden behavior" charter.

## 3. Manifest (Designed)

```toml
# manifest.toml
[package]
name        = "thu-files"
version     = "0.1.0"
summary     = "THU Desktop file browser"
license     = "MIT"
arch        = "x86"            # i386, 32-bit (matches the kernel target)

[depends]
thu-coreutils = ">=0.2.0"
thu-runtime   = ">=0.1.0"

[permissions]                  # enforced by the sandbox (see SECURITY_MODEL)
filesystem = ["/home", "/mnt"]
devices    = []
network    = false

[files]                        # what lands where on install
"bin/thu-files"        = "/apps/thu-files/bin/thu-files"
"share/icons/files.png"= "/apps/thu-files/share/icons/files.png"
```

## 4. Metadata & dependency graph (Designed)

Resolution is a standard topological sort over declared dependencies, with
version constraints checked against the repository index:

```text
  install thu-files
        │
        ▼
  read manifest -> depends: thu-coreutils >=0.2.0, thu-runtime >=0.1.0
        │
        ▼
  resolve from repo index -> [ thu-runtime, thu-coreutils, thu-files ]
        │                     (dependencies first, in order)
        ▼
  verify each (sig + hashes) -> install in order -> record in package DB
```

Conflicts (incompatible version constraints, missing packages) **fail loudly**
with a clear message and change nothing — never a partial install.

## 5. Signature & security (Designed)

Verification is mandatory and happens **before** any file is written:

```text
verify(pkg):
  1. recompute SHA-256 of each payload file
  2. compare against hashes.txt              -> mismatch  => REJECT
  3. verify signature.sig over manifest+hashes with a trusted key
                                             -> bad/absent => REJECT
  4. only on full success is install allowed
```

This is the same chain described in `06_SECURITY_MODEL.md`. Unsigned or tampered
packages are refused. Offline installs from a local repo still require valid
signatures — being offline is not an excuse to skip verification.

## 6. Repositories: offline first, online later

| Stage            | Description                                              | Status  |
|------------------|----------------------------------------------------------|---------|
| Local package    | Install a single `.thupkg` file by path                  | Designed|
| Offline repo     | A directory of packages + a signed `index.toml`          | Designed|
| Online repo      | Fetch index + packages over the network                  | Planned |

The online repository is **Planned** and gated on the network driver
(`05_DRIVER_MODEL.md`) and the security review that any networking requires.
THUOS makes no network connections today.

## 7. Commands (Designed)

The intended command set. (In the current shell, only `thupkg list` and a usage
hint print anything, and they are clearly marked as a preview.)

```text
thupkg list                 # list installed / known packages
thupkg info <name>          # show a package's metadata and dependencies
thupkg install <name|file>  # resolve, verify, and install (signed + sandboxed)
thupkg remove <name>        # uninstall and clean up
thupkg verify <file>        # check signature + hashes without installing
thupkg update               # refresh index / upgrade installed packages
```

Example design-preview session (illustrative of the intended UX):

```text
thuos> thupkg list
thupkg - installed/known packages (design preview):
  thu-coreutils   0.2.0   [designed]
  thu-terminal    0.2.0   [designed]
  thu-files       0.1.0   [planned]
  thu-settings    0.1.0   [planned]
Note: thupkg is a design preview; no real install backend yet.
```

## 8. Dependencies on the rest of THUOS

`thupkg` cannot be real until the layers it stands on are real:

| Needs                          | Provided by                         | Status   |
|--------------------------------|-------------------------------------|----------|
| A filesystem to install into   | VFS + THUFS (`04`, `THUFS_SPEC`)    | Planned  |
| Userspace to run installers    | syscalls + userspace (milestone 0.5)| Planned  |
| Signature verification         | crypto in the security model (`06`) | Designed |
| Online repo fetch              | network driver (`05`)               | Planned  |

---

## Status summary

The `.thupkg` format, manifest, dependency resolution, signing, and command set
are **Designed**; the offline repo is **Designed** and the online repo is
**Planned**; the shell's `thupkg` command is a **design preview only** with no
install backend.
