/*
 * Link script of the Asia release. The IPL3 copies one MiB from cartridge offset 0x1000 to
 * 0x80200000 and jumps there. ramMain() comes first. Routines not yet converted to C link from the
 * assembly under src/n64/asm/asia. The data follows the text. Every reference to the data is
 * symbolic. Text and data together must end before the first asset at cartridge offset 0xacec0
 * (RAM 0x802abec0).
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
    ASSERT(_image_end <= 0x802abec0, "The program overflows into the assets of the cartridge.")

    . = 0x802abec0;
    _bss_start = .;
    /* IDO statics reuse their original BSS areas. */
    OVERLAY 0x802abec0 : NOCROSSREFS {
        .light_ramp_bss {
            . = 0x8031ae20 - 0x802abec0;
            *fog.c.obj(.bss.light_ramp)
        }
        .season_records_bss {
            . = 0x803159f8 - 0x802abec0;
            *racemenu.c.obj(.bss.season_records)
        }
        .decompress_len_bss {
            . = 0x8031ae08 - 0x802abec0;
            *system.c.obj(.bss.decompress_len)
        }
        .bss {
            *(EXCLUDE_FILE (*carselect.c.obj *decaleditor.c.obj *paintshop.c.obj *racemenu.c.obj
                            *trackrender.c.obj *visibleinstances.c.obj) .bss .bss.*)
            *(COMMON)
            . = ALIGN(8);
        }
        .carselect_bss {
            . = 0x80315d08 - 0x802abec0;
            *carselect.c.obj(.bss .bss.*)
        }
        .racemenu_bss {
            . = 0x80315a48 - 0x802abec0;
            *racemenu.c.obj(.bss .bss.*)
        }
        .paintshop_bss {
            . = 0x80369630 - 0x802abec0;
            *paintshop.c.obj(.bss .bss.*)
        }
        .decaleditor_bss {
            . = 0x80369618 - 0x802abec0;
            *decaleditor.c.obj(.bss .bss.*)
        }
        .trackrender_bss {
            . = g_fViewMatrixMaxAbs;
            *trackrender.c.obj(.bss .bss.*)
        }
        .visibleinstances_bss {
            . = g_nNearbyVehicleCount;
            *visibleinstances.c.obj(.bss .bss.*)
        }
    }
    _bss_size = SIZEOF(.bss);
    /* The game uses the memory after the BSS by address, so C may not add to the BSS. */
    ASSERT(ADDR(.bss) + SIZEOF(.bss) <= 0x80382670, "The BSS grew past its original end.")
    ASSERT(ADDR(.carselect_bss) + SIZEOF(.carselect_bss) <= 0x80315e40,
           "Car-select state exceeds its original BSS area.")
    ASSERT(ADDR(.season_records_bss) + SIZEOF(.season_records_bss) <= 0x80315a48,
           "Season-record text exceeds its original BSS area.")
    ASSERT(ADDR(.racemenu_bss) + SIZEOF(.racemenu_bss) <= 0x80315ab0,
           "Track-menu state exceeds its original BSS area.")
    ASSERT(ADDR(.paintshop_bss) + SIZEOF(.paintshop_bss) <= 0x80369780,
           "Paint-shop palette exceeds its original BSS area.")
    ASSERT(ADDR(.decaleditor_bss) + SIZEOF(.decaleditor_bss) == g_nPaintAnchorX + 20,
           "Decal-editor state moved outside its original BSS slot.")
    ASSERT(g_aDecalSwatches == g_dwAnchorYInitialized + 8,
           "Paint-shop palette moved outside its original BSS slot.")
    ASSERT(ADDR(.trackrender_bss) + SIZEOF(.trackrender_bss) <= g_dwInstanceHideFlags + 4,
           "Track-render state exceeds its original BSS area.")
    ASSERT(ADDR(.visibleinstances_bss) + SIZEOF(.visibleinstances_bss) <= g_nNearbyVehicleCount + 4,
           "Visible-instance state exceeds its original BSS area.")

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

g_pFirstCreditLineText = g_aCreditScrollLines + 12;
g_nNumLocalPlayersSigned = g_dwNumLocalPlayers;
g_OptionSettingsSfxLevel = g_OptionSettings + 4;
g_MusicRestartOrder = g_MusicPlayer + 0x12;

/* The assets the game reads from the cartridge by their segment symbols. */
_ghostRecordingSegmentRomStart = 0xACEC0;
_creditsRecordingSegmentRomStart = 0xAD200;
_endingRecordingSegmentRomStart = 0xAD5C0;
_menuMusicSegmentRomStart = 0xEB6C0;
_scrollSoundSegmentRomStart = 0x1BEF40;
_selectSoundSegmentRomStart = 0x1BF980;
_startLightsSegmentRomStart = 0x1CAED0;
_startLightsSegmentRomEnd = 0x1D97B0;
gRomSplashSoundA = 0x1B4F00;
gRomSplashSoundB = 0x1B8E80;
gRomSplashFirstLogo = 0x1DB500;
gRomSplashDeveloperLogo = 0x1DC530;
gRomSplashTitleA = 0x1DF3E0;
gRomSplashTitleB = 0x1E27A0;
gRomSplashTitleC = 0x1E1530;
gRomSplashTitleD = 0x1E0690;

/* The original boot stack, inside the BSS. */
g_abBootStackTop = 0x80316390;

/* The depth buffer, below the program. */
g_aDepthBuffer = 0x80000400;

/* The menu music module, which the main thread decompresses below the program. */
g_abMenuMusicModule = 0x80025C00;

/* The loaded track, which shares the memory of the menu music module. */
g_LoadedTrack = 0x80025C00;
g_pRootTrackSection = g_LoadedTrack + 0x70;
g_aTrackGates = g_LoadedTrack + 0x98;

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
