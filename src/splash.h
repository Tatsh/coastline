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
 * @ghidraAddress JPN: 0x802722b4
 * @ghidraAddress EUR: 0x802727f0
 */
extern s32 g_bSplashStarted;

/** Whether the splash screens fade out. @ghidraAddress USA: 0x8027081c */
extern s32 g_bSplashFadingOut;

/** The lights of the logos. @ghidraAddress USA: 0x80270820 */
extern Ambient g_SplashAmbientLight;

/**
 * @ghidraAddress USA: 0x80270828
 * @ghidraAddress JPN: 0x802722c8
 * @ghidraAddress EUR: 0x80272800
 */
extern Light g_SplashDirectionalLight;

/* The warnings of the controller check. */
#define CONTROLLER_WARNING_NO_CONTROLLER 0 /*!< No controller is plugged in. */
#define CONTROLLER_WARNING_NO_PAK 1        /*!< The controller has no Controller Pak. */
#define CONTROLLER_WARNING_FOREIGN_PAK 2   /*!< The Controller Pak has no notes of this game. */

/**
 * What the controller check warns of, as the CONTROLLER_WARNING_ values: no controller, no
 * Controller Pak, or a Controller Pak with the data of another game.
 *
 * @ghidraAddress USA: 0x80270840
 * @ghidraAddress JPN: 0x802722e0
 * @ghidraAddress EUR: 0x80272820
 */
extern u8 g_bControllerWarningType;

/**
 * Whether the controller check has loaded its pictures.
 *
 * @ghidraAddress USA: 0x80270844
 * @ghidraAddress JPN: 0x802722e4
 * @ghidraAddress EUR: 0x80272824
 */
extern u8 g_bControllerCheckStarted;

/**
 * The logos, in the order they show.
 *
 * @ghidraAddress USA: 0x80315e80
 * @ghidraAddress EUR: 0x80317b70
 */
extern Model *g_pSplashFirstLogo;

/**
 * @ghidraAddress USA: 0x80315e84
 * @ghidraAddress EUR: 0x80314fc4
 */
extern Model *g_pSplashProducerLogo;

/**
 * @ghidraAddress USA: 0x80315e88
 * @ghidraAddress EUR: 0x80317b74
 */
extern Model *g_pSplashDeveloperLogo;

/**
 * The parts of the title, in the order they show.
 *
 * @ghidraAddress USA: 0x80315e8c
 * @ghidraAddress EUR: 0x80317b78
 */
extern Model *g_pSplashTitleModelA;

/**
 * @ghidraAddress USA: 0x80315e90
 * @ghidraAddress JPN: 0x803165b4
 * @ghidraAddress EUR: 0x80317b7c
 */
extern Model *g_pSplashTitleModelB;

/**
 * @ghidraAddress USA: 0x80315e94
 * @ghidraAddress EUR: 0x80317b80
 */
extern Model *g_pSplashTitleModelD;

/** @ghidraAddress USA: 0x80315e98 */
extern Model *g_pSplashTitleModelC;

/**
 * The time since the splash screens started, in seconds.
 *
 * @ghidraAddress USA: 0x80315e9c
 * @ghidraAddress JPN: 0x803167b8
 * @ghidraAddress EUR: 0x80317b88
 */
extern f32 g_fSplashTime;

/**
 * The sound a logo makes as it flies in and leaves, and its length.
 *
 * @ghidraAddress USA: 0x80315ea0
 * @ghidraAddress JPN: 0x803167bc
 */
extern u32 g_dwSplashSoundALength;

/**
 * @ghidraAddress USA: 0x80315ea4
 * @ghidraAddress JPN: 0x803167c0
 */
extern s8 *g_pSplashSoundA;

/**
 * The second sound a logo makes as it flies in, and its length.
 *
 * @ghidraAddress USA: 0x80315ea8
 * @ghidraAddress JPN: 0x803167c4
 */
extern u32 g_dwSplashSoundBLength;

/** @ghidraAddress USA: 0x80315eac */
extern s8 *g_pSplashSoundB;

/**
 * For each logo: the channel of the sound it makes as it leaves, how far its first sound has gone
 * (1 once it played as the logo flew in, 2 once it plays as the logo leaves) and whether its
 * second sound played.
 *
 * @ghidraAddress USA: 0x80315eb0
 * @ghidraAddress JPN: 0x803167cc
 * @ghidraAddress EUR: 0x80317ba8
 */
extern s32 g_dwSplashFirstLogoVoice;

/**
 * @ghidraAddress USA: 0x80315eb4
 * @ghidraAddress JPN: 0x803167d0
 * @ghidraAddress EUR: 0x80317ba0
 */
extern s32 g_dwSplashFirstLogoSoundA;

/**
 * @ghidraAddress USA: 0x80315eb8
 * @ghidraAddress JPN: 0x803167d4
 * @ghidraAddress EUR: 0x80317ba4
 */
extern s32 g_bSplashFirstLogoSoundB;

/**
 * @ghidraAddress USA: 0x80315ebc
 * @ghidraAddress JPN: 0x803167d8
 */
extern s32 g_dwSplashProducerVoice;

/**
 * @ghidraAddress USA: 0x80315ec0
 * @ghidraAddress JPN: 0x803167dc
 */
extern s32 g_dwSplashProducerSoundA;

/**
 * @ghidraAddress USA: 0x80315ec4
 * @ghidraAddress JPN: 0x803167e0
 */
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
 * @ghidraAddress JPN: 0x80206a90 (stub)
 * @ghidraAddress EUR: 0x802069f0
 */
void AnimateSplashModels(void);

/**
 * Run one frame of the splash screens.
 *
 * @ghidraAddress USA: 0x8020686c
 * @ghidraAddress JPN: 0x80206ab4 (stub)
 * @ghidraAddress EUR: 0x80206a2c (stub)
 */
void ProcessSplashScreens(void);

/**
 * Run one frame of the warning about the controller or the Controller Pak.
 *
 * @ghidraAddress USA: 0x80208570
 * @ghidraAddress JPN: 0x80207f30 (stub)
 * @ghidraAddress EUR: 0x80208310 (stub)
 */
void ProcessControllerCheck(void);

#endif
