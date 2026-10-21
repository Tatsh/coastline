#include "weather.h"

#include <PR/gbi.h>
#include <PR/gu.h>

#include "audiomix.h"
#include "camera.h"
#include "render.h"
#include "system.h"
#include "timing.h"
#include "track.h"
#include "vehicle.h"

/* Turns 16 random bits into a number from 0 to 2, which less 1 is from -1 to 1. */
#define RANDOM_TO_TWO (2.0f / 65535.0f)

/* A full turn of the wind. */
#define WIND_TURN 6.2831855f

/* The limits of the strength of the wind. */
#define WIND_STRENGTH_MAX 1.0f
#define WIND_STRENGTH_MIN 0.5f

/* The time between strikes runs at this rate, and a strike may come once it passes the limit. */
#define LIGHTNING_TIMER_RATE 343.0f
#define LIGHTNING_TIMER_LIMIT 2048.0

/* A strike comes in a frame when 16 random bits fall below this. */
#define LIGHTNING_CHANCE 0x80

/* The frames a strike flashes for, and the extent of the area it strikes in. */
#define LIGHTNING_FLASH_FRAMES 3
#define LIGHTNING_AREA_MASK 0x7FF

/* The rain is kept around a point this far ahead of the camera. */
#define EYE_AHEAD 3.0f

/* Above this speed the rain follows the camera by less than the camera moves. */
#define RAIN_MIN_CAMERA_SPEED 0.2777777612f
#define RAIN_CAMERA_SPEED_MUL 3.600000143f

/* How fast snow falls, and how far it drifts at most. */
#define SNOW_FALL_RATE 0.5f
#define SNOW_DRIFT 0.25f

/* The scale from the movement of the camera to the units of a particle position. */
#define PARTICLE_UNITS 16383.5

/* The most particles a step of drift in x or y can leave unmoved. */
#define DRIFT_HOLD_MASK 0xF

/* The scale from a particle position back to the units of the world. */
#define PARTICLE_SCALE 6.10370189e-05f

/* The streak a view's rain draws: where it starts, the steps it takes and how far each moves. */
#define STREAK_CENTER 32.0f
#define STREAK_START 28.0f
#define STREAK_STEP 3.5f
#define STREAK_STEPS 16
#define STREAK_MARGIN 2

/* The size of a particle a unit away, for snow and rain. */
#define SNOW_PARTICLE_WIDTH 24.0f
#define SNOW_PARTICLE_HEIGHT 36.0f
#define RAIN_PARTICLE_SIZE 128.0f

/* The closest a particle is drawn. */
#define PARTICLE_NEAR 0.1f

/* The center of the screen in 10.2 fixed point, and the span from it to an edge in x and y. */
#define SCREEN_CENTER_X 640
#define SCREEN_CENTER_Y 480
#define SCREEN_HALF_WIDTH 640.0f
#define SCREEN_HALF_HEIGHT 480.0f

void InitRandomRainParticles(void) {
    s32 view, i;

    g_dwRainParticleCount = RAIN_PARTICLES;
    for (view = 0; view < WEATHER_VIEWS; view++) {
        for (i = 0; i < RAIN_PARTICLES; i++) {
            g_awRainParticlePos[view][i][0] = AdvanceRng();
            g_awRainParticlePos[view][i][1] = AdvanceRng();
            g_awRainParticlePos[view][i][2] = AdvanceRng();
        }
    }
}

void UpdateWindDirection(void) {
    g_fWindAngle += ((f32)(s32)(AdvanceRng() & 0xFFFF) * RANDOM_TO_TWO - 1.0f) * g_fFrameDeltaTime;
    if (g_fWindAngle >= WIND_TURN) {
        g_fWindAngle -= WIND_TURN;
    } else if (g_fWindAngle < 0.0f) {
        g_fWindAngle += WIND_TURN;
    }
    g_fWindStrength +=
        ((f32)(s32)(AdvanceRng() & 0xFFFF) * RANDOM_TO_TWO - 1.0f) * g_fFrameDeltaTime;
    if (g_fWindStrength > WIND_STRENGTH_MAX) {
        g_fWindStrength = WIND_STRENGTH_MAX;
    } else if (g_fWindStrength < WIND_STRENGTH_MIN) {
        g_fWindStrength = WIND_STRENGTH_MIN;
    }
    g_vWindVelocity.x = cosf(g_fWindAngle) * (g_fFrameDeltaTime * g_fWindStrength);
    g_vWindVelocity.y = sinf(g_fWindAngle) * (g_fFrameDeltaTime * g_fWindStrength);
    g_vWindVelocity.z = 0.0f;
}

