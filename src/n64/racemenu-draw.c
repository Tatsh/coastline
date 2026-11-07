#include <PR/gu.h>
#include <PR/os.h>
#include <PR/os_libc.h>

#include "carselect.h"
#include "common/audio.h"
#include "common/audiomix.h"
#include "common/fade.h"
#include "common/game.h"
#include "common/heap.h"
#include "common/input.h"
#include "common/race.h"
#include "common/render.h"
#include "common/replay.h"
#include "common/system.h"
#include "common/text.h"
#include "common/timing.h"
#include "common/trackselect.h"
#include "common/util.h"
#include "common/weather.h"
#include "mainmenu.h"
#include "options.h"
#include "playerselect.h"
#include "racemenu.h"
#include "weatherselect.h"

/* The sounds of moving the cursor and of choosing, on the cartridge. */
#ifdef COASTLINE_ROM_POINTERS
#define ROM_SCROLL_SOUND _scrollSoundSegmentRomStart
#define ROM_SELECT_SOUND _selectSoundSegmentRomStart
#define ROM_MODEL_SOURCE(offset) ((char *)(offset))
#else
#define ROM_SCROLL_SOUND ((u32)_scrollSoundSegmentRomStart)
#define ROM_SELECT_SOUND ((u32)_selectSoundSegmentRomStart)
#define ROM_MODEL_SOURCE(offset) (offset)
#endif

/* The degrees of a radian. */
#define RAD_TO_DEG 57.29577637f

/* The scale of a model on the stage of a screen. */
#define STAGE_MODEL_SCALE 0.03125f

/* The points of the beam of the spotlight, and its radius. */
#define SPOTLIGHT_POINTS 31
#define SPOTLIGHT_RADIUS 64.0f

/* Draw the buttons at the bottom of a screen and their names. */
#define DRAW_FOOTER_BUTTONS(leftX, rightX, leftText, leftTextX, rightText, rightTextX)             \
    SetTextStyleNormal();                                                                          \
    SetTextAlignLeft();                                                                            \
    DrawTexturedRect(                                                                              \
        &g_ArrowLeftTextureDesc, leftX, 216, 12, 12, 0, 0, 0, 0xFF, 0, 0, 0, 0xFF, 0);             \
    DrawTexturedRect(                                                                              \
        &g_ArrowRightTextureDesc, rightX, 216, 12, 12, 0, 0, 0, 0xFF, 0, 0, 0, 0xFF, 0);           \
    SetTextSize(11);                                                                               \
    DrawText(leftText, leftTextX, g_dwScreenHeight * 19 / 20 - 3);                                 \
    DrawText(rightText, rightTextX, g_dwScreenHeight * 19 / 20 - 3)

void DrawRotatedModel(Model *model, f32 x, f32 y, f32 z, f32 angle) {
    if (g_dwTrackMirrored != g_dwViewportMirrored) {
        guRotateF(g_afMvpMatrix, -angle * RAD_TO_DEG, 0.0f, 0.0f, 1.0f);
        guScaleF(g_afScratchMatrix, -STAGE_MODEL_SCALE, STAGE_MODEL_SCALE, STAGE_MODEL_SCALE);
    } else {
        guRotateF(g_afMvpMatrix, angle * RAD_TO_DEG, 0.0f, 0.0f, 1.0f);
        guScaleF(g_afScratchMatrix, STAGE_MODEL_SCALE, STAGE_MODEL_SCALE, STAGE_MODEL_SCALE);
    }
    guMtxCatF(g_afMvpMatrix, g_afScratchMatrix, g_afMvpMatrix);
    guRotateF(g_afScratchMatrix, -90.0f, 1.0f, 0.0f, 0.0f);
    guMtxCatF(g_afMvpMatrix, g_afScratchMatrix, g_afMvpMatrix);
    guTranslateF(g_afScratchMatrix, x, y, z);
    guMtxCatF(g_afMvpMatrix, g_afScratchMatrix, g_afMvpMatrix);
    DrawModelList(model, g_afMvpMatrix);
}

