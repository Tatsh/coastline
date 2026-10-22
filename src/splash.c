#include "splash.h"

#include <PR/gu.h>
#include <PR/os.h>

#include "audio.h"
#include "audiomix.h"
#include "fade.h"
#include "game.h"
#include "heap.h"
#include "input.h"
#include "mainmenu.h"
#include "pakmenu.h"
#include "render.h"
#include "sprite.h"
#include "system.h"
#include "text.h"
#include "timing.h"
#include "util.h"

/* Assets at fixed cartridge offsets. */
#define ROM_SPLASH_SOUND_A 0x1B5440
#define ROM_SPLASH_SOUND_B 0x1B93C0
#define ROM_SPLASH_FIRST_LOGO 0x1DA030
#define ROM_SPLASH_PRODUCER_LOGO 0x1DBA40
#define ROM_SPLASH_DEVELOPER_LOGO 0x1DCD70
#define ROM_SPLASH_TITLE_D 0x1E1CA0
#define ROM_SPLASH_TITLE_C 0x1E2B40
#define ROM_SPLASH_TITLE_A 0x1E09F0
#define ROM_SPLASH_TITLE_B 0x1E3DB0

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

/* When each logo starts, in seconds. */
#define FIRST_LOGO_START 0.0f
#define PRODUCER_LOGO_START 4.0f
#define DEVELOPER_LOGO_START 8.0f

/* The times of a logo after its start: flying in, holding, leaving and gone. */
#define LOGO_SOUND_A_TIME 0.35f
#define LOGO_SOUND_B_TIME 0.5f
#define LOGO_LANDED_TIME 2.0f
#define LOGO_LEAVE_TIME 3.5f
#define LOGO_END_TIME 6.0f

/* The developer logo is gone a second sooner than the other two. */
#define DEVELOPER_LOGO_END_TIME 5.0f

/* The depth of a logo at rest, and its scale. */
#define LOGO_DEPTH (-50.0f)
#define LOGO_SCALE 0.03125f

/* The brightest a caption or the copyright gets, and how fast they fade. */
#define TEXT_ALPHA 200
#define TEXT_FADE_RATE 4.0

/* When the parts of the title show, and when the copyright does. */
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

/* The buttons that skip the splash screens. */
#define SKIP_BUTTONS (INPUT_Z | INPUT_START | INPUT_A)

/* The buttons held together that show the date of the build. */
#define BUILD_DATE_BUTTONS (INPUT_C_UP | INPUT_C_RIGHT | INPUT_C_DOWN | INPUT_C_LEFT)

/* The frames before the controller check proceeds by itself. */
#define CONTROLLER_CHECK_FRAMES 910

void AnimateSplashModels(void) {
#if TGR_REGION != TGR_REGION_JP
    AdvanceModelAnimations(g_pSplashDeveloperLogo);
    AdvanceModelAnimations(g_pSplashTitleModelA);
#endif
    AdvanceModelAnimations(g_pSplashTitleModelB);
}

/* The Japanese and European bodies still come from the assembly. */
#if TGR_REGION == TGR_REGION_US
/* Allocate a model on the heap and decompress it there. */
static inline Model *LoadSplashModel(u32 romOffset) {
    return DecompressModel(AllocHeapAligned(DmaReadAssetSize(romOffset)), romOffset);
}

/* Play a sound on a free sound effect channel at a volume, and return the channel or -1. */
static s16 PlaySplashSound(s8 *sample, u32 length, s32 volume) {
    s16 channel = FindFreeSoundChannel();

    if (channel != -1) {
        InitSoundChannel(channel, sample, length, 0);
        g_aSoundEffectChannels[channel].volume = volume;
        g_aSoundEffectChannels[channel].step = SFX_STEP_ONE;
    }
    return channel;
}

/*
 * Fly a logo in with a spin from the distance and two sounds, hold it, then slide it off to the
 * right with a sound that pans after it.
 */
