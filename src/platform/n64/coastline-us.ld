/*
 * Link script of the USA release. The IPL3 copies one MiB from cartridge offset 0x1000 to
 * 0x80200000 and jumps there. ramMain() comes first. Routines not yet converted to C link from the
 * assembly under asm/us. The data follows the text. Every reference to the data is symbolic. Text
 * and data together must end before the first asset at cartridge offset 0xad400 (RAM 0x802ac400).
 * The BSS is pinned at its original address. The game refers to the BSS by absolute address, and
 * ramMain() clears the BSS through _bss_start and _bss_size.
 */
OUTPUT_ARCH(mips)
ENTRY(ramMain)

SECTIONS
{
    . = 0x80200000;
    .text : {
        *main.c.o*(.text .text.*)
        *(.text .text.*)
    }
    . = ALIGN(16);
    .rodata : {
        *(.rodata .rodata.*)
    }
    . = ALIGN(16);
    .data : {
        *(.data .data.*)
    }
    . = ALIGN(16);
    _image_end = .;
    ASSERT(_image_end <= 0x802ac400, "The program overflows into the assets of the cartridge.")

    . = 0x802ac400;
    _bss_start = .;
    .bss (NOLOAD) : {
        *(.bss .bss.*)
        *(COMMON)
        . = ALIGN(8);
    }
    _bss_size = . - _bss_start;

    /DISCARD/ : {
        *(.MIPS.abiflags)
        *(.reginfo)
        *(.pdr)
        *(.mdebug*)
        *(.gnu.attributes)
        *(.note*)
        *(.comment)
    }
}

/* The original boot stack, inside the BSS. */
g_abBootStackTop = 0x803168D0;
