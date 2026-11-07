/*
 * Link script of the European release. The IPL3 copies one MiB from cartridge offset 0x1000 to
 * 0x80200000 and jumps there. ramMain() comes first. Routines not yet converted to C, and those
 * whose European body differs from the USA one, link from the assembly under src/n64/asm/eu. The data
 * follows the text. Every reference to the data is symbolic. Text and data together must end
 * before the first asset at cartridge offset 0xaf0f0 (RAM 0x802ae0f0). The BSS is pinned at its
 * original address. The game refers to the BSS by absolute address, and ramMain() clears the BSS
 * through _bss_start and _bss_size.
 */
OUTPUT_ARCH(mips)
ENTRY(ramMain)

SECTIONS
{
    . = 0x80200000;
    .text : SUBALIGN(4) {
        *main.s.o*(.text .text.*)
        . = 0x50;
        *game.c.obj(.text .text.*)
        *race-bgm.trim.o(.text .text.*)
        *race-bgm.c.obj(.text .text.*)
        *startlights.c.obj(.text .text.*)
        *race-demo.c.obj(.text .text.*)
        *racecredits.c.obj(.text .text.*)
        *racetick.trim.o(.text .text.*)
        *racetick.c.obj(.text .text.*)
        *raceresults.trim.o(.text .text.*)
        *raceresults.c.obj(.text .text.*)
        *seasontrack.c.obj(.text .text.*)
        *race.trim.o(.text .text.*)
        *libcoastline_n64.a:race.c.obj(.text .text.*)
        . = ALIGN(16);
        *splash.c.obj(.text .text.*)
        *splash-controller.c.obj(.text .text.*)
        *libcoastline_n64.a:racemenu.c.obj(.text .text.*)
        *modeldraw.trim.o(.text .text.*)
        *libcoastline_n64.a:modeldraw.c.obj(.text .text.*)
        *libcoastline_n64.a:racemenu-draw.c.obj(.text .text.*)
        . = ALIGN(16);
        *libcoastline_n64.a:carselect.c.obj(.text .text.*)
        *libcoastline_n64.a:weatherselect.c.obj(.text .text.*)
        *libcoastline_n64.a:mainmenu.c.obj(.text .text.*)
        *libcoastline_n64.a:playerselect.c.obj(.text .text.*)
        *volume-bgm.trim.o(.text .text.*)
        *options-volume.trim.o(.text .text.*)
        *volume-sfx.trim.o(.text .text.*)
        *options.trim.o(.text .text.*)
        *libcoastline_n64.a:options.c.obj(.text .text.*)
        *libcoastline_n64.a:options-regional.c.obj(.text .text.*)
        *libcoastline_n64.a:options-pak.c.obj(.text .text.*)
        . = ALIGN(16);
        *libcoastline_n64.a:controllerscreen.c.obj(.text .text.*)
        *iomessage.trim.o(.text .text.*)
        *libcoastline_common.a:iomessage.c.obj(.text .text.*)
        *vblankquery.trim.o(.text .text.*)
        *stats.trim.o(.text .text.*)
        *vecmath-light.trim.o(.text .text.*)
        *texturebind.trim.o(.text .text.*)
        *depth.trim.o(.text .text.*)
        *libcoastline_n64.a:depth.c.obj(.text .text.*)
        *libcoastline_common.a:fill.c.obj(.text .text.*)
        *fog.trim.o(.text .text.*)
        *libcoastline_common.a:fog.c.obj(.text .text.*)
        *frame.trim.o(.text .text.*)
        *libcoastline_n64.a:frame.c.obj(.text .text.*)
        *libcoastline_common.a:jolt.c.obj(.text .text.*)
        *renderinit.trim.o(.text .text.*)
        *input-init.trim.o(.text .text.*)
        *frametime.trim.o(.text .text.*)
        *depth-stub.trim.o(.text .text.*)
        *screen.trim.o(.text .text.*)
        *game-mode-main-loop.s.obj(.text .text.*)
        *screen-stub.trim.o(.text .text.*)
        *libcoastline_common.a:assetdma.c.obj(.text .text.*)
        *replay.trim.o(.text .text.*)
        *rledecompress.trim.o(.text .text.*)
        *assetdma-init.trim.o(.text .text.*)
        *libcoastline_n64.a:decompress.c.obj(.text .text.*)
        *boot-relocation.slice.o(.text .text.*)
        *libcoastline_common.a:materialcolor.c.obj(.text .text.*)
        *boot-color.slice.o(.text .text.*)
        *vehiclemodel.trim.o(.text .text.*)
        *loadcar.trim.o(.text .text.*)
        *vehiclemodel-valid.trim.o(.text .text.*)
        *libcoastline_common.a:modelanimationflags.c.obj(.text .text.*)
        *modelanimation.trim.o(.text .text.*)
        *system.trim.o(.text .text.*)
        *boot-rest.slice.o(.text .text.*)
        *libcoastline_common.a:trackgrid.c.obj(.text .text.*)
        *libcoastline_common.a:vecmath-frame.c.obj(.text .text.*)
        *vehicle-track-position.trim.o(.text .text.*)
        *vehiclestatus.trim.o(.text .text.*)
        *ground-ray.slice.o(.text .text.*)
        *physics-update.trim.o(.text .text.*)
        *(.text .text.*)
    }
    . = ALIGN(16);
    .data : {
        *(.data .data.*)
    }
    . = ALIGN(16);
    .rodata : {
        *(.rodata .rodata.*)
    }
    . = ALIGN(16);
    _image_end = .;
    ASSERT(_image_end <= 0x802ae0f0, "The program overflows into the assets of the cartridge.")

    . = 0x802ae0f0;
    _bss_start = .;
    /* IDO statics reuse their original BSS areas. */
    OVERLAY 0x802ae0f0 : NOCROSSREFS {
        .light_ramp_bss {
            . = 0x8031d858 - 0x802ae0f0;
            *fog.trim.o(.bss.light_ramp)
            *fog.c.obj(.bss.light_ramp)
        }
        .season_records_bss {
            . = 0x80317c18 - 0x802ae0f0;
            *racemenu.c.obj(.bss.season_records)
        }
        .decompress_len_bss {
            . = 0x8031d868 - 0x802ae0f0;
            *decompress.c.obj(.bss.decompress_len)
        }
        .bss {
            *(EXCLUDE_FILE (*carselect.c.obj *decaleditor.c.obj *paintshop.c.obj *racemenu.c.obj
                            *racemenu-draw.c.obj *trackrender.c.obj *visibleinstances.c.obj) .bss .bss.*)
            *(COMMON)
            . = ALIGN(8);
        }
        .carselect_bss {
            . = 0x80317f28 - 0x802ae0f0;
            *carselect.c.obj(.bss .bss.*)
        }
        .racemenu_bss {
            . = 0x80317c68 - 0x802ae0f0;
            *racemenu.c.obj(.bss .bss.*)
        }
        .screen_bss {
            . = 0x80317f00 - 0x802ae0f0;
            *racemenu-draw.c.obj(.bss .bss.*)
        }
        .paintshop_bss {
            . = 0x8036c078 - 0x802ae0f0;
            *paintshop.c.obj(.bss .bss.*)
        }
        .decaleditor_bss {
            . = 0x8036c060 - 0x802ae0f0;
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
    ASSERT(ADDR(.bss) + SIZEOF(.bss) <= 0x80384b30, "The BSS grew past its original end.")
    ASSERT(ADDR(.carselect_bss) + SIZEOF(.carselect_bss) <= 0x80318060,
           "Car-select state exceeds its original BSS area.")
    ASSERT(ADDR(.season_records_bss) + SIZEOF(.season_records_bss) <= 0x80317c68,
           "Season-record text exceeds its original BSS area.")
    ASSERT(ADDR(.racemenu_bss) + SIZEOF(.racemenu_bss) <= 0x80317cd0,
           "Track-menu state exceeds its original BSS area.")
    ASSERT(ADDR(.paintshop_bss) + SIZEOF(.paintshop_bss) <= 0x8036c1b8,
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
_ghostRecordingSegmentRomStart = 0xAF0F0;
_creditsRecordingSegmentRomStart = 0xB0860;
_endingRecordingSegmentRomStart = 0xB0B40;
_menuMusicSegmentRomStart = 0xEB310;
_scrollSoundSegmentRomStart = 0x1BE5C0;
_selectSoundSegmentRomStart = 0x1BF000;
_startLightsSegmentRomStart = 0x1CA550;
_startLightsSegmentRomEnd = 0x1D8E30;
gRomSplashSoundA = 0x1B4580;
gRomSplashSoundB = 0x1B8500;
gRomSplashFirstLogo = 0x1DA840;
gRomSplashDeveloperLogo = 0x1DB870;
gRomSplashTitleD = 0x1DF9D0;
gRomSplashTitleC = 0x1E0870;
gRomSplashTitleA = 0x1DE720;
gRomSplashTitleB = 0x1E1AE0;

/* The original boot stack, inside the BSS. */
g_abBootStackTop = 0x803185B0;

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
