#include "common/splash.h"

#include <PR/gu.h>
#include <PR/os.h>

#include "common/audio.h"
#include "common/audiomix.h"
#include "common/fade.h"
#include "common/game.h"
#include "common/heap.h"
#include "common/input.h"
#include "common/render.h"
#include "common/system.h"
#include "common/text.h"
#include "common/timing.h"
#include "common/util.h"
#include "mainmenu.h"
#include "pakmenu.h"
#include "sprite.h"

/* Assets at fixed cartridge offsets. */
#ifdef COASTLINE_ROM_POINTERS
extern char gRomSplashSoundA[];
extern char gRomSplashSoundB[];
extern char gRomSplashFirstLogo[];
extern char gRomSplashDeveloperLogo[];
extern char gRomSplashProducerLogo[];
extern char gRomSplashTitleA[];
extern char gRomSplashTitleB[];
extern char gRomSplashTitleC[];
extern char gRomSplashTitleD[];
extern char gRomSplashTitlePalette[];

#define ROM_SPLASH_SOUND_A gRomSplashSoundA
#define ROM_SPLASH_SOUND_B gRomSplashSoundB
#define ROM_SPLASH_FIRST_LOGO gRomSplashFirstLogo
#define ROM_SPLASH_DEVELOPER_LOGO gRomSplashDeveloperLogo
#if TGR_REGION == TGR_REGION_JP
#define ROM_SPLASH_TITLE_PALETTE gRomSplashTitlePalette
#else
#define ROM_SPLASH_TITLE_A gRomSplashTitleA
#define ROM_SPLASH_TITLE_B gRomSplashTitleB
#define ROM_SPLASH_TITLE_C gRomSplashTitleC
#define ROM_SPLASH_TITLE_D gRomSplashTitleD
#endif
#if TGR_REGION == TGR_REGION_US
#define ROM_SPLASH_PRODUCER_LOGO gRomSplashProducerLogo
#endif
#else
#if TGR_REGION == TGR_REGION_EU
#define ROM_SPLASH_SOUND_A 0x1B4580
#define ROM_SPLASH_SOUND_B 0x1B8500
#define ROM_SPLASH_FIRST_LOGO 0x1DA840
#define ROM_SPLASH_DEVELOPER_LOGO 0x1DB870
#define ROM_SPLASH_TITLE_D 0x1DF9D0
#define ROM_SPLASH_TITLE_C 0x1E0870
#define ROM_SPLASH_TITLE_A 0x1DE720
#define ROM_SPLASH_TITLE_B 0x1E1AE0
#elif TGR_REGION == TGR_REGION_JP
#define ROM_SPLASH_SOUND_A 0x1CBDC0
#define ROM_SPLASH_SOUND_B 0x1CFD40
#define ROM_SPLASH_FIRST_LOGO 0x1EEAF0
#define ROM_SPLASH_DEVELOPER_LOGO 0x1EFB20
#define ROM_SPLASH_TITLE_PALETTE 0x1F5EA0
#elif TGR_REGION == TGR_REGION_ASIA
#define ROM_SPLASH_SOUND_A 0x1B4F00
#define ROM_SPLASH_SOUND_B 0x1B8E80
#define ROM_SPLASH_FIRST_LOGO 0x1DB500
#define ROM_SPLASH_DEVELOPER_LOGO 0x1DC530
#define ROM_SPLASH_TITLE_A 0x1DF3E0
#define ROM_SPLASH_TITLE_B 0x1E27A0
#define ROM_SPLASH_TITLE_C 0x1E1530
#define ROM_SPLASH_TITLE_D 0x1E0690
#else
#define ROM_SPLASH_SOUND_A 0x1B5440
#define ROM_SPLASH_SOUND_B 0x1B93C0
#define ROM_SPLASH_FIRST_LOGO 0x1DA030
#define ROM_SPLASH_PRODUCER_LOGO 0x1DBA40
#define ROM_SPLASH_DEVELOPER_LOGO 0x1DCD70
#define ROM_SPLASH_TITLE_D 0x1E1CA0
#define ROM_SPLASH_TITLE_C 0x1E2B40
#define ROM_SPLASH_TITLE_A 0x1E09F0
#define ROM_SPLASH_TITLE_B 0x1E3DB0
#endif /* TGR_REGION == TGR_REGION_EU */
#endif /* COASTLINE_ROM_POINTERS */

/* The silence after each sound, so that the mixer reads zeros past its end. */
#define SOUND_PADDING 0x100

#define HALF_PI 1.570796371f
#define PI 3.141592741f

/* The time of each fade, in seconds. */
#define FADE_TIME 0.2f

/* A volume of a sound effect channel from the levels of the right and the left. */
#define SFX_VOLUME(right, left) (((right) << 16) + (left))

/* The 32.32 step that plays a sample at its own rate. */
#define SFX_STEP_ONE (1ULL << 32)

/* The sound effect channels the splash screens silence when they end. */
#define SPLASH_SFX_CHANNELS 5

#if TGR_REGION == TGR_REGION_ASIA
#define LEGAL_TEXT_Y 190
#define LEGAL_TRADEMARK_Y 197
#else
#define LEGAL_TEXT_Y 195
#define LEGAL_TRADEMARK_Y 202
#endif

/* When each logo starts, in seconds. */
#define FIRST_LOGO_START 0.0f
#define PRODUCER_LOGO_START 4.0f
#if TGR_REGION != TGR_REGION_US
#define DEVELOPER_LOGO_START 4.0f
#else
#define DEVELOPER_LOGO_START 8.0f
#endif

/* The times of a logo after its start: flying in, holding, leaving and gone. */
#define LOGO_SOUND_A_TIME 0.35f
#define LOGO_SOUND_B_TIME 0.5f
#define LOGO_LANDED_TIME 2.0f
#define LOGO_LEAVE_TIME 3.5f
#define LOGO_END_TIME 6.0f

/* The developer logo is gone sooner than the other two. */
#if TGR_REGION == TGR_REGION_JP
#define DEVELOPER_LOGO_END_TIME 4.5f
#else
#define DEVELOPER_LOGO_END_TIME 5.0f
#endif

/* The depth of a logo at rest, and its scale. */
#define LOGO_DEPTH (-50.0f)
#define LOGO_SCALE 0.03125f

/* The brightest a caption or the copyright gets, and how fast they fade. */
#define TEXT_ALPHA 200
#define TEXT_FADE_RATE 4.0

/* When the parts of the title show, and when the copyright does. The European release, without
 * the first logo, shows them 4 earlier. */
#if TGR_REGION == TGR_REGION_EU || TGR_REGION == TGR_REGION_ASIA
#define TITLE_START 8.0f
#define TITLE_A_END 11.0f
#define TITLE_B_START 9.0f
#define TITLE_B_END 11.0f
#define TITLE_C_START 11.2f
#define TITLE_C_END 12.2f
#define TITLE_D_START 11.0f
#define TITLE_D_END 12.0f
#define COPYRIGHT_START 12.95f

#define SKIP_TIME 12.95f
#define SKIP_END_TIME 14.95f
#define SPLASH_END_TIME 18.2f
#elif TGR_REGION == TGR_REGION_JP
/* The Japanese release shows a picture of the title, fading it in for its first second. */
#define TITLE_START 8.5f
#define TITLE_FADE_TIME 1.0f
#define COPYRIGHT_START 10.5f

