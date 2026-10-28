#include "physics.h"

#include <PR/gu.h>
#include <PR/os.h>
#include <PR/os_libc.h>

#include "game.h"
#include "physicsimpact.h"
#include "timing.h"
#include "track.h"
#include "trianglebox.h"
#include "vehicle.h"
#include "weather.h"

s32 UnusedCollisionResponseStub(void *body, void *normal, void *damp) {
    return 1;
}

void PrintCollisionTriangleCount(void) {
    CollisionTriangleNode *node;
    s16 count;

    node = g_pCollisionTriangleList;
    if (node != NULL) {
        count = 0;
        for (; node != NULL; node = node->next) {
            count++;
        }
        osSyncPrintf("----%d-------------------------------\n", count);
    }
}

s32 UnusedTrackCollisionStub(void *body) {
    return 0;
}

/* The cell of the grid under a point, as its row times 64 plus its column. */
#if TGR_PLATFORM == TGR_PLATFORM_WIN
/* The original converts through 64 bits, so NaN and the infinities give the cell of 0. */
#define GRID_COORDINATE(v) ((s32)(s64)(v))
#else
#define GRID_COORDINATE(v) ((s32)(v))
#endif
#define GRID_SECTOR(x, y) (GRID_COORDINATE(y) / 32 * 64 + GRID_COORDINATE(x) / 32)

s32 InvalidateTriCacheSectorAt(f32 x, f32 y) {
    s16 sector = GRID_SECTOR(x, y);
    s32 slot;

    for (slot = 0; slot < TRI_CACHE_SLOTS; slot++) {
        if (sector == g_awTriCacheSectorId[slot]) {
            g_adwTriCacheLruStamp[slot] = 0;
            g_awTriCacheSectorId[slot] = 0;
            return 1;
        }
    }
    return 0;
}
