# Arsitektur THUOS — belajar dari Linux, macOS (XNU), Windows (NT), seL4

> **Status kejujuran.** Dokumen ini adalah **desain arsitektur**, bukan klaim
> bahwa semuanya sudah jalan. Yang **sudah dibangun & teruji** hari ini ada di
> §6 (tabel realita). Sisanya adalah rancangan ber-alasan yang dibangun
> bertahap. THUOS saat ini: kernel i386 Multiboot, **0.4 "Kernel Heap"**,
> COMPILE-ONLY di lingkungan dev (BOOT-VERIFIED: tidak ada di sini).

Tujuan: mempelajari **bagaimana OS besar benar-benar dirancang**, mengambil
keputusan terbaiknya, membuang kelemahannya, lalu menyusun arsitektur THUOS
yang punya sikap sendiri — **local-first, privacy-first, capability-secured,
honest engineering**.

---

## 1. Cara membaca

Tiap subsistem ditulis dengan pola yang sama:

> **Belajar dari:** apa yang OS besar lakukan · **Pilihan THUOS:** keputusan kita ·
> **Alasan:** kenapa ini lebih baik untuk tujuan THUOS.

---

## 2. Studi banding: bagaimana empat kernel dibangun

### 2.1 Linux — *monolitik + modul*
Semua layanan inti (scheduler, manajemen memori, driver, jaringan, VFS) jalan
dalam **satu ruang alamat ber-privilege**. Fleksibilitas datang dari **Loadable
Kernel Modules (LKM)**: driver/filesystem bisa dimuat/dilepas tanpa reboot.
- **Memori:** buddy allocator (halaman fisik) → **slab/SLUB** (cache objek) →
  **VMA** per-proses, **demand paging**, swap, OOM killer.
- **Scheduler:** CFS, kini **EEVDF** (adil berbasis virtual-deadline).
- **VFS:** lapisan abstrak; ext4/btrfs/xfs/tmpfs mendaftar ke API open/read/write.
- **Keamanan modern:** namespaces + cgroups (basis container), SELinux/AppArmor
  (MAC), **eBPF** (program sandboxed di dalam kernel).
- **Trade-off:** performa tinggi (semua di satu ruang, panggilan fungsi langsung)
  tapi **satu bug driver = bisa menjatuhkan seluruh kernel**; permukaan serang besar.

### 2.2 macOS / iOS — *XNU, hibrida (Mach + BSD + IOKit)*
"X is Not Unix": inti **Mach** (mikrokernel) + lapisan **BSD** (monolitik) + **IOKit**.
- **Mach (inti rendah):** IPC berbasis **port/pesan**, **task & thread**,
  **manajemen virtual memory**, traps. Filosofinya mikrokernel.
- **BSD (di atas Mach):** POSIX, stack jaringan, VFS, manajemen proses,
  keamanan (MAC & ACL).
- **IOKit:** framework driver **berorientasi objek (C++ terbatas)** — model
  driver, power management, plug-and-play, dengan banyak driver lebih terisolasi.
- 2025: Apple memindah sebagian ke **"security exclaves"** (isolasi lebih keras).
- **Trade-off:** modular & rapi secara desain, tapi karena Mach+BSD digabung
  dalam satu ruang kernel, ia **bukan mikrokernel murni** — kompromi
  performa-vs-kemurnian.

### 2.3 Windows — *NT, hibrida berlapis*
Mode kernel berisi: **HAL** (Hardware Abstraction Layer), **Kernel** kecil
(penjadwalan tingkat rendah, sinkronisasi, interrupt), dan **Executive**
(`ntoskrnl.exe`) berisi subsistem:
- **Object Manager** — *semua* sumber daya (proses, file, event, port) adalah
  **objek** ber-handle dengan ACL seragam → model keamanan & penamaan konsisten.
- **Memory Manager** — virtual memory + **working sets** per-proses.
- **I/O Manager** — model driver berlapis (**WDM/WDF**, filter/minifilter).
- **Cache Manager, ALPC, Security Reference Monitor, Process/Config Manager.**
- **Subsistem lingkungan** di user-mode (Win32, dulu POSIX) → satu kernel,
  banyak "kepribadian" API.
- **Trade-off:** desain objek yang sangat bersih & seragam; kompleks dan besar.

### 2.4 seL4 — *mikrokernel berkapabilitas, terverifikasi formal*
Inti **sangat kecil**: hanya IPC, penjadwalan, dan VM dasar. Driver &
filesystem jadi **server di user-space**.
- **Capability-based security:** akses ke objek butuh **capability** —
  token tak-terpalsukan berisi izin spesifik. Tak ada "ambient authority".
- **Terverifikasi formal:** bukti fungsi-benar dari spesifikasi → kode C
  (Isabelle/HOL) — jaminan keamanan/keandalan terkuat di kelasnya.
- **Tren 2025:** isolasi driver (mis. driver ethernet **Rust** terisolasi penuh),
  multikernel, pengerasan side-channel.
