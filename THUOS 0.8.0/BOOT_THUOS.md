# Menjalankan THUOS sendiri (boot-verify)

> **Catatan jujur.** Di lingkungan pengembangan ini THUOS hanya **dikompilasi**
> (COMPILE-ONLY) — tidak ada QEMU di sini, jadi **BOOT-VERIFIED: tidak ada**.
> Dokumen ini menjelaskan cara **kamu** mem-boot-nya di mesinmu sendiri, satu-satunya
> tempat di mana "berhasil boot" bisa benar-benar dibuktikan. Begitu kamu menjalankan
> langkah di bawah dan melihat shell THUOS, **kamu** sudah memverifikasi boot.

THUOS adalah kernel i386 (32-bit) Multiboot 1 dari nol. Hasil build adalah
`build/kernel.elf` — sebuah ELF Multiboot yang bisa diboot langsung oleh QEMU
(tanpa perlu ISO) atau lewat GRUB di hardware nyata.

---

## 1. Yang dibutuhkan

Mesin Linux (atau WSL2 di Windows, atau VM Linux). Paket:

```bash
# Debian/Ubuntu
sudo apt-get update
sudo apt-get install build-essential gcc-multilib qemu-system-x86 \
                     grub-pc-bin grub-common xorriso mtools
```

- `gcc` + `gcc-multilib` → kompilasi freestanding `-m32`.
- `qemu-system-x86` → menyediakan `qemu-system-i386` untuk boot cepat.
- `grub-pc-bin` + `xorriso` + `mtools` → hanya untuk membuat **ISO** (opsional).

---

## 2. Build

```bash
make            # menghasilkan build/kernel.elf
make verify     # build + cek header Multiboot, simbol, ukuran section (jujur)
make test       # unit test di host (PMM + kernel heap), tanpa QEMU
```

`make verify` akan **jujur** memberitahu jika QEMU/GRUB tidak terpasang dan
menandai boot/ISO sebagai "NOT VERIFIED".

---

## 3. Boot di QEMU (paling cepat — pakai Multiboot, tanpa ISO)

```bash
make run            # qemu-system-i386 -kernel build/kernel.elf
# atau langsung:
qemu-system-i386 -kernel build/kernel.elf
```

Output ke jendela serial+VGA, atau khusus serial di terminal:

```bash
make run-serial     # qemu-system-i386 -kernel build/kernel.elf -serial stdio -display none
```

Keluar dari QEMU: tutup jendelanya, atau `Ctrl-A` lalu `X` (mode serial).

---

## 4. Bikin ISO (untuk USB / hardware nyata) — opsional

```bash
make iso            # menghasilkan build/thuos.iso (perlu grub-mkrescue + xorriso)
qemu-system-i386 -cdrom build/thuos.iso        # uji ISO di QEMU
```

**Menulis ke USB (BERBAHAYA — menghapus isi drive!).** Pastikan `/dev/sdX`
benar-benar USB-mu, bukan disk sistem:

```bash
sudo dd if=build/thuos.iso of=/dev/sdX bs=4M status=progress && sync
```

Lalu boot mesin dari USB itu. (Disarankan coba di QEMU/VM dulu.)

---

## 5. Yang akan kamu lihat

Log boot baris demi baris, lalu shell THUOS:

```
THUOS 0.4.0 "Kernel Heap"
THU Kernel - x86 (i386, 32-bit)
From-scratch OS foundation. Booting...

  [ OK ] Serial COM1 debug channel
  [ OK ] Global Descriptor Table
  [ OK ] Interrupt Descriptor Table
  [ OK ] CPU exception handlers (0-31)
  [ OK ] PIC remap + IRQ routing
  [ OK ] PIT timer @ 100 Hz
  [ OK ] PS/2 keyboard (IRQ1)
  [ OK ] Physical memory manager (4 KiB frames)
  [ OK ] Kernel heap (1 MiB fixed arena, kmalloc/kfree)

thuos> _
```

Perintah yang bisa dicoba:

```
help        daftar perintah
sysinfo     ringkasan sistem (timer, memori, build)
status      status subsistem yang jujur (done / plan)
mem         statistik physical memory manager
pages       statistik page-frame
allocpage   alokasikan 1 frame 4 KiB fisik
heap        statistik kernel heap (arena kmalloc)
kmalloc 256 alokasikan 256 byte dari heap → cetak alamatnya
kfree 0x..  bebaskan pointer kmalloc tadi
uptime      uptime dari PIT
crash div0  uji fault handler (sengaja men-trigger #DE)
reboot / halt
```

---

## 6. Status kejujuran

- **HOST-TESTED:** logika PMM dan kernel heap diuji dengan `make test` (gcc native).
- **COMPILE-ONLY (di sini):** `build/kernel.elf` ter-link sebagai ELF Multiboot i386,
  tapi **belum di-boot** di lingkungan pengembangan ini.
- **BOOT-VERIFIED:** **tidak ada di sini.** Akan menjadi terverifikasi **di mesinmu**
  begitu langkah §3/§4 berhasil menampilkan shell THUOS.
