#!/usr/bin/env bash
# THUOS — full build verification battery.
# Prints results to the terminal AND writes them to BUILD_VERIFICATION.txt.
# Every check here is real; nothing about QEMU boot or ISO is faked.
set -u
cd "$(dirname "$0")/.."

KERNEL="build/kernel.elf"
LOG="BUILD_VERIFICATION.txt"
PASS=0
FAIL=0

# Mirror all output to the log file.
exec > >(tee "$LOG") 2>&1

line() { printf '%s\n' "------------------------------------------------------------"; }
ok()   { echo "  [PASS] $1"; PASS=$((PASS+1)); }
no()   { echo "  [FAIL] $1"; FAIL=$((FAIL+1)); }
chk()  { if echo "$2" | grep -qiE "$3"; then ok "$1"; else no "$1 (got: $2)"; fi; }

echo "THUOS BUILD VERIFICATION"
line
echo "Generated : $(date -u '+%Y-%m-%d %H:%M:%SZ')"
echo "Host      : $(uname -srm)"
echo "Toolchain : gcc=$(gcc -dumpversion 2>/dev/null) ld=$(ld --version 2>/dev/null | head -1 | grep -oE '[0-9.]+$') make=$(make --version 2>/dev/null | head -1)"
echo "Helpers   : python3=$(python3 --version 2>&1 | grep -oE '[0-9.]+') node=$(node --version 2>/dev/null)"
line

if [ ! -f "$KERNEL" ]; then
  echo "  [FAIL] $KERNEL not found — run 'make kernel' first."
  exit 1
fi

echo "1) ELF identity — file(1)"
FILEOUT="$(file -b "$KERNEL")"
echo "    $FILEOUT"
chk "ELF is 32-bit"            "$FILEOUT" "ELF 32-bit"
chk "ELF is Intel 80386"       "$FILEOUT" "Intel 80386|i386"
chk "ELF is statically linked" "$FILEOUT" "statically linked"
line

echo "2) ELF header — readelf -h"
RH="$(readelf -h "$KERNEL" 2>/dev/null)"
echo "$RH" | grep -E 'Class|Machine|Type|Entry' | sed 's/^/    /'
chk "Class is ELF32"        "$(echo "$RH" | grep Class)"   "ELF32"
chk "Machine is Intel 80386" "$(echo "$RH" | grep Machine)" "Intel 80386"
chk "Type is EXEC"          "$(echo "$RH" | grep Type)"    "EXEC"
line

echo "3) Multiboot 1 header + checksum (first 8 KiB)"
if python3 scripts/verify_multiboot.py "$KERNEL"; then ok "Multiboot header + checksum valid"; else no "Multiboot header/checksum"; fi
line

echo "4) Required symbols — nm"
for s in _start kernel_main isr_handler irq_handler shell_run; do
  if nm "$KERNEL" | grep -qE "[Tt] $s$"; then echo "    [ ok ] $s"; ok "symbol $s present";
  else echo "    [miss] $s"; no "symbol $s"; fi
done
line

echo "5) Section sizes — size(1)"
size "$KERNEL" | sed 's/^/    /'
line

echo "NOT VERIFIED IN THIS ENVIRONMENT (honest):"
command -v qemu-system-i386 >/dev/null 2>&1 \
  && echo "  - QEMU present: you may run 'make run' to boot." \
  || echo "  - QEMU boot: qemu-system-i386 not installed here, so boot is NOT verified."
command -v grub-mkrescue >/dev/null 2>&1 && command -v xorriso >/dev/null 2>&1 \
  && echo "  - ISO tools present: you may run 'make iso'." \
  || echo "  - Bootable ISO: grub-mkrescue/xorriso not installed here, so no ISO was built."
line

echo "SUMMARY: $PASS passed, $FAIL failed."
if [ "$FAIL" -eq 0 ]; then echo "RESULT : VERIFICATION PASSED (build + structure)."; else echo "RESULT : VERIFICATION FAILED."; fi
line
[ "$FAIL" -eq 0 ]
