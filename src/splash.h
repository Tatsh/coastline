#ifndef COASTLINE_SPLASH_H
#define COASTLINE_SPLASH_H

/**
 * @file
 * The splash screens and the controller check at start-up.
 */

#include <PR/gbi.h>
#include <PR/ultratypes.h>

#include "model.h"

/**
 * Whether the splash screens have loaded their models and sounds.
 *
 * @ghidraAddress USA: 0x80270810
 */
extern s32 g_bSplashStarted;

/** Whether the splash screens fade out. @ghidraAddress USA: 0x8027081c */
extern s32 g_bSplashFadingOut;

/** The lights of the logos. @ghidraAddress USA: 0x80270820 */
extern Ambient g_SplashAmbientLight;

/** @ghidraAddress USA: 0x80270828 */
extern Light g_SplashDirectionalLight;

/* The warnings of the controller check. */
#define CONTROLLER_WARNING_NO_CONTROLLER 0
#define CONTROLLER_WARNING_NO_PAK 1
#define CONTROLLER_WARNING_FOREIGN_PAK 2

/**
 * What the controller check warns of, as the CONTROLLER_WARNING_ values: no controller, no
 * Controller Pak, or a Controller Pak with the data of another game.
 *
 * @ghidraAddress USA: 0x80270840
 */
extern u8 g_bControllerWarningType;

/** Whether the controller check has loaded its pictures. @ghidraAddress USA: 0x80270844 */
extern u8 g_bControllerCheckStarted;

/** The logos, in the order they show. @ghidraAddress USA: 0x80315e80 */
extern Model *g_pSplashFirstLogo;

/** @ghidraAddress USA: 0x80315e84 */
extern Model *g_pSplashProducerLogo;

/** @ghidraAddress USA: 0x80315e88 */
extern Model *g_pSplashDeveloperLogo;

/** The parts of the title, in the order they show. @ghidraAddress USA: 0x80315e8c */
extern Model *g_pSplashTitleModelA;

/** @ghidraAddress USA: 0x80315e90 */
extern Model *g_pSplashTitleModelB;

/** @ghidraAddress USA: 0x80315e94 */
extern Model *g_pSplashTitleModelD;

/** @ghidraAddress USA: 0x80315e98 */
extern Model *g_pSplashTitleModelC;

/** The time since the splash screens started, in seconds. @ghidraAddress USA: 0x80315e9c */
extern f32 g_fSplashTime;

/**
 * The sound a logo makes as it flies in and leaves, and its length.
 *
 * @ghidraAddress USA: 0x80315ea0
 */
extern u32 g_dwSplashSoundALength;

/** @ghidraAddress USA: 0x80315ea4 */
extern s8 *g_pSplashSoundA;

/** The second sound a logo makes as it flies in, and its length. @ghidraAddress USA: 0x80315ea8 */
extern u32 g_dwSplashSoundBLength;

/** @ghidraAddress USA: 0x80315eac */
extern s8 *g_pSplashSoundB;

/**
 * For each logo: the channel of the sound it makes as it leaves, how far its first sound has gone
 * (1 once it played as the logo flew in, 2 once it plays as the logo leaves) and whether its
 * second sound played.
 *
 * @ghidraAddress USA: 0x80315eb0
 */
extern s32 g_dwSplashFirstLogoVoice;

/** @ghidraAddress USA: 0x80315eb4 */
extern s32 g_dwSplashFirstLogoSoundA;

/** @ghidraAddress USA: 0x80315eb8 */
extern s32 g_bSplashFirstLogoSoundB;

/** @ghidraAddress USA: 0x80315ebc */
extern s32 g_dwSplashProducerVoice;

/** @ghidraAddress USA: 0x80315ec0 */
extern s32 g_dwSplashProducerSoundA;

/** @ghidraAddress USA: 0x80315ec4 */
extern s32 g_bSplashProducerSoundB;

/** @ghidraAddress USA: 0x80315ec8 */
extern s32 g_dwSplashDeveloperVoice;

/** @ghidraAddress USA: 0x80315ecc */
extern s32 g_dwSplashDeveloperSoundA;

/** @ghidraAddress USA: 0x80315ed0 */
extern s32 g_bSplashDeveloperSoundB;

/**
 * Whether a player skipped the splash screens, which then lead to the main menu.
 *
 * @ghidraAddress USA: 0x80315ed4
 */
extern s32 g_bSplashSkipped;

/**
 * Animate the models of the splash screens when the frame before is drawn.
 *
 * @ghidraAddress USA: 0x80206830
 */
void AnimateSplashModels(void);

/**
 * Run one frame of the splash screens.
 *
 * @ghidraAddress USA: 0x8020686c
 */
void ProcessSplashScreens(void);

/**
 * Run one frame of the warning about the controller or the Controller Pak.
 *
 * @ghidraAddress USA: 0x80208570
 */
void ProcessControllerCheck(void);

#endif
