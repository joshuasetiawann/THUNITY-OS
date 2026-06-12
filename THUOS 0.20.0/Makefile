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
            -Ikernel/drivers -Ikernel/drivers/usb -Ikernel/shell -Ikernel/mm -Ikernel/sched -Ikernel/fs \
            -Ikernel/ai -Ikernel/os -Ikernel/gui -Ikernel/include -Ikernel/include/thuos

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

.PHONY: all kernel verify status test stress deep-verify scan package export boottest iso run run-serial demo clean

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

# Host-side unit tests (native gcc, no QEMU needed): page-frame allocator
# and kernel heap. These compile the same core .c files the kernel uses.
test:
	@mkdir -p $(BUILD)
	@gcc -O2 -std=gnu11 -Wall -Wextra -o $(BUILD)/test_pmm tests/test_pmm.c
	@./$(BUILD)/test_pmm
	@gcc -O2 -std=gnu11 -Wall -Wextra -o $(BUILD)/test_kheap tests/test_kheap.c
	@./$(BUILD)/test_kheap
	@gcc -O2 -std=gnu11 -Wall -Wextra -o $(BUILD)/test_vmm tests/test_vmm.c
	@./$(BUILD)/test_vmm
	@gcc -O2 -std=gnu11 -Wall -Wextra -o $(BUILD)/test_sched tests/test_sched.c
	@./$(BUILD)/test_sched
	@gcc -O2 -std=gnu11 -Wall -Wextra -o $(BUILD)/test_task tests/test_task.c
	@./$(BUILD)/test_task
	@gcc -O2 -std=gnu11 -Wall -Wextra -o $(BUILD)/test_fs tests/test_fs.c
	@./$(BUILD)/test_fs
	@gcc -O2 -std=gnu11 -Wall -Wextra -o $(BUILD)/test_syscall tests/test_syscall.c
	@./$(BUILD)/test_syscall
	@gcc -O2 -std=gnu11 -Wall -Wextra -o $(BUILD)/test_usermode tests/test_usermode.c
	@./$(BUILD)/test_usermode
	@gcc -O2 -std=gnu11 -Wall -Wextra -o $(BUILD)/test_ai tests/test_ai.c
	@./$(BUILD)/test_ai
	@gcc -O2 -std=gnu11 -Wall -Wextra -o $(BUILD)/test_features tests/test_features.c
	@./$(BUILD)/test_features

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

# Real boot verification: boot in QEMU, capture COM1 serial, check boot markers.
boottest: kernel
	@bash scripts/boottest.sh $(KERNEL)

demo:
	@echo "==> THU Desktop demo: http://localhost:8080/preview/thuos_os.html"
	@python3 -m http.server 8080

# Stress: re-run the already-built host test binaries many times to shake out
# any nondeterminism in the pure cores. Non-destructive (no Docker, no volumes).
stress: test
	@echo "==> stress: re-running host test binaries 50x"
	@for i in $$(seq 1 50); do \
	  for t in test_pmm test_kheap test_vmm test_sched test_task test_fs test_syscall test_usermode test_ai test_features; do \
	    ./$(BUILD)/$$t >/dev/null 2>&1 || { echo "stress FAILED: $$t (iteration $$i)"; exit 1; }; \
	  done; \
	done
	@echo "==> stress OK (50 iterations x 10 host tests)"

# Overclaim / fake-claim scan: fail if forbidden marketing claims appear as
# positive statements (honesty doctrine). Negated/qualified mentions are allowed.
scan:
	@bash scripts/check_overclaims.sh

# Deep verify: build + multiboot verify + host tests + overclaim scan + shell
# script syntax. The honest "everything we can check here" gate.
deep-verify: kernel verify test scan
	@bash -n scripts/*.sh && echo "==> shell scripts: syntax OK"
	@echo "==> deep-verify OK (build + verify + host tests + scan + shell syntax)"

# Package: clean source+docs+verification release archive (existing script).
package: verify
	@bash scripts/package.sh

# Export: collect the buildable artifacts + verification into build/export/ with
# a manifest, for handing the milestone off. Never touches Docker/volumes.
export: kernel verify
	@mkdir -p $(BUILD)/export
	@cp $(KERNEL) BUILD_VERIFICATION.txt CHANGELOG.md $(BUILD)/export/ 2>/dev/null || true
	@cp -r preview/screenshots $(BUILD)/export/screenshots 2>/dev/null || true
	@{ echo "THUOS export manifest"; date; echo; \
	   echo "kernel.elf:"; ls -l $(KERNEL); \
	   echo; echo "contents:"; ls -1 $(BUILD)/export; } > $(BUILD)/export/MANIFEST.txt
	@echo "==> export -> $(BUILD)/export/ (kernel.elf, verification, screenshots, manifest)"

clean:
	rm -rf $(BUILD)
	@echo "==> Cleaned $(BUILD)"
