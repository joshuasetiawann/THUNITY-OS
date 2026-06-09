#!/usr/bin/env bash
# THUOS — QEMU boot smoke-test.
#
# Unlike the host unit tests (which check pure logic with native gcc), this
# actually BOOTS the kernel on an emulated i386 and reads COM1 serial output,
# then checks for the boot markers the kernel prints on the way up to its shell.
# This is real boot verification. It is wired into CI (which installs QEMU), and
# skips gracefully when QEMU is not present (e.g. this dev sandbox).
#
# Usage:  scripts/boottest.sh [path/to/kernel.elf]
# Env:    BOOT_TIMEOUT=seconds (default 25)
set -u

KERNEL="${1:-build/kernel.elf}"
TIMEOUT="${BOOT_TIMEOUT:-25}"

if ! command -v qemu-system-i386 >/dev/null 2>&1; then
  echo "[skip] qemu-system-i386 not installed; cannot boot-verify here."
  echo "       install: sudo apt-get install qemu-system-x86"
  exit 0
fi
if [ ! -f "$KERNEL" ]; then
  echo "boottest: $KERNEL not found (run 'make kernel' first)"
  exit 1
fi

LOG="$(mktemp)"
echo "==> Booting $KERNEL in QEMU (COM1 serial, ${TIMEOUT}s watchdog)"
# The kernel idles in its shell read-loop, so it never exits on its own; the
# timeout stops QEMU after the boot output has been captured. -no-reboot makes a
# triple-fault halt (and fail the test) instead of looping silently.
timeout "${TIMEOUT}s" qemu-system-i386 \
  -kernel "$KERNEL" \
  -serial "file:$LOG" \
  -display none -no-reboot >/dev/null 2>&1 || true

echo "----- serial output -----"
cat "$LOG" 2>/dev/null || true
echo "-------------------------"

rc=0
for marker in "THUOS" "Physical memory manager" "Kernel heap" "Paging ENABLED" "Scheduler" "thuos>"; do
  if grep -qF "$marker" "$LOG" 2>/dev/null; then
    echo "  [ ok ] saw: $marker"
  else
    echo "  [FAIL] missing marker: $marker"
    rc=1
  fi
done
rm -f "$LOG"

echo "-------------------------"
if [ "$rc" -eq 0 ]; then
  echo "==> BOOT-VERIFIED: THUOS booted to the shell prompt (verified over serial)."
else
  echo "==> BOOT TEST FAILED: kernel did not reach the expected boot state."
fi
exit "$rc"