#define SKIP_TIME 10.5f
#define SKIP_END_TIME 12.5f
#define SPLASH_END_TIME 16.8f
#else
#define TITLE_START 12.0f
#define TITLE_A_END 15.0f
#define TITLE_B_START 13.0f
#define TITLE_B_END 15.0f
#define TITLE_C_START 15.2f
#define TITLE_C_END 16.2f
#define TITLE_D_START 15.0f
#define TITLE_D_END 16.0f
#define COPYRIGHT_START 17.2f

/* A skip before this jumps here; after the next time it ends the splash screens. */
#define SKIP_TIME 17.2f
#define SKIP_END_TIME 19.2f
#define SPLASH_END_TIME 24.2f
#endif /* TGR_REGION == TGR_REGION_EU */

/* The buttons that skip the splash screens. */
#define SKIP_BUTTONS (INPUT_Z | INPUT_START | INPUT_A)

/* The buttons held together that show the date of the build. */
#define BUILD_DATE_BUTTONS (INPUT_C_UP | INPUT_C_RIGHT | INPUT_C_DOWN | INPUT_C_LEFT)

/* The date of the build. The patched game shows the project, its commit, and when it was built. */
#ifdef ENABLE_PATCHES
#define BUILD_DATE_TEXT "github.com/Tatsh/coastline " COASTLINE_GIT_SHA " " __DATE__ " " __TIME__
#elif TGR_REGION == TGR_REGION_EU
#define BUILD_DATE_TEXT "25Sep97 13:46:05"
#elif TGR_REGION == TGR_REGION_JP
#define BUILD_DATE_TEXT "16Oct97 11:22:44"
#elif TGR_REGION == TGR_REGION_ASIA
#define BUILD_DATE_TEXT "12Sep97 10:45:38"
#else
#define BUILD_DATE_TEXT "25Aug97 11:46:52"
#endif

/* The frames before the controller check proceeds by itself. */
#define CONTROLLER_CHECK_FRAMES 910

void AnimateSplashModels(void) {
    AdvanceModelAnimations(g_pSplashDeveloperLogo);
#if TGR_REGION != TGR_REGION_JP
    AdvanceModelAnimations(g_pSplashTitleModelA);
    AdvanceModelAnimations(g_pSplashTitleModelB);
#endif
}

#if TGR_REGION != TGR_REGION_US
#if TGR_REGION == TGR_REGION_EU || TGR_REGION == TGR_REGION_ASIA
#define REGIONAL_TITLE_ASSET g_pSplashTitleModelA
#define INTRO_LOGO_SCALE 0.03125f
#else
#define REGIONAL_TITLE_ASSET g_awSplashTitlePalette
#define INTRO_LOGO_SCALE 0.028125f
#endif

