#!/usr/bin/env bash
# THUOS — one-screen honest project status.
set -u
cd "$(dirname "$0")/.."

ver="0.3.0"
code="Memory Foundation"

cfiles=$(find kernel -name '*.c'  2>/dev/null | wc -l | tr -d ' ')
hfiles=$(find kernel -name '*.h'  2>/dev/null | wc -l | tr -d ' ')
sfiles=$(find kernel -name '*.S'  2>/dev/null | wc -l | tr -d ' ')
loc=$(find kernel -name '*.c' -o -name '*.h' -o -name '*.S' 2>/dev/null | xargs cat 2>/dev/null | wc -l | tr -d ' ')
docs=$(find docs -name '*.md' 2>/dev/null | wc -l | tr -d ' ')

if [ -f build/kernel.elf ]; then
  build="built  ($(file -b build/kernel.elf | cut -d, -f1-2))"
else
  build="not built  (run: make kernel)"
fi

qemu=$(command -v qemu-system-i386 >/dev/null 2>&1 && echo "available" || echo "MISSING (boot unverified here)")
iso=$(command -v grub-mkrescue >/dev/null 2>&1 && command -v xorriso >/dev/null 2>&1 && echo "available" || echo "MISSING (no ISO here)")

cat <<EOF
==================== THUOS STATUS ====================
 Project   : THUOS  (THU Kernel)
 Version   : ${ver} "${code}"   Arch: x86 (i386, 32-bit)   Boot: Multiboot 1
 Kernel    : ${build}
 Source    : ${cfiles} .c, ${hfiles} .h, ${sfiles} .S   (~${loc} lines)   Docs: ${docs} markdown
------------------------------------------------------
 Implemented : boot, VGA, serial, kprintf, GDT, IDT,
               exceptions 0-31, PIC, IRQ, PIT, keyboard,
               panic, shell (21 cmds), freestanding libc,
               physical memory manager (4 KiB frames)
 In progress : paging + kernel heap (designed, not built)
 Planned     : VFS/initrd, userspace, graphics, THU Desktop,
               thupkg backend, installer
------------------------------------------------------
 Toolchain   : qemu = ${qemu}
               iso  = ${iso}
 Verify      : make verify   ->  BUILD_VERIFICATION.txt
               make test     ->  page-frame allocator unit test
 Preview     : make demo     ->  preview/thuos_preview.html
======================================================
EOF
