# THUOS / THUNITY-OS — Ringkasan & Handoff
> Paket konteks untuk melanjutkan proyek di sesi/percakapan baru.
> Tanggal: 2026-06-09 · Versi terakhir: **0.13.0 "Desktop"** (GUI grafis — desktop + terminal di window)

---

## 0. Mulai cepat untuk sesi baru (PENTING)
- **Repo GitHub (publik):** https://github.com/joshuasetiawann/THUNITY-OS
  · **0.12 "User Mode" + 0.13 "Desktop" di branch `claude/serene-dijkstra-earnge`** (PR #2, base loving-ptolemy) · CI build + boot-smoke **hijau** (boot-verified di QEMU).
  · Snapshot tiap versi ada di folder `versions/v0.2.0 … v0.13.0` (root = tree aktif terbaru yang di-build CI).
- **Sandbox kadang rollback lokal** ke commit lama tiap pergantian giliran. **Selalu** awali sesi dengan menarik kode terbaru dari GitHub (read publik selalu jalan); pakai branch milestone terbaru:
  ```bash
  git clone -b claude/serene-dijkstra-earnge https://github.com/joshuasetiawann/THUNITY-OS.git thuos
  cd thuos
  ```
- Build/test (gcc ada di sandbox; QEMU TIDAK ada → boot diuji lewat CI): `make test`, `make verify`, `make kernel`.

---

## 1. Apa ini
THUOS = **sistem operasi dari nol** (x86 i386, Multiboot 1), kernel C + assembly, **benar-benar boot di QEMU** dan diverifikasi otomatis tiap push (GitHub Actions). Bukan distro Linux, bukan app Electron.

**Filosofi / wedge** (lihat `docs/THUOS_VISION.md` & `THUOS_REALITY_CHECK.md`):
local-first · privacy-first · capability-secured · honest engineering.
Strateginya **bukan** menyaingi Windows/macOS langsung (mustahil untuk tim kecil — ~60.000 person-year), tapi memenangkan **wedge sempit**: OS-terminal untuk developer/AI-agent yang **boot kilat di VM/microVM** (menghindari masalah driver).

---

## 2. Status — sudah jadi & BOOT-VERIFIED (di QEMU via CI)

| Ver | Milestone | Status |
|-----|-----------|--------|
| 0.1 | Multiboot boot stub | boot-verified |
| 0.2 | VGA, serial, GDT/IDT, exception 0-31, PIC/IRQ, PIT, keyboard, panic, shell | boot-verified |
| 0.3 | Physical memory manager (bitmap 4 KiB) | host-tested |
| 0.4 | Kernel heap (kmalloc/kfree, free-list) | host-tested |
| 0.5 | Paging tables + translasi (staged) | host-tested |
| 0.6 | Scheduler round-robin (policy) | host-tested |
| 0.6.1 | **Boot-verified** (CI `boot-smoke` QEMU) | boot-verified |
| 0.7 | **Paging ENABLED** (CR0.PG) | boot-verified |
| 0.8 | Context switch (stack/register) | boot-verified |
| 0.9 | Cooperative multitasking (3 task) | boot-verified |
| 0.10 | RAM filesystem (ls/cat/write) | host-tested + boot-verified |
| 0.11 | **Syscalls** (`int 0x80`: uptime/write/getpid/version) | host-tested + boot-verified |
| 0.12 | **User mode (ring 3)** (TSS + `iret` ke CPL 3 + `int 0x80` dari userspace) | host-tested + boot-verified |
| 0.13 | **THU Desktop** (VGA mode 13h grafis + terminal di dalam window) | boot-verified + screenshot |

**Verifikasi:** 8 unit test host (`make test`) + 2 job CI (`build-and-test`, `boot-smoke`).
`boot-smoke` mem-boot kernel di QEMU, baca serial COM1, dan meng-assert marker
(THUOS → Kernel heap → Paging ENABLED → Context switch OK → all tasks finished →
RAM filesystem → Syscall interface → **User mode** → **THU Desktop** → `thuos>`).
Grafis 320x200 sulit di-assert lewat serial → diverifikasi lewat screenshot QEMU.

---

## 3. Yang BELUM ada (jujur)
- **GUI masih dasar** — sudah grafis (VGA mode 13h 320x200, desktop + 1 window terminal), tapi **belum ada mouse, belum banyak window, belum window manager**. Resolusi rendah (retro); resolusi tinggi butuh VBE/framebuffer (langkah lanjut).
- **Isolasi memori per-proses** — ring 3 sudah jalan tapi map masih *flat* (kernel & user berbagi identity map yang kini user-accessible). Pemisahan ruang alamat per-proses = langkah berikutnya.
- **ELF loader & program userspace pertama** (yang dimuat dari file, bukan fungsi kernel).
- Preemptive multitasking (timer IRQ), persistensi file (masih RAM), driver disk/GPU/Wi-Fi/USB (sengaja tak dikejar), jaringan, libc, mesin browser.
- Belum di **hardware fisik** (baru emulator/CI).
- Belum menyaingi Windows/macOS (dan memang bukan target langsung).

---

## 4. Cara kerja & disiplin (jaga konsistensi ini)
- **Pola tiap subsistem:** *core murni* (tanpa dependensi kernel → **host-tested**) + *glue kernel* + *boot self-test* (di-assert `boot-smoke`).
  Contoh pasangan: `frame_bitmap`+`pmm`, `kheap_core`+`kheap`, `vmm_core`+`vmm`, `sched_core`+`sched`, `ramfs_core`+`fs`, `syscall_core`+`syscall`.
- **Verifikasi-dulu:** jangan klaim "boot/jalan" tanpa `boot-smoke` hijau. Langkah berisiko (paging enable, context switch, syscall) di-boot-verify dulu sebelum ditumpuk yang berikutnya.
- **Label kejujuran:** COMPILE-ONLY / HOST-TESTED / BOOT-VERIFIED. Jangan over-claim.
- **Git:** **commit + push tiap milestone** (push-per-milestone). Krusial karena sandbox rollback; yang sudah di-push aman di GitHub.

---

## 5. Build / Run / Test (di Linux / WSL2 / VM)
```bash
sudo apt-get install -y build-essential gcc-multilib qemu-system-x86
make test       # 7 unit test di host (PMM, heap, paging, sched, task, fs, syscall)
make run        # BOOT THUOS di QEMU → muncul log boot lalu prompt thuos>
make boottest   # boot otomatis + cek marker (skip kalau tak ada QEMU)
make verify     # cek struktur (ELF, header Multiboot, simbol)
make demo       # sajikan preview/thuos_os.html di http://localhost:8080
```
Tanpa setup apa pun: buka **`preview/thuos_os.html`** di browser (simulasi desktop bisa-pakai).

---

## 6. Shell `thuos>` (perintah inti)
`help about version status sysinfo uptime ticks mem memmap pages allocpage freepage`
`heap kmalloc kfree vmm ps sched tasks ls cat write sys user gui echo banner color clear crash reboot halt`

Coba: `sysinfo` · `heap` lalu `kmalloc 256` lalu `heap` · `vmm 0x400000` · `tasks` · `write notes halo` lalu `ls` lalu `cat notes` · `sys` · `user` (turun ke ring 3 lalu balik via `int 0x80`, lapor CPL).

---

## 7. Struktur repo (file penting)
```
kernel/boot/boot.S                 multiboot + stack + call kernel_main
kernel/arch/x86/                    gdt(+TSS),idt,isr,irq,pic,pit + syscall_core/.c/.h, syscall_stub.S
                                    tss.c/.h, usermode_core.c/.h, usermode.c/.h, usermode_entry.S (ring 3)
kernel/core/                        kernel.c (urutan init), kprintf, panic
kernel/drivers/                     vga (text), serial (COM1 mirror), keyboard, gfx (VGA mode 13h grafis)
kernel/gui/                         gconsole (terminal grafis) + desktop (THU Desktop)
kernel/lib/                         string, types
kernel/mm/                          frame_bitmap+pmm, kheap_core+kheap, vmm_core+vmm
kernel/sched/                       sched_core+sched, task+context.S, coop
kernel/fs/                          ramfs_core+fs
kernel/shell/shell.c               shell + semua perintah
kernel/include/thuos/version.h     versi/codename/milestone (single source of truth)
tests/                             test_pmm,kheap,vmm,sched,task,fs,syscall
scripts/boottest.sh                boot QEMU + cek marker (dipakai CI & make boottest)
.github/workflows/ci.yml           CI: build-and-test + boot-smoke
docs/                              THUOS_VISION, THUOS_ARCHITECTURE, THUOS_REALITY_CHECK, BOOT_THUOS, 00-11
preview/                           thuos_os.html (desktop), thuos_sim.html, thuos_progress.html, thuos_aurora.html
Makefile README.md CHANGELOG.md PROJECT_STATUS.md linker.ld grub.cfg
```

---

## 8. Roadmap berikutnya (urut dependensi)
- ✅ **0.12 Ring 3 / user mode** — SELESAI: GDT user segments + **TSS** (`ltr`), `iret` ke ring 3, `int 0x80` dari ring 3, kembali bersih ke ring 0. Bukti CPL 3 = `CS & 3`. Marker `User mode`.
- ✅ **0.13 THU Desktop (grafis)** — SELESAI: VGA mode 13h (320x200x256, framebuffer 0xA0000), font diambil dari plane 2 VGA, desktop + window + taskbar, **shell jalan di terminal grafis**. Marker `THU Desktop`. Diverifikasi screenshot QEMU.
1. **0.14 Mouse + window** — driver PS/2 mouse + kursor, beberapa window yang bisa diklik di THU Desktop.
2. **Isolasi memori per-proses** — pisahkan page user vs kernel (jangan map seluruh kernel `PTE_USER`); page-fault policy.
3. **ELF loader** + lebih banyak syscall (`open`/`read`/`write` via ramfs) → program userspace pertama (dimuat dari file).
4. Preemptive multitasking (timer IRQ → context switch), persistensi (initrd), resolusi tinggi (VBE), libc kecil.
Tiap milestone: *core host-tested* + *boot self-test* + marker baru di `boottest.sh`.

---

## 9. Catatan operasional untuk asisten di sesi baru
- **Awali dengan `git clone -b claude/loving-ptolemy-JDEAO https://github.com/joshuasetiawann/THUNITY-OS.git`** untuk kode terbaru (sandbox lokal sering stale/rollback).
- **Push dari sandbox kadang putus** (proxy auth). Kalau tak bisa push → kirim hasil sebagai **zip** untuk user push sendiri. Jangan korek kredensial.
- Repo dulu `CORE-OS-APPLICATION-`, kini **`THUNITY-OS`** (di-rename; GitHub redirect).
- Jangan pernah memalsukan hasil boot. QEMU tak ada di sandbox → boot dibuktikan **hanya** lewat CI `boot-smoke`.

---

## 10. Verdikt jujur (one-liner)
THUOS = **kernel nyata, boot-verified**, dengan fondasi sehat (virtual memory, multitasking, filesystem, syscalls, ring 3/user mode) dan kini **tampilan grafis sendiri (THU Desktop, mode 13h)** — *belum* menyaingi Windows/macOS, tapi diarahkan ke wedge yang realistis & bisa dimenangkan. Analisis penuh: `docs/THUOS_REALITY_CHECK.md`.
