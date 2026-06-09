# THUOS — Apakah Ini "OS Masa Depan"? Laporan Deep-Research

**Tanggal:** 2026-06-06 · **Fokus (pilihanmu):** kriteria *real OS* teknis + jalur menuju OS yang **benar-benar bisa dipakai**.

---

## Catatan metode & kejujuran
Riset dijalankan live lewat 5 agen paralel (WebSearch). **`WebSearch` berfungsi**; **`WebFetch` diblokir 403 global** di lingkungan ini — jadi klaim bersandar pada kutipan hasil-pencarian dari sumber otoritatif (konsisten lintas-sumber, **bukan** baca halaman penuh). Angle *definisi real-OS*, *arsitektur*, *Rust/verifikasi*, *UI baru* = bukti live. *Roadmap teknis* = praktik OS-dev baku + sumber kanonik yang dinamai. Kondisi THUOS = dari pekerjaan langsung. Tidak ada URL yang dikarang. Yang **established** vs **spekulatif** dipisahkan.

---

## 1. Vonis jujur: **belum**
Bedakan tiga tingkat:
- **Kernel bootable nyata** — image ELF Multiboot yang bisa dieksekusi CPU.
- **OS sungguhan** — kernel + kemampuan inti yang **berjalan**: context switch nyata, memori virtual + isolasi MMU, filesystem, driver, syscall yang dipanggil userspace, dst.
- **OS masa depan** — di atas "real OS" + ide frontier (microkernel/capability, memory-safety, verifikasi formal, local-first/AI-native).

**THUOS hari ini = kernel from-scratch yang jujur + subsistem host-tested, di tahap PRA-boot.** Belum "real OS", apalagi "masa depan".
- **Istimewa:** dibangun dari nol, jujur, tiap subsistem **host-tested** (deep-verify 57/0, 27 suite, 0 warning).
- **Belum jadi OS:** **belum pernah di-boot** (tak ada QEMU di env ini), **belum ada context switch nyata**, paging & ring-3 **gated off** (jadi belum ada isolasi memori — usercopy hanya *range-policy*, bukan proteksi MMU), filesystem **read-only**, driver cuma keyboard/PIT/serial, **tanpa** networking/libc/POSIX, userspace **dimuat tapi tak dieksekusi**, tak self-hosting.

Arsitekturnya benar & logikanya teruji — tapi **kemampuan inti OS belum hidup**.

---