void SubdivideLightningBolt(s32 index, s32 span) {
    s32 mask = span * 8 - 1;
    f32 half = (f32)mask * 0.5f;
    Vec3f *point = &g_avLightningBolt[index];
    Vec3f *before = &g_avLightningBolt[index - span];
    Vec3f *after = &g_avLightningBolt[index + span];

    point->x = (f32)(s32)(AdvanceRng() & mask) + (before->x + after->x) * 0.5f - half;
    point->y = (f32)(s32)(AdvanceRng() & mask) + (before->y + after->y) * 0.5f - half;
    point->z = (before->z + after->z) * 0.5f;
    span >>= 1;
    if (span != 0) {
        g_nLightningBoltDepth += 4;
        SubdivideLightningBolt(index - span, span);
        SubdivideLightningBolt(index + span, span);
        g_nLightningBoltDepth -= 4;
    }
}

void UpdateLightningFlash(void) {
    if (g_dwLightningFlashCounter >= 0) {
        g_fLightningFlashTimer += LIGHTNING_TIMER_RATE * g_fFrameDeltaTime;
        if (g_dwLightningFlashCounter > 0) {
            g_dwLightningFlashCounter--;
        } else if (g_fLightningFlashTimer > LIGHTNING_TIMER_LIMIT) {
            g_dwLightningFlashCounter = -1;
        }
    } else if ((AdvanceRng() & 0xFFFF) < LIGHTNING_CHANCE) {
        g_dwLightningFlashCounter = LIGHTNING_FLASH_FRAMES;
        g_fLightningFlashTimer = 0.0f;
        g_avLightningBolt[0].x = (f32)(s32)(AdvanceRng() & LIGHTNING_AREA_MASK);
        g_avLightningBolt[0].y = (f32)(s32)(AdvanceRng() & LIGHTNING_AREA_MASK);
        g_avLightningBolt[0].z = g_LoadedTrack.minZ;
    }
}

void StubWeatherUpdate(void) {
}

