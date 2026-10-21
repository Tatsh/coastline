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
        *main.s.o*(.text .text.*)
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
    /* The game uses the memory after the BSS by address, so C may not add to the BSS. */
    ASSERT(. <= 0x80382bb0, "The BSS grew past its original end.")

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

/* The assets the game reads from the cartridge by their segment symbols. */
_ghostRecordingSegmentRomStart = 0xAD400;
_creditsRecordingSegmentRomStart = 0xAD740;
_endingRecordingSegmentRomStart = 0xADB00;
_menuMusicSegmentRomStart = 0xEBC00;
_scrollSoundSegmentRomStart = 0x1BF480;
_selectSoundSegmentRomStart = 0x1BFEC0;
_startLightsSegmentRomStart = 0x1CB410;
_startLightsSegmentRomEnd = 0x1D9CF0;

/* The original boot stack, inside the BSS. */
g_abBootStackTop = 0x803168D0;

/* The depth buffer, below the program. */
g_aDepthBuffer = 0x80000400;

/* The menu music module, which the main thread decompresses below the program. */
g_abMenuMusicModule = 0x80025C00;

/* The loaded track, which shares the memory of the menu music module. */
g_LoadedTrack = 0x80025C00;

/* Two words of the loaded track its loading prints. */
g_awTrackDebugWords = 0x80029560;

/* The models of the cars the vehicles drive, above the program. */
g_aVehicleModels = 0x803C8000;

/* The last laps the instant replay plays, kept in the last slots of the models. */
g_aLastLapBuffers = 0x803C8000;

/* The recording a race replays, and those the local players record into, below the frame buffers.
 * The recording of the second player runs over the first frame buffer. */
g_abPlaybackRecording = 0x80197000;
g_aabInputRecordings = 0x801A6000;

/* The two frame buffers, below the program. */
g_aFramebuffer0 = 0x801B5000;
g_aFramebuffer1 = 0x801DA800;

/* The boot parameters the IPL3 writes at the start of RAM. */
osTvType = 0x80000300;
osRomType = 0x80000304;
osRomBase = 0x80000308;
osResetType = 0x8000030C;
osCicId = 0x80000310;
osVersion = 0x80000314;
osMemSize = 0x80000318;
osAppNMIBuffer = 0x8000031C;