static void
UpdateSplashLogo(Model *logo, f32 start, f32 end, s32 *voice, s32 *soundA, s32 *soundB) {
    f32 t = g_fSplashTime;
    f32 left;
    f32 since;
    f32 x;
    s32 pan;

    if (t < start + LOGO_LANDED_TIME) {
        if (t > start + LOGO_SOUND_A_TIME && !*soundA) {
            PlaySplashSound(g_pSplashSoundA, g_dwSplashSoundALength, SFX_VOLUME(32, 32));
            *soundA = 1;
        }
        if (g_fSplashTime > start + LOGO_SOUND_B_TIME && !*soundB) {
            PlaySplashSound(g_pSplashSoundB, g_dwSplashSoundBLength, SFX_VOLUME(32, 32));
            *soundB = 1;
        }
        t = g_fSplashTime;
        left = start + LOGO_LANDED_TIME - t;
        since = t - start;
        DrawModelAt(logo,
                    -left * 32.0f,
                    0.0f,
                    LOGO_DEPTH - left * 750.0f,
                    since * HALF_PI * since * since * since * 0.0625f + -PI,
                    LOGO_SCALE);
        return;
    }
    if (t >= start + end) {
        return;
    }

    if (t > start + LOGO_LEAVE_TIME) {
        since = t - (start + LOGO_LEAVE_TIME);
        x = since * since * 256.0f;
    } else {
        x = 0.0f;
    }
    DrawModelAt(logo, x, 0.0f, LOGO_DEPTH, -HALF_PI, LOGO_SCALE);

    t = g_fSplashTime;
    if (t > start + LOGO_LEAVE_TIME) {
        if (*soundA < 2) {
            /* The channel is stored first and read back from the voice at each use. */
            *voice = FindFreeSoundChannel();
            if (*voice != -1) {
                InitSoundChannel(*voice, g_pSplashSoundA, g_dwSplashSoundALength, 0);
                g_aSoundEffectChannels[*voice].volume = SFX_VOLUME(20, 20);
                g_aSoundEffectChannels[*voice].step = SFX_STEP_ONE;
            }
            *soundA = 2;
        } else {
            pan = (s32)((t - (start + LOGO_LEAVE_TIME)) * 32.0f);
            if (pan > 20) {
                pan = 20;
            }
            /* With no free channel the original writes this just before the channels. */
            g_aSoundEffectChannels[*voice].volume = SFX_VOLUME(20 - pan, 20 + pan);
        }
    }
}

/* Draw the caption over a logo, fading in as the logo lands and out as it leaves. */
static void DrawSplashCaption(const char *text, f32 start, s32 y) {
    f32 t = g_fSplashTime;
    s32 alpha;

    if (t < start + LOGO_LANDED_TIME) {
        alpha = TEXT_ALPHA - (s32)((f64)(start + LOGO_LANDED_TIME - t) * 255.0 * TEXT_FADE_RATE);
    } else if (t > start + LOGO_LEAVE_TIME) {
        alpha = TEXT_ALPHA - (s32)((f64)(t - (start + LOGO_LEAVE_TIME)) * 255.0 * TEXT_FADE_RATE);
    } else {
        alpha = TEXT_ALPHA;
    }
    if (alpha > 0) {
        SetTextCustomColors(alpha, alpha, alpha, alpha, alpha, alpha);
        SetTextStyleNormal();
        SetTextSize(10);
        SetTextAlignCenter();
        DrawText(text, 160, y);
    }
}

/* The depth of a part of the title, which slows as it comes forward and rests at 0 by its end. */
static inline f32 TitleDepth(f32 start, f32 end, f32 speed) {
    if (g_fSplashTime < end) {
        return (sqrtf(end - start) - sqrtf(g_fSplashTime - start)) * speed;
    }
    return 0.0f;
}

/* Draw the title, tilted, its parts coming forward one after another. */
static void DrawSplashTitle(void) {
    Mtx *mtx;
    f32 z;

    if (g_fSplashTime > TITLE_START) {
        gSPNumLights(gDisplayListPtr++, NUMLIGHTS_1);
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
        if (g_fSplashTime > TITLE_A_END) {
            SetScissorRect(40, 100, 250, 50);
        }
        /* The original rests this part at negative zero. */
        z = g_fSplashTime < TITLE_A_END ?
                (sqrtf(TITLE_A_END - TITLE_START) - sqrtf(g_fSplashTime - TITLE_START)) * 300.0f :
                -0.0f;
        DrawModelAt(g_pSplashTitleModelA,
                    0.0f,
                    0.0f,
                    z,
                    sqrtf(g_fSplashTime - TITLE_START) * 0.01f + -HALF_PI,
                    0.05f);
        gSPPopMatrix(gDisplayListPtr++, G_MTX_MODELVIEW);
        SetScissorRect(0, 0, 320, 240);
    }
    if (g_fSplashTime > TITLE_B_START) {
        g_dwDrawModelListNoZBuffer = 1;
        DrawModelAt(g_pSplashTitleModelB,
                    0.0f,
                    0.0f,
                    TitleDepth(TITLE_B_START, TITLE_B_END, 500.0f),
                    -HALF_PI,
                    1.0f / 12.0f);
    }
    if (g_fSplashTime > TITLE_C_START) {
        DrawModelAt(g_pSplashTitleModelC,
                    0.0f,
                    0.0f,
                    TitleDepth(TITLE_C_START, TITLE_C_END, 500.0f),
                    -HALF_PI,
                    1.0f / 12.0f);
    }
    if (g_fSplashTime > TITLE_D_START) {
        DrawModelAt(g_pSplashTitleModelD,
                    0.0f,
                    0.0f,
                    TitleDepth(TITLE_D_START, TITLE_D_END, 500.0f),
                    -HALF_PI,
                    1.0f / 12.0f);
    }
}

