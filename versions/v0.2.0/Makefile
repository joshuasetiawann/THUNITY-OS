# THUOS — kernel build system.
#
# Targets:
#   make kernel      Build build/kernel.elf (default)
#   make verify      Build, then check the ELF + Multiboot header honestly
#   make iso         Build a bootable ISO (needs grub-mkrescue + xorriso)
#   make run         Boot in QEMU (needs qemu-system-i386)
#   make run-serial  Boot in QEMU with serial on stdio
#   make demo        Serve the THU Desktop web demo on http://localhost:8080
#   make clean       Remove build artifacts

CC      := gcc
LD      := gcc

INCLUDES := -Ikernel/lib -Ikernel/arch/x86 -Ikernel/core \
            -Ikernel/drivers -Ikernel/shell \
            -Ikernel/include -Ikernel/include/thuos

# Freestanding 32-bit kernel. We provide our own mem*/str* so we disable the
# builtin recognition and loop-idiom rewrites that would otherwise recurse.
CFLAGS  := -m32 -std=gnu11 -ffreestanding -O2 \
           -fno-stack-protector -fno-pic -fno-pie \
           -fno-builtin -fno-tree-loop-distribute-patterns \
           -Wall -Wextra -Wno-unused-parameter $(INCLUDES)

ASFLAGS := -m32 -ffreestanding -fno-pic -fno-pie $(INCLUDES)

# --build-id=none keeps the Multiboot header in the first 8 KiB of the file
# (a build-id note would otherwise be emitted before .text).
LDFLAGS := -m32 -ffreestanding -nostdlib -no-pie -static \
           -Wl,--build-id=none -T linker.ld

BUILD   := build
OBJDIR  := $(BUILD)/obj
KERNEL  := $(BUILD)/kernel.elf

SRCS_C  := $(shell find kernel -name '*.c' | sort)
SRCS_S  := $(shell find kernel -name '*.S' | sort)
OBJS    := $(patsubst kernel/%,$(OBJDIR)/%,$(SRCS_C:.c=.o)) \
           $(patsubst kernel/%,$(OBJDIR)/%,$(SRCS_S:.S=.o))

.PHONY: all kernel verify status iso run run-serial demo clean

all: kernel

kernel: $(KERNEL)

$(OBJDIR)/%.o: kernel/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.o: kernel/%.S
	@mkdir -p $(dir $@)
	$(CC) $(ASFLAGS) -c $< -o $@

$(KERNEL): $(OBJS) linker.ld
	@mkdir -p $(BUILD)
	$(LD) $(LDFLAGS) $(OBJS) -o $@
	@echo "==> Linked $@"

verify: kernel
	@bash scripts/run_verify.sh

status:
	@bash scripts/status.sh

iso: kernel
	@if command -v grub-mkrescue >/dev/null 2>&1 && command -v xorriso >/dev/null 2>&1; then \
	  mkdir -p $(BUILD)/iso/boot/grub; \
	  cp $(KERNEL) $(BUILD)/iso/boot/kernel.elf; \
	  cp grub.cfg $(BUILD)/iso/boot/grub/grub.cfg; \
	  grub-mkrescue -o $(BUILD)/thuos.iso $(BUILD)/iso; \
	  echo "==> ISO: $(BUILD)/thuos.iso"; \
	else \
	  echo "[skip] grub-mkrescue/xorriso not installed; cannot build a real ISO here."; \
	  echo "       Install with: sudo apt-get install grub-pc-bin grub-common xorriso mtools"; \
	fi

run: kernel
	@if command -v qemu-system-i386 >/dev/null 2>&1; then \
	  qemu-system-i386 -kernel $(KERNEL); \
	else \
	  echo "[skip] qemu-system-i386 not installed; cannot boot THUOS here."; \
	  echo "       Install with: sudo apt-get install qemu-system-x86"; \
	fi

run-serial: kernel
	@if command -v qemu-system-i386 >/dev/null 2>&1; then \
	  qemu-system-i386 -kernel $(KERNEL) -serial stdio -display none; \
	else \
	  echo "[skip] qemu-system-i386 not installed; cannot boot THUOS here."; \
	  echo "       Install with: sudo apt-get install qemu-system-x86"; \
	fi

demo:
	@echo "==> THU Desktop demo: http://localhost:8080/preview/thuos_preview.html"
	@python3 -m http.server 8080

clean:
	rm -rf $(BUILD)
	@echo "==> Cleaned $(BUILD)"
