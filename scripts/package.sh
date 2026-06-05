#!/usr/bin/env bash
# THUOS — build a clean milestone release archive.
# Includes source, docs, preview, scripts, Makefile, and the verification log.
# Excludes the build cache and any temporary server files.
set -eu
cd "$(dirname "$0")/.."

NAME="thuos_milestone_0_3"
OUT="${NAME}.tar.gz"

# Refresh verification log so the archive carries current, real results.
make verify >/dev/null 2>&1 || true

rm -f "$OUT"
tar czf "$OUT" \
  --transform "s,^,${NAME}/," \
  README.md PROJECT_STATUS.md CHANGELOG.md BUILD_VERIFICATION.txt \
  Makefile linker.ld grub.cfg .gitignore \
  kernel tests docs preview scripts

echo "Created $OUT ($(du -h "$OUT" | cut -f1))"