void ProcessSplashScreens(void) {
#ifdef __sgi
    /* IDO places local splash state at the original BSS addresses. */
    static float g_fSplashTime;
    static int g_dwSplashSoundALength;
    static s8 *g_pSplashSoundA;
    static int g_dwSplashSoundBLength;
    static s8 *g_pSplashSoundB;
    static int g_dwSplashFirstLogoVoice;
    static int g_dwSplashFirstLogoSoundA;
    static int g_bSplashFirstLogoSoundB;
    static int g_dwSplashDeveloperVoice;
    static int g_dwSplashDeveloperSoundA;
    static int g_bSplashDeveloperSoundB;
    static int g_bSplashSkipped;
#endif
    int i;
    Mtx *mtx;

    if (g_bSplashStarted == 0) {
#if TGR_REGION != TGR_REGION_ASIA
        g_dwKeepScreenBlanked = 1;
        BLANK_SCREEN();
        StartHighResFrame();
        FlushDisplayList();
#endif
        SetFadeTarget(1.0f, 0.2f);
        FadeMusicTo(1.0f, 0.2f);
        FadeSfxTo(1.0f, 0.2f);
        ResetHeap();
#if TGR_REGION == TGR_REGION_EU || TGR_REGION == TGR_REGION_ASIA
        g_pSplashDeveloperLogo = AllocHeapAligned(DmaReadAssetSize(ROM_SPLASH_DEVELOPER_LOGO));
        DecompressModel(g_pSplashDeveloperLogo, ROM_SPLASH_DEVELOPER_LOGO);
        g_pSplashFirstLogo = AllocHeapAligned(DmaReadAssetSize(ROM_SPLASH_FIRST_LOGO));
        DecompressModel(g_pSplashFirstLogo, ROM_SPLASH_FIRST_LOGO);
        REGIONAL_TITLE_ASSET = AllocHeapAligned(DmaReadAssetSize(ROM_SPLASH_TITLE_A));
        DecompressModel(REGIONAL_TITLE_ASSET, ROM_SPLASH_TITLE_A);
        LoopModelAnimations(REGIONAL_TITLE_ASSET);
        g_pSplashTitleModelB = AllocHeapAligned(DmaReadAssetSize(ROM_SPLASH_TITLE_B));
        DecompressModel(g_pSplashTitleModelB, ROM_SPLASH_TITLE_B);
        LoopModelAnimations(g_pSplashTitleModelB);
        g_pSplashTitleModelD = AllocHeapAligned(DmaReadAssetSize(ROM_SPLASH_TITLE_D));
        DecompressModel(g_pSplashTitleModelD, ROM_SPLASH_TITLE_D);
        LoopModelAnimations(g_pSplashTitleModelD);
        g_pSplashTitleModelC = AllocHeapAligned(DmaReadAssetSize(ROM_SPLASH_TITLE_C));
        DecompressModel(g_pSplashTitleModelC, ROM_SPLASH_TITLE_C);
        LoopModelAnimations(g_pSplashTitleModelC);
#else
        g_pSplashDeveloperLogo = AllocHeapAligned(DmaReadAssetSize(ROM_SPLASH_DEVELOPER_LOGO));
        DecompressModel(g_pSplashDeveloperLogo, ROM_SPLASH_DEVELOPER_LOGO);
        g_pSplashFirstLogo = AllocHeapAligned(DmaReadAssetSize(ROM_SPLASH_FIRST_LOGO));
        DecompressModel(g_pSplashFirstLogo, ROM_SPLASH_FIRST_LOGO);
        LoadAndDecompressAsset(&g_SplashTitleLogoTexture, AllocHeapAligned);
        DecompressFromRom((unsigned char *)REGIONAL_TITLE_ASSET, ROM_SPLASH_TITLE_PALETTE, 0);
#endif
        g_dwSplashSoundALength = DmaReadAssetSize(ROM_SPLASH_SOUND_A);
        g_pSplashSoundA = AllocHeapAligned(g_dwSplashSoundALength + 0x100);
        DecompressFromRom(g_pSplashSoundA, ROM_SPLASH_SOUND_A, 0);
        g_dwSplashSoundBLength = DmaReadAssetSize(ROM_SPLASH_SOUND_B);
        g_pSplashSoundB = AllocHeapAligned(g_dwSplashSoundBLength + 0x100);
        DecompressFromRom(g_pSplashSoundB, ROM_SPLASH_SOUND_B, 0);
        for (i = 0; i != 0x100; i++) {
            g_pSplashSoundA[g_dwSplashSoundALength + i] = 0;
            g_pSplashSoundB[g_dwSplashSoundBLength + i] = 0;
        }
        g_bSplashFirstLogoSoundB = 0;
        g_dwSplashFirstLogoSoundA = 0;
        g_bSplashDeveloperSoundB = 0;
        g_dwSplashDeveloperSoundA = 0;
        g_bSplashSkipped = 0;
        g_bSplashStarted = 1;
#if TGR_REGION != TGR_REGION_ASIA
        g_dwKeepScreenBlanked = 0;
#endif
        g_fSplashTime = 0.0f;
    }
    UpdateFrameTiming();
    UpdateFadeTransitions();
    StartHighResFrame();
#if TGR_REGION == TGR_REGION_ASIA
    ClearDepthBuffer();
    ClearScreen(0, 0, 0);
#else
    ClearScreen(0, 0, 0);
    ClearDepthBuffer();
#endif
    AddTimingMarker(0, 0, 0, 200, 0xFF);
    if (IsFadingIn() == 0 || IsFadeComplete() != 0) {
        g_fSplashTime += g_fFrameDeltaTime;
    }
    g_dwKeepCustomClipPlanes = 1;
    g_fCameraNearScale = 10.0f;
    g_fCameraFarDistance = 5000.0f;
    SetupMenuProjection(g_dwScreenWidth, g_dwScreenHeight);
    SetViewport(0, 0, g_dwScreenWidth, g_dwScreenHeight, 1);
    gSPNumLights(gDisplayListPtr++, 1);
    gSPLight(gDisplayListPtr++, &g_SplashDirectionalLight, 1);
    gSPLight(gDisplayListPtr++, &g_SplashAmbientLight, 2);
    gSPClipRatio(gDisplayListPtr++, 6);
    gSPClipRatio(gDisplayListPtr++, 1);
    mtx = (Mtx *)AllocDisplayListBlock();
    guTranslateF(g_afMvpMatrix, g_dwScreenWidth / 2, g_dwScreenHeight / 2, 100.0f);
    guMtxF2L(g_afMvpMatrix, mtx);
    gSPMatrix(gDisplayListPtr++, mtx, G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_PUSH);

    if (g_fSplashTime >= 0.0f) {
        int a;

        if (g_fSplashTime < 2.0f) {
            a = 200 - (int)((2.0f - g_fSplashTime) * 255.0 * 4.0);
        } else {
            a = g_fSplashTime > 3.5f ? 200 - (int)((g_fSplashTime - 3.5f) * 255.0 * 4.0) : 200;
        }
        if (a > 0) {
            SetTextCustomColors(a, a, a, a, a, a);
            SetTextStyleNormal();
            SetTextSize(10);
            SetTextAlignCenter();
            DrawText("Produced by", 160, 70);
        }
        if (g_fSplashTime < 2.0f) {
            if (g_fSplashTime > 0.35f && g_dwSplashFirstLogoSoundA == 0) {
                short v1 = FindFreeSoundChannel();

                if (v1 != -1) {
                    InitSoundChannel(v1, g_pSplashSoundA, g_dwSplashSoundALength, 0);
                    g_aSoundEffectChannels[v1].volume = 0x200020;
                    g_aSoundEffectChannels[v1].step = 0x100000000;
                }
                g_dwSplashFirstLogoSoundA = 1;
            }
            if (g_fSplashTime > 0.5f && g_bSplashFirstLogoSoundB == 0) {
                short v2 = FindFreeSoundChannel();

                if (v2 != -1) {
                    InitSoundChannel(v2, g_pSplashSoundB, g_dwSplashSoundBLength, 0);
                    g_aSoundEffectChannels[v2].volume = 0x200020;
                    g_aSoundEffectChannels[v2].step = 0x100000000;
                }
                g_bSplashFirstLogoSoundB = 1;
            }
            DrawModelAt(g_pSplashFirstLogo,
                        -(2.0f - g_fSplashTime) * 32.0f,
                        0.0f,
                        -50.0f - (2.0f - g_fSplashTime) * 750.0f,
                        g_fSplashTime * 1.5707964f * g_fSplashTime * g_fSplashTime * g_fSplashTime *
                                0.0625f +
                            -3.1415927f,
                        INTRO_LOGO_SCALE);
        } else if (g_fSplashTime < 6.0f) {
            DrawModelAt(g_pSplashFirstLogo,
                        (g_fSplashTime > 3.5f ?
                             (g_fSplashTime - 3.5f) * (g_fSplashTime - 3.5f) * 256.0f :
                             0.0f),
                        0.0f,
                        -50.0f,
                        -1.5707964f,
                        INTRO_LOGO_SCALE);
            if (g_fSplashTime > 3.5f) {
                if (g_dwSplashFirstLogoSoundA < 2) {
                    g_dwSplashFirstLogoVoice = FindFreeSoundChannel();
                    if (g_dwSplashFirstLogoVoice != -1) {
                        InitSoundChannel(
                            g_dwSplashFirstLogoVoice, g_pSplashSoundA, g_dwSplashSoundALength, 0);
                        g_aSoundEffectChannels[g_dwSplashFirstLogoVoice].volume = 0x140014;
                        g_aSoundEffectChannels[g_dwSplashFirstLogoVoice].step = 0x100000000;
                    }
                    g_dwSplashFirstLogoSoundA = 2;
                } else {
                    int v;

                    v = (g_fSplashTime - 3.5f) * 32.0f;
                    if (v > 20) {
                        v = 20;
                    }
                    g_aSoundEffectChannels[g_dwSplashFirstLogoVoice].volume =
                        ((20 - v) << 16) + v + 20;
                }
            }
        }
    }

    if (g_fSplashTime >= 4.0f) {
        int a;

        if (g_fSplashTime < 6.0f) {
            a = 200 - (int)((6.0f - g_fSplashTime) * 255.0 * 4.0);
        } else {
            a = g_fSplashTime > 7.5f ? 200 - (int)((g_fSplashTime - 7.5f) * 255.0 * 4.0) : 200;
        }
        if (a > 0) {
            SetTextCustomColors(a, a, a, a, a, a);
            SetTextStyleNormal();
            SetTextSize(10);
            SetTextAlignCenter();
            DrawText("Developed by", 160, 60);
        }
        if (g_fSplashTime < 6.0f) {
            if (g_fSplashTime > 4.35f && g_dwSplashDeveloperSoundA == 0) {
                short v3 = FindFreeSoundChannel();

                if (v3 != -1) {
                    InitSoundChannel(v3, g_pSplashSoundA, g_dwSplashSoundALength, 0);
                    g_aSoundEffectChannels[v3].volume = 0x200020;
                    g_aSoundEffectChannels[v3].step = 0x100000000;
                }
                g_dwSplashDeveloperSoundA = 1;
            }
            if (g_fSplashTime > 4.5f && g_bSplashDeveloperSoundB == 0) {
                short v4 = FindFreeSoundChannel();

                if (v4 != -1) {
                    InitSoundChannel(v4, g_pSplashSoundB, g_dwSplashSoundBLength, 0);
                    g_aSoundEffectChannels[v4].volume = 0x200020;
                    g_aSoundEffectChannels[v4].step = 0x100000000;
                }
                g_bSplashDeveloperSoundB = 1;
            }
            DrawModelAt(g_pSplashDeveloperLogo,
                        -(6.0f - g_fSplashTime) * 32.0f,
                        0.0f,
                        -50.0f - (6.0f - g_fSplashTime) * 750.0f,
                        (g_fSplashTime - 4.0f) * 1.5707964f * (g_fSplashTime - 4.0f) *
                                (g_fSplashTime - 4.0f) * (g_fSplashTime - 4.0f) * 0.0625f +
                            -3.1415927f,
                        INTRO_LOGO_SCALE);
#if TGR_REGION == TGR_REGION_EU || TGR_REGION == TGR_REGION_ASIA
        } else if (g_fSplashTime < 9.0f) {
#else
        } else if (g_fSplashTime < 8.5f) {
#endif
            DrawModelAt(g_pSplashDeveloperLogo,
                        (g_fSplashTime > 7.5f ?
                             (g_fSplashTime - 7.5f) * (g_fSplashTime - 7.5f) * 256.0f :
                             0.0f),
                        0.0f,
                        -50.0f,
                        -1.5707964f,
                        INTRO_LOGO_SCALE);
            if (g_fSplashTime > 7.5f) {
                if (g_dwSplashDeveloperSoundA < 2) {
                    g_dwSplashDeveloperVoice = FindFreeSoundChannel();
                    if (g_dwSplashDeveloperVoice != -1) {
                        InitSoundChannel(
                            g_dwSplashDeveloperVoice, g_pSplashSoundA, g_dwSplashSoundALength, 0);
                        g_aSoundEffectChannels[g_dwSplashDeveloperVoice].volume = 0x140014;
                        g_aSoundEffectChannels[g_dwSplashDeveloperVoice].step = 0x100000000;
                    }
                    g_dwSplashDeveloperSoundA = 2;
                } else {
                    int v;

                    v = (g_fSplashTime - 7.5f) * 32.0f;
                    if (v > 20) {
                        v = 20;
                    }
                    g_aSoundEffectChannels[g_dwSplashDeveloperVoice].volume =
                        ((20 - v) << 16) + v + 20;
                }
            }
        }
    }

#if TGR_REGION == TGR_REGION_EU || TGR_REGION == TGR_REGION_ASIA
    if (g_fSplashTime > 8.0f) {
        gSPNumLights(gDisplayListPtr++, 1);
        gSPLight(gDisplayListPtr++, &g_MenuDirectionalLight, 1);
        gSPLight(gDisplayListPtr++, &g_MenuAmbientLight, 2);
        mtx = (Mtx *)AllocDisplayListBlock();
        guRotateF(g_afMvpMatrix, 7.0f, 0.0f, 0.0f, 1.0f);
        guMtxF2L(g_afMvpMatrix, mtx);
        gSPMatrix(gDisplayListPtr++, mtx, G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_PUSH);
        mtx = (Mtx *)AllocDisplayListBlock();
        guRotateF(g_afMvpMatrix, -7.0f, 1.0f, 0.0f, 0.0f);
        guMtxF2L(g_afMvpMatrix, mtx);
        gSPMatrix(gDisplayListPtr++, mtx, G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_NOPUSH);
        g_dwDrawModelListNoZBuffer = 1;
        if (g_fSplashTime > 11.0f) {
            SetScissorRect(40, 100, 250, 50);
        }
        DrawModelAt(
            REGIONAL_TITLE_ASSET,
            0.0f,
            0.0f,
            (g_fSplashTime < 11.0f ? (sqrtf(3.0f) - sqrtf(g_fSplashTime - 8.0f)) * 300.0f : -0.0f),
            sqrtf(g_fSplashTime - 8.0f) * 0.01f + -1.5707964f,
            0.05f);
        gSPPopMatrix(gDisplayListPtr++, 0);
        SetScissorRect(0, 0, 320, 240);
    }
    if (g_fSplashTime > 9.0f) {
        g_dwDrawModelListNoZBuffer = 1;
        DrawModelAt(
            g_pSplashTitleModelB,
            0.0f,
            0.0f,
            (g_fSplashTime < 11.0f ? (sqrtf(2.0f) - sqrtf(g_fSplashTime - 9.0f)) * 500.0f : 0.0f),
            -1.5707964f,
            0.083333336f);
    }
    if (g_fSplashTime > 11.2f) {
        DrawModelAt(
            g_pSplashTitleModelC,
            0.0f,
            0.0f,
            (g_fSplashTime < 12.2f ? (sqrtf(1.0f) - sqrtf(g_fSplashTime - 11.2f)) * 500.0f : 0.0f),
            -1.5707964f,
            0.083333336f);
    }
    if (g_fSplashTime > 11.0f) {
        DrawModelAt(
            g_pSplashTitleModelD,
            0.0f,
            0.0f,
            (g_fSplashTime < 12.0f ? (sqrtf(1.0f) - sqrtf(g_fSplashTime - 11.0f)) * 500.0f : 0.0f),
            -1.5707964f,
            0.083333336f);
    }
#define INTRO_LEGAL 12.95f
#define INTRO_SKIP 14.95f
#define INTRO_END 18.2f
#else
#define INTRO_LEGAL 10.5f
#define INTRO_SKIP 12.5f
#define INTRO_END 16.8f
    if (g_fSplashTime > 8.5f) {
        gDPPipeSync(gDisplayListPtr++);
        gDPSetTextureImage(gDisplayListPtr++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, REGIONAL_TITLE_ASSET);
        gDPTileSync(gDisplayListPtr++);
        gDPSetTile(gDisplayListPtr++, 0, 0, 0, 256, 7, 0, 0, 0, 0, 0, 0, 0);
        gDPLoadSync(gDisplayListPtr++);
        gDPLoadTLUTCmd(gDisplayListPtr++, 7, 255);
        gDPPipeSync(gDisplayListPtr++);
        DrawTexturedRect(
            &g_SplashTitleLogoTexture, 60, 36, 200, 150, 0, 0, 0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0);
        if (g_fSplashTime - 8.5f < 1.0f) {
            DrawScreenFadeOverlay(g_fSplashTime - 8.5f);
        }
    }
#endif
    if (g_fSplashTime > INTRO_LEGAL) {
        int a;

        a = (g_fSplashTime - INTRO_LEGAL) * 255.0 * 4.0;
        if (a > 200) {
            a = 200;
        }
        SetTextCustomColors(a, a, a, a, a, a);
        SetTextStyleNormal();
        SetTextSize(10);
        SetTextAlignCenter();
        if ((g_aPlayerInputState[0].buttons & 0xF00) == 0xF00) {
            DrawText(BUILD_DATE_TEXT, 160, 40);
        }
        SetTextSize(7);
        DrawText("O 1997 Boss Game Studios, Inc.  All Rights Reserved.", 160, LEGAL_TEXT_Y);
        DrawText("Top Gear Rally is a trademark of Kemco.", 160, LEGAL_TRADEMARK_Y);
        DrawText("LICENSED BY NINTENDO", 160, 214);
        SetTextSize(3);
        DrawText("c", 78, LEGAL_TEXT_Y);
    }
    gSPPopMatrix(gDisplayListPtr++, 0);
    ApplyScreenFade();
    if (g_dwHudDebugLevel >= 2) {
        SetViewport(0, 0, g_dwScreenWidth, g_dwScreenHeight, 1);
        DrawTimingBars();
    }
    SetNextScreenCallback(AnimateSplashModels);
    FlushDisplayList();
    if (IsFadingOut() != 0) {
        if (IsFadeComplete() != 0) {
            g_bSplashStarted = 0;
            g_aSoundEffectChannels[0].volume = g_aSoundEffectChannels[0].step = 0;
            g_aSoundEffectChannels[1].volume = g_aSoundEffectChannels[1].step = 0;
            g_aSoundEffectChannels[2].volume = g_aSoundEffectChannels[2].step = 0;
            g_aSoundEffectChannels[3].volume = g_aSoundEffectChannels[3].step = 0;
            g_aSoundEffectChannels[4].volume = g_aSoundEffectChannels[4].step = 0;
#if TGR_REGION == TGR_REGION_ASIA
            ClearFramebuffersForWideMode();
#endif
            if (g_bSplashSkipped != 0) {
#if TGR_REGION != TGR_REGION_ASIA
                ClearFramebuffersForWideMode();
#endif
                SetCurrentScreenCallback(ProcessMainMenu);
            } else {
#if TGR_REGION != TGR_REGION_ASIA
                BLANK_SCREEN();
                g_dwKeepScreenBlanked = 1;
                ClearFramebuffersLowRes();
                g_dwKeepScreenBlanked = 0;
#endif
                StartAttractDemo();
            }
        }
        return;
    }
    if (g_bSplashSkipped != 0 && g_fSplashTime > INTRO_SKIP) {
        goto fade;
    }
    for (i = 0; i != 2; i++) {
#if TGR_REGION == TGR_REGION_EU || TGR_REGION == TGR_REGION_ASIA
        if (g_aPlayerInputState[i].buttons & 0xC010) {
            TransferFlags(&g_aPlayerInputState[i], 0xC010);
#else
        PlayerInputState *pad = &g_aPlayerInputState[i];

        if (pad->buttons & 0xC010) {
            TransferFlags(pad, 0xC010);
#endif
            g_bSplashSkipped = 1;
            if (g_bSplashFadingOut != 0) {
                goto fade;
            }
            if (g_fSplashTime < INTRO_LEGAL) {
                g_fSplashTime = INTRO_LEGAL;
            } else if (g_fSplashTime > INTRO_SKIP) {
                goto fade;
            }
        }
    }
    if (g_fSplashTime >= INTRO_END) {
    fade:
        g_bSplashFadingOut = 1;
#if TGR_REGION != TGR_REGION_ASIA
        osSyncPrintf("start fade out\n");
#endif
        SetFadeTarget(0.0f, 0.2f);
        FadeSfxTo(0.0f, 0.2f);
    }
}
#else
void ProcessSplashScreens(void) {
#ifdef __sgi
    /* IDO allocates these in the original BSS; GCC uses the equivalent global addresses. */
    static float g_fSplashTime;
    static int g_dwSplashSoundALength;
    static s8 *g_pSplashSoundA;
    static int g_dwSplashSoundBLength;
    static s8 *g_pSplashSoundB;
    static int g_dwSplashFirstLogoVoice;
    static int g_dwSplashFirstLogoSoundA;
    static int g_bSplashFirstLogoSoundB;
    static int g_dwSplashProducerVoice;
    static int g_dwSplashProducerSoundA;
    static int g_bSplashProducerSoundB;
    static int g_dwSplashDeveloperVoice;
    static int g_dwSplashDeveloperSoundA;
    static int g_bSplashDeveloperSoundB;
    static int g_bSplashSkipped;
#endif
    int i;
    Mtx *mtx;

    if (g_bSplashStarted == 0) {
        SetFadeTarget(1.0f, 0.2f);
        FadeMusicTo(1.0f, 0.2f);
        FadeSfxTo(1.0f, 0.2f);
        ResetHeap();
        g_pSplashDeveloperLogo = AllocHeapAligned(DmaReadAssetSize(ROM_SPLASH_DEVELOPER_LOGO));
        DecompressModel(g_pSplashDeveloperLogo, ROM_SPLASH_DEVELOPER_LOGO);
        g_pSplashProducerLogo = AllocHeapAligned(DmaReadAssetSize(ROM_SPLASH_PRODUCER_LOGO));
        DecompressModel(g_pSplashProducerLogo, ROM_SPLASH_PRODUCER_LOGO);
        g_pSplashFirstLogo = AllocHeapAligned(DmaReadAssetSize(ROM_SPLASH_FIRST_LOGO));
        DecompressModel(g_pSplashFirstLogo, ROM_SPLASH_FIRST_LOGO);
        g_pSplashTitleModelA = AllocHeapAligned(DmaReadAssetSize(ROM_SPLASH_TITLE_A));
        DecompressModel(g_pSplashTitleModelA, ROM_SPLASH_TITLE_A);
        LoopModelAnimations(g_pSplashTitleModelA);
        g_pSplashTitleModelB = AllocHeapAligned(DmaReadAssetSize(ROM_SPLASH_TITLE_B));
        DecompressModel(g_pSplashTitleModelB, ROM_SPLASH_TITLE_B);
        LoopModelAnimations(g_pSplashTitleModelB);
        g_pSplashTitleModelD = AllocHeapAligned(DmaReadAssetSize(ROM_SPLASH_TITLE_D));
        DecompressModel(g_pSplashTitleModelD, ROM_SPLASH_TITLE_D);
        LoopModelAnimations(g_pSplashTitleModelD);
        g_pSplashTitleModelC = AllocHeapAligned(DmaReadAssetSize(ROM_SPLASH_TITLE_C));
        DecompressModel(g_pSplashTitleModelC, ROM_SPLASH_TITLE_C);
        LoopModelAnimations(g_pSplashTitleModelC);
        g_dwSplashSoundALength = DmaReadAssetSize(ROM_SPLASH_SOUND_A);
        g_pSplashSoundA = AllocHeapAligned(g_dwSplashSoundALength + 0x100);
        DecompressFromRom(g_pSplashSoundA, ROM_SPLASH_SOUND_A, 0);
        g_dwSplashSoundBLength = DmaReadAssetSize(ROM_SPLASH_SOUND_B);
        g_pSplashSoundB = AllocHeapAligned(g_dwSplashSoundBLength + 0x100);
        DecompressFromRom(g_pSplashSoundB, ROM_SPLASH_SOUND_B, 0);
        for (i = 0; i != 0x100; i++) {
            g_pSplashSoundA[g_dwSplashSoundALength + i] = 0;
            g_pSplashSoundB[g_dwSplashSoundBLength + i] = 0;
        }
        g_bSplashFirstLogoSoundB = 0;
        g_dwSplashFirstLogoSoundA = 0;
        g_bSplashProducerSoundB = 0;
        g_dwSplashProducerSoundA = 0;
        g_bSplashDeveloperSoundB = 0;
        g_dwSplashDeveloperSoundA = 0;
        g_bSplashSkipped = 0;
        g_bSplashStarted = 1;
        g_fSplashTime = 0.0f;
    }
    UpdateFrameTiming();
    UpdateFadeTransitions();
    StartHighResFrame();
    ClearDepthBuffer();
    ClearScreen(0, 0, 0);
    AddTimingMarker(0, 0, 0, 200, 0xFF);
    if (IsFadingIn() == 0 || IsFadeComplete() != 0) {
        g_fSplashTime += g_fFrameDeltaTime;
    }
    g_dwKeepCustomClipPlanes = 1;
    g_fCameraNearScale = 10.0f;
    g_fCameraFarDistance = 5000.0f;
    SetupMenuProjection(g_dwScreenWidth, g_dwScreenHeight);
    SetViewport(0, 0, g_dwScreenWidth, g_dwScreenHeight, 1);
    gSPNumLights(gDisplayListPtr++, 1);
    gSPLight(gDisplayListPtr++, &g_SplashDirectionalLight, 1);
    gSPLight(gDisplayListPtr++, &g_SplashAmbientLight, 2);
    gSPClipRatio(gDisplayListPtr++, 6);
    gSPClipRatio(gDisplayListPtr++, 1);
    mtx = (Mtx *)AllocDisplayListBlock();
    guTranslateF(g_afMvpMatrix, g_dwScreenWidth / 2, g_dwScreenHeight / 2, 100.0f);
    guMtxF2L(g_afMvpMatrix, mtx);
    gSPMatrix(gDisplayListPtr++, mtx, G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_PUSH);

    if (g_fSplashTime >= 0.0f) {
        if (g_fSplashTime < 2.0f) {
            if (g_fSplashTime > 0.35f && g_dwSplashFirstLogoSoundA == 0) {
                short v1 = FindFreeSoundChannel();

                if (v1 != -1) {
                    InitSoundChannel(v1, g_pSplashSoundA, g_dwSplashSoundALength, 0);
                    g_aSoundEffectChannels[v1].volume = 0x200020;
                    g_aSoundEffectChannels[v1].step = 0x100000000;
                }
                g_dwSplashFirstLogoSoundA = 1;
            }
            if (g_fSplashTime > 0.5f && g_bSplashFirstLogoSoundB == 0) {
                short v2 = FindFreeSoundChannel();

                if (v2 != -1) {
                    InitSoundChannel(v2, g_pSplashSoundB, g_dwSplashSoundBLength, 0);
                    g_aSoundEffectChannels[v2].volume = 0x200020;
                    g_aSoundEffectChannels[v2].step = 0x100000000;
                }
                g_bSplashFirstLogoSoundB = 1;
            }
            DrawModelAt(g_pSplashFirstLogo,
                        -(2.0f - g_fSplashTime) * 32.0f,
                        0.0f,
                        -50.0f - (2.0f - g_fSplashTime) * 750.0f,
                        g_fSplashTime * 1.5707964f * g_fSplashTime * g_fSplashTime * g_fSplashTime *
                                0.0625f +
                            -3.1415927f,
                        0.03125f);
        } else if (g_fSplashTime < 6.0f) {
            DrawModelAt(g_pSplashFirstLogo,
                        (g_fSplashTime > 3.5f ?
                             (g_fSplashTime - 3.5f) * (g_fSplashTime - 3.5f) * 256.0f :
                             0.0f),
                        0.0f,
                        -50.0f,
                        -1.5707964f,
                        0.03125f);
            if (g_fSplashTime > 3.5f) {
                if (g_dwSplashFirstLogoSoundA < 2) {
                    g_dwSplashFirstLogoVoice = FindFreeSoundChannel();
                    if (g_dwSplashFirstLogoVoice != -1) {
                        InitSoundChannel(
                            g_dwSplashFirstLogoVoice, g_pSplashSoundA, g_dwSplashSoundALength, 0);
                        g_aSoundEffectChannels[g_dwSplashFirstLogoVoice].volume = 0x140014;
                        g_aSoundEffectChannels[g_dwSplashFirstLogoVoice].step = 0x100000000;
                    }
                    g_dwSplashFirstLogoSoundA = 2;
                } else {
                    int v;

                    v = (g_fSplashTime - 3.5f) * 32.0f;
                    if (v > 20) {
                        v = 20;
                    }
                    g_aSoundEffectChannels[g_dwSplashFirstLogoVoice].volume =
                        ((20 - v) << 16) + v + 20;
                }
            }
        }
    }

    if (g_fSplashTime >= 4.0f) {
        int a;

        if (g_fSplashTime < 6.0f) {
            a = 200 - (int)((6.0f - g_fSplashTime) * 255.0 * 4.0);
        } else {
            a = g_fSplashTime > 7.5f ? 200 - (int)((g_fSplashTime - 7.5f) * 255.0 * 4.0) : 200;
        }
        if (a > 0) {
            SetTextCustomColors(a, a, a, a, a, a);
            SetTextStyleNormal();
            SetTextSize(10);
            SetTextAlignCenter();
            DrawText("Produced by", 160, 70);
        }
        if (g_fSplashTime < 6.0f) {
            if (g_fSplashTime > 4.35f && g_dwSplashProducerSoundA == 0) {
                short v3 = FindFreeSoundChannel();

                if (v3 != -1) {
                    InitSoundChannel(v3, g_pSplashSoundA, g_dwSplashSoundALength, 0);
                    g_aSoundEffectChannels[v3].volume = 0x200020;
                    g_aSoundEffectChannels[v3].step = 0x100000000;
                }
                g_dwSplashProducerSoundA = 1;
            }
            if (g_fSplashTime > 4.5f && g_bSplashProducerSoundB == 0) {
                short v4 = FindFreeSoundChannel();

                if (v4 != -1) {
                    InitSoundChannel(v4, g_pSplashSoundB, g_dwSplashSoundBLength, 0);
                    g_aSoundEffectChannels[v4].volume = 0x200020;
                    g_aSoundEffectChannels[v4].step = 0x100000000;
                }
                g_bSplashProducerSoundB = 1;
            }
            DrawModelAt(g_pSplashProducerLogo,
                        -(6.0f - g_fSplashTime) * 32.0f,
                        0.0f,
                        -50.0f - (6.0f - g_fSplashTime) * 750.0f,
                        (g_fSplashTime - 4.0f) * 1.5707964f * (g_fSplashTime - 4.0f) *
                                (g_fSplashTime - 4.0f) * (g_fSplashTime - 4.0f) * 0.0625f +
                            -3.1415927f,
                        0.03125f);
        } else if (g_fSplashTime < 10.0f) {
            DrawModelAt(g_pSplashProducerLogo,
                        (g_fSplashTime > 7.5f ?
                             (g_fSplashTime - 7.5f) * (g_fSplashTime - 7.5f) * 256.0f :
                             0.0f),
                        0.0f,
                        -50.0f,
                        -1.5707964f,
                        0.03125f);
            if (g_fSplashTime > 7.5f) {
                if (g_dwSplashProducerSoundA < 2) {
                    g_dwSplashProducerVoice = FindFreeSoundChannel();
                    if (g_dwSplashProducerVoice != -1) {
                        InitSoundChannel(
                            g_dwSplashProducerVoice, g_pSplashSoundA, g_dwSplashSoundALength, 0);
                        g_aSoundEffectChannels[g_dwSplashProducerVoice].volume = 0x140014;
                        g_aSoundEffectChannels[g_dwSplashProducerVoice].step = 0x100000000;
                    }
                    g_dwSplashProducerSoundA = 2;
                } else {
                    int v;

                    v = (g_fSplashTime - 7.5f) * 32.0f;
                    if (v > 20) {
                        v = 20;
                    }
                    g_aSoundEffectChannels[g_dwSplashProducerVoice].volume =
                        ((20 - v) << 16) + v + 20;
                }
            }
        }
    }

    if (g_fSplashTime >= 8.0f) {
        int a;

        if (g_fSplashTime < 10.0f) {
            a = 200 - (int)((10.0f - g_fSplashTime) * 255.0 * 4.0);
        } else {
            a = g_fSplashTime > 11.5f ? 200 - (int)((g_fSplashTime - 11.5f) * 255.0 * 4.0) : 200;
        }
        if (a > 0) {
            SetTextCustomColors(a, a, a, a, a, a);
            SetTextStyleNormal();
            SetTextSize(10);
            SetTextAlignCenter();
            DrawText("Developed by", 160, 60);
        }
        if (g_fSplashTime < 10.0f) {
            if (g_fSplashTime > 8.35f && g_dwSplashDeveloperSoundA == 0) {
                short v5 = FindFreeSoundChannel();

                if (v5 != -1) {
                    InitSoundChannel(v5, g_pSplashSoundA, g_dwSplashSoundALength, 0);
                    g_aSoundEffectChannels[v5].volume = 0x200020;
                    g_aSoundEffectChannels[v5].step = 0x100000000;
                }
                g_dwSplashDeveloperSoundA = 1;
            }
            if (g_fSplashTime > 8.5f && g_bSplashDeveloperSoundB == 0) {
                short v6 = FindFreeSoundChannel();

                if (v6 != -1) {
                    InitSoundChannel(v6, g_pSplashSoundB, g_dwSplashSoundBLength, 0);
                    g_aSoundEffectChannels[v6].volume = 0x200020;
                    g_aSoundEffectChannels[v6].step = 0x100000000;
                }
                g_bSplashDeveloperSoundB = 1;
            }
            DrawModelAt(g_pSplashDeveloperLogo,
                        -(10.0f - g_fSplashTime) * 32.0f,
                        0.0f,
                        -50.0f - (10.0f - g_fSplashTime) * 750.0f,
                        (g_fSplashTime - 8.0f) * 1.5707964f * (g_fSplashTime - 8.0f) *
                                (g_fSplashTime - 8.0f) * (g_fSplashTime - 8.0f) * 0.0625f +
                            -3.1415927f,
                        0.03125f);
        } else if (g_fSplashTime < 13.0f) {
            DrawModelAt(g_pSplashDeveloperLogo,
                        (g_fSplashTime > 11.5f ?
                             (g_fSplashTime - 11.5f) * (g_fSplashTime - 11.5f) * 256.0f :
                             0.0f),
                        0.0f,
                        -50.0f,
                        -1.5707964f,
                        0.03125f);
            if (g_fSplashTime > 11.5f) {
                if (g_dwSplashDeveloperSoundA < 2) {
                    g_dwSplashDeveloperVoice = FindFreeSoundChannel();
                    if (g_dwSplashDeveloperVoice != -1) {
                        InitSoundChannel(
                            g_dwSplashDeveloperVoice, g_pSplashSoundA, g_dwSplashSoundALength, 0);
                        g_aSoundEffectChannels[g_dwSplashDeveloperVoice].volume = 0x140014;
                        g_aSoundEffectChannels[g_dwSplashDeveloperVoice].step = 0x100000000;
                    }
                    g_dwSplashDeveloperSoundA = 2;
                } else {
                    int v;

                    v = (g_fSplashTime - 11.5f) * 32.0f;
                    if (v > 20) {
                        v = 20;
                    }
                    g_aSoundEffectChannels[g_dwSplashDeveloperVoice].volume =
                        ((20 - v) << 16) + v + 20;
                }
            }
        }
    }

    if (g_fSplashTime > 12.0f) {
        gSPNumLights(gDisplayListPtr++, 1);
        gSPLight(gDisplayListPtr++, &g_MenuDirectionalLight, 1);
        gSPLight(gDisplayListPtr++, &g_MenuAmbientLight, 2);
        mtx = (Mtx *)AllocDisplayListBlock();
        guRotateF(g_afMvpMatrix, 7.0f, 0.0f, 0.0f, 1.0f);
        guMtxF2L(g_afMvpMatrix, mtx);
        gSPMatrix(gDisplayListPtr++, mtx, G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_PUSH);
        mtx = (Mtx *)AllocDisplayListBlock();
        guRotateF(g_afMvpMatrix, -7.0f, 1.0f, 0.0f, 0.0f);
        guMtxF2L(g_afMvpMatrix, mtx);
        gSPMatrix(gDisplayListPtr++, mtx, G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_NOPUSH);
        g_dwDrawModelListNoZBuffer = 1;
        if (g_fSplashTime > 15.0f) {
            SetScissorRect(40, 100, 250, 50);
        }
        DrawModelAt(
            g_pSplashTitleModelA,
            0.0f,
            0.0f,
            (g_fSplashTime < 15.0f ? (sqrtf(3.0f) - sqrtf(g_fSplashTime - 12.0f)) * 300.0f : -0.0f),
            sqrtf(g_fSplashTime - 12.0f) * 0.01f + -1.5707964f,
            0.05f);
        gSPPopMatrix(gDisplayListPtr++, 0);
        SetScissorRect(0, 0, 320, 240);
    }
    if (g_fSplashTime > 13.0f) {
        g_dwDrawModelListNoZBuffer = 1;
        DrawModelAt(
            g_pSplashTitleModelB,
            0.0f,
            0.0f,
            (g_fSplashTime < 15.0f ? (sqrtf(2.0f) - sqrtf(g_fSplashTime - 13.0f)) * 500.0f : 0.0f),
            -1.5707964f,
            0.083333336f);
    }
    if (g_fSplashTime > 15.2f) {
        DrawModelAt(g_pSplashTitleModelC,
                    0.0f,
                    0.0f,
                    (g_fSplashTime < 16.2f ?
                         (sqrtf(1.000001f) - sqrtf(g_fSplashTime - 15.2f)) * 500.0f :
                         0.0f),
                    -1.5707964f,
                    0.083333336f);
    }
    if (g_fSplashTime > 15.0f) {
        DrawModelAt(
            g_pSplashTitleModelD,
            0.0f,
            0.0f,
            (g_fSplashTime < 16.0f ? (sqrtf(1.0f) - sqrtf(g_fSplashTime - 15.0f)) * 500.0f : 0.0f),
            -1.5707964f,
            0.083333336f);
    }
    if (g_fSplashTime > 17.2f) {
        int a;

        a = (g_fSplashTime - 17.2f) * 255.0 * 4.0;
        if (a > 200) {
            a = 200;
        }
        SetTextCustomColors(a, a, a, a, a, a);
        SetTextStyleNormal();
        SetTextSize(10);
        SetTextAlignCenter();
        if ((g_aPlayerInputState[0].buttons & 0xF00) == 0xF00) {
            DrawText(BUILD_DATE_TEXT, 160, 40);
        }
        SetTextSize(7);
        DrawText("O 1997 Boss Game Studios, Inc.  All Rights Reserved.", 160, 190);
        DrawText("Top Gear Rally is a trademark of Kemco.", 160, 197);
        DrawText("Distributed by Midway Home Entertainment under license.", 160, 204);
        DrawText("Midway is a trademark of Midway Games Inc.  Used by permission.", 160, 211);
        DrawText("LICENSED BY NINTENDO", 160, 224);
        SetTextSize(3);
        DrawText("c", 78, 190);
    }
    gSPPopMatrix(gDisplayListPtr++, 0);
    ApplyScreenFade();
    if (g_dwHudDebugLevel >= 2) {
        SetViewport(0, 0, g_dwScreenWidth, g_dwScreenHeight, 1);
        DrawTimingBars();
    }
    SetNextScreenCallback(AnimateSplashModels);
    FlushDisplayList();
    if (IsFadingOut() != 0) {
        if (IsFadeComplete() != 0) {
            g_bSplashStarted = 0;
            g_aSoundEffectChannels[0].volume = g_aSoundEffectChannels[0].step = 0;
            g_aSoundEffectChannels[1].volume = g_aSoundEffectChannels[1].step = 0;
            g_aSoundEffectChannels[2].volume = g_aSoundEffectChannels[2].step = 0;
            g_aSoundEffectChannels[3].volume = g_aSoundEffectChannels[3].step = 0;
            g_aSoundEffectChannels[4].volume = g_aSoundEffectChannels[4].step = 0;
            ClearFramebuffersForWideMode();
            if (g_bSplashSkipped != 0) {
                SetCurrentScreenCallback(ProcessMainMenu);
            } else {
                StartAttractDemo();
            }
        }
        return;
    }
    if (g_bSplashSkipped != 0 && g_fSplashTime > 19.2f) {
        goto fade;
    }
    for (i = 0; i != 2; i++) {
        if (g_aPlayerInputState[i].buttons & 0xC010) {
            TransferFlags(&g_aPlayerInputState[i], 0xC010);
            g_bSplashSkipped = 1;
            if (g_bSplashFadingOut != 0) {
                goto fade;
            }
            if (g_fSplashTime < 17.2f) {
                g_fSplashTime = 17.2f;
            } else if (g_fSplashTime > 19.2f) {
                goto fade;
            }
        }
    }
    if (g_fSplashTime >= 24.2f) {
    fade:
        g_bSplashFadingOut = 1;
        SetFadeTarget(0.0f, 0.2f);
        FadeSfxTo(0.0f, 0.2f);
    }
}
#endif

void ProcessControllerCheck(void) {
#if TGR_REGION != TGR_REGION_US && TGR_REGION != TGR_REGION_ASIA
    /* The value of the flag comes from a variable, loaded in a register of its own. */
    u32 blanked = TRUE;

#endif
    if (!g_bControllerCheckStarted) {
        g_bControllerCheckStarted = 1;
#if TGR_REGION == TGR_REGION_US || TGR_REGION == TGR_REGION_ASIA
        ResetHeap();
        /* Show two black frames before the pictures load. */
        osViBlack(1);
        StartHighResFrame();
        FlushDisplayList();
        osViBlack(1);
        StartHighResFrame();
        FlushDisplayList();
#else
        /* Show three black frames, the heap reset after the first, before the pictures load. */
        g_dwKeepScreenBlanked = blanked;
        BlankDisplay();
        StartHighResFrame();
        FlushDisplayList();
        ResetHeap();
        BlankDisplay();
        StartHighResFrame();
        FlushDisplayList();
        BlankDisplay();
        StartHighResFrame();
        FlushDisplayList();
#endif /* TGR_REGION == TGR_REGION_US */
        LoadTextureFromRom(&g_texBackground);
        LoadTextureFromRom(&g_texLogo);
        LoadTextureFromRom(&g_texButtonA);
        SetFadeTarget(1.0f, FADE_TIME);
        g_wScreenFrameCount = 0;
#if TGR_REGION != TGR_REGION_US && TGR_REGION != TGR_REGION_ASIA
        g_dwKeepScreenBlanked = FALSE;
#endif
    }

    UpdateFrameTiming();
    UpdateFadeTransitions();
    StartHighResFrame();
    SetupProjectionMatrix((f32)g_dwScreenWidth, (f32)g_dwScreenHeight);
    SetViewport(0, 0, g_dwScreenWidth, g_dwScreenHeight, 1);
    ClearDepthBuffer();
    ClearScreen(0, 0, 0);
    DrawSpriteTinted(&g_texBackground, 192, 0, 64);
    DrawTexturedSprite(&g_texLogo, g_texLogo.originX, g_texLogo.originY + 8);

    SetTextStyleNormal();
    SetTextAlignCenter();
    SetTextSize(30);
    DrawText("%ryWARNING", 160, 41);
#if TGR_REGION == TGR_REGION_US || TGR_REGION == TGR_REGION_ASIA
    SetTextSize(18);
    switch (g_bControllerWarningType) {
    case 0:
        DrawText("%ywCONTROLLER NOT FOUND", 160, 124);
        break;
    case 1:
        DrawText("%ywCONTROLLER PAK NOT FOUND", 160, 105);
        DrawText("%ywGAME DATA CANNOT BE SAVED", 160, 125);
        DrawText("%ywIF YOU CHOOSE TO PROCEED", 160, 145);
        break;
    case 2:
        DrawText("%ywCONTROLLER PAK CONTAINS GAME", 160, 85);
        DrawText("%ywDATA SAVED FROM ANOTHER GAME.", 160, 105);
        DrawText("%ywTHERE MAY NOT BE ENOUGH PAGES", 160, 125);
        DrawText("%ywAND/OR FILES FREE WHEN YOU NEED", 160, 145);
        DrawText("%ywTO SAVE GAME DATA FROM THIS GAME.", 160, 165);
        break;
    }
    DrawTexturedSprite(&g_texButtonA, 254, 432);
    SetTextAlignLeft();
    SetTextSize(11);
    DrawText("%wwPROCEED", 142, g_dwScreenHeight * 19 / 20 - 3);
#elif TGR_REGION == TGR_REGION_EU
    SetTextSize(18);
    switch (g_bControllerWarningType) {
    case 0:
        DrawText("%ywCONTROLLER NOT FOUND", 160, 124);
        break;
    case 1:
        DrawText("%ywCONTROLLER PAK NOT FOUND", 160, 95);
        DrawText("%ywGHOST CAR AND PAINT SHOP DATA", 160, 115);
        DrawText("%ywCANNOT BE SAVED", 160, 135);
        DrawText("%ywIF YOU CHOOSE TO PROCEED", 160, 155);
        break;
    case 2:
        DrawText("%ywCONTROLLER PAK CONTAINS GAME", 160, 85);
        DrawText("%ywDATA SAVED FROM ANOTHER GAME.", 160, 105);
        DrawText("%ywTHERE MAY NOT BE ENOUGH PAGES", 160, 125);
        DrawText("%ywAND/OR FILES FREE WHEN YOU NEED", 160, 145);
        DrawText("%ywTO SAVE GAME DATA FROM THIS GAME.", 160, 165);
        break;
    }
    SetTextAlignLeft();
    SetTextSize(11);
    DrawTexturedSprite(&g_texButtonA, 254, 432);
    DrawText("%wwPROCEED", 142, g_dwScreenHeight * 19 / 20 - 3);
#else
    SetTextSize(20);
    switch (g_bControllerWarningType) {
    case 0:
        DrawText(" %yw%jコントローラがありません。", 160, 124);
        break;
    case 1:
        DrawText("%yw%j コントローラパックがないので、", 160, 95);
        DrawText("%yw%jゴーストカーデータとペイントショップ", 160, 115);
        DrawText("%yw %jのデカールデータがセーブできません。", 160, 135);
        DrawText("%yw%jゲームをはじめますか？", 160, 155);
        break;
    case 2:
        DrawText("%yw%j他のゲームデータが", 160, 85);
        DrawText(" %yw%jセーブされています。", 160, 105);
        DrawText("%yw%j空きページが足らないので", 160, 125);
        DrawText("%yw%jこのゲームデータを", 160, 145);
        DrawText(" %yw%jセーブできません。", 160, 165);
        break;
    }
    SetTextAlignLeft();
    SetTextSize(11);
    DrawTexturedSprite(&g_texButtonA, 278, 432);
    DrawText("%wwOK", 154, g_dwScreenHeight * 19 / 20 - 3);
#endif /* TGR_REGION == TGR_REGION_US */
    gDPPipeSync(gDisplayListPtr++);
    ApplyScreenFade();
    FlushDisplayList();

    if (g_aPlayerInputState[0].buttons & INPUT_A) {
        TransferFlags(&g_aPlayerInputState[0], INPUT_A);
        SetFadeTarget(0.0f, FADE_TIME);
    } else if (g_wScreenFrameCount++ == CONTROLLER_CHECK_FRAMES) {
        SetFadeTarget(0.0f, FADE_TIME);
    }

    if (IsFadedToBlack()) {
        g_bControllerCheckStarted = 0;
        ClearFramebuffersForWideMode();
        gControllerPakState = 0;
        g_wScreenFrameCount = 0;
        SetCurrentScreenCallback(ProcessMainMenu);
    }
}
