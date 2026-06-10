# Booting THUOS on real hardware (honest guide)

THUOS is a 32-bit x86 kernel. **Intel vs AMD is not the variable** — it runs in
protected mode on the common x86 ISA and legacy PC devices, which are identical
on both. What actually decides whether a physical machine boots THUOS is the
**firmware, the framebuffer, and the input devices**. This page is the honest
state of each, as of 0.18 "Portable".

## What changed in 0.18

The display driver no longer assumes QEMU. THUOS now:

- requests a linear framebuffer in its **Multiboot 1 header**, and uses the
  framebuffer the **bootloader** reports (`framebuffer_addr/pitch/width/height/
  bpp`), honoring the real **pitch** (bytes per row);
- carries an **embedded 8×16 font**, so text renders in a graphics-mode
  framebuffer (a bootloader/UEFI hands you graphics mode, where the live VGA
  font can't be read);
- falls back to the **Bochs/QEMU VBE** device when there is no Multiboot
  framebuffer (i.e. `qemu -kernel`).

This is verified end-to-end in QEMU by booting a real **GRUB ISO** (`make iso`
→ `qemu -cdrom build/thuos.iso`): GRUB sets the mode and passes the framebuffer,
and THUOS renders the desktop through it.

## The three moats

| Layer | Works today | Needed for a modern laptop |
|------|-------------|----------------------------|
| **Boot / firmware** | Legacy **BIOS / CSM** via GRUB (Multiboot 1) | Most laptops are **UEFI-only** → need **grub-efi** (a UEFI GRUB build) |
| **Graphics** | ✅ bootloader framebuffer (BIOS VBE **or** UEFI GOP via GRUB-EFI), 32bpp | Exotic pixel layouts not yet handled; native GPU drivers out of scope |
| **Input** | **PS/2** keyboard + mouse (desktops, or laptops whose EC emulates PS/2) | Most laptops are **USB / I²C-HID** → need a **USB-HID** stack (xHCI + HID) |

So the machine most likely to boot THUOS **today** is a **desktop/PC with a
legacy or CSM-enabled BIOS and PS/2 (or PS/2-emulated) keyboard**. A modern
UEFI laptop needs the two future milestones above (grub-efi boot + USB-HID).

## Making a bootable USB (BIOS / CSM machines)

> ⚠️ This **erases** the target disk. Double-check the device node.

```bash
make iso                      # builds build/thuos.iso (needs grub-mkrescue + xorriso)
sudo dd if=build/thuos.iso of=/dev/sdX bs=4M conv=fsync status=progress
```

Then on the target machine, enter the firmware setup and:

- enable **CSM / Legacy boot** (and, if present, disable **Secure Boot**);
- boot from the USB device.

You should see the GRUB menu, then the THUOS desktop. If you have a **serial
cable / adapter**, the kernel mirrors everything to **COM1** and prints which
display path it took, e.g.:

```
[lfb] bootloader framebuffer 1024x768 x32bpp pitch=4096 @0xc0000000 type=1
```

If the screen stays blank but serial reaches `thuos>`, the framebuffer wasn't
usable (wrong bpp/layout) and the kernel fell back; send the serial log.

## What is NOT claimed

- THUOS has **not** been run on physical hardware from this repo's CI/sandbox;
  the above is verified in QEMU (incl. the GRUB path) and reasoned from the
  Multiboot/GOP specs.
- No **UEFI** boot path yet (needs grub-efi or a UEFI stub).
- No **USB** stack yet → USB-only keyboards/touchpads won't work.
- No storage/network drivers; the desktop runs entirely from RAM.

See also [`THUOS_REALITY_CHECK.md`](THUOS_REALITY_CHECK.md).
