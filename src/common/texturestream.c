#include "common/pause.h"
#include "common/render.h"
#include "common/system.h"
#include "common/track.h"
#include "common/trackselect.h"
#include "common/weather.h"

#if TGR_BOSS_RALLY
#include "common/instantreplay.h"
#include "common/renderbackend.h"
#endif

#define TEXTURE_ROM_NONE (-1)
#define CI4_PALETTE_BYTES (16 * sizeof(u16))
#define PALETTE_BYTES (256 * sizeof(u16))

void StreamAnimatedTrackTextures(void) {
    s32 i;
    TextureAnimation *animation;
    s32 imageRom, paletteRom;
    u32 period;
    s32 frame, snowTint;
    u32 phase;

    snowTint = g_dwSnowMode != 0 && g_dwSelectedTrackId != TRACK_COASTLINE &&
               g_dwSelectedTrackId != TRACK_MIRROR_COASTLINE;
    for (i = 0; i < g_LoadedTrack.textureCount; i++) {
        if (g_LoadedTrack.textures[i].image == NULL) {
            continue;
        }
        if (MODEL_TEXTURE_ANIMATED(&g_LoadedTrack.textures[i])) {
            animation = g_LoadedTrack.textures[i].source.animation;
            /* The phase is taken from the time of the first frame onward, as if it were 0. */
            period = animation->frames[animation->frameCount - 2].time - animation->startTime;
            phase = (u32)g_dwElapsedTimeMs - (u32)g_dwElapsedTimeMs / period * period;
            if (animation->frameCount == 2 && animation->startTime == (u32)TEXTURE_ROM_NONE) {
#if TGR_BOSS_RALLY
                continue;
#else
                /* A texture of the snow, then the texture of the other weathers. */
                if (g_dwStreamedSnowMode != (s32)g_dwSnowMode) {
                    frame = g_dwSnowMode == 0;
                    if (g_dwSnowMode == 0) {
                        imageRom = TEXTURE_ROM_NONE;
                        goto palette;
                    }
                    goto frame;
                }
                continue;
#endif /* TGR_BOSS_RALLY */
            }
            if (g_dwRacePauseState
#if TGR_BOSS_RALLY
                || gInstantReplayActive == 2
#endif
            ) {
                continue;
            }
            if (snowTint &&
                MODEL_TEXTURE_FORMAT(&g_LoadedTrack.textures[i]) == TEXTURE_FORMAT_SNOW_STILL) {
                frame = 1;
            } else {
                for (frame = 1; frame < animation->frameCount; frame++) {
                    if (phase < animation->frames[frame - 1].time) {
                        break;
                    }
                }
            }
            frame--;
#if !TGR_BOSS_RALLY
        frame:
#endif
            imageRom = animation->frames[frame].imageRom;
#if !TGR_BOSS_RALLY
        palette:
#endif
            paletteRom = animation->frames[frame].paletteRom;
            if (MODEL_TEXTURE_SIZE(&g_LoadedTrack.textures[i]) != 0 &&
                imageRom != TEXTURE_ROM_NONE) {
#if TGR_BOSS_RALLY
                g_pfnAliasTextureEntrySurface((imageRom >> 16) & 0xffffu, imageRom & 0xffffu);
#else
                osPiStartDma(AllocIoMessage(),
                             OS_MESG_PRI_NORMAL,
                             OS_READ,
                             imageRom + g_dwTrackTextureRomOffset,
                             g_LoadedTrack.textures[i].image,
                             MODEL_TEXTURE_SIZE(&g_LoadedTrack.textures[i]),
                             &g_DmaMessageQueue);
#endif /* TGR_BOSS_RALLY */
            }
        load:
            if (g_LoadedTrack.textures[i].palette != NULL && paletteRom != TEXTURE_ROM_NONE) {
#if TGR_BOSS_RALLY
                /* The transfer stub ignores the allocated message and transfer arguments. */
                AllocIoMessage();
                osContInit(NULL, NULL, NULL);
#else
                osPiStartDma(AllocIoMessage(),
                             OS_MESG_PRI_NORMAL,
                             OS_READ,
                             paletteRom + g_dwTrackTextureRomOffset,
                             g_LoadedTrack.textures[i].palette,
                             MODEL_TEXTURE_FORMAT(&g_LoadedTrack.textures[i]) ==
                                     TEXTURE_FORMAT_CI4 ?
                                 CI4_PALETTE_BYTES :
                                 PALETTE_BYTES,
                             &g_DmaMessageQueue);
#endif /* TGR_BOSS_RALLY */
            }
        } else if (g_dwSnowMode != (u32)g_dwStreamedSnowMode) {
            paletteRom = (g_LoadedTrack.textures[i].source.paletteRomIndex & 0xFFF) << 5;
            goto load;
        }
    }
    g_dwStreamedSnowMode = g_dwSnowMode;
}
