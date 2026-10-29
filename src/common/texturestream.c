#include "common/pause.h"
#include "common/render.h"
#include "common/system.h"
#include "common/track.h"
#include "common/trackselect.h"
#include "common/weather.h"

#if TGR_BOSS_RALLY
#include "common/instantreplay.h"
#include "win32/renderbackend.h"
#endif

#define TEXTURE_ROM_NONE ((u32) - 1)
#define CI4_PALETTE_BYTES (16 * sizeof(u16))
#define PALETTE_BYTES (256 * sizeof(u16))

void StreamAnimatedTrackTextures(void) {
    ModelTexture *texture;
    TextureAnimation *animation;
#if TGR_PLATFORM == TGR_PLATFORM_N64
    OSIoMesg *message;
#endif
    s32 snowTint, i, frame, frameCount;
    u32 phase, imageRom, paletteRom;

    snowTint = g_dwSnowMode != 0 && g_dwSelectedTrackId != TRACK_COASTLINE &&
               g_dwSelectedTrackId != TRACK_MIRROR_COASTLINE;
    for (i = 0; i < g_LoadedTrack.textureCount; i++) {
        texture = &g_LoadedTrack.textures[i];
        if (texture->image == NULL) {
            continue;
        }
        if (!MODEL_TEXTURE_ANIMATED(texture)) {
            if (g_dwSnowMode == (u32)g_dwStreamedSnowMode) {
                continue;
            }
            paletteRom = (texture->source.paletteRomIndex & 0xFFF) << 5;
        } else {
            animation = texture->source.animation;
            frameCount = animation->frameCount;
            /* The phase is taken from the time of the first frame onward, as if it were 0. */
            phase = (u32)g_dwElapsedTimeMs %
                    (animation->frames[frameCount - 1].time - animation->frames[0].time);
            if (frameCount == 2 && animation->frames[0].time == TEXTURE_ROM_NONE) {
#if TGR_BOSS_RALLY
                continue;
#else
                /* A texture of the snow, then the texture of the other weathers. */
                if ((u32)g_dwStreamedSnowMode == g_dwSnowMode) {
                    continue;
                }
                frame = g_dwSnowMode == 0;
                imageRom = frame ? TEXTURE_ROM_NONE : animation->frames[frame].imageRom;
                paletteRom = animation->frames[frame].paletteRom;
#endif /* TGR_BOSS_RALLY */
            } else {
                if (g_dwRacePauseState
#if TGR_BOSS_RALLY
                    || gInstantReplayActive == 2
#endif
                ) {
                    continue;
                }
                if (snowTint && MODEL_TEXTURE_FORMAT(texture) == TEXTURE_FORMAT_SNOW_STILL) {
                    frame = 0;
                } else {
                    for (frame = 1; frame < frameCount; frame++) {
                        if (phase < animation->frames[frame].time) {
                            break;
                        }
                    }
                    frame--;
                }
                imageRom = animation->frames[frame].imageRom;
                paletteRom = animation->frames[frame].paletteRom;
            }
            if (MODEL_TEXTURE_SIZE(texture) != 0 && imageRom != TEXTURE_ROM_NONE) {
#if TGR_BOSS_RALLY
                g_pfnAliasTextureEntrySurface((imageRom >> 16) & 0xffffu, imageRom & 0xffffu);
                texture = &g_LoadedTrack.textures[i];
#else
                message = AllocIoMessage();
                texture = &g_LoadedTrack.textures[i];
                osPiStartDma(message,
                             OS_MESG_PRI_NORMAL,
                             OS_READ,
                             imageRom + g_dwTrackTextureRomOffset,
                             texture->image,
                             MODEL_TEXTURE_SIZE(texture),
                             &g_DmaMessageQueue);
                texture = &g_LoadedTrack.textures[i];
#endif /* TGR_BOSS_RALLY */
            }
        }
        if (texture->palette != NULL && paletteRom != TEXTURE_ROM_NONE) {
#if TGR_BOSS_RALLY
            /* The transfer stub ignores the allocated message and transfer arguments. */
            AllocIoMessage();
            osContInit(NULL, NULL, NULL);
#else
            message = AllocIoMessage();
            texture = &g_LoadedTrack.textures[i];
            osPiStartDma(message,
                         OS_MESG_PRI_NORMAL,
                         OS_READ,
                         paletteRom + g_dwTrackTextureRomOffset,
                         texture->palette,
                         MODEL_TEXTURE_FORMAT(texture) == TEXTURE_FORMAT_CI4 ? CI4_PALETTE_BYTES :
                                                                               PALETTE_BYTES,
                         &g_DmaMessageQueue);
#endif /* TGR_BOSS_RALLY */
        }
    }
    g_dwStreamedSnowMode = g_dwSnowMode;
}
