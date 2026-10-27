#include <PR/gu.h>

#include "effects.h"
#include "frametime.h"
#include "instantreplay.h"
#include "pause.h"
#include "trackselect.h"
#include "vehiclecount.h"
#include "weather.h"

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#include <string.h>
#else
#include <PR/os_libc.h>
#endif

#define SPRAY_UNITS 127.0f
#define SPRAY_STEP_SHIFT 12
#define SPRAY_FADE_SEGMENT 5
#define SPRAY_TEXTURE_NEAR 0x10
#define SPRAY_TEXTURE_FAR 0x410
#define SPRAY_TEXTURE_SEGMENT_SHIFT 11

#if TGR_PLATFORM == TGR_PLATFORM_WIN
typedef ConvertedModelVertex SprayMoveVertex;
typedef long double SprayMoveReal;
static inline f32 &SprayColor(SprayMoveVertex *vertices, s32 vertex, s32 channel) {
    if (channel == 0) {
        return vertices[vertex].color.x;
    }
    if (channel == 1) {
        return vertices[vertex].color.y;
    }
    return vertices[vertex].color.z;
}
#define SPRAY_COLOR(vertices, vertex, channel) SprayColor(vertices, vertex, channel)
#define SPRAY_WET_COLOR(value) (32.0L - (long double)(value) * -0.5f)
#define SPRAY_NIGHT_COLOR(value) ((long double)(value) * 0.5f)
#define SPRAY_COLOR_SEGMENT(vehicle, wheel) ((vehicle)->ColorSpraySegment(wheel))
#define SPRAY_AGE_SEGMENT(vehicle, wheel, age) ((vehicle)->AgeSpraySegment(wheel, age))
#define SPRAY_MOVE_INTEGER(value) ((s64)(value))
#define SPRAY_MATRIX(matrix) (matrix)
#define SPRAY_TRANSLATE guTranslateF
#define SPRAY_SCALE guScaleF
#define SPRAY_MATRIX_CAT guMtxCatF
#define SPRAY_POSITION(segment, vertex, coordinate, field) ((segment)[vertex].position.field)
#define SPRAY_TEXCOORD(segment, vertex, coordinate, field) ((segment)[vertex].texture.field)
#define SPRAY_VELOCITY(velocity, step)                                                             \
    (((s32)((u32)(s32)(velocity) * (u32)(step))) >> SPRAY_STEP_SHIFT)
#define SPRAY_MOVE_POSITION(segment, vertex, coordinate, field, velocity, shift)                   \
    (SPRAY_POSITION(segment, vertex, coordinate, field) =                                          \
         (long double)(SPRAY_VELOCITY(velocity, step) + (shift)) +                                 \
         SPRAY_POSITION(segment, vertex, coordinate, field))
#else
typedef Vtx SprayMoveVertex;
typedef f32 SprayMoveReal;
#define SPRAY_COLOR(vertices, vertex, channel) ((vertices)[vertex].v.cn[channel])
#define SPRAY_WET_COLOR(value) ((value) / 2 + 0x20)
#define SPRAY_NIGHT_COLOR(value) ((value) >> 1)
#define SPRAY_COLOR_SEGMENT(vehicle, wheel) ColorSpraySegment(vehicle, wheel)
#define SPRAY_AGE_SEGMENT(vehicle, wheel, age) AgeSpraySegment(vehicle, wheel, age)
#define SPRAY_MOVE_INTEGER(value) ((s32)(value))
#define SPRAY_MATRIX(matrix) (&(matrix))
#define SPRAY_TRANSLATE guTranslate
#define SPRAY_SCALE guScale
#define SPRAY_MATRIX_CAT guMtxCatL
#define SPRAY_POSITION(segment, vertex, coordinate, field) ((segment)[vertex].v.ob[coordinate])
#define SPRAY_TEXCOORD(segment, vertex, coordinate, field) ((segment)[vertex].v.tc[coordinate])
#define SPRAY_MOVE_POSITION(segment, vertex, coordinate, field, velocity, shift)                   \
    (SPRAY_POSITION(segment, vertex, coordinate, field) =                                          \
         SPRAY_POSITION(segment, vertex, coordinate, field) +                                      \
         (((velocity) * step) >> SPRAY_STEP_SHIFT) + (shift))
#endif

