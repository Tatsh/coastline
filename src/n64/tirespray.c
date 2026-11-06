#include "common/tirespray.h"

#include <PR/gu.h>
#include <PR/os_libc.h>

#include "common/frametime.h"
#include "common/pause.h"
#include "common/racerslot.h"
#include "common/trackselect.h"
#include "common/vehicle.h"
#include "common/vehiclecount.h"
#include "common/weather.h"

void MoveTireSpray(void) {
    int step;
    int slotIndex;
    short shiftX;
    short shiftY;
    short shiftZ;
    int age;
    int wheel;
    int surface;
    Vtx *lowerVertex;
    Vtx *upperVertex;
    Vehicle *vehicle;
    short *upperVelocity;
    int unusedCounter;
    int unusedWords[1];
    short fall;
    Mtx scaleMatrix;

    step = g_fFrameDeltaTime * 4096.0f;
    fall = 9.81f * g_fFrameDeltaTime * 127.0f;
    if (g_dwRacePauseState != 0) {
        return;
    }
    for (slotIndex = 0; slotIndex < g_dwNumVehicles; slotIndex++) {
        if ((vehicle = g_aRacerSlots[slotIndex].vehicle) == 0) {
            continue;
        }
        shiftX = (vehicle->sprayOrigin.x - vehicle->matrix[3][0]) * 127.0f;
        shiftY = (vehicle->sprayOrigin.y - vehicle->matrix[3][1]) * 127.0f;
        shiftZ = (vehicle->sprayOrigin.z - vehicle->matrix[3][2]) * 127.0f;
        vehicle->sprayOrigin.x -= shiftX * 0.007874016f;
        vehicle->sprayOrigin.y -= shiftY * 0.007874016f;
        vehicle->sprayOrigin.z -= shiftZ * 0.007874016f;
        guTranslate(&vehicle->sprayMtx,
                    vehicle->sprayOrigin.x,
                    vehicle->sprayOrigin.y,
                    vehicle->sprayOrigin.z);
        guScale(&scaleMatrix, 0.007874016f, 0.007874016f, 0.007874016f);
        guMtxCatL(&scaleMatrix, &vehicle->sprayMtx, &vehicle->sprayMtx);
        for (wheel = 0; wheel < 4; wheel++) {
            if (vehicle->sprayAdvanced[wheel] != 0) {
                lowerVertex = &vehicle->sprayVertices[wheel][0][0];
                upperVertex = &vehicle->sprayVertices[wheel][0][1];
                switch (vehicle->spraySegmentSurfaces[wheel][0]) {
                case 4:
                    if (g_dwSelectedTrackId == 2 || g_dwSelectedTrackId == 7) {
                        if (g_dwSnowMode != 0) {
                            lowerVertex->v.cn[0] = 70;
                            lowerVertex->v.cn[1] = 100;
                            lowerVertex->v.cn[2] = 100;
                            upperVertex->v.cn[0] = 215;
                            upperVertex->v.cn[1] = 235;
                            upperVertex->v.cn[2] = 195;
                        } else {
                            lowerVertex->v.cn[0] = 50;
                            lowerVertex->v.cn[1] = 100;
                            lowerVertex->v.cn[2] = 95;
                            upperVertex->v.cn[0] = 210;
                            upperVertex->v.cn[1] = 240;
                            upperVertex->v.cn[2] = 190;
                        }
                    } else if (g_dwSelectedTrackId == 3 || g_dwSelectedTrackId == 8) {
                        lowerVertex->v.cn[0] = 16;
                        lowerVertex->v.cn[1] = 16;
                        lowerVertex->v.cn[2] = 0;
                        upperVertex->v.cn[0] = 64;
                        upperVertex->v.cn[1] = 64;
                        upperVertex->v.cn[2] = 16;
                    } else {
                        lowerVertex->v.cn[0] = 48;
                        lowerVertex->v.cn[1] = 24;
                        lowerVertex->v.cn[2] = 8;
                        upperVertex->v.cn[0] = 128;
                        upperVertex->v.cn[1] = 96;
                        upperVertex->v.cn[2] = 64;
                    }
                    if (g_dwRainMode != 0) {
                        lowerVertex->v.cn[0] = lowerVertex->v.cn[0] / 2 + 32;
                        lowerVertex->v.cn[1] = lowerVertex->v.cn[1] / 2 + 32;
                        lowerVertex->v.cn[2] = lowerVertex->v.cn[2] / 2 + 32;
                        upperVertex->v.cn[0] = upperVertex->v.cn[0] / 2 + 32;
                        upperVertex->v.cn[1] = upperVertex->v.cn[1] / 2 + 32;
                        upperVertex->v.cn[2] = upperVertex->v.cn[2] / 2 + 32;
                    } else if (g_dwNightMode != 0) {
                        lowerVertex->v.cn[0] >>= 1;
                        lowerVertex->v.cn[1] >>= 1;
                        lowerVertex->v.cn[2] >>= 1;
                        upperVertex->v.cn[0] >>= 1;
                        upperVertex->v.cn[1] >>= 1;
                        upperVertex->v.cn[2] >>= 1;
                    }
                    break;
                case 0:
                    lowerVertex->v.cn[0] = upperVertex->v.cn[0] = 0x30;
                    lowerVertex->v.cn[1] = upperVertex->v.cn[1] = 0x20;
                    lowerVertex->v.cn[2] = upperVertex->v.cn[2] = 0x10;
                    break;
                case 3:
                    if (g_dwRainMode == 0) {
                        goto none;
                    }
                    lowerVertex->v.cn[0] = 100;
                    lowerVertex->v.cn[1] = 104;
                    lowerVertex->v.cn[2] = 108;
                    upperVertex->v.cn[0] = 160;
                    upperVertex->v.cn[1] = 168;
                    upperVertex->v.cn[2] = 176;
                    break;
                default:
                none:
                    vehicle->spraySegmentSurfaces[wheel][0] = -1;
                    upperVertex->v.cn[0] = vehicle->sprayVertices[wheel][1][1].v.cn[0];
                    upperVertex->v.cn[1] = vehicle->sprayVertices[wheel][1][1].v.cn[1];
                    upperVertex->v.cn[2] = vehicle->sprayVertices[wheel][1][1].v.cn[2];
                    lowerVertex->v.cn[0] = vehicle->sprayVertices[wheel][1][0].v.cn[0];
                    lowerVertex->v.cn[1] = vehicle->sprayVertices[wheel][1][0].v.cn[1];
                    lowerVertex->v.cn[2] = vehicle->sprayVertices[wheel][1][0].v.cn[2];
                    break;
                }
                memcpy(&vehicle->sprayVertices[wheel][1][0],
                       &vehicle->sprayVertices[wheel][0][0],
                       0x40);
                memcpy(
                    &vehicle->sprayVelocities[wheel][1], &vehicle->sprayVelocities[wheel][0], 0x18);
                vehicle->spraySegmentSurfaces[wheel][1] = vehicle->spraySegmentSurfaces[wheel][0];
            }
            for (age = 8; age != 0; age--) {
                lowerVertex = &vehicle->sprayVertices[wheel][age][0];
                upperVertex = &vehicle->sprayVertices[wheel][age][1];
                if (age == 1 || vehicle->sprayAdvanced[wheel] != 0) {
                    memcpy(lowerVertex, &vehicle->sprayVertices[wheel][age - 1][0], 0x40);
                    memcpy(&vehicle->sprayVelocities[wheel][age],
                           &vehicle->sprayVelocities[wheel][age - 1],
                           0x18);
                    vehicle->spraySegmentSurfaces[wheel][age] =
                        vehicle->spraySegmentSurfaces[wheel][age - 1];
                }
                surface = vehicle->spraySegmentSurfaces[wheel][age];
                if (surface == -1) {
                    upperVertex->v.cn[3] = lowerVertex->v.cn[3] = 0;
                } else if (age >= 5) {
                    upperVertex->v.cn[3] = lowerVertex->v.cn[3] = (8 - age) * 0xff / 4;
                } else {
                    upperVertex->v.cn[3] = lowerVertex->v.cn[3] = 0xff;
                }
                if (surface == 3) {
                    lowerVertex->v.tc[0] = 0x410;
                    upperVertex->v.tc[0] = 0x10;
                } else {
                    lowerVertex->v.tc[0] = 0x10;
                    upperVertex->v.tc[0] = 0x410;
                }
                upperVertex->v.tc[1] = lowerVertex->v.tc[1] = age << 11;
                if (surface == -1 || age == 1 || (age == 2 && vehicle->sprayAdvanced[wheel] != 0)) {
                    lowerVertex->v.ob[0] += shiftX;
                    lowerVertex->v.ob[1] += shiftY;
                    lowerVertex->v.ob[2] += shiftZ;
                    upperVertex->v.ob[0] += shiftX;
                    upperVertex->v.ob[1] += shiftY;
                    upperVertex->v.ob[2] += shiftZ;
                } else if (age != 8) {
                    lowerVertex->v.ob[0] +=
                        ((vehicle->sprayVelocities[wheel][age][0][0] * step) >> 12) + shiftX;
                    lowerVertex->v.ob[1] +=
                        ((vehicle->sprayVelocities[wheel][age][0][1] * step) >> 12) + shiftY;
                    lowerVertex->v.ob[2] += shiftZ;
                    upperVelocity = vehicle->sprayVelocities[wheel][age][1];
                    upperVertex->v.ob[0] +=
                        ((vehicle->sprayVelocities[wheel][age][1][0] * step) >> 12) + shiftX;
                    upperVertex->v.ob[1] += ((upperVelocity[1] * step) >> 12) + shiftY;
                    upperVertex->v.ob[2] += ((upperVelocity[2] * step) >> 12) + shiftZ;
                    do {
                    } while (0);
                    if (upperVertex->v.ob[2] < lowerVertex->v.ob[2]) {
                        upperVertex->v.ob[2] = lowerVertex->v.ob[2];
                    } else {
                        upperVelocity[2] -= fall;
                    }
                }
            }
        }
    }
}