/* Fade in the copyright, and the date of the build while every C button is held. */
static void DrawSplashCopyright(void) {
    s32 alpha;

    if (!(g_fSplashTime > COPYRIGHT_START)) {
        return;
    }
    alpha = (s32)((f64)(g_fSplashTime - COPYRIGHT_START) * 255.0 * TEXT_FADE_RATE);
    if (alpha > TEXT_ALPHA) {
        alpha = TEXT_ALPHA;
    }
    SetTextCustomColors(alpha, alpha, alpha, alpha, alpha, alpha);
    SetTextStyleNormal();
    SetTextSize(10);
    SetTextAlignCenter();
    if ((g_aPlayerInputState[0].buttons & BUILD_DATE_BUTTONS) == BUILD_DATE_BUTTONS) {
        DrawText("25Aug97 11:46:52", 160, 40);
    }
    SetTextSize(7);
    DrawText("O 1997 Boss Game Studios, Inc.  All Rights Reserved.", 160, 190);
    DrawText("Top Gear Rally is a trademark of Kemco.", 160, 197);
    DrawText("Distributed by Midway Home Entertainment under license.", 160, 204);
    DrawText("Midway is a trademark of Midway Games Inc.  Used by permission.", 160, 211);
    DrawText("LICENSED BY NINTENDO", 160, 224);
    /* A small c inside the O of the first line makes the copyright sign. */
    SetTextSize(3);
    DrawText("c", 78, 190);
}

/* Fade out on a skip or at the end, and leave for the main menu or the demo once faded. */
static void UpdateSplashExit(void) {
    PlayerInputState *input;
    s32 i;

    if (IsFadingOut()) {
        if (IsFadeComplete()) {
            g_bSplashStarted = 0;
            for (i = 0; i < SPLASH_SFX_CHANNELS; i++) {
                g_aSoundEffectChannels[i].step = 0;
                g_aSoundEffectChannels[i].volume = 0;
            }
            ClearFramebuffersForWideMode();
            if (g_bSplashSkipped) {
                SetCurrentScreenCallback(ProcessMainMenu);
            } else {
                StartAttractDemo();
            }
        }
        return;
    }

    if (!g_bSplashSkipped || !(g_fSplashTime > SKIP_END_TIME)) {
        for (input = g_aPlayerInputState; input != g_aPlayerInputState + MAX_PLAYERS; input++) {
            if (input->buttons & SKIP_BUTTONS) {
                TransferFlags(input, SKIP_BUTTONS);
                g_bSplashSkipped = 1;
                if (g_bSplashFadingOut) {
                    break;
                }
                if (g_fSplashTime < SKIP_TIME) {
                    g_fSplashTime = SKIP_TIME;
                } else if (g_fSplashTime > SKIP_END_TIME) {
                    break;
                }
            }
        }
        if (input == g_aPlayerInputState + MAX_PLAYERS && !(g_fSplashTime >= SPLASH_END_TIME)) {
            return;
        }
    }
    g_bSplashFadingOut = 1;
    SetFadeTarget(0.0f, FADE_TIME);
    FadeSfxTo(0.0f, FADE_TIME);
}

