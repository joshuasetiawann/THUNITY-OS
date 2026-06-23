#!/usr/bin/env python3
"""THUOS — verify a kernel ELF carries a valid Multiboot 1 header.

A compliant Multiboot kernel must place the header (magic 0x1BADB002) within
the first 8192 bytes of the file, 4-byte aligned, with magic+flags+checksum
summing to zero mod 2**32. This is an honest structural check; it does not
claim the kernel was booted.
"""
import struct
import sys

MAGIC = 0x1BADB002


def main(path: str) -> int:
    with open(path, "rb") as fh:
        blob = fh.read(8192)

    for off in range(0, len(blob) - 12, 4):
        magic, flags, checksum = struct.unpack_from("<III", blob, off)
        if magic != MAGIC:
            continue
        total = (magic + flags + checksum) & 0xFFFFFFFF
        if total == 0:
            print(f"  [ OK ] Multiboot header at file offset {off}")
            print(f"         magic=0x{magic:08X} flags=0x{flags:08X} "
                  f"checksum=0x{checksum:08X}")
            return 0
        print(f"  [WARN] header at {off} but checksum invalid (sum=0x{total:08X})")

    print("  [FAIL] No valid Multiboot header in first 8 KiB")
    return 1


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("usage: verify_multiboot.py <kernel.elf>")
        sys.exit(2)
    sys.exit(main(sys.argv[1]))
