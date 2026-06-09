# THUOS — Visi & Positioning

> Pernyataan resmi arah proyek, diturunkan dari `THUOS_REALITY_CHECK.md`
> (riset mendalam bersumber). Singkat sengaja: ini kompas, bukan ensiklopedia.

## Satu kalimat
**THUOS adalah OS-terminal untuk developer & AI-agent: capability-secured,
local-first, boot kilat di dalam VM/microVM — bukan untuk mengganti Windows,
tapi untuk jadi tempat kerja paling jujur, privat, dan terisolasi.**

## Kenapa begini (bukan "saingi Windows")
Riset menunjukkan menyaingi Windows/macOS head-on = ~60.000 person-year /
~$10,8 miliar, dan **tak ada preseden** OS from-scratch menggusur inkumben.
Yang membunuh OS hobi adalah **driver** dan **keluasan**. Maka THUOS memilih
jalan yang bisa dimenangkan satu orang/ tim kecil:

1. **Jalan di VM (virtio) → masalah driver hilang.** Kita hanya mendukung
   hardware virtual yang stabil (QEMU/KVM/Firecracker), bukan zoo hardware nyata.
2. **Terminal/dev-env sebagai wedge.** Domain terbatas & jelas; terbukti bisa
   dibangun solo (Ghostty dkk). Celah baru = lapisan agent.
3. **Menang pada NILAI, bukan jumlah fitur.** Privasi, local-first, isolasi
   kapabilitas, dan *honest engineering* — hal yang inkumben sulit tiru karena
   terjebak warisan (POSIX, ambient authority, kompatibilitas mundur).

## Prinsip
- **Honest engineering** — tiap klaim berlabel jujur: COMPILE-ONLY /
  HOST-TESTED / BOOT-VERIFIED. Tak ada yang dibesar-besarkan.
- **Local-first & privacy-first** — data milik pengguna, di perangkat; tanpa
  telemetri diam-diam.
- **Capability-secured** — wewenang eksplisit & bisa dicabut; no ambient authority.
- **Minimal di inti, kaya di tepi** — inti kecil yang bisa dipercaya/diuji.
- **Developer-first** — diadopsi dari bawah; "time to first value" cepat.

## Target pengguna (beachhead)
Developer dan operator AI-agent yang butuh **sandbox reproducible, terisolasi,
dan privat** untuk menjalankan tool/agent — lalu meluas ke tim & infrastruktur
agent. Bukan "semua orang"; itu justru kekuatannya (pelajaran Qubes/GrapheneOS/Tails).

## Non-goals (sengaja TIDAK dikejar)
- Driver GPU/Wi-Fi/dukungan hardware fisik luas.
- Mesin browser sendiri.
- Paritas Win32 / menjalankan aplikasi Windows-macOS.
- Menjadi "daily driver desktop untuk semua orang".

Semua itu jebakan person-year yang sudah membunuh OS lain.

## Bagaimana kita mengukur kemajuan (definition of done)
Sebuah fitur "selesai" hanya jika:
1. **Host-tested** (logika murni lulus `make test`), DAN
2. **Boot-verified** di QEMU (muncul di smoke-test serial CI) bila menyangkut runtime.

Status hari ini: v0.6.1 — PMM, heap, tabel paging (staged), scheduler policy core
(semua host-tested) dan **boot-verified di QEMU**: job CI `boot-smoke` mem-boot
kernel tiap push hingga mencapai prompt `thuos>` (terverifikasi via serial).

## North-star 6–24 bulan
Boot-verify di QEMU → ring 3 + syscall berkapabilitas → terminal sungguhan di VM
→ penyimpanan local-first → sandbox dev/agent yang bisa dibagikan sebagai image
microVM. Lihat roadmap rinci di `THUOS_REALITY_CHECK.md` §6 dan arsitektur di
`THUOS_ARCHITECTURE.md`.
