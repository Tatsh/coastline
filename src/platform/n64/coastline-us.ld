/*
 * Link script of the USA release. The IPL3 copies one MiB from cartridge offset 0x1000 to
 * 0x80200000 and jumps there, so ramMain() comes first. The data of the original image is pinned
 * at its original address, and the text and the rodata of the new code must fit below it. The BSS
 * follows the data at its original address, and ramMain() clears it through _bss_start and
 * _bss_size.
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
    _image_data_start = .;
    ASSERT(_image_data_start <= 0x8026ea00, "The code overflows into the original data.")

    . = 0x8026ea00;
    .data : {
        *(.data .data.*)
    }
    ASSERT(. == 0x802ac400, "The data does not end where the original BSS begins.")

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
    }
}
