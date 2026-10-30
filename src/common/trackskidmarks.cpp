#include "common/trackskidmarks.h"

#include <cmath>

// Parse the SDK error fields before Windows headers define the errno macro.
#include <PR/os_cont.h>

#include "common/combine.h"
#include "common/displaylistptr.h"
#include "common/rallyconfig.h"
#include "common/renderbackend.h"
#include "common/settings.h"
#include "common/shadowbuffers.h"
#include "common/system.h"
#include "common/terrain.h"
#include "common/trackrender.h"
#include "common/vehicle.h"
#include "common/vehiclecount.h"
#include "common/weather.h"

namespace {
struct SkidMarkSegment {
    Vec3f left;
    Vec3f right;
    unsigned flags;
};

constexpr unsigned ACTIVE = 0x08000000;
constexpr int SEGMENT_COUNT = 500;

// @ghidraAddress BRD3D: 0x10277ea8
// @ghidraAddress BRGlide: 0x10273690
SkidMarkSegment g_aSkidMarkSegments[SKID_MARK_TRACK_COUNT][SEGMENT_COUNT];
// @ghidraAddress BRD3D: 0x100a65d8
// @ghidraAddress BRGlide: 0x100a5d98
int g_anSkidMarkWheelIndices[4] = {2, 1, 3, 0};
// @ghidraAddress BRD3D: 0x118a9874
// @ghidraAddress BRGlide: 0x118ec98c
unsigned g_dwSkidMarkTextureId;

inline int PreviousSegment(int index) {
    return index > 0 ? index - 1 : SEGMENT_COUNT - 1;
}

inline int NextSegment(int index) {
    return index + 1 < SEGMENT_COUNT ? index + 1 : 0;
}

inline void AdvanceTrail(int trail, int head) {
    g_anSkidMarkHead[trail] = NextSegment(head);
    if (g_anSkidMarkHead[trail] == g_anSkidMarkTail[trail]) {
        g_anSkidMarkTail[trail] = NextSegment(g_anSkidMarkTail[trail]);
    }
}

inline void EndTrail(int trail) {
    int head = g_anSkidMarkHead[trail];
    if (head != g_anSkidMarkTail[trail] &&
        (g_aSkidMarkSegments[trail][PreviousSegment(head)].flags & ACTIVE)) {
        g_aSkidMarkSegments[trail][head].flags &= ~ACTIVE;
        AdvanceTrail(trail, head);
    }
}

inline void RecordTrackSkidMarks(Vehicle *vehicle, int vehicleIndex) {
    for (int wheel = 0; wheel < 4; ++wheel) {
        int trail = vehicleIndex * 4 + wheel;
        int contactWheel = (wheel & 1) * 2 + wheel / 2;
        RigidBody *body = vehicle->GetBody().wheels[contactWheel];
        bool active;
        if (g_dwChosenWeather == 2 || g_dwChosenWeather == 3) {
            active = body && body->contactFrames;
        } else {
            active = body && body->surface == TRACK_SURFACE_PAVED && body->contactFrames &&
                     vehicle->GetBody().skidding;
        }
        if (!active) {
            EndTrail(trail);
            continue;
        }
        WheelFrame &frame = vehicle->GetWheelFrames()[g_anSkidMarkWheelIndices[wheel]];
        long double x = frame.side.x;
        long double y = frame.side.y;
        float length = static_cast<float>(std::sqrt(x * x + y * y));
        long double offsetX = x / length * 0.156f;
        long double offsetY = y / length * 0.156f;
        float leftX = static_cast<float>(static_cast<long double>(frame.position.x) -
                                         static_cast<float>(offsetX));
        float rightX = static_cast<float>(offsetX + frame.position.x);
        float leftY = static_cast<float>(static_cast<long double>(frame.position.y) -
                                         static_cast<float>(offsetY));
        float rightY = static_cast<float>(offsetY + frame.position.y);
        float terrainHeight = RaycastTerrainHeight(&frame.position);
        long double height = static_cast<long double>(frame.position.z) - terrainHeight - -0.054f;
        int head = g_anSkidMarkHead[trail];
        int tail = g_anSkidMarkTail[trail];
        if (head != tail) {
            int previous = PreviousSegment(head);
            if (previous != tail && (g_aSkidMarkSegments[trail][previous].flags & ACTIVE)) {
                int earlier = PreviousSegment(previous);
                const SkidMarkSegment &segment = g_aSkidMarkSegments[trail][earlier];
                if (earlier != tail && (segment.flags & ACTIVE)) {
                    long double leftDeltaX = static_cast<long double>(segment.left.x) - leftX;
                    long double leftDeltaY = static_cast<long double>(segment.left.y) - leftY;
                    long double rightDeltaX = static_cast<long double>(segment.right.x) - rightX;
                    long double rightDeltaY = static_cast<long double>(segment.right.y) - rightY;
                    long double leftDistance =
                        leftDeltaY * leftDeltaY + leftDeltaX * static_cast<float>(leftDeltaX);
                    long double rightDistance = rightDeltaY * static_cast<float>(rightDeltaY) +
                                                rightDeltaX * static_cast<float>(rightDeltaX);
                    // The x87 test also takes an unordered distance as near.
                    if (!(leftDistance >= 5.0f) && !(rightDistance >= 5.0f)) {
                        g_anSkidMarkHead[trail] = previous;
                    }
                }
            }
        }
        head = g_anSkidMarkHead[trail];
        SkidMarkSegment &segment = g_aSkidMarkSegments[trail][head];
        segment.left.x = leftX;
        segment.right.x = rightX;
        segment.left.y = leftY;
        segment.right.y = rightY;
        segment.left.z = static_cast<float>(height);
        segment.right.z = static_cast<float>(height);
        segment.flags = vehicle->GetNearbyInstances()[0] | ACTIVE;
        AdvanceTrail(trail, head);
    }
}

inline void WriteTrailVertex(const Vec3f &position, float s, float t) {
    g_pTrackQuadVertexWritePtr->position.x = position.x;
    g_pTrackQuadVertexWritePtr->position.y = position.y;
    g_pTrackQuadVertexWritePtr->position.z = position.z;
    g_pTrackQuadVertexWritePtr->texture.x = s;
    g_pTrackQuadVertexWritePtr->texture.y = t;
    ++g_pTrackQuadVertexWritePtr;
}

inline GbiCommand *DrawSkidTrails(GbiCommand *command) {
    unsigned char active[SKID_MARK_TRACK_COUNT];
    int indices[SKID_MARK_TRACK_COUNT + 1];
    for (int vehicle = 0; vehicle < g_dwNumActiveVehicles; ++vehicle) {
        for (int wheel = 0; wheel < 4; ++wheel) {
            int trail = vehicle * 4 + wheel;
            indices[trail] = PreviousSegment(g_anSkidMarkHead[trail]);
            active[trail] = g_anSkidMarkHead[trail] != g_anSkidMarkTail[trail];
        }
    }
    bool more;
    do {
        more = false;
        for (int vehicle = 0; vehicle < g_dwNumActiveVehicles; ++vehicle) {
            for (int wheel = 0; wheel < 4; ++wheel) {
                int trail = vehicle * 4 + wheel;
                if (!active[trail]) {
                    continue;
                }
                int index = indices[trail];
                if (index == g_anSkidMarkTail[trail]) {
                    active[trail] = false;
                    continue;
                }
                int previous = PreviousSegment(index);
                const SkidMarkSegment &segment = g_aSkidMarkSegments[trail][index];
                if (segment.flags & ACTIVE) {
                    if (previous == g_anSkidMarkTail[trail]) {
                        active[trail] = false;
                        continue;
                    }
                    const SkidMarkSegment &old = g_aSkidMarkSegments[trail][previous];
                    if ((old.flags & ACTIVE) &&
                        (!(g_abInstanceVisibilityMask[segment.flags & ~ACTIVE] & 0x80) ||
                         !(g_abInstanceVisibilityMask[old.flags & ~ACTIVE] & 0x80))) {
                        if (g_pTrackQuadVertexStart + 8000 <= g_pTrackQuadVertexWritePtr + 4) {
                            return command;
                        }
                        command->w0 = 0x0400107f;
                        command->vertexData.pConverted = g_pTrackQuadVertexWritePtr;
                        ++command;
                        WriteTrailVertex(old.right, 0.0f, 0.0f);
                        WriteTrailVertex(segment.right, 1024.0f, 0.0f);
                        WriteTrailVertex(old.left, 0.0f, 1024.0f);
                        WriteTrailVertex(segment.left, 1024.0f, 1024.0f);
                        command->w0 = 0xb1000103;
                        command->w1 = 0x302;
                        ++command;
                    }
                }
                more = true;
                indices[trail] = previous;
            }
        }
    } while (more);
    return command;
}
} // namespace