void DrawModelAt(Model *model, f32 x, f32 y, f32 z, f32 angle, f32 scale) {
    if (g_dwTrackMirrored != g_dwViewportMirrored) {
        guRotateF(g_afMvpMatrix, -angle * RAD_TO_DEG, 0.0f, 0.0f, 1.0f);
        guScaleF(g_afScratchMatrix, -scale, scale, scale);
    } else {
        guRotateF(g_afMvpMatrix, angle * RAD_TO_DEG, 0.0f, 0.0f, 1.0f);
        guScaleF(g_afScratchMatrix, scale, scale, scale);
    }
    guMtxCatF(g_afMvpMatrix, g_afScratchMatrix, g_afMvpMatrix);
    guRotateF(g_afScratchMatrix, -90.0f, 1.0f, 0.0f, 0.0f);
    guMtxCatF(g_afMvpMatrix, g_afScratchMatrix, g_afMvpMatrix);
    guTranslateF(g_afScratchMatrix, x, y, z);
    guMtxCatF(g_afMvpMatrix, g_afScratchMatrix, g_afMvpMatrix);
    DrawModelList(model, g_afMvpMatrix);
}

void DrawMenuSpotlight(void) {
    f32 angle;
    s32 i;

    /* Nothing sets the flag, so the vertices are built again every frame. */
    if (!g_bMenuSpotlightBuilt) {
        /* A ring under the stage, then its source above it in the middle. */
        for (i = 0; i < SPOTLIGHT_POINTS; i++) {
            angle = (f32)i * 0.2094395161f;
            g_aMenuSpotlightVertices[i].v.ob[0] =
                (s16)(s32)(sinf(angle) * SPOTLIGHT_RADIUS) + g_dwScreenWidth / 2;
            g_aMenuSpotlightVertices[i].v.ob[1] = g_dwScreenHeight * 6 / 16;
            g_aMenuSpotlightVertices[i].v.ob[2] = (s32)(cosf(angle) * SPOTLIGHT_RADIUS);
            g_aMenuSpotlightVertices[i].v.cn[0] = 0xFF;
            g_aMenuSpotlightVertices[i].v.cn[1] = 0xFF;
            g_aMenuSpotlightVertices[i].v.cn[2] = 0xFF;
            g_aMenuSpotlightVertices[i].v.cn[3] = 0xFF;
        }
        g_aMenuSpotlightVertices[0].v.ob[0] = g_dwScreenWidth / 2;
        g_aMenuSpotlightVertices[0].v.ob[1] = g_dwScreenHeight * 5 / 16;
        g_aMenuSpotlightVertices[0].v.ob[2] = 0;
    }
    gSPDisplayList(gDisplayListPtr++, g_aMenuSpotlightDisplayList);
}

void DrawMenuChoiceMarker(void) {
    guScaleF(g_afMvpMatrix, 0.26f, 0.26f, 0.26f);
    guRotateF(g_afScratchMatrix, -90.0f, 1.0f, 0.0f, 0.0f);
    guMtxCatF(g_afMvpMatrix, g_afScratchMatrix, g_afMvpMatrix);
    guTranslateF(g_afScratchMatrix, 160.0f, (f32)(g_dwScreenHeight * 6 / 16 - 4), 0.0f);
    guMtxCatF(g_afMvpMatrix, g_afScratchMatrix, g_afMvpMatrix);
    guMtxF2L(g_afMvpMatrix, &g_MenuChoiceMarkerMtx);
    gSPDisplayList(gDisplayListPtr++, g_aMenuChoiceMarkerDisplayList);
}

void DrawMenuFooterButtons(void) {
    DRAW_FOOTER_BUTTONS(100, 160, "%wwSelect", 115, "%wwGo Back", 175);
}

void DrawPauseMenuButtons(void) {
    DRAW_FOOTER_BUTTONS(102, 178, "%wwContinue", 118, "%wwExit", 193);
}

void SetScreenPendingSelection(s32 item) {
    g_nScreenPendingSelection = item;
}

/*
 * Load the textures and the sounds of the screens, and silence the end of each sound, counting
 * with i.
 */