- **Trade-off:** isolasi & jaminan tertinggi; butuh IPC sangat cepat agar
  performa layak (historisnya kelemahan mikrokernel).

### Tabel keputusan

| Dimensi | Linux | XNU (macOS) | Windows NT | seL4 | **THUOS (pilihan)** |
|---|---|---|---|---|---|
| Model kernel | Monolitik+modul | Hibrida (Mach+BSD) | Hibrida berlapis | Mikrokernel | **Hibrida ramping berkapabilitas** |
| Driver | In-kernel (LKM) | IOKit (OO) | WDM/WDF | User-space server | **Isolasi bertahap; jalur driver aman (Rust/WASM) di luar inti** |
| Keamanan | uid+MAC+eBPF | MAC/ACL+exclaves | Object+ACL+SRM | **Capability** | **Capability sejak awal** |
| Memori | buddy+SLUB+VMA | Mach VM | VMM+working set | VM minimal | **PMM→heap→paging→VM space (sedang dibangun)** |
| IPC | syscalls/pipes/futex | port/pesan Mach | ALPC | endpoint capability | **Pesan ber-capability** |
| Filosofi | "semua ada di kernel" | Unix di atas Mach | objek seragam | "minimal & terbukti" | **minimal, jujur, milik pengguna** |

---

## 3. Pelajaran lintas-OS (yang THUOS ambil)

1. **Inti kecil menang untuk keandalan** (seL4) — tapi **IPC harus ngebut**
   supaya tidak kalah performa. → THUOS: inti ramping, IPC cepat, layanan besar
   bisa di luar inti seiring waktu.
2. **Abstraksi seragam itu kekuatan** (VFS Linux, Object Manager NT). → THUOS:
   satu model objek + handle untuk sumber daya, satu lapisan VFS.
3. **Keamanan harus desain inti, bukan tambalan** (capability seL4 > bolt-on
   uid). → THUOS: **capability dari hari pertama**, no ambient authority.
4. **Isolasi driver mengurangi 70%+ penyebab crash** (driver = sumber bug OS
   terbesar; tren Rust/seL4). → THUOS: driver di jalur yang bisa diisolasi.
5. **Pisahkan mekanisme dari kebijakan** (Mach menyediakan VM, BSD pakai). →
   THUOS: core menyediakan mekanisme, layanan menentukan kebijakan.
6. **Modularitas tanpa reboot** (LKM). → THUOS: modul/driver dinamis nanti.
7. **Bukti & uji, bukan klaim** (seL4 verifikasi formal). → THUOS: tiap fitur
   **host-tested**; boot **harus** diverifikasi di QEMU/hardware sebelum diklaim.

---

## 4. Arsitektur THUOS

### 4.1 Prinsip ("pemikiran THUOS")
- **Local-first & privacy-first** — milik pengguna, bukan cloud. Tidak ada
  telemetri diam-diam; data tinggal di perangkat.
- **Capability-secured** — wewenang itu eksplisit & bisa dicabut.
- **Minimal di inti, kaya di tepi** — inti kecil yang bisa dipercaya.
- **Honest engineering** — label COMPILE-ONLY / HOST-TESTED / BOOT-VERIFIED
  selalu jujur.

### 4.2 Lapisan
```
┌─────────────────────────────────────────────────────────────┐
│  USERSPACE  apps · THU Desktop (Aurora) · thupkg · libc      │  ring 3
├─────────────────────────────────────────────────────────────┤
│  LAYANAN    VFS/THUFS · driver terisolasi · jaringan         │  (bertahap dipindah keluar inti)
├─────────────────────────────────────────────────────────────┤
│  THU CORE   capability+IPC · scheduler · VM/paging · heap    │  ring 0, kecil
│             PMM · GDT/IDT · IRQ/PIT · trap                   │
├─────────────────────────────────────────────────────────────┤
│  HAL        abstraksi CPU/arch (i386 dulu, lalu x86-64/ARM)  │
└─────────────────────────────────────────────────────────────┘
```
> Belajar dari **HAL Windows** (port arch bersih) + **inti kecil seL4** +
> **objek seragam NT** + **VFS Linux** + **IOKit XNU** (driver OO terisolasi).

### 4.3 Manajemen memori
> **Belajar dari:** buddy+SLUB (Linux), Mach VM (XNU), working set (NT).
> **Pilihan THUOS:** PMM (bitmap 4 KiB, **selesai**) → kernel heap (**selesai**)
> → **paging + ruang alamat virtual per-proses** (sedang dibangun) → demand
> paging + copy-on-write (nanti). **Alasan:** isolasi & overcommit butuh paging;
> ini fondasi semua hal lain.

### 4.4 Proses & penjadwalan
> **Belajar dari:** EEVDF (Linux), thread Mach (XNU). **Pilihan THUOS:** proses
> dengan ruang alamat terpisah; scheduler preemptif sederhana dulu (round-robin
> berbasis PIT), naik ke fair-scheduler. **Alasan:** mulai benar, optimalkan
> kemudian.