void DrawTrackShadows(Vehicle *vehicles) {
    GbiCommand *command = g_pShadowDisplayListWritePtr;
    gDisplayListPtr->w0 = 0x06000000;
    gDisplayListPtr->pTarget = command;
    ++gDisplayListPtr;
    command[0].w0 = 0x01060040;
    command[0].pMatrix = g_mtxIdentity;
    command[1].w0 = 0xdc000000 | (g_dwHudOverlayTextureId & 0xffffff);
    command[1].w1 = 1;
    command[2].w0 = 0xb900031d;
    command[2].w1 = !g_dwNightMode && !g_dwRainMode ? 0x504b50 : 0x504f50;
    command[3].w0 = 0xb900031d;
    command[3].w1 = 1;
    command[4].w0 = 0xb9000002;
    command[4].w1 = 1;
    command[5].w0 = 0xf9000000;
    command[5].w1 = 8;
    PackCombineMode(command + 6,
                    1000 + G_CCMUX_ENVIRONMENT,
                    0,
                    1000 + G_CCMUX_ENV_ALPHA,
                    0,
                    0,
                    0,
                    0,
                    1000 + G_ACMUX_TEXEL0,
                    1000 + G_CCMUX_ENVIRONMENT,
                    0,
                    1000 + G_CCMUX_ENV_ALPHA,
                    0,
                    0,
                    0,
                    0,
                    1000 + G_ACMUX_TEXEL0);
    command += 7;
    for (int index = 0; index < g_dwNumActiveVehicles; ++index) {
        Vehicle &vehicle = vehicles[index];
        const auto &valid = vehicle.GetShadowCornerValid();
        if (g_bDisableCarShadow && valid[0] && valid[1] && valid[2] && valid[3]) {
            command[0].w0 = 0xdc000000 | (g_dwHudOverlayTextureId & 0xffffff);
            command[0].w1 = 1;
            command[1].w0 = 0x0400107f;
            command[1].vertexData.pConverted = g_pShadowVertexWritePtr;
            for (int corner = 0; corner < 4; ++corner) {
                CopyConvertedModelVertex(g_pShadowVertexWritePtr, &vehicle.GetShadowQuad()[corner]);
                g_pShadowVertexWritePtr->position.z -= -0.03125f;
                ++g_pShadowVertexWritePtr;
            }
            command[2].w0 = 0xb900031d;
            command[2].w1 = 1;
            command[3].w0 = 0xb1000103;
            command[3].w1 = 0x302;
            command += 4;
        }
        if (g_GlobalRallyConfig.bSkidMarksSelection) {
            RecordTrackSkidMarks(&vehicle, index);
        }
    }
    if (g_GlobalRallyConfig.bSkidMarksSelection) {
        command[0].w0 = 0xb6000000;
        command[0].w1 = 0x3000;
        command[1].w0 = 0xdc000000 | (g_dwSkidMarkTextureId & 0xffffff);
        command[1].w1 = 1;
        command = DrawSkidTrails(command + 2);
        command->w0 = 0xb7000000;
        command->w1 = g_dwTrackMirrored != g_dwViewportMirrored ? 0x1000 : 0x2000;
        ++command;
    }
    command[0].w0 = 0xf9000000;
    command[0].w1 = 0;
    command[1].w0 = 0xb900031d;
    command[1].w1 = 0;
    command[2].w0 = 0xba001402;
    command[2].w1 = 0x100000;
    command[3].w0 = 0xb900031d;
    command[3].w1 = g_dwTrackOtherModeLFlags | g_dwTrackOtherModeLBlend;
    PackCombineMode(command + 4,
                    1000 + G_CCMUX_TEXEL1,
                    1000 + G_CCMUX_TEXEL0,
                    1000 + G_CCMUX_LOD_FRACTION,
                    1000 + G_CCMUX_TEXEL0,
                    1000 + G_ACMUX_TEXEL1,
                    1000 + G_ACMUX_TEXEL0,
                    1000 + G_CCMUX_LOD_FRACTION,
                    1000 + G_ACMUX_TEXEL0,
                    1000 + G_CCMUX_COMBINED,
                    0,
                    1000 + G_CCMUX_SHADE,
                    0,
                    0,
                    0,
                    0,
                    1000 + G_ACMUX_COMBINED);
    command[5].w0 = 0xba000602;
    command[5].w1 = g_dwColorDitherMode;
    command[6].w0 = 0xbd000000;
    command[6].w1 = 0;
    command[7].w0 = 0xb8000000;
    command[7].w1 = 0;
    // The native pool advances past one unwritten command.
    g_pShadowDisplayListWritePtr = command + 9;
}

