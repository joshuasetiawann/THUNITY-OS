/* THUOS — kernel linker script.
 * Loads the kernel at the conventional 1 MiB mark and places the Multiboot
 * header first so the bootloader can find it within the first 8 KiB. */
ENTRY(_start)

SECTIONS
{
    . = 1M;
    thuos_kernel_start = .;

    .text BLOCK(4K) : ALIGN(4K)
    {
        *(.multiboot)
        *(.text)
    }

    .rodata BLOCK(4K) : ALIGN(4K)
    {
        *(.rodata)
    }

    .data BLOCK(4K) : ALIGN(4K)
    {
        *(.data)
    }

    .bss BLOCK(4K) : ALIGN(4K)
    {
        *(COMMON)
        *(.bss)
    }

    thuos_kernel_end = .;

    /DISCARD/ :
    {
        *(.comment)
        *(.note.gnu.build-id)
        *(.eh_frame)
    }
}