s32 ProcessScreenState(const char *title,
                       s32 itemCount,
                       MenuItem **items,
                       s32 *selection,
                       MenuItemFilter isSelectable,
                       s32 topRed,
                       s32 topGreen,
                       s32 topBlue,
                       s32 bottomRed,
                       s32 bottomGreen,
                       s32 bottomBlue) {
#ifdef __sgi
    /* IDO places local menu state at the original BSS addresses. */
    static int g_nPrevScreenSelection;
    static float g_fScreenSlideOffset;
    static float g_fScreenRotationAngle;
    static int g_bScreenItemChosen;
    static s8 *g_pScreenScrollSoundBuf;
    static int g_dwScreenScrollSoundSize;
    static s8 *g_pScreenSelectSoundBuf;
    static int g_dwScreenSelectSoundSize;
    static float g_fScreenIdleTime;
    static int g_nScreenCursor;
#endif
    float off;
    int i;
    int result;

    result = 0;
    if (g_bScreenStateInitFlag == 0) {
        SetFadeTarget(1.0f, 0.2f);
        FadeMusicTo(1.0f, 0.2f);
        FadeSfxTo(1.0f, 0.2f);
        g_nScreenCursor = *selection;
        g_fScreenRotationAngle = 4.712389f;
        g_fScreenSlideOffset = 0.0f;
        LoadAndDecompressAsset(&g_ArrowLeftTextureDesc, AllocHeapAligned);
        LoadAndDecompressAsset(&g_ArrowRightTextureDesc, AllocHeapAligned);
        LoadAndDecompressAsset(&g_BgTextureDesc, AllocHeapAligned);
        g_dwScreenScrollSoundSize = DmaReadAssetSize(ROM_SCROLL_SOUND);
        g_pScreenScrollSoundBuf =
            AllocHeapAligned(g_dwScreenScrollSoundSize + g_dwScreenScrollSoundSize);
        DecompressFromRom(g_pScreenScrollSoundBuf, ROM_SCROLL_SOUND, 0);
        g_dwScreenSelectSoundSize = DmaReadAssetSize(ROM_SELECT_SOUND);
        g_pScreenSelectSoundBuf =
            AllocHeapAligned(g_dwScreenSelectSoundSize + g_dwScreenSelectSoundSize);
        DecompressFromRom(g_pScreenSelectSoundBuf, ROM_SELECT_SOUND, 0);
        for (i = 0; i != 0x100; i++) {
            g_pScreenScrollSoundBuf[g_dwScreenScrollSoundSize + i] = 0;
            g_pScreenSelectSoundBuf[g_dwScreenSelectSoundSize + i] = 0;
        }
        if (g_pMenuTitleImage != 0) {
            LoadAndDecompressAsset(g_pMenuTitleImage, AllocHeapAligned);
        }
        g_fScreenIdleTime = 0.0f;
        g_nScreenPendingSelection = -1;
        g_bScreenStateInitFlag = 1;
    }
    if (g_nScreenPendingSelection >= 0) {
        short v;

        g_nPrevScreenSelection = g_nScreenCursor;
        g_nScreenCursor = g_nScreenPendingSelection;
        if (g_nScreenCursor != g_nPrevScreenSelection) {
            g_fScreenSlideOffset = -1.0f;
            v = FindFreeSoundChannel();
            if (v != -1) {
                InitSoundChannel(v, g_pScreenScrollSoundBuf, g_dwScreenScrollSoundSize, 0);
            }
        }
        g_nScreenPendingSelection = -1;
    }
    UpdateFrameTiming();
    UpdateFadeTransitions();
    StartHighResFrame();
    AddTimingMarker(0, 0, 0, 200, 0xFF);
    ClearDepthBuffer();
    DrawTexturedRect(&g_BgTextureDesc,
                     0,
                     0,
                     320,
                     240,
                     topRed,
                     topGreen,
                     topBlue,
                     0xFF,
                     bottomRed,
                     bottomGreen,
                     bottomBlue,
                     0xFF,
                     4);
    g_fScreenRotationAngle += g_fFrameDeltaTime + g_fFrameDeltaTime;
    while (g_fScreenRotationAngle > 6.2831855f) {
        g_fScreenRotationAngle -= 6.2831855f;
    }
    if (g_fScreenSlideOffset < 0.0f) {
        g_fScreenSlideOffset += g_fFrameDeltaTime + g_fFrameDeltaTime;
        if (g_fScreenSlideOffset > 0.0f) {
            g_fScreenSlideOffset = 0.0f;
        }
    } else if (g_fScreenSlideOffset > 0.0f) {
        g_fScreenSlideOffset -= g_fFrameDeltaTime + g_fFrameDeltaTime;
        if (g_fScreenSlideOffset < 0.0f) {
            g_fScreenSlideOffset = 0.0f;
        }
    }
    if (g_fScreenSlideOffset < 0.0f) {
        off = g_fScreenSlideOffset * g_fScreenSlideOffset * -5.0f;
    } else {
        off = g_fScreenSlideOffset * g_fScreenSlideOffset * 5.0f;
    }
    SetupMenuProjection(g_dwScreenWidth, g_dwScreenHeight);
    SetViewport(0, 0, g_dwScreenWidth, g_dwScreenHeight, 1);
    gSPNumLights(gDisplayListPtr++, 1);
    gSPLight(gDisplayListPtr++, &g_MenuDirectionalLight, 1);
    gSPLight(gDisplayListPtr++, &g_MenuAmbientLight, 2);
    gSPClipRatio(gDisplayListPtr++, 6);
    if (g_pMenuTitleImage != 0) {
        DrawTexturedRect(g_pMenuTitleImage,
                         g_dwMenuTitleImageX,
                         g_dwMenuTitleImageY,
                         g_dwMenuTitleImageWidth,
                         g_dwMenuTitleImageHeight,
                         0,
                         0,
                         0,
                         0xFF,
                         0xFF,
                         0xFF,
                         0xFF,
                         0xFF,
                         2);
    }
    {
        char buf[80];
        short v;

        SetTextSize(30);
        SetTextAlignCenter();
        sprintf(buf, "%%ry%s", title);
        DrawText(buf, g_dwScreenWidth / 2, g_dwScreenHeight / 6 - 2);
        if (g_pfnScreenPreDrawCallback != 0) {
            g_pfnScreenPreDrawCallback();
        } else {
            Mtx *mtx = (Mtx *)AllocDisplayListBlock();
            int row;

            guRotateF(g_afMvpMatrix, 12.0f, 1.0f, 0.0f, 0.0f);
            guMtxF2L(g_afMvpMatrix, mtx);
            gSPMatrix(gDisplayListPtr++, mtx, G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_PUSH);
            DrawMenuSpotlight();
            row = g_nScreenCursor;
            do {
                row = (row + itemCount - 1) % itemCount;
            } while (isSelectable != 0 && isSelectable(row) == 0);
            if (row != g_nScreenCursor) {
                DrawMenuChoiceMarker();
            }
            if (items[g_nScreenCursor]->flags & 4) {
                int shown;

                if (g_fScreenSlideOffset > 0.75f || g_fScreenSlideOffset < -0.75f) {
                    shown = g_nPrevScreenSelection;
                } else {
                    shown = g_nScreenCursor;
                }
                AdvanceModelAnimations(items[shown]->model);
                if (items[shown]->flags & 0x10) {
                    g_dwViewportMirrored = 1;
                }
                DrawRotatedModel(items[shown]->model,
                                 g_dwScreenWidth / 2,
                                 (g_dwScreenHeight * 6) / 16,
                                 0.0f,
                                 -1.5707964f - g_fScreenSlideOffset * 6.2831855f);
                g_dwViewportMirrored = 0;
                if (g_fScreenSlideOffset == 0) {
                    goto label;
                }
            } else if (off != 0) {
                if (items[g_nScreenCursor]->model != 0) {
                    AdvanceModelAnimations(items[g_nScreenCursor]->model);
                    if (items[g_nScreenCursor]->flags & 0x10) {
                        g_dwViewportMirrored = 1;
                    }
                    DrawRotatedModel(items[g_nScreenCursor]->model,
                                     g_dwScreenWidth / 2 - off * g_dwScreenWidth / 6.0f,
                                     (g_dwScreenHeight * 6) / 16,
                                     0.0f,
                                     g_fScreenRotationAngle);
                    g_dwViewportMirrored = 0;
                } else {
                    sprintf(buf, "%%ww%s", items[g_nScreenCursor]->name);
                    SetTextStyleNormal();
                    SetTextAlignCenter();
                    DrawText(buf,
                             g_dwScreenWidth / 2 - (int)(off * g_dwScreenWidth / 6.0f),
                             (g_dwScreenHeight * 9) / 16);
                }
                if (items[g_nPrevScreenSelection]->model != 0) {
                    AdvanceModelAnimations(items[g_nPrevScreenSelection]->model);
                    if (items[g_nPrevScreenSelection]->flags & 0x10) {
                        g_dwViewportMirrored = 1;
                    }
                    DrawRotatedModel(items[g_nPrevScreenSelection]->model,
                                     g_dwScreenWidth / 2 - ((off < 0.0f ? 5.0f : -5.0f) + off) *
                                                               g_dwScreenWidth / 6.0f,
                                     (g_dwScreenHeight * 6) / 16,
                                     0.0f,
                                     g_fScreenRotationAngle);
                    g_dwViewportMirrored = 0;
                } else {
                    sprintf(buf, "%%ww%s", items[g_nPrevScreenSelection]->name);
                    SetTextStyleNormal();
                    SetTextAlignCenter();
                    DrawText(buf,
                             g_dwScreenWidth / 2 - (int)(((off < 0.0f ? 5.0f : -5.0f) + off) *
                                                         g_dwScreenWidth / 6.0f),
                             (g_dwScreenHeight * 9) / 16);
                }
            } else {
                SetTextStyleNormal();
                SetTextAlignCenter();
                if (items[g_nScreenCursor]->model != 0) {
                    AdvanceModelAnimations(items[g_nScreenCursor]->model);
                    if (items[g_nScreenCursor]->flags & 0x10) {
                        g_dwViewportMirrored = 1;
                    }
                    DrawRotatedModel(items[g_nScreenCursor]->model,
                                     g_dwScreenWidth / 2,
                                     (g_dwScreenHeight * 6) / 16,
                                     0.0f,
                                     g_fScreenRotationAngle);
                    g_dwViewportMirrored = 0;
                } else {
                    sprintf(buf, "%%ww%s", items[g_nScreenCursor]->name);
                    DrawText(buf, g_dwScreenWidth / 2, (g_dwScreenHeight * 9) / 16);
                }
            label:
                SetTextSize(20);
                sprintf(buf, "%%ry%s", items[g_nScreenCursor]->name);
                DrawText(buf, g_dwScreenWidth / 2, (g_dwScreenHeight * 20) / 64);
                if (g_pfnScreenPostDrawCallback != 0) {
                    g_pfnScreenPostDrawCallback(g_nScreenCursor);
                }
            }
            gSPPopMatrix(gDisplayListPtr++, 0);
        }
        DrawMenuFooterButtons();
        ApplyScreenFade();
        if (g_dwHudDebugLevel >= 2) {
            SetViewport(0, 0, g_dwScreenWidth, g_dwScreenHeight, 1);
            DrawTimingBars();
        }
        FlushDisplayList();
        if (IsFadingOut() != 0) {
            if (IsFadeComplete() != 0) {
                int ch;

                g_bScreenStateInitFlag = 0;
                *selection = g_nScreenCursor;
                ClearFramebuffersForWideMode();
                for (ch = 0; ch < 6; ch++) {
                    g_aSoundEffectChannels[ch].volume = 0;
                    g_aSoundEffectChannels[ch].mixVolume = 0;
                    g_aSoundEffectChannels[ch].step = 0;
                    g_aSoundEffectChannels[ch].sample = g_SilenceSampleData;
                }
                if (g_bScreenItemChosen != 0) {
                    return 1;
                }
                return 2;
            }
        } else {
            for (i = 0; i < (s32)g_dwNumLocalPlayers; i++) {
                if (g_dwScreenPlayerIdx != -1 && ((1 << i) & g_dwMenuInputPlayerMask) == 0) {
                    continue;
                }
                UpdateStickRepeat(g_aVehicles[i].input);
                if (g_aVehicles[i].input->buttons != 0) {
                    g_fScreenIdleTime = 0.0f;
                }
                if (g_fScreenSlideOffset == 0) {
                    if (g_aVehicles[i].input->buttons & 4) {
                        TransferFlags(g_aVehicles[i].input, 10);
                        g_aVehicles[i].input->buttons &= ~10;
                        g_nPrevScreenSelection = g_nScreenCursor;
                        do {
                            g_nScreenCursor = (g_nScreenCursor + itemCount - 1) % itemCount;
                        } while (isSelectable != 0 && isSelectable(g_nScreenCursor) == 0);
                        if (g_nScreenCursor != g_nPrevScreenSelection) {
                            g_fScreenSlideOffset = 1.0f;
                            goto move;
                        }
                    }
                    if (g_aVehicles[i].input->buttons & 1) {
                        TransferFlags(g_aVehicles[i].input, 10);
                        g_aVehicles[i].input->buttons &= ~10;
                        g_nPrevScreenSelection = g_nScreenCursor;
                        do {
                            g_nScreenCursor = (g_nScreenCursor + 1) % itemCount;
                        } while (isSelectable != 0 && isSelectable(g_nScreenCursor) == 0);
                        if (g_nScreenCursor != g_nPrevScreenSelection) {
                            g_fScreenSlideOffset = -1.0f;
                        move:
                            v = FindFreeSoundChannel();
                            if (v != -1) {
                                InitSoundChannel(
                                    v, g_pScreenScrollSoundBuf, g_dwScreenScrollSoundSize, 0);
                            }
                        }
                    }
                    if (items[g_nScreenCursor]->flags & 1) {
                        if (g_aVehicles[i].input->buttons & 8) {
                            *selection = g_nScreenCursor;
                            TransferFlags(g_aVehicles[i].input, 8);
                            result = 3;
                            goto choose;
                        }
                        if (g_aVehicles[i].input->buttons & 2) {
                            *selection = g_nScreenCursor;
                            TransferFlags(g_aVehicles[i].input, 2);
                            result = 4;
                            goto choose;
                        }
                    }
                    if (g_aVehicles[i].input->buttons & 0x20) {
                        TransferFlags(g_aVehicles[i].input, 0x20);
                        g_bScreenItemChosen = 0;
                        SetFadeTarget(0.0f, 0.2f);
                        FadeSfxTo(0.0f, 0.2f);
                        goto choose;
                    }
                }
                if (!(items[g_nScreenCursor]->flags & 8) &&
                    !(items[g_nScreenCursor]->flags & 0x40) &&
                    (g_aVehicles[i].input->buttons & 0xC010)) {
                    TransferFlags(g_aVehicles[i].input, 0xC010);
                    if (i == 0 || !(items[g_nScreenCursor]->flags & 0x20)) {
                        g_dwMenuChoosingPlayer = i;
                        if ((items[g_nScreenCursor]->flags & 2) == 0) {
                            g_bScreenItemChosen = 1;
                            FadeSfxTo(0.0f, 0.2f);
                            SetFadeTarget(0.0f, 0.2f);
                        } else {
                            *selection = g_nScreenCursor;
                            result = 1;
                        }
                    choose:
                        v = FindFreeSoundChannel();
                        if (v != -1) {
                            InitSoundChannel(
                                v, g_pScreenSelectSoundBuf, g_dwScreenSelectSoundSize, 0);
                        }
                    }
                }
            }
        }
        if (g_fScreenIdleTimeout != 0 && result == 0) {
            g_fScreenIdleTime = g_fScreenIdleTime + g_fFrameDeltaTime;
            if (g_fScreenIdleTimeout <= g_fScreenIdleTime && IsFadingOut() == 0) {
                g_bScreenItemChosen = 0;
                SetFadeTarget(0.0f, 0.2f);
                FadeSfxTo(0.0f, 0.2f);
            }
        }
    }
    g_nScreenLastActiveIndex = g_nScreenCursor;
    return result;
}
void LoadMenuItemModels(MenuItem **items, s32 *itemCount) {
    s32 i;

    /* Two frames of black in the mode of the screen. */
    g_dwKeepScreenBlanked = TRUE;
    if (g_dwScreenScaleShift) {
        BLANK_SCREEN();
        StartHighResFrame();
        FlushDisplayList();
        BLANK_SCREEN();
        StartHighResFrame();
        FlushDisplayList();
    } else {
        BLANK_SCREEN();
        StartLowResFrame();
        FlushDisplayList();
        BLANK_SCREEN();
        StartLowResFrame();
        FlushDisplayList();
    }
    g_dwKeepScreenBlanked = FALSE;

    /* Items with the same model on the cartridge share it. */
    for (*itemCount = 0; items[*itemCount] != NULL; (*itemCount)++) {
        if (items[*itemCount]->romStart == 0) {
            continue;
        }
        for (i = 0; i < *itemCount; i++) {
            if (items[*itemCount]->romStart == items[i]->romStart) {
                items[*itemCount]->model = items[i]->model;
                break;
            }
        }
        if (i == *itemCount) {
            items[*itemCount]->model =
                AllocHeapAligned(DmaReadAssetSize(ROM_MODEL_SOURCE(items[*itemCount]->romStart)));
            DecompressModel(items[*itemCount]->model,
                            ROM_MODEL_SOURCE(items[*itemCount]->romStart));
        }
    }
}

void ResetAndInitDisplay(MenuItem **items, s32 *itemCount) {
    ResetHeap();
    g_fScreenIdleTimeout = 0.0f;
    g_pfnScreenPostDrawCallback = NULL;
    g_pfnScreenPreDrawCallback = NULL;
    LoadMenuItemModels(items, itemCount);
}