### 4.5 Keamanan — capability
> **Belajar dari:** seL4 (capability) > model uid+ACL. **Pilihan THUOS:** setiap
> sumber daya diwakili **objek**; akses lewat **capability** ber-izin yang bisa
> dicabut; **no ambient authority**. **Alasan:** mencegah seluruh kelas
> privilege-escalation by design.

### 4.6 Driver & isolasi
> **Belajar dari:** driver = penyebab crash OS terbesar; IOKit & Rust-on-seL4.
> **Pilihan THUOS:** core menyediakan akses perangkat lewat capability; driver
> di **jalur yang bisa diisolasi** (user-space/server, atau bahasa aman). Inti
> tidak ikut jatuh saat driver gagal. **Alasan:** keandalan.

### 4.7 IPC
> **Belajar dari:** port/pesan Mach, ALPC NT, endpoint seL4. **Pilihan THUOS:**
> **pesan ber-capability** antar proses; sinkron cepat untuk panggilan layanan.
> **Alasan:** IPC adalah urat nadi desain hibrida-ramping.

### 4.8 Filesystem
> **Belajar dari:** VFS Linux + Object Manager NT. **Pilihan THUOS:** lapisan
> **VFS** seragam; **THUFS** (lokal, terenkripsi opsional) + initrd read-only
> dulu. **Alasan:** abstraksi seragam = banyak FS, satu API.

### 4.9 Syscall ABI & userspace
> **Belajar dari:** subsistem lingkungan NT, POSIX BSD. **Pilihan THUOS:** ABI
> syscall kecil & stabil (capability-aware), lalu port **libc** (newlib/musl)
> agar program nyata jalan. **Alasan:** ekosistem butuh ABI stabil.

---

## 5. Peta ke kenyataan (jujur) & roadmap

| Lapisan/subsistem | Status hari ini | Bukti |
|---|---|---|
| HAL/boot (Multiboot, GDT/IDT, IRQ/PIT, serial/VGA) | **Selesai (host/compile)** | `kernel/arch`, `kernel/boot` |
| Trap & exception 0–31, panic | **Selesai** | `kernel/arch/x86/isr.c` |
| Shell interaktif (24 perintah) | **Selesai** | `kernel/shell/shell.c` |
| PMM (bitmap 4 KiB) | **Selesai + host-tested** | `kernel/mm/pmm.c`, `tests/test_pmm.c` |
| Kernel heap (kmalloc/kfree) | **Selesai + host-tested** | `kernel/mm/kheap*.c`, `tests/test_kheap.c` |
| **Paging / VM space** | **Sedang dibangun** (tabel + translasi host-tested; *enable* belum boot-verified) | `kernel/mm/vmm*` |
| Proses + scheduler | Dirancang | §4.4 |
| Capability + IPC | Dirancang | §4.5, §4.7 |
| Driver terisolasi | Dirancang | §4.6 |
| VFS + THUFS | Dirancang | §4.8 |
| Userspace + syscall + libc | Dirancang | §4.9 |

Urutan (dependency-ordered): **paging → proses/scheduler → ring 3 + syscall →
VFS → driver → jaringan → libc → userland**. Langkah paling berisiko: paging
(triple-fault diam) dan ring-3 (TSS/privilege).

---

## 6. Kejujuran
- **HOST-TESTED:** PMM, kernel heap, logika paging, scheduler (di `make test`).
- **BOOT-VERIFIED: ya — di QEMU via CI.** Job `boot-smoke` mem-boot kernel tiap
  push hingga prompt `thuos>` (terverifikasi lewat serial COM1). Belum di
  hardware fisik; sandbox dev ini sendiri tanpa QEMU (jadi di sini skip).
- Boot sendiri: `BOOT_THUOS.md`. Dokumen ini diperbarui saat subsistem terbukti.

---

## Sumber
- Linux kernel architecture — TheLinuxVault, CircuitLabs, abhik.ai:
  <https://www.thelinuxvault.net/linux-kernel-basics/linux-kernel-architecture-explained/> ·
  <https://circuitlabs.net/the-linux-kernel-high-level-architecture-overview/>
- XNU / Darwin — tansanrao.com, mac-monitor wiki, The Register (security exclaves):
  <https://tansanrao.com/blog/2025/04/xnu-kernel-and-darwin-evolution-and-architecture/> ·
  <https://github.com/redcanaryco/mac-monitor/wiki/3.-macOS-System-Architecture> ·
  <https://www.theregister.com/2025/03/08/kernel_sanders_apple_rearranges_xnu/>
- Windows NT architecture — dlab.epfl.ch (Wikipedia mirror), itprotoday:
  <https://dlab.epfl.ch/wikispeedia/wpcd/wp/a/Architecture_of_Windows_NT.htm> ·
  <https://www.itprotoday.com/it-infrastructure/windows-nt-architecture-part-2>
- seL4 — sel4.systems, formal verification paper:
  <https://sel4.systems/> ·
  <https://sel4.systems/Research/pdfs/comprehensive-formal-verification-os-microkernel.pdf>