#if TGR_PLATFORM == TGR_PLATFORM_SDL
void RecordFrameSkidMarks(Vehicle *vehicles) {
    if (!g_GlobalRallyConfig.bSkidMarksSelection) {
        return;
    }
    for (int index = 0; index < g_dwNumActiveVehicles; ++index) {
        RecordTrackSkidMarks(&vehicles[index], index);
    }
}

// DrawSkidTrails(), with the pool of the vertices of the quads of the track to itself.
// ponytail: the shadows of the cars take vertices of the pool before the skid marks on the PC
// release, and the instances near the eighth vehicle read as out of view there.
void VisitSkidMarkQuads(void (*visit)(const Vec3f *oldRight,
                                      const Vec3f *newRight,
                                      const Vec3f *oldLeft,
                                      const Vec3f *newLeft,
                                      void *context),
                        void *context,
                        const unsigned char *listed) {
    int vertices = 0;
    bool active[SKID_MARK_TRACK_COUNT] = {};
    int indices[SKID_MARK_TRACK_COUNT] = {};
    for (int trail = 0; trail < g_dwNumActiveVehicles * 4; ++trail) {
        indices[trail] = PreviousSegment(g_anSkidMarkHead[trail]);
        active[trail] = g_anSkidMarkHead[trail] != g_anSkidMarkTail[trail];
    }
    bool more;
    do {
        more = false;
        for (int trail = 0; trail < g_dwNumActiveVehicles * 4; ++trail) {
            if (!active[trail]) {
                continue;
            }
            int index = indices[trail];
            int previous = PreviousSegment(index);
            const SkidMarkSegment &segment = g_aSkidMarkSegments[trail][index];
            if (index == g_anSkidMarkTail[trail] ||
                ((segment.flags & ACTIVE) && previous == g_anSkidMarkTail[trail])) {
                active[trail] = false;
                continue;
            }
            const SkidMarkSegment &old = g_aSkidMarkSegments[trail][previous];
            if ((segment.flags & ACTIVE) && (old.flags & ACTIVE) &&
                (listed[segment.flags & ~ACTIVE] || listed[old.flags & ~ACTIVE])) {
                if (TRACK_QUAD_VERTICES <= vertices + 4) {
                    return;
                }
                vertices += 4;
                visit(&old.right, &segment.right, &old.left, &segment.left, context);
            }
            more = true;
            indices[trail] = previous;
        }
    } while (more);
}
#endif /* TGR_PLATFORM == TGR_PLATFORM_SDL */
