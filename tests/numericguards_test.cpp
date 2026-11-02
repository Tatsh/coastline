#include <cassert>
#include <cfloat>
#include <cmath>
#include <initializer_list>

#include "common/fog.c"
#include "common/mathutil.c"
#include "common/tricache.c"

const unsigned char g_abPtr32Anchor[1] = {};
TrackHeader g_LoadedTrack;
TrackTriangle g_aTriCacheTriangles[TRI_CACHE_SLOTS][TRI_CACHE_TRIANGLES];
u16 g_awTriCacheSectorId[TRI_CACHE_SLOTS], g_awTriCacheTriCount[TRI_CACHE_SLOTS];
u32 g_adwTriCacheLruStamp[TRI_CACHE_SLOTS];
u32 g_dwWeatherActive = 1;

static f32 clipDepth, clipW;

void Vec3fTransformByMatrix(f32 out[4], const Vec3f *, f32[4][4]) {
    out[2] = clipDepth;
    out[3] = clipW;
}

u32 LookupTrackGridCell(f32, f32) {
    return 0;
}

u16 NextGridCellTriangle(TrackGridCursor *) {
    assert(false);
    return 0;
}

void Vec3fNormalizeFast(Vec3f *) {
    assert(false);
}

int main() {
    const f32 invalid[] = {NAN,
                           INFINITY,
                           -INFINITY,
                           FLT_MAX,
                           -FLT_MAX,
                           9223372036854775808.0f,
                           -18446744073709551616.0f};
    for (f32 value : invalid) {
        assert(FloatToInt(value) == 0);
        s16 slot = BuildTrackTriangleCache(value, 40.0f);
        assert(g_awTriCacheSectorId[slot] == 64);
        assert(BuildTrackTriangleCache(0.0f, 40.0f) == slot);
        slot = BuildTrackTriangleCache(40.0f, value);
        assert(g_awTriCacheSectorId[slot] == 1);
        assert(BuildTrackTriangleCache(40.0f, 0.0f) == slot);
    }
    assert(FloatToInt(1.9f) == 1);
    assert(FloatToInt(-1.9f) == -1);
    assert(FloatToInt(2147483648.0f) == (-2147483647 - 1));
    assert(FloatToInt(-9223372036854775808.0f) == 0);
    assert(GRID_SECTOR(0.0f, 2147483520.0f) == -256);
    assert(GRID_SECTOR(0.0f, -2147483648.0f) == 0);
    assert(GRID_SECTOR(40.0f, 40.0f) == 65);

    Vec3f point{};
    g_dwFogFactorScale = 255;
    g_dwFogFactorOffset = 0;
    for (f32 w : {0.0f, -0.0f}) {
        clipW = w;
        clipDepth = 1.0f;
        assert(ComputeFogFactorAtPoint(&point) == (std::signbit(w) ? 0.0f : 1.0f));
        clipDepth = -1.0f;
        assert(ComputeFogFactorAtPoint(&point) == (std::signbit(w) ? 1.0f : 0.0f));
        clipDepth = 0.0f;
        assert(ComputeFogFactorAtPoint(&point) == 0.0f);
    }
    clipW = 2.0f;
    clipDepth = 1.0f;
    assert(std::fabs(ComputeFogFactorAtPoint(&point) - 0.5f) < 0.00001f);
    return 0;
}