void UpdatePrecipitationParticles(void) {
    CameraFrame *camera;
    Vec3f eye, move;
    Vec3f *previous, *velocity;
    s16 *particle;
    s16 driftX, driftY, moveX, moveY, moveZ;
    s32 view, i, holdX, holdY;
    f32 dx, dy, dz, speed, scale, sine, driftStepX, driftStepY;

    UpdateWindDirection();
    if (g_dwRainMode) {
        UpdateLightningFlash();
    }
    g_dwRainParticleCount = RAIN_PARTICLES / g_dwNumViewports;
    for (view = 0; view < g_dwNumViewports; view++) {
        camera = g_aVehicles[view].activeCamera;
        Vec3fMulAdd(&eye, &camera->position, &camera->forward, EYE_AHEAD);
        if (!g_bRainParticlesInitialized) {
            InitRandomRainParticles();
            g_avRainPrevCameraPos[0] = eye;
            g_avRainPrevCameraPos[1] = eye;
            g_bRainParticlesInitialized = TRUE;
        }
        if (!g_dwSnowMode && !g_dwRainMode) {
            return;
        }

        /* The rain moves with the camera, by less than the camera when it is fast. */
        previous = &g_avRainPrevCameraPos[view];
        dx = eye.x - previous->x;
        dy = eye.y - previous->y;
        dz = eye.z - previous->z;
        g_fRainCameraSpeed = sqrtf(dx * dx + dy * dy + dz * dz) / g_fFrameDeltaTime;
        speed = g_fRainCameraSpeed;
        if (speed > RAIN_MIN_CAMERA_SPEED) {
            scale = sqrtf(speed * RAIN_CAMERA_SPEED_MUL) * RAIN_MIN_CAMERA_SPEED / speed;
            g_fRainCameraSpeedScale = scale;
            dx *= scale;
            dy *= scale;
            dz *= scale;
            g_fRainCameraSpeed = speed * scale;
        } else {
            g_fRainCameraSpeedScale = 1.0f;
        }

        /* Snow falls slowly. Rain falls faster and blows with the wind. */
        if (g_dwSnowMode) {
            dz += g_fFrameDeltaTime * SNOW_FALL_RATE;
        } else {
            velocity = &g_avRainVelocity[view];
            velocity->x = cosf(g_fWindAngle) * (g_fFrameDeltaTime * g_fWindStrength);
            sine = sinf(g_fWindAngle);
            velocity->z = g_fFrameDeltaTime + g_fFrameDeltaTime;
            move.x = dx;
            move.y = dy;
            move.z = dz;
            dz += velocity->z;
            velocity->y = sine * (g_fFrameDeltaTime * g_fWindStrength);
            dx += velocity->x;
            dy += velocity->y;
            Vec3fScaleInPlace(&move, g_fRainCameraSpeedScale * 0.5f);
            Vec3fAddInPlace(velocity, &move);
        }

        /* Snow drifts from side to side. */
        if (g_dwSnowMode) {
            driftStepX = ((f32)(s32)(AdvanceRng() & 0xFFFF) * RANDOM_TO_TWO - 1.0f) *
                         g_fFrameDeltaTime * SNOW_DRIFT;
            driftStepY = ((f32)(s32)(AdvanceRng() & 0xFFFF) * RANDOM_TO_TWO - 1.0f) *
                         g_fFrameDeltaTime * SNOW_DRIFT;
        } else {
            driftStepX = 0.0f;
            driftStepY = 0.0f;
        }

        /*
         * Every particle moves the same, and a drift goes to a run of a random length of
         * particles at a time, its direction alternating.
         */
        *previous = eye;
        holdX = AdvanceRng() & DRIFT_HOLD_MASK;
        holdY = AdvanceRng() & DRIFT_HOLD_MASK;
        driftX = (s32)((f64)driftStepX * PARTICLE_UNITS);
        driftY = (s32)((f64)driftStepY * PARTICLE_UNITS);
        moveX = (s32)((f64)dx * PARTICLE_UNITS);
        moveY = (s32)((f64)dy * PARTICLE_UNITS);
        moveZ = (s32)((f64)dz * PARTICLE_UNITS);
        particle = g_awRainParticlePos[view][0];
        for (i = 0; i < g_dwRainParticleCount; i++, particle += 3) {
            if (holdX != 0) {
                holdX--;
                particle[0] += moveX;
            } else {
                particle[0] += moveX + driftX;
                driftX = -driftX;
                holdX = AdvanceRng() & DRIFT_HOLD_MASK;
            }
            if (holdY != 0) {
                holdY--;
                particle[1] += moveY;
            } else {
                particle[1] += moveY + driftY;
                driftY = -driftY;
                holdY = AdvanceRng() & DRIFT_HOLD_MASK;
            }
            particle[2] += moveZ;
        }
    }
}