void ProcessSplashScreens(void) {
    Mtx *mtx;
    s32 i;

    if (!g_bSplashStarted) {
        SetFadeTarget(1.0f, FADE_TIME);
        FadeMusicTo(1.0f, FADE_TIME);
        FadeSfxTo(1.0f, FADE_TIME);
        ResetHeap();
        g_pSplashDeveloperLogo = LoadSplashModel(ROM_SPLASH_DEVELOPER_LOGO);
        g_pSplashProducerLogo = LoadSplashModel(ROM_SPLASH_PRODUCER_LOGO);
        g_pSplashFirstLogo = LoadSplashModel(ROM_SPLASH_FIRST_LOGO);
        g_pSplashTitleModelA = LoadSplashModel(ROM_SPLASH_TITLE_A);
        LoopModelAnimations(g_pSplashTitleModelA);
        g_pSplashTitleModelB = LoadSplashModel(ROM_SPLASH_TITLE_B);
        LoopModelAnimations(g_pSplashTitleModelB);
        g_pSplashTitleModelD = LoadSplashModel(ROM_SPLASH_TITLE_D);
        LoopModelAnimations(g_pSplashTitleModelD);
        g_pSplashTitleModelC = LoadSplashModel(ROM_SPLASH_TITLE_C);
        LoopModelAnimations(g_pSplashTitleModelC);

        g_dwSplashSoundALength = DmaReadAssetSize(ROM_SPLASH_SOUND_A);
        g_pSplashSoundA = AllocHeapAligned(g_dwSplashSoundALength + SOUND_PADDING);
        DecompressFromRom(g_pSplashSoundA, ROM_SPLASH_SOUND_A, NULL);
        g_dwSplashSoundBLength = DmaReadAssetSize(ROM_SPLASH_SOUND_B);
        g_pSplashSoundB = AllocHeapAligned(g_dwSplashSoundBLength + SOUND_PADDING);
        DecompressFromRom(g_pSplashSoundB, ROM_SPLASH_SOUND_B, NULL);
        for (i = 0; i < SOUND_PADDING; i++) {
            g_pSplashSoundA[g_dwSplashSoundALength + i] = 0;
            g_pSplashSoundB[g_dwSplashSoundBLength + i] = 0;
        }

        /* g_bSplashFadingOut keeps what an earlier showing left in it. */
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
    AddTimingMarker(0, 0, 0, 200, 255);
    if (!IsFadingIn() || IsFadeComplete()) {
        g_fSplashTime += g_fFrameDeltaTime;
    }

    g_dwKeepCustomClipPlanes = 1;
    g_fCameraNearScale = 10.0f;
    g_fCameraFarDistance = 5000.0f;
    SetupMenuProjection((f32)g_dwScreenWidth, (f32)g_dwScreenHeight);
    SetViewport(0, 0, g_dwScreenWidth, g_dwScreenHeight, 1);
    gSPNumLights(gDisplayListPtr++, NUMLIGHTS_1);
    gSPLight(gDisplayListPtr++, &g_SplashDirectionalLight, 1);
    gSPLight(gDisplayListPtr++, &g_SplashAmbientLight, 2);
    gSPClipRatio(gDisplayListPtr++, FRUSTRATIO_6);
    gSPClipRatio(gDisplayListPtr++, FRUSTRATIO_1);
    mtx = (Mtx *)AllocDisplayListBlock();
    guTranslateF(g_afMvpMatrix, (f32)(g_dwScreenWidth / 2), (f32)(g_dwScreenHeight / 2), 100.0f);
    guMtxF2L(g_afMvpMatrix, mtx);
    gSPMatrix(gDisplayListPtr++, mtx, G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_PUSH);

    if (g_fSplashTime >= FIRST_LOGO_START) {
        UpdateSplashLogo(g_pSplashFirstLogo,
                         FIRST_LOGO_START,
                         LOGO_END_TIME,
                         &g_dwSplashFirstLogoVoice,
                         &g_dwSplashFirstLogoSoundA,
                         &g_bSplashFirstLogoSoundB);
    }
    if (g_fSplashTime >= PRODUCER_LOGO_START) {
        DrawSplashCaption("Produced by", PRODUCER_LOGO_START, 70);
        UpdateSplashLogo(g_pSplashProducerLogo,
                         PRODUCER_LOGO_START,
                         LOGO_END_TIME,
                         &g_dwSplashProducerVoice,
                         &g_dwSplashProducerSoundA,
                         &g_bSplashProducerSoundB);
    }
    if (g_fSplashTime >= DEVELOPER_LOGO_START) {
        DrawSplashCaption("Developed by", DEVELOPER_LOGO_START, 60);
        UpdateSplashLogo(g_pSplashDeveloperLogo,
                         DEVELOPER_LOGO_START,
                         DEVELOPER_LOGO_END_TIME,
                         &g_dwSplashDeveloperVoice,
                         &g_dwSplashDeveloperSoundA,
                         &g_bSplashDeveloperSoundB);
    }
    DrawSplashTitle();
    DrawSplashCopyright();

    gSPPopMatrix(gDisplayListPtr++, G_MTX_MODELVIEW);
    ApplyScreenFade();
    if (g_dwHudDebugLevel >= 2) {
        SetViewport(0, 0, g_dwScreenWidth, g_dwScreenHeight, 1);
        DrawTimingBars();
    }
    SetNextScreenCallback(AnimateSplashModels);
    FlushDisplayList();
    UpdateSplashExit();
}
#endif

void ProcessControllerCheck(void) {
#if TGR_REGION != TGR_REGION_US
    /* The value of the flag comes from a variable, loaded in a register of its own. */
    u32 blanked = TRUE;

#endif
    if (!g_bControllerCheckStarted) {
        g_bControllerCheckStarted = 1;
#if TGR_REGION == TGR_REGION_US
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
#endif
        LoadTextureFromRom(&g_texBackground);
        LoadTextureFromRom(&g_texLogo);
        LoadTextureFromRom(&g_texButtonA);
        SetFadeTarget(1.0f, FADE_TIME);
        g_wScreenFrameCount = 0;
#if TGR_REGION != TGR_REGION_US
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
#if TGR_REGION == TGR_REGION_US
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
#endif
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