/** Set the colours of the two drawn vertices of the newest segment of a spray. */
static inline void SetSprayColors(
    SprayMoveVertex *head, s32 red, s32 green, s32 blue, s32 farRed, s32 farGreen, s32 farBlue) {
    SPRAY_COLOR(head, 0, 0) = red;
    SPRAY_COLOR(head, 0, 1) = green;
    SPRAY_COLOR(head, 0, 2) = blue;
    SPRAY_COLOR(head, 1, 0) = farRed;
    SPRAY_COLOR(head, 1, 1) = farGreen;
    SPRAY_COLOR(head, 1, 2) = farBlue;
}

/**
 * Colour the newest segment of a spray by its surface, the track, and the weather; a segment
 * without a spraying surface preserves the previous colours and has surface -1.
 */
#if TGR_PLATFORM == TGR_PLATFORM_WIN
inline void Vehicle::ColorSpraySegment(s32 wheel) {
    Vehicle *vehicle = this;
#else
static inline void ColorSpraySegment(Vehicle *vehicle, s32 wheel) {
#endif
    SprayMoveVertex *head = vehicle->sprayVertices[wheel][0];
    SprayMoveVertex *last = vehicle->sprayVertices[wheel][1];
    s32 i;

    switch (vehicle->spraySegmentSurfaces[wheel][0]) {
    case 0:
        SPRAY_COLOR(head, 0, 0) = SPRAY_COLOR(head, 1, 0) = 0x30;
        SPRAY_COLOR(head, 0, 1) = SPRAY_COLOR(head, 1, 1) = 0x20;
        SPRAY_COLOR(head, 0, 2) = SPRAY_COLOR(head, 1, 2) = 0x10;
        return;
    case TRACK_SURFACE_PAVED:
        if (g_dwRainMode) {
            SetSprayColors(head, 0x64, 0x68, 0x6C, 0xA0, 0xA8, 0xB0);
            return;
        }
        break;
    case TRACK_SURFACE_SLOW:
        if (g_dwSelectedTrackId == TRACK_COASTLINE ||
            g_dwSelectedTrackId == TRACK_MIRROR_COASTLINE) {
            if (g_dwSnowMode) {
                SetSprayColors(head, 0x46, 0x64, 0x64, 0xD7, 0xEB, 0xC3);
            } else {
                SetSprayColors(head, 0x32, 0x64, 0x5F, 0xD2, 0xF0, 0xBE);
            }
        } else if (g_dwSelectedTrackId == TRACK_STRIP_MINE ||
                   g_dwSelectedTrackId == TRACK_MIRROR_STRIP_MINE) {
            SetSprayColors(head, 0x10, 0x10, 0x00, 0x40, 0x40, 0x10);
        } else {
            SetSprayColors(head, 0x30, 0x18, 0x08, 0x80, 0x60, 0x40);
        }
        if (g_dwRainMode) {
            for (i = 0; i < 3; i++) {
                SPRAY_COLOR(head, 0, i) = SPRAY_WET_COLOR(SPRAY_COLOR(head, 0, i));
                SPRAY_COLOR(head, 1, i) = SPRAY_WET_COLOR(SPRAY_COLOR(head, 1, i));
            }
        } else if (g_dwNightMode) {
            for (i = 0; i < 3; i++) {
                SPRAY_COLOR(head, 0, i) = SPRAY_NIGHT_COLOR(SPRAY_COLOR(head, 0, i));
                SPRAY_COLOR(head, 1, i) = SPRAY_NIGHT_COLOR(SPRAY_COLOR(head, 1, i));
            }
        }
        return;
    }
    vehicle->spraySegmentSurfaces[wheel][0] = -1;
    SPRAY_COLOR(head, 1, 0) = SPRAY_COLOR(last, 1, 0);
    SPRAY_COLOR(head, 1, 1) = SPRAY_COLOR(last, 1, 1);
    SPRAY_COLOR(head, 1, 2) = SPRAY_COLOR(last, 1, 2);
    SPRAY_COLOR(head, 0, 0) = SPRAY_COLOR(last, 0, 0);
    SPRAY_COLOR(head, 0, 1) = SPRAY_COLOR(last, 0, 1);
    SPRAY_COLOR(head, 0, 2) = SPRAY_COLOR(last, 0, 2);
}

/** Copy a segment of a spray over the next older one. */
#if TGR_PLATFORM == TGR_PLATFORM_WIN
inline void Vehicle::AgeSpraySegment(s32 wheel, s32 age) {
    Vehicle *vehicle = this;
#else
static inline void AgeSpraySegment(Vehicle *vehicle, s32 wheel, s32 age) {
#endif
    memcpy(vehicle->sprayVertices[wheel][age],
           vehicle->sprayVertices[wheel][age - 1],
           sizeof(vehicle->sprayVertices[wheel][age]));
    memcpy(vehicle->sprayVelocities[wheel][age],
           vehicle->sprayVelocities[wheel][age - 1],
           sizeof(vehicle->sprayVelocities[wheel][age]));
    vehicle->spraySegmentSurfaces[wheel][age] = vehicle->spraySegmentSurfaces[wheel][age - 1];
}

#if TGR_PLATFORM == TGR_PLATFORM_WIN
void Vehicle::MoveTireSpraySegments(s32 step, s16 fall) {
    Vehicle *vehicle = this;
    f32 scale[4][4];
    SprayMoveVertex *segment;
    s16(*velocities)[3];
    s32 wheel, age;
    s16 shiftX, shiftY, shiftZ, surface;
    volatile long double topZ;
#else
void MoveTireSpray(void) {
    RacerSlot *slot;
    Vehicle *vehicle;
    Mtx scale;
    Vtx *segment;
    s16(*velocities)[3];
    s32 step, wheel, age, alpha;
    s16 fall, shiftX, shiftY, shiftZ, surface;

    step = (s32)(g_fFrameDeltaTime * (1 << SPRAY_STEP_SHIFT));
    fall = (s32)(9.81f * g_fFrameDeltaTime * SPRAY_UNITS);
    if (g_dwRacePauseState != 0) {
        return;
    }
    for (slot = g_aRacerSlots; slot < &g_aRacerSlots[g_dwNumVehicles]; slot++) {
        vehicle = slot->vehicle;
        if (vehicle == NULL) {
            continue;
        }

#endif
    /* The origin moves to the vehicle in whole units of the vertices, which move back. */
    shiftX = SPRAY_MOVE_INTEGER(
        ((SprayMoveReal)vehicle->sprayOrigin.x - vehicle->frame.position.x) * SPRAY_UNITS);
    shiftY = SPRAY_MOVE_INTEGER(
        ((SprayMoveReal)vehicle->sprayOrigin.y - vehicle->frame.position.y) * SPRAY_UNITS);
    shiftZ = SPRAY_MOVE_INTEGER(
        ((SprayMoveReal)vehicle->sprayOrigin.z - vehicle->frame.position.z) * SPRAY_UNITS);
    vehicle->sprayOrigin.x -= (SprayMoveReal)shiftX * (1.0f / SPRAY_UNITS);
    vehicle->sprayOrigin.y -= (SprayMoveReal)shiftY * (1.0f / SPRAY_UNITS);
    vehicle->sprayOrigin.z -= (SprayMoveReal)shiftZ * (1.0f / SPRAY_UNITS);
    SPRAY_TRANSLATE(SPRAY_MATRIX(vehicle->sprayMtx),
                    vehicle->sprayOrigin.x,
                    vehicle->sprayOrigin.y,
                    vehicle->sprayOrigin.z);
    SPRAY_SCALE(SPRAY_MATRIX(scale), 1.0f / SPRAY_UNITS, 1.0f / SPRAY_UNITS, 1.0f / SPRAY_UNITS);
    SPRAY_MATRIX_CAT(
        SPRAY_MATRIX(scale), SPRAY_MATRIX(vehicle->sprayMtx), SPRAY_MATRIX(vehicle->sprayMtx));

    for (wheel = 0; wheel < VEHICLE_WHEELS; wheel++) {
        if (vehicle->sprayAdvanced[wheel]) {
            SPRAY_COLOR_SEGMENT(vehicle, wheel);
            SPRAY_AGE_SEGMENT(vehicle, wheel, 1);
        }
        /* The second segment follows the newest every frame, and is aged again here. */
        for (age = SPRAY_SEGMENTS - 1; age != 0; age--) {
            if (age == 1 || vehicle->sprayAdvanced[wheel]) {
                SPRAY_AGE_SEGMENT(vehicle, wheel, age);
            }
            surface = vehicle->spraySegmentSurfaces[wheel][age];
            segment = vehicle->sprayVertices[wheel][age];
#if TGR_PLATFORM == TGR_PLATFORM_N64
            if (surface == -1) {
                alpha = 0;
            } else if (age >= SPRAY_FADE_SEGMENT) {
                alpha = 255 * (SPRAY_SEGMENTS - 1 - age) / 4;
            } else {
                alpha = 255;
            }
            segment[0].v.cn[3] = alpha;
            segment[1].v.cn[3] = alpha;
#endif
            if (surface == TRACK_SURFACE_PAVED) {
                SPRAY_TEXCOORD(segment, 0, 0, x) = SPRAY_TEXTURE_FAR;
                SPRAY_TEXCOORD(segment, 1, 0, x) = SPRAY_TEXTURE_NEAR;
            } else {
                SPRAY_TEXCOORD(segment, 0, 0, x) = SPRAY_TEXTURE_NEAR;
                SPRAY_TEXCOORD(segment, 1, 0, x) = SPRAY_TEXTURE_FAR;
            }
            SPRAY_TEXCOORD(segment, 0, 1, y) = age << SPRAY_TEXTURE_SEGMENT_SHIFT;
            SPRAY_TEXCOORD(segment, 1, 1, y) = age << SPRAY_TEXTURE_SEGMENT_SHIFT;

            /*
             * The newest segments and those thrown from nothing only follow the origin. The
             * oldest of the others does not follow it, since it is drawn clear.
             */
            if (surface == -1 || age == 1 || (age == 2 && vehicle->sprayAdvanced[wheel])) {
                SPRAY_POSITION(segment, 0, 0, x) += shiftX;
                SPRAY_POSITION(segment, 0, 1, y) += shiftY;
                SPRAY_POSITION(segment, 0, 2, z) += shiftZ;
                SPRAY_POSITION(segment, 1, 0, x) += shiftX;
                SPRAY_POSITION(segment, 1, 1, y) += shiftY;
                SPRAY_POSITION(segment, 1, 2, z) += shiftZ;
            } else if (age != SPRAY_SEGMENTS - 1) {
                velocities = vehicle->sprayVelocities[wheel][age];
                SPRAY_MOVE_POSITION(segment, 0, 0, x, velocities[0][0], shiftX);
                SPRAY_MOVE_POSITION(segment, 0, 1, y, velocities[0][1], shiftY);
                SPRAY_POSITION(segment, 0, 2, z) += shiftZ;
                SPRAY_MOVE_POSITION(segment, 1, 0, x, velocities[1][0], shiftX);
                SPRAY_MOVE_POSITION(segment, 1, 1, y, velocities[1][1], shiftY);
#if TGR_PLATFORM == TGR_PLATFORM_WIN
                topZ = (long double)(SPRAY_VELOCITY(velocities[1][2], step) + shiftZ) +
                       SPRAY_POSITION(segment, 1, 2, z);
                SPRAY_POSITION(segment, 1, 2, z) = topZ;
                if (!(topZ >= SPRAY_POSITION(segment, 0, 2, z))) {
                    SPRAY_POSITION(segment, 1, 2, z) = SPRAY_POSITION(segment, 0, 2, z);
                } else {
                    velocities[1][2] -= fall;
                }
#else
                    SPRAY_POSITION(segment, 1, 2, z) =
                        SPRAY_POSITION(segment, 1, 2, z) +
                        ((velocities[1][2] * step) >> SPRAY_STEP_SHIFT) + shiftZ;
                    /* The top edge falls until it lands on the bottom one. */
                    if (SPRAY_POSITION(segment, 1, 2, z) < SPRAY_POSITION(segment, 0, 2, z)) {
                        SPRAY_POSITION(segment, 1, 2, z) = SPRAY_POSITION(segment, 0, 2, z);
                    } else {
                        velocities[1][2] -= fall;
                    }
#endif
            }
        }
    }
#if TGR_PLATFORM == TGR_PLATFORM_WIN
}
#else
    }
}
#endif

#if TGR_PLATFORM == TGR_PLATFORM_WIN
void MoveTireSpray(void) {
    s32 step = (s32)(s64)((long double)g_fFrameDeltaTime * (1 << SPRAY_STEP_SHIFT));
    s16 fall = (s16)(s64)((long double)g_fFrameDeltaTime * 9.81f * SPRAY_UNITS);
    s32 slotIndex;

    if (g_dwRacePauseState != 0 || gInstantReplayActive == 2) {
        return;
    }
    for (slotIndex = 0; slotIndex < g_dwNumVehicles; slotIndex++) {
        Vehicle *vehicle = g_aRacerSlots[slotIndex].GetVehicle();
        if (vehicle != NULL) {
            vehicle->MoveTireSpraySegments(step, fall);
        }
    }
}
#endif