void RenderPrecipitationEffect(void) {
    Vec3f fall;
    s16 *particle;
    u8 *texel;
    u8 shade;
    s32 i, step, row, column, dx, dy, sizeX, sizeY, x, y;
    f32 u, v, du, dv, width, height, halfWidth, px, py, pz, clipX, clipY, clipW, scale;

    if (!g_dwSnowMode && !g_dwRainMode) {
        return;
    }
    for (i = 0; i < g_dwCameraRayHitCount; i++) {
        if (g_LoadedTrack.instances[g_awCameraRayHitInstances[i]].flags & TRACK_INSTANCE_SHELTER) {
            return;
        }
    }
    particle = g_awRainParticlePos[g_dwCurrentSplitScreenIndex][0];
    if (g_dwRainMode) {
        StubWeatherUpdate();
    }

    gDPPipeSync(gDisplayListPtr++);
    gDPSetCycleType(gDisplayListPtr++, G_CYC_1CYCLE);
    gSPTexture(gDisplayListPtr++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
    gDPSetTextureFilter(gDisplayListPtr++, g_dwTextureFilterMode);
    gDPSetCombineLERP(gDisplayListPtr++,
                      0,
                      0,
                      0,
                      PRIMITIVE,
                      0,
                      0,
                      0,
                      TEXEL0,
                      0,
                      0,
                      0,
                      PRIMITIVE,
                      0,
                      0,
                      0,
                      TEXEL0);
    gDPSetRenderMode(gDisplayListPtr++,
                     FORCE_BL | CVG_DST_FULL | IM_RD |
                         GBL_c1(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA),
                     GBL_c2(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA));
    gDPSetTextureImage(gDisplayListPtr++,
                       G_IM_FMT_I,
                       G_IM_SIZ_16b,
                       1,
                       g_dwRainMode ? g_abPrecipitationTextures[g_dwCurrentSplitScreenIndex] :
                                      g_aParticleTexture);
    gDPSetTile(gDisplayListPtr++,
               G_IM_FMT_I,
               G_IM_SIZ_16b,
               0,
               0,
               G_TX_LOADTILE,
               0,
               G_TX_WRAP,
               6,
               G_TX_NOLOD,
               G_TX_WRAP,
               6,
               G_TX_NOLOD);
    gDPLoadSync(gDisplayListPtr++);
    gDPLoadBlock(gDisplayListPtr++, G_TX_LOADTILE, 0, 0, PARTICLE_LOAD_TEXELS, PARTICLE_LOAD_DXT);
    gDPPipeSync(gDisplayListPtr++);
    gDPSetTile(gDisplayListPtr++,
               G_IM_FMT_I,
               G_IM_SIZ_8b,
               8,
               0,
               G_TX_RENDERTILE,
               0,
               G_TX_WRAP,
               6,
               G_TX_NOLOD,
               G_TX_WRAP,
               6,
               G_TX_NOLOD);
    gDPSetTileSize(gDisplayListPtr++, G_TX_RENDERTILE, 0, 0, 63 << 2, 63 << 2);
    gDPSetTextureLUT(gDisplayListPtr++, G_TT_NONE);
    gDPSetTexturePersp(gDisplayListPtr++, G_TP_NONE);
    if (g_dwSnowMode) {
        gDPSetPrimColor(gDisplayListPtr++, 0xFF, 0xFF, 0xE0, 0xE0, 0xFF, 0xFF);
    } else {
        gDPSetPrimColor(gDisplayListPtr++, 0xFF, 0xFF, 0x78, 0x80, 0x88, 0xFF);
    }

    /* The view without its translation, taking the axes of a particle to those of the world. */
    MtxInvert4x4(g_afMvpMatrix, (f32(*)[4])g_pActiveCamera);
    g_afMvpMatrix[3][2] = 0.0f;
    g_afScratchMatrix[0][0] = 0.0f;
    g_afScratchMatrix[0][1] = 0.0f;
    g_afScratchMatrix[0][2] = PARTICLE_SCALE;
    g_afScratchMatrix[0][3] = 0.0f;
    g_afScratchMatrix[1][0] = PARTICLE_SCALE;
    g_afScratchMatrix[1][1] = 0.0f;
    g_afScratchMatrix[1][2] = 0.0f;
    g_afScratchMatrix[1][3] = 0.0f;
    g_afScratchMatrix[2][0] = 0.0f;
    g_afScratchMatrix[2][1] = PARTICLE_SCALE;
    g_afScratchMatrix[2][2] = 0.0f;
    g_afScratchMatrix[2][3] = 0.0f;
    g_afScratchMatrix[3][0] = 0.0f;
    g_afScratchMatrix[3][1] = 0.0f;
    g_afScratchMatrix[3][2] = 0.0f;
    g_afScratchMatrix[3][3] = 1.0f;
    g_afMvpMatrix[3][1] = 0.0f;
    g_afMvpMatrix[3][0] = 0.0f;
    Mtx4x4Multiply(g_afMvpMatrix, g_afMvpMatrix, g_afScratchMatrix);

    /* A streak along the fall of the rain on the screen, brighter towards its end. */
    if (g_dwRainMode) {
        MemFill64Aligned(g_abPrecipitationTextures[g_dwCurrentSplitScreenIndex],
                         sizeof(g_abPrecipitationTextures[0]),
                         0);
        Mtx4x4TransformNoTranslation(
            &fall, &g_avRainVelocity[g_dwCurrentSplitScreenIndex], g_afMvpMatrix);
        Vec3fNormalize(&fall);
        if (g_dwViewportMirrored != g_dwTrackMirrored) {
            fall.x = -fall.x;
        }
        u = STREAK_CENTER - fall.x * STREAK_START;
        v = fall.y * STREAK_START + STREAK_CENTER;
        du = fall.x * STREAK_STEP;
        dv = -fall.y * STREAK_STEP;
        v += dv;
        for (step = 0; step < STREAK_STEPS; step++) {
            u += du;
            row = (s32)v;
            if (row >= STREAK_MARGIN && row < RAIN_STREAK_TEXTURE_SIDE - STREAK_MARGIN) {
                column = (s32)u;
                if (column >= STREAK_MARGIN && column < RAIN_STREAK_TEXTURE_SIDE - STREAK_MARGIN) {
                    /* A disc five texels across without its corners. */
                    shade = (step << 4) | step;
                    texel = &g_abPrecipitationTextures[g_dwCurrentSplitScreenIndex]
                                                      [row * RAIN_STREAK_TEXTURE_SIDE + column];
                    for (dy = -2; dy <= 2; dy++) {
                        for (dx = -2; dx <= 2; dx++) {
                            if ((dy == -2 || dy == 2) && (dx == -2 || dx == 2)) {
                                continue;
                            }
                            texel[dy * RAIN_STREAK_TEXTURE_SIDE + dx] = shade;
                        }
                    }
                }
            }
            v += dv;
        }
    }

    AddTimingMarker(0, 0x80, 0xFF, 0xFF, 0xFF);
    Mtx4x4Multiply(g_afMvpMatrix, g_afMvpMatrix, g_afPerspectiveMatrix);
    if (g_dwSnowMode) {
        width = SNOW_PARTICLE_WIDTH;
        height = SNOW_PARTICLE_HEIGHT;
    } else {
        width = RAIN_PARTICLE_SIZE;
        height = RAIN_PARTICLE_SIZE;
    }
    halfWidth = g_dwViewportMirrored != g_dwTrackMirrored ? -SCREEN_HALF_WIDTH : SCREEN_HALF_WIDTH;

    /* Each particle in front of the camera and on the screen is a rectangle of the texture. */
    for (i = 0; i < g_dwRainParticleCount; i++, particle += 3) {
        px = particle[0];
        py = particle[1];
        pz = particle[2];
        clipX = g_afMvpMatrix[3][0] +
                (px * g_afMvpMatrix[0][0] + py * g_afMvpMatrix[1][0] + pz * g_afMvpMatrix[2][0]);
        clipY = g_afMvpMatrix[3][1] +
                (px * g_afMvpMatrix[0][1] + py * g_afMvpMatrix[1][1] + pz * g_afMvpMatrix[2][1]);
        clipW = g_afMvpMatrix[3][3] +
                (px * g_afMvpMatrix[0][3] + py * g_afMvpMatrix[1][3] + pz * g_afMvpMatrix[2][3]);
        if (clipW < PARTICLE_NEAR) {
            continue;
        }
        scale = 0.5f / clipW;
        clipY *= scale;
        if (clipY < -1.0f || clipY > 1.0f) {
            continue;
        }
        clipX *= scale;
        if (clipX < -1.0f || clipX > 1.0f) {
            continue;
        }
        sizeX = (s32)(width * scale);
        if (sizeX < 2) {
            continue;
        }
        sizeY = (s32)(height * scale);
        if (sizeY < 2) {
            continue;
        }
        x = (s32)(clipX * halfWidth);
        y = (s32)(clipY * SCREEN_HALF_HEIGHT);
        gSPTextureRectangle(gDisplayListPtr++,
                            x + SCREEN_CENTER_X,
                            y + SCREEN_CENTER_Y,
                            x + sizeX + SCREEN_CENTER_X,
                            y + sizeY + SCREEN_CENTER_Y,
                            G_TX_RENDERTILE,
                            0,
                            PARTICLE_TEXTURE_LAST_T,
                            PARTICLE_TEXTURE_STEPS / sizeX,
                            -PARTICLE_TEXTURE_STEPS / sizeY);
    }

    gDPPipeSync(gDisplayListPtr++);
    gDPSetTexturePersp(gDisplayListPtr++, G_TP_PERSP);
}