## 2. Apa yang membuat sesuatu "real OS" — checklist (bersumber) → THUOS
Kernel = inti OS yang "selalu punya kontrol penuh atas sistem", tapi **kernel ≠ OS**; OS = kernel + userspace (UI, libc, init, services) ([Wikipedia: Kernel](https://en.wikipedia.org/wiki/Kernel_(operating_system))). Tugas wajib kernel: proses+penjadwalan, memori, I/O & driver, arbitrase sumber daya ([OSDev: Creating an OS](https://wiki.osdev.org/Creating_an_Operating_System)). Rantai boot: firmware → bootloader → kernel → userspace ([Wikipedia: Firmware](https://en.wikipedia.org/wiki/Firmware)).

| Kemampuan inti OS | Sumber | Status THUOS |
|---|---|---|
| Boot di HW/VM nyata | ([Firmware](https://en.wikipedia.org/wiki/Firmware)) | ❌ belum di-boot (no QEMU) |
| Memori virtual + paging + **isolasi MMU** | MMU translasi + cek izin ([ScienceDirect: MMU](https://www.sciencedirect.com/topics/computer-science/memory-management-unit)) | ⚠️ tabel host-tested, **enable gated off**; isolasi belum ada |
| **Context switch** nyata (save/restore via PCB) | ([Wikipedia: Context switch](https://en.wikipedia.org/wiki/Context_switch)) | ❌ scheduler hanya **model** |
| Ring 0/3 user vs kernel benar-benar jalan | ([GeeksforGeeks](https://www.geeksforgeeks.org/user-mode-and-kernel-mode-switching/)) | ⚠️ struktur ada, **gated off** |
| **Syscall** dipanggil userspace (ABI) | ([System V ABI](https://wiki.osdev.org/System_V_ABI)) | ⚠️ tabel host-tested; gerbang int 0x80 belum dipasang |
| **Filesystem writable** + storage | ([Silberschatz Ch.9](https://codex.cs.yale.edu/avi/os-book/OS8/os8e/slide-dir/PDF-dir/ch9.pdf)) | ⚠️ VFS/ramfs **read-only**; tak ada disk |
| **Driver** + model driver | ([Wikipedia: Kernel](https://en.wikipedia.org/wiki/Kernel_(operating_system))) | ⚠️ cuma keyboard/PIT/serial |
| **IPC** | ([OSDev: IPC](https://wiki.osdev.org/Category:IPC)) | ❌ belum ada |
| **Networking** (TCP/IP) | ([Monolithic](https://en.wikipedia.org/wiki/Monolithic_kernel)) | ❌ belum ada |
| Userland + init + libc/POSIX | ([Wikipedia: Kernel](https://en.wikipedia.org/wiki/Kernel_(operating_system))) | ⚠️ init memuat /bin/init (READY), tak jalan; tak ada libc |
| Self-hosting | tonggak kematangan | ❌ belum |

**Punya/teruji:** boot stub, GDT/IDT, PMM, heap A, THUAR/VFS RO, validator ELF32 (terima ELF gcc nyata, entry 0x400000), model proses/scheduler/fd/gfx/desktop — semua host-tested.

---

## 3. Roadmap dependency-ordered → OS yang bisa dipakai
*(urutan kanonik OS-dev: [OSDev: What order should I make things in?](https://wiki.osdev.org/What_order_should_I_make_things_in%3F); rujukan: [Going Further on x86](https://wiki.osdev.org/Going_Further_on_x86), [MIT xv6 book](https://pdos.csail.mit.edu/6.828/2023/xv6/book-riscv-rev3.pdf), [os.phil-opp](https://os.phil-opp.com/), [littleosbook](https://littleosbook.github.io/), [OSTEP](https://pages.cs.wisc.edu/~remzi/OSTEP/); contoh utuh: [httpe/simple-os](https://github.com/httpe/simple-os))*

1. **Boot-verify di QEMU** (lalu hardware). Tanpa ini, semua "host-tested" belum tervalidasi runtime. Risiko: triple-fault diam (reboot-loop tanpa pesan). → harness `make boot-test` sudah siap.
2. **Aktifkan paging + isolasi.** Page directory, identity-map kernel/VGA/initrd/stack, set CR3, CR0.PG, **#PF handler**; lalu higher-half. **Tersulit #1** bagi pemula; prasyarat semua isolasi. (ref: [os.phil-opp Paging](https://os.phil-opp.com/paging-introduction/).)
3. **Context switch + scheduler preemptive nyata.** Save/restore register+stack per-proses, didorong timer. Risiko: race, stack korup, register bocor antar-proses. (xv6 `swtch` + [OSTEP](https://pages.cs.wisc.edu/~remzi/OSTEP/) = referensi minimal terbaik.)
4. **Ring 3 + syscall hidup.** TSS (ltr), gerbang `int 0x80` DPL-3, iret ke user, fault policy (bunuh proses, bukan kernel). **Lompatan ring-3 pertama = tersulit #2** (RPL/DPL/TSS salah → #GP/triple-fault). (ref: [OSDev: Getting to Ring 3](https://wiki.osdev.org/Getting_to_Ring_3).)
5. **Filesystem writable + driver blok.** Driver: [ATA PIO](https://wiki.osdev.org/ATA_PIO_Mode) → [AHCI](https://wiki.osdev.org/AHCI)/[virtio](https://wiki.osdev.org/Virtio); lalu FS R/W (FAT/ext2) di atas [VFS](https://wiki.osdev.org/VFS). **Tersulit #3: konsistensi saat crash** (urutan tulis, journaling) — [OSTEP Ch.42](https://pages.cs.wisc.edu/~remzi/OSTEP/file-journaling.pdf). Tanpa ini OS tak bisa "menyimpan".
6. **libc + POSIX.** Port **[newlib](https://wiki.osdev.org/Porting_Newlib)** (atau musl) → program nyata bisa jalan; pintu ke ekosistem app.
7. **Networking (TCP/IP).** Driver NIC (e1000/virtio-net) + stack ([lwIP](https://wiki.osdev.org/Network_Stack)/sendiri).
8. **Userland + shell + init**, lalu **self-hosting** (toolchain jalan di atas THUOS, ala Linux From Scratch). **Tersulit #4: bug codegen saat compiler meng-compile dirinya sendiri.**
9. **SMP/multi-core**, lalu fitur masa depan (§4).

**Empat langkah paling berisiko** (sumber sepakat): (1) enable-paging pertama + higher-half, (2) lompatan ring-3 pertama, (3) race di context switch, (4) konsistensi FS saat crash. Wajib berlapis: paging→ring-3→userspace nyata→FS+driver→app→libc→ekosistem.

---

## 4. Apa yang membuatnya "masa depan" — pilihan arsitektur
- **Microkernel vs monolitik.** Microkernel = inti minimal (IPC, scheduling, address-space/thread) di ring 0; driver/FS/net di server user-space → TCB kecil + isolasi gangguan ([Microkernel](https://en.wikipedia.org/wiki/Microkernel)). Gap performa kini lebih kecil di HW modern — **masih diperdebatkan** ([dev.to](https://dev.to/adityabhuyan/monolithic-kernel-vs-microkernel-understanding-the-key-trade-offs-in-modern-operating-systems-23ln)).
- **seL4 (verifikasi formal, jujur batasnya).** Bukti fungsional ~**8.700 baris C** + *translation validation* ke biner (tak perlu percaya compiler) ([CACM](https://cacm.acm.org/research/sel4-formal-verification-of-an-operating-system-kernel/); [SOSP'09](https://www.sigops.org/s/conferences/sosp/2009/papers/klein-sosp09.pdf)). **Tapi** mengasumsikan hardware/assembly/boot benar & **tidak** menutup timing channel — jangan klaim "tak bisa diretas" ([Asumsi seL4](https://sel4.systems/Verification/assumptions.html)).
- **Capability vs ACL.** Capability = token tak-terpalsukan (subject-centric), mencegah "confused deputy" secara desain; ACL Unix object-centric ([Capability-based security](https://en.wikipedia.org/wiki/Capability-based_security); [MPI-SWS](https://people.mpi-sws.org/~dg/papers/csf16-caps.pdf)).
- **L4/Liedtke:** IPC ~**10–20×** lebih cepat dari Mach; L4 di miliaran perangkat; seL4 turunannya ([L4 family](https://en.wikipedia.org/wiki/L4_microkernel_family)).
- **Fuchsia/Zircon (Google):** dari nol, **bukan** Linux, ~**100 syscall**, **tanpa konsep "user"**, sumber daya = objek via capability, komponen update-independen; produksi di Nest Hub ([Fuchsia](https://en.wikipedia.org/wiki/Fuchsia_(operating_system))).
- **Unikernel:** app + pustaka OS seperlunya → satu image **single-address-space** di hypervisor; tanpa transisi user/kernel; attack surface kecil ([Unikernel](https://en.wikipedia.org/wiki/Unikernel)).
- **"Hybrid kernel"** (NT, XNU) sering dianggap **label marketing** untuk monolitik terstruktur ([Hybrid kernel](https://en.wikipedia.org/wiki/Hybrid_kernel)).
- **IOMMU:** blok serangan DMA, tapi praktiknya sering salah-konfig/bypassable (Thunderclap) ([MS Learn](https://learn.microsoft.com/en-us/windows/security/hardware-security/kernel-dma-protection-for-thunderbolt)).
- **Memory safety (Rust):** ~**70%** bug keamanan serius = memory-safety (Google/Microsoft) ([TechSpot/Google](https://www.techspot.com/news/85368-google-70-percent-serious-security-bugs-memory-safety.html)). **Redox OS** = microkernel Rust ([Redox FAQ](https://www.redox-os.org/faq/)) — **bukan peluru perak** (`unsafe` di batas HW, tak menutup bug logika/side-channel) ([Rust Book: unsafe](https://doc.rust-lang.org/book/ch20-01-unsafe-rust.html)). Rust masuk kernel Linux 6.1 (Okt 2022) ([Rust for Linux](https://en.wikipedia.org/wiki/Rust_for_Linux)).

**Saran realistis THUOS:** (1) isolasi MMU sungguhan (prasyarat) → (2) syscall ber-rasa capability + TCB kecil → (3) jangka jauh: microkernel-ish split dan/atau bagian aman ditulis Rust. **Hindari** menjanjikan seL4-style verified microkernel (upaya riset besar). Semua **setelah** dasar real-OS hidup.

---

## 5. Tampilan baru (tanpa meniru OS mana pun)
Desktop metaphor (Xerox PARC ~1970) kini dikritik "batasan tak perlu" di era cloud ([Desktop metaphor](https://en.wikipedia.org/wiki/Desktop_metaphor)). Arah orisinal:
- **ZUI / kanvas zoomable tak-terbatas** (Pad++, Jef Raskin **Archy/Zoomworld**): konten di kanvas, navigasi *incremental search*, bukan window/folder ([ZUI](https://en.wikipedia.org/wiki/Zooming_user_interface); [Archy](https://en.wikipedia.org/wiki/Archy_(software))).
- **Calm/ambient** (Weiser & Brown 1995): info di periferi, ke pusat saat perlu ([Calm technology](https://en.wikipedia.org/wiki/Calm_technology)).
- **Intent/natural-language** ([NLUI](https://en.wikipedia.org/wiki/Natural_language_user_interface)).
- **AI-native (spekulatif):** "LLM sebagai kernel OS" (Karpathy, [X](https://x.com/karpathy/status/1707437820045062561)); prototipe **AIOS** ([GitHub](https://github.com/agiresearch/AIOS)) — riset, bukan OS matang.
- **Local-first** (Ink & Switch 2019): data utama di perangkat pengguna + sync via **CRDT/Automerge** ([Local-first](https://www.inkandswitch.com/essay/local-first/)). **Paling selaras** dengan nilai THUOS — jadi UI baru yang jujur: bukan "desktop", tapi *ruang kerja milik-sendiri yang tenang* (ZUI + calm + local-first) — arah "Aurora" kita.

---

## Intinya
THUOS = **kernel from-scratch jujur + subsistem host-tested di tahap pra-boot** — bukan OS sungguhan (apalagi "masa depan") *hari ini*, tapi **fondasinya benar** & jalannya jelas. Langkah penentu (mengubah "model" → "OS hidup"): **boot-verify di QEMU → aktifkan paging → context switch → ring 3** (§3 langkah 1–4) di lingkungan ber-QEMU.

*Provenance: 5/5 angle live (definisi real-OS, roadmap teknis, arsitektur, Rust/verifikasi, UI baru). WebFetch 403-blocked; klaim dari kutipan WebSearch lintas-sumber otoritatif (bukan baca halaman penuh). BOOT-VERIFIED: tidak ada — tak ada yang di-boot di sini.*
