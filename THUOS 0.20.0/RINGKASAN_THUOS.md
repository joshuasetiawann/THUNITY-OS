# THUOS / THUNITY-OS — Ringkasan & Handoff
> Paket konteks untuk melanjutkan proyek di sesi/percakapan baru.
> Tanggal: 2026-06-09 · Versi terakhir: **0.17.0 "Suite"** (menu Settings + tema bisa diganti + app Notes & Paint)

---

## 0. Mulai cepat untuk sesi baru (PENTING)
- **Repo GitHub (publik):** https://github.com/joshuasetiawann/THUNITY-OS
  · **0.12 → 0.15 (User Mode, Desktop, Aurora, Apps) di branch `claude/serene-dijkstra-earnge`** (PR #2, base loving-ptolemy) · CI build + boot-smoke **hijau** (boot-verified di QEMU).
  · **Branch `main`** = versi terbaru (kode bersih, di-build & boot-verified CI). **Branch `master`** = arsip tiap versi sebagai folder top-level (`THUOS 0.2.0 … 0.17.0`).
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
| 0.14 | **Aurora** (desktop modern 1024×768×32 truecolor via Bochs VBE; PCI+DISPI+map LFB) | boot-verified + screenshot |
| 0.15 | **Apps** (PS/2 mouse + kursor, dock bisa diklik, app: Terminal/Kalkulator/Files/System/About) | boot-verified + screenshot |
| 0.16 | **Polish** (desktop diperhalus, ikon app pictogram, jam top-bar, indikator app aktif) | boot-verified + screenshot |
| 0.17 | **Suite** (menu Settings: tema/display/devices + app Notes & Paint) | boot-verified + screenshot |

**Verifikasi:** 8 unit test host (`make test`) + 2 job CI (`build-and-test`, `boot-smoke`).
`boot-smoke` mem-boot kernel di QEMU, baca serial COM1, dan meng-assert marker
(THUOS → Kernel heap → Paging ENABLED → Context switch OK → all tasks finished →
RAM filesystem → Syscall interface → **User mode** → **THU Desktop** → `thuos>`).
Grafis 320x200 sulit di-assert lewat serial → diverifikasi lewat screenshot QEMU.

---

## 3. Yang BELUM ada (jujur)
- **GUI sudah modern + bisa diklik** (1024×768×32, mouse + kursor, dock klik buka app, app: Terminal/Kalkulator/Files/System/About). Belum ada: **window yang bisa digeser / banyak window sekaligus**, font masih bitmap VGA di-scale (belum TrueType/anti-alias).
- **Kamera, WiFi, Bluetooth: TIDAK didukung** (QEMU tak punya kamera; WiFi/BT butuh firmware+driver vendor yang masif — di luar jangkauan OS from-scratch). Ditampilkan jujur di app **System**, bukan dipalsukan.
- **"Install aplikasi"**: belum. Realistisnya = **ELF loader** (muat program dari file) — langkah lanjut. App sekarang masih built-in di kernel.
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
`heap kmalloc kfree vmm ps sched tasks ls cat write sys user gui files notes calc paint settings apps echo banner color clear crash reboot halt`
Di desktop grafis: **klik ikon dock** untuk ganti app (Terminal/Files/Notes/Kalkulator/Paint/Settings); **Esc** kembali ke terminal. Settings ▸ Appearance bisa ganti tema.

Coba: `sysinfo` · `heap` lalu `kmalloc 256` lalu `heap` · `vmm 0x400000` · `tasks` · `write notes halo` lalu `ls` lalu `cat notes` · `sys` · `user` (turun ke ring 3 lalu balik via `int 0x80`, lapor CPL).

---

## 7. Struktur repo (file penting)
```
kernel/boot/boot.S                 multiboot + stack + call kernel_main
kernel/arch/x86/                    gdt(+TSS),idt,isr,irq,pic,pit + syscall_core/.c/.h, syscall_stub.S
                                    tss.c/.h, usermode_core.c/.h, usermode.c/.h, usermode_entry.S (ring 3)
kernel/core/                        kernel.c (urutan init), kprintf, panic
kernel/drivers/                     vga (text), serial (COM1), keyboard, mouse (PS/2 IRQ12), gfx (font), lfb (framebuffer hi-res)
kernel/gui/                         gconsole (terminal) + desktop (WM + event loop + dock) + apps (calc/files/system/about)
kernel/mm/vmm.c                     + vmm_map_lfb() (petakan framebuffer MMIO tinggi)
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
- ✅ **0.13 THU Desktop (grafis)** — SELESAI: VGA mode 13h (320x200x256), font dari plane 2 VGA, desktop + window, shell di terminal grafis.
- ✅ **0.14 Aurora (desktop modern)** — SELESAI: framebuffer 1024×768×32 (Bochs VBE/DISPI), LFB via PCI + `vmm_map_lfb`, tema gelap flat (gradien, window ber-shadow, dock).
- ✅ **0.15 Apps** — SELESAI: PS/2 mouse (IRQ12) + kursor composited, dock bisa diklik, event loop (`desktop_run`), app: Terminal/Kalkulator(integer)/Files(ramfs)/System(jujur)/About.
- ✅ **0.16 Polish** — SELESAI: ikon app jadi pictogram via `lfb_disc`+`lfb_line`, jam top-bar, indikator app aktif, chrome diperhalus.
- ✅ **0.17 Suite** — SELESAI: menu **Settings** (sidebar: Appearance/Display/Date&Time/Devices/About; tema bisa diganti live via `desktop_set_theme`), app **Notes** (editor, autosave ke ramfs `notes.txt`), app **Paint** (gambar pakai mouse, palet warna, stroke di-interpolasi). Dock jadi 6 app.
1. **0.18 Window bisa digeser / banyak window** + window manager.
2. **ELF loader** + isolasi memori per-proses → app **dimuat dari file** (arti realistis "install app"). Lebih banyak syscall (`open`/`read`/`write`).
3. Jaringan **kabel** (NIC e1000 emulasi + TCP/IP minimal) — track besar terpisah.
4. Preemptive multitasking, persistensi (initrd), font TrueType/anti-alias.
**Catatan jujur:** Kamera/WiFi/Bluetooth tetap di luar jangkauan (tak ada device / butuh firmware vendor).
Tiap milestone: *core host-tested* + *boot self-test* + marker baru di `boottest.sh`.

---

## 9. Catatan operasional untuk asisten di sesi baru
- **Awali dengan `git clone -b claude/loving-ptolemy-JDEAO https://github.com/joshuasetiawann/THUNITY-OS.git`** untuk kode terbaru (sandbox lokal sering stale/rollback).
- **Push dari sandbox kadang putus** (proxy auth). Kalau tak bisa push → kirim hasil sebagai **zip** untuk user push sendiri. Jangan korek kredensial.
- Repo dulu `CORE-OS-APPLICATION-`, kini **`THUNITY-OS`** (di-rename; GitHub redirect).
- Jangan pernah memalsukan hasil boot. QEMU tak ada di sandbox → boot dibuktikan **hanya** lewat CI `boot-smoke`.

---

## 10. Verdikt jujur (one-liner)
THUOS = **kernel nyata, boot-verified**, dengan fondasi sehat (virtual memory, multitasking, filesystem, syscalls, ring 3) dan kini **desktop modern yang bisa diklik dengan mouse + app bawaan (Kalkulator, Files, Terminal, System)** — *belum* menyaingi Windows/macOS (dan jujur: tanpa kamera/WiFi/Bluetooth), tapi diarahkan ke wedge yang realistis & bisa dimenangkan. Analisis penuh: `docs/THUOS_REALITY_CHECK.md`.
