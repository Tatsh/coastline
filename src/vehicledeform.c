#include "vehicledeform.h"

#include <PR/gbi.h>

#include "vehicleaccess.h"
#include "vehiclemodel.h"

#if TGR_PC
#include "win32/gbicommand.h"
#define MAX_DENT g_wVehicleDeformMaxDent
#define DENT_LESS(value, limit) (!((value) >= (limit)))
#define DENT_COORDINATE(vertex, index, field)                                                      \
    ((s16)(u16)(s64)((long double)(vertex)->position.field + jitter))
#define DENT_MOVE(vertex, index, field, offset)                                                    \
    ((vertex)->position.field = (long double)(offset) + (vertex)->position.field)
#else
#define MAX_DENT 0x100
#define DENT_LESS(value, limit) ((value) < (limit))
#define DENT_COORDINATE(vertex, index, field) ((vertex)->v.ob[index] + jitter)
#define DENT_MOVE(vertex, index, field, offset) ((vertex)->v.ob[index] += (offset))
#endif /* TGR_PC */

#define UNDENTED_DISPLAY_LIST VEHICLE_DL_HIGHLIGHTED

/** How much a coordinate of a vertex moves, by the low bits of another coordinate as noise. */
static inline s32 DentOffset(s32 dent, s32 noise, s32 below, s32 above, s32 shift) {
    if (noise & 0x80) {
        return (dent * (below - (noise & 0xF))) >> shift;
    }
    return (dent * ((noise & 0xF) - above)) >> shift;
}

void DeformVehicleMeshByImpact(Vehicle *vehicle, Vec3f *direction, s16 amount) {
#if TGR_PC
    GbiCommand *command;
    ConvertedModelVertex *vertex;
#else
    Gfx *command;
    Vtx *vertex;
#endif
    s32 degrees, side, maxX, minX, maxY, minY, jitter, dentX, dentY, dentZ, count, lod, list;
    s16 x, y, z, xy;

#if TGR_PC
    degrees = (s32)(s64)(Atan2f(direction->x, direction->y) * 57.2957763671875f);
#else
    degrees = (s32)(Atan2f(direction->x, direction->y) * 180.0f / 3.141592741f);
#endif
    /* The side of the blow picks the box of the model it dents. */
    maxX = 0x3FFF;
    minX = 0xFF;
    maxY = 0x3FFF;
    minY = -0x3FFF;
    if (degrees < 20 || degrees >= 340) {
        side = 0;
    } else if (degrees < 50) {
        minX = 0x80;
        minY = 0x40;
        side = 1;
    } else if (degrees < 130) {
        minX = -0x3FFF;
        minY = 0x40;
        side = 2;
    } else if (degrees < 160) {
        maxX = -0x80;
        minX = -0x3FFF;
        minY = 0x40;
        side = 3;
    } else if (degrees < 200) {
        maxX = -0xFF;
        minX = -0x3FFF;
        side = 4;
    } else if (degrees < 230) {
        maxX = -0x80;
        minX = -0x3FFF;
        maxY = -0x40;
        side = 5;
    } else if (degrees < 310) {
        minX = -0x3FFF;
        maxY = -0x40;
        side = 6;
    } else {
        minX = 0x80;
        maxY = -0x40;
        side = 7;
    }
#if TGR_PC
    amount = (s16)((u32)(s32)amount << 2);
    /* Texture damage precedes the maximum-dent check. */
    vehicle->ApplyImpactDamageTexture(side);
#else
    amount = (s16)(amount << 2);
#endif
    if (VEHICLE_GET(vehicle, Dents)[side] >= MAX_DENT) {
        return;
    }
    VEHICLE_GET(vehicle, Dents)[side] += amount;
    VEHICLE_SET(vehicle, DentJitter, ((VEHICLE_GET(vehicle, DentJitter) + 5) & 7) - 4);
    jitter = VEHICLE_GET(vehicle, DentJitter);
    /* A negative offset is moved up by one. */
    if (jitter < 0) {
        jitter++;
    }

    /* The blow dents across by its sideways part, and up or down by its vertical part. */
    if (1.0f < direction->y) {
        dentY = amount;
    } else if (0.0f < direction->y) {
        dentY = (s16)(amount >> 1);
    } else if (DENT_LESS(direction->y, -1.0f)) {
        dentY = (s16)-amount;
    } else {
        dentY = (s16) - (amount >> 1);
    }
    if (1.0f < direction->z) {
        dentZ = (s16)(amount >> 2);
    } else if (0.0f < direction->z) {
        dentZ = (s16)(amount >> 3);
    } else if (DENT_LESS(direction->z, -1.0f)) {
        dentZ = (s16) - (amount >> 2);
    } else {
        dentZ = (s16) - (amount >> 3);
    }
    if (1.25f < direction->x) {
        dentX = amount;
    } else if (0.0f < direction->x) {
        dentX = (s16)(amount >> 1);
    } else {
        /* A blow from behind dents fully along, and doubles the vertical dent. */
        dentX = (s16)-amount;
        if (DENT_LESS(direction->x, -1.25f)) {
            dentZ = (s16)(dentZ << 1);
        }
    }

    /* Displace vertices inside the box with coordinate noise, excluding highlighted parts. */
    for (lod = 0; lod < VEHICLE_LODS; lod++) {
        for (list = 0; list < VEHICLE_DISPLAY_LISTS; list++) {
            if (list == UNDENTED_DISPLAY_LIST) {
                continue;
            }
            command = VEHICLE_GET(vehicle, Model)->displayLists[lod][list];
            if (command == NULL) {
                continue;
            }
            /* The original tests for vertices before the end of the list. */
            for (;; command++) {
                if ((command->words.w0 >> 24) != G_VTX) {
                    if ((command->words.w0 >> 24) == G_ENDDL) {
                        break;
                    }
                    continue;
                }
                count = (command->words.w0 >> 10) & 0x3F;
#if TGR_PC
                vertex = command->vertexData.pConverted;
#else
                vertex = (Vtx *)command->words.w1;
#endif
                for (; count != 0; count--, vertex++) {
                    x = DENT_COORDINATE(vertex, 0, x);
                    if (!(minX < x && x < maxX)) {
                        continue;
                    }
                    y = DENT_COORDINATE(vertex, 1, y);
                    if (!(minY < y && y < maxY)) {
                        continue;
                    }
                    z = DENT_COORDINATE(vertex, 2, z);
                    if (z < -0x2F || z >= 0xE0) {
                        continue;
                    }
                    DENT_MOVE(vertex, 0, x, DentOffset(dentX, y, 4, 12, 5));
                    DENT_MOVE(vertex, 1, y, DentOffset(dentY, x, 4, 12, 5));
                    xy = x + y;
                    DENT_MOVE(vertex, 2, z, DentOffset(dentZ, xy, 8, 8, 6));
                }
            }
        }
    }
}
