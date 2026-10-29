#include <string.h>

#include <PR/gu.h>

#include "common/camera.h"
#include "common/mathutil.h"
#include "common/render.h"
#include "common/screen.h"
#include "common/system.h"
#include "common/timing.h"
#include "common/track.h"
#include "common/trackdisplaylist.h"
#include "common/trackrender.h"
#include "common/trackshadow.h"
#include "common/util.h"
#include "common/vehicleaccess.h"
#include "common/weather.h"

#if TGR_BOSS_RALLY
#include "common/settings.h"
#include "win32/rallyconfig.h"
#include "win32/render.h"
#else
#include "common/audiomix.h"
#endif

#if TGR_BOSS_RALLY
namespace {
    /**
     * Whether shadows include triangles outside their texture and extend beyond their vehicle's
     * screen rectangle.
     *
     * @ghidraAddress USA: 0x8028aa60
     * @ghidraAddress EUR: 0x8028cc68
     * @ghidraAddress JPN: 0x8028c738
     * @ghidraAddress BRD3D: 0x106c660c
     * @ghidraAddress BRGlide: 0x106ed69c
     */
    s32 g_bShadowCullDisabled;
} // namespace
#endif /* TGR_BOSS_RALLY */

/* The words of the display list of the shadows a placed model may fill. */
#define SHADOW_LIST_WORDS 969

/* The texture coordinates past which a vertex is off the texture of a shadow, in s and t. */
#define SHADOW_TEXTURE_S 0x400
#if TGR_BOSS_RALLY
#define SHADOW_TEXTURE_T 0x400
typedef ConvertedModelVertex ShadowVertex;
#define SHADOW_PRODUCT(a, b) ((long double)(a) * (b))
#define SHADOW_DIFFERENCE(a, b) ((long double)(a) - (b))
#else
#define SHADOW_TEXTURE_T 0x1000
typedef Vtx ShadowVertex;
#define SHADOW_PRODUCT(a, b) ((a) * (b))
#define SHADOW_DIFFERENCE(a, b) ((a) - (b))
#endif /* TGR_BOSS_RALLY */

/* The bits of a vertex off the texture: to the left, the right, above, and below. */
#define SHADOW_OFF_LOW_S 0x1
#define SHADOW_OFF_HIGH_S 0x2
#define SHADOW_OFF_LOW_T 0x4
#define SHADOW_OFF_HIGH_T 0x8

/* The most vertices in a load of a placed model. */
#define MAX_LOADED_VERTICES 32

/* The texture of a shadow per unit along and across the car, for its length and width. */
#define SHADOW_LENGTH_SCALE 235.52f
#define SHADOW_WIDTH_SCALE 161.28f

/* The centre of the texture of a shadow, in its coordinates. */
#define SHADOW_CENTER 512.0f

/* The minimum level of detail of the primitive colour, and the colour itself. */
#define PRIM_MIN_LEVEL 0x17

/** The bits of the texture a vertex of a triangle is off, which all of its vertices share. */
static inline u8 TriangleOffTexture(u8 *offTexture, u32 word) {
    u8 bits;

#if !TGR_BOSS_RALLY
    word >>= 1;
#endif
    bits = offTexture[word & 0x1F];
    word >>= 8;
    bits &= offTexture[word & 0x1F];
    word >>= 8;
    bits &= offTexture[word & 0x1F];
    return bits;
}

static inline s32 ShadowListHasRoom(DisplayListCommand *out) {
#if TGR_BOSS_RALLY
    return (s32)((u32)(out - g_pShadowDisplayListStart) * sizeof(*out)) <= 0x13800;
#else
    return (out - g_pShadowDisplayListStart) * 2 < SHADOW_LIST_WORDS;
#endif
}

static inline s32 UnusedShadowLoad(DisplayListCommand *out, DisplayListCommand *lastLoadEnd) {
#if TGR_BOSS_RALLY
    if (g_bDisableCarShadow) {
        return FALSE;
    }
#endif
    return out == lastLoadEnd;
}

static inline ShadowVertex *ShadowVertexSource(DisplayListCommand *command) {
#if TGR_BOSS_RALLY
    return command->vertexData.pConverted;
#else
    return (Vtx *)command->words.w1;
#endif
}

static inline void SetShadowVertexSource(DisplayListCommand *command, ShadowVertex *vertex) {
#if TGR_BOSS_RALLY
    command->vertexData.pConverted = vertex;
#else
    command->words.w1 = (u32)vertex;
#endif
}

#if TGR_BOSS_RALLY
static inline void ProjectShadowTriangleWord(TrackInstance *instance,
                                             Vehicle *vehicle,
                                             ShadowVertex *loaded,
                                             u32 word) {
    ProjectClippedShadowTriangle(instance,
                                 VEHICLE_GET(vehicle, ShadowQuad),
                                 VEHICLE_GET(vehicle, ShadowCornerValid),
                                 &loaded[word & 0x1f],
                                 &loaded[(word >> 8) & 0x1f],
                                 &loaded[(word >> 16) & 0x1f],
                                 VEHICLE_GET(vehicle, Matrix));
}

void RenderTrackInstance(
    const ViewportRect *viewports, s32 index, s32 vehicles, s32 deferred, Vehicle *firstVehicle) {
#else
void RenderTrackInstance(s32 index, s32 vehicles, s32 deferred) {
    ViewportRect *viewports = g_aViewportRects;
    Vehicle *firstVehicle = g_aVehicles;
#endif /* TGR_BOSS_RALLY */
    TrackInstance *instance = &g_LoadedTrack.instances[index];
    DisplayListCommand *displayList = instance->displayList;
    DisplayListCommand *out = g_pShadowDisplayListWritePtr;
    ShadowVertex *vertex = g_pShadowVertexWritePtr;
    DisplayListCommand *source, *lastLoadEnd;
    DisplayListCommand *saved;
    ShadowVertex *from, *savedVertex;
    Vehicle *vehicle;
    f32 *matrix;
#if TGR_BOSS_RALLY
    u8 offTexture[32];
    Vec2f direction;
    long double rawLength, rawWidth, rawS, rawT, sum;
    f32 length, fogAlpha;
#else
    u8 offTexture[64];
    f32 sum;
#endif
    u8 *flag;
    f32 lengthScale, widthScale, alongX, alongY, acrossX, acrossY, scale, x, y, z;
    f32 m00, m10, m20, m30, m01, m11, m21, m31;
    s32 i, count;
#if !TGR_BOSS_RALLY
    s32 s, t;
#endif
    u32 w0;
    u8 bits;

    if (vehicles == 0 ||
        g_pActiveCamera == &VEHICLE_GET(g_pActiveCameraVehicle, Cameras)[VEHICLE_CAMERA_MIRROR] ||
        (instance->flags & TRACK_INSTANCE_NO_SHADOW)
#if TGR_BOSS_RALLY
        || !g_GlobalRallyConfig.bCarShadowSelection
#endif
    ) {
        gSPTexture(gDisplayListPtr++, 0xFFFF, 0xFFFF, 2, G_TX_RENDERTILE, G_ON);
        gDPTileSync(gDisplayListPtr++);
        gSPDisplayList(gDisplayListPtr++, displayList);
        g_dwStatTrianglesDrawn += instance->triangleCount;
        g_dwStatVerticesDrawn += instance->vertexCount;
        g_dwStatPolygonsDrawn += instance->polygonCount;
    } else {
#if TGR_BOSS_RALLY
        NullStub();
#else
        AddTimingMarker(0, 0x50, 0xFA, 0x50, 0xFF);
#endif
        gSPTexture(gDisplayListPtr++, 0xFFFF, 0xFFFF, 2, G_TX_RENDERTILE, G_ON);
        gDPTileSync(gDisplayListPtr++);
        gDPSetPrimColor(gDisplayListPtr++, PRIM_MIN_LEVEL, 0, 0xFF, 0, 0, 0xFF);
        gSPDisplayList(gDisplayListPtr++, displayList);

        /* The shadows: the particle texture, clamped and doubled, in the colour of the fog. */
#if TGR_BOSS_RALLY
        gDisplayListPtr->words.w0 = 0xdc000000 | (g_dwHudOverlayTextureId & 0xffffff);
        gDisplayListPtr->words.w1 = 1;
        gDisplayListPtr++;
#else
        gDPPipeSync(gDisplayListPtr++);
        gDPSetCycleType(gDisplayListPtr++, G_CYC_1CYCLE);
        gDPSetTextureImage(gDisplayListPtr++, G_IM_FMT_I, G_IM_SIZ_16b, 1, g_aParticleTexture);
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
        gDPLoadBlock(
            gDisplayListPtr++, G_TX_LOADTILE, 0, 0, PARTICLE_LOAD_TEXELS, PARTICLE_LOAD_DXT);
        gDPSetTile(gDisplayListPtr++,
                   G_IM_FMT_I,
                   G_IM_SIZ_8b,
                   8,
                   0,
                   G_TX_RENDERTILE,
                   0,
                   G_TX_CLAMP,
                   6,
                   15,
                   G_TX_CLAMP,
                   6,
                   15);
        gDPSetTileSize(gDisplayListPtr++, G_TX_RENDERTILE, 0, 0, 63 << 2, 63 << 2);
        gSPTexture(gDisplayListPtr++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
#endif /* TGR_BOSS_RALLY */
        gDPSetTextureLOD(gDisplayListPtr++, G_TL_TILE);
        gDPSetPrimColor(gDisplayListPtr++, PRIM_MIN_LEVEL, 0, 0xFF, 0, 0, 0xFF);
        if (deferred && !g_dwNightMode && !g_dwRainMode) {
            gDPSetRenderMode(gDisplayListPtr++,
                             Z_CMP | IM_RD | CVG_DST_SAVE | ZMODE_XLU | FORCE_BL |
                                 GBL_c1(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA),
                             GBL_c2(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA));
        } else {
            gDPSetRenderMode(gDisplayListPtr++,
                             Z_CMP | IM_RD | CVG_DST_SAVE | ZMODE_DEC | FORCE_BL |
                                 GBL_c1(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA),
                             GBL_c2(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA));
        }
        gDPSetAlphaCompare(gDisplayListPtr++, G_AC_THRESHOLD);
        gDPSetBlendColor(gDisplayListPtr++, 0, 0, 0, 8);
        gDPSetCombineLERP(gDisplayListPtr++,
                          ENVIRONMENT,
                          0,
                          ENV_ALPHA,
                          0,
                          0,
                          0,
                          0,
                          TEXEL0,
                          ENVIRONMENT,
                          0,
                          ENV_ALPHA,
                          0,
                          0,
                          0,
                          0,
                          TEXEL0);
        gSPClearGeometryMode(gDisplayListPtr++, G_SHADE | G_FOG | G_LIGHTING | G_TEXTURE_GEN);
        gDPSetColorDither(gDisplayListPtr++, G_CD_DISABLE);
        gSPDisplayList(gDisplayListPtr++, out);

        /* Then the modes of the track again. */
        gDPPipeSync(gDisplayListPtr++);
        if (!g_bShadowCullDisabled) {
            SetScissorRect(viewports[g_dwCurrentSplitScreenIndex].x,
                           viewports[g_dwCurrentSplitScreenIndex].y,
                           viewports[g_dwCurrentSplitScreenIndex].width,
                           viewports[g_dwCurrentSplitScreenIndex].height);
        }
        gDPSetColorDither(gDisplayListPtr++, g_dwColorDitherMode);
        gDPSetBlendColor(gDisplayListPtr++, 0, 0, 0, 0);
        if (g_dwWeatherActive) {
            gSPSetGeometryMode(gDisplayListPtr++, G_SHADE | G_FOG | G_LIGHTING);
        } else {
            gSPSetGeometryMode(gDisplayListPtr++, G_SHADE | G_LIGHTING);
        }
        gDPSetTextureLOD(gDisplayListPtr++, g_bTrackTextureLod ? G_TL_LOD : G_TL_TILE);
        gDPSetPrimColor(gDisplayListPtr++, PRIM_MIN_LEVEL, 0, 0xFF, 0, 0, 0xFF);
#if TGR_BOSS_RALLY
        gDPSetRenderMode(gDisplayListPtr++, 0, 0);
#endif
        gDPSetCycleType(gDisplayListPtr++, G_CYC_2CYCLE);
        gDPSetRenderMode(gDisplayListPtr++, g_dwTrackOtherModeLBlend, g_dwTrackOtherModeLFlags);
        SetTrackCombiner(gDisplayListPtr++);
        /* A model with shadows is counted three times over. */
        g_dwStatTrianglesDrawn += instance->triangleCount * 3;
        g_dwStatVerticesDrawn += instance->vertexCount * 3;
        g_dwStatPolygonsDrawn += instance->polygonCount * 3;

        lastLoadEnd = NULL;
        savedVertex = NULL;
        matrix = g_afMvpMatrix[0];
        for (i = 0; vehicles != 0; i++, vehicles >>= 1) {
            if (!(vehicles & 1)) {
                continue;
            }
            /* A vehicle casts no shadow seen from its own cockpit or bumper. */
            if ((g_pActiveCamera ==
                     &VEHICLE_GET(g_pActiveCameraVehicle, Cameras)[VEHICLE_CAMERA_COCKPIT] ||
                 g_pActiveCamera ==
                     &VEHICLE_GET(g_pActiveCameraVehicle, Cameras)[VEHICLE_CAMERA_BUMPER]) &&
                i == VEHICLE_GET(g_pActiveCameraVehicle, Index)) {
                continue;
            }
            vehicle = &firstVehicle[i];
#if TGR_BOSS_RALLY
            if (g_bDisableCarShadow) {
                ResetRenderCommandBufferStack();
            }
            fogAlpha = (long double)VEHICLE_GET(vehicle, FogFactor) * 255.0f;
#endif
            gDPPipeSync(out++);
#if TGR_BOSS_RALLY
            gDPSetEnvColor(out, g_bFogR, g_bFogG, g_bFogB, FloatToInt(fogAlpha));
#else
            gDPSetEnvColor(out, g_bFogR, g_bFogG, g_bFogB, FloatToInt(vehicle->fogFactor * 255.0f));
#endif
            out++;

            /* The texture lies along the vehicle on the ground, centred under it. */
            g_vShadowForward = VEHICLE_GET(vehicle, Frame).forward;
#if TGR_BOSS_RALLY
            if (!(g_vShadowForward.x < 0.0f || g_vShadowForward.x > 0.0f) &&
                !(g_vShadowForward.y < 0.0f || g_vShadowForward.y > 0.0f)) {
#else
            if (g_vShadowForward.x == 0.0f && g_vShadowForward.y == 0.0f) {
#endif
                g_vShadowForward = VEHICLE_GET(vehicle, Frame).up;
            }
            g_vShadowForward.z = 0.0f;
#if TGR_BOSS_RALLY
            direction.x = g_vShadowForward.x;
            direction.y = g_vShadowForward.y;
            rawLength = Vec2fLength(&direction);
            length = rawLength;
            if (!(rawLength >= 0.5f)) {
                length = 0.5f;
            }
            direction.x = VEHICLE_GET(vehicle, Frame).side.x;
            direction.y = VEHICLE_GET(vehicle, Frame).side.y;
            rawWidth = Vec2fLength(&direction);
            if (!(rawWidth >= 0.5f)) {
                rawWidth = 0.5f;
            }
            lengthScale = (long double)SHADOW_LENGTH_SCALE /
                          VEHICLE_GET(vehicle, Model)->wheels[0].x / length;
            widthScale = (long double)SHADOW_WIDTH_SCALE /
                         VEHICLE_GET(vehicle, Model)->wheels[0].y / rawWidth;
#else
            lengthScale = Vec2fLength((Vec2f *)&g_vShadowForward);
            if (lengthScale < 0.5f) {
                lengthScale = 0.5f;
            }
            widthScale = Vec2fLength((Vec2f *)&VEHICLE_GET(vehicle, Frame).side);
            if (widthScale < 0.5f) {
                widthScale = 0.5f;
            }
            /* The first wheel of the model sizes the shadow. */
            lengthScale = SHADOW_LENGTH_SCALE / vehicle->model->wheels[0].x / lengthScale;
            widthScale = SHADOW_WIDTH_SCALE / vehicle->model->wheels[0].y / widthScale;
#endif /* TGR_BOSS_RALLY */
            Vec3fNormalize(&g_vShadowForward);
            alongX = SHADOW_PRODUCT(g_vShadowForward.x, lengthScale);
            alongY = SHADOW_PRODUCT(-g_vShadowForward.y, lengthScale);
            acrossX = SHADOW_PRODUCT(g_vShadowForward.x, widthScale);
            acrossY = SHADOW_PRODUCT(-g_vShadowForward.y, widthScale);
            memcpy(g_afMvpMatrix, instance->matrix, sizeof(g_afMvpMatrix));
            g_afMvpMatrix[3][0] =
                SHADOW_DIFFERENCE(g_afMvpMatrix[3][0], VEHICLE_GET(vehicle, Frame).position.x);
            g_afMvpMatrix[3][1] =
                SHADOW_DIFFERENCE(g_afMvpMatrix[3][1], VEHICLE_GET(vehicle, Frame).position.y);
            g_afScratchMatrix[0][0] = alongX;
            g_afScratchMatrix[0][1] = acrossY;
            g_afScratchMatrix[0][2] = 0.0f;
            g_afScratchMatrix[0][3] = 0.0f;
            g_afScratchMatrix[1][0] = -alongY;
            g_afScratchMatrix[1][1] = acrossX;
            g_afScratchMatrix[1][2] = 0.0f;
            g_afScratchMatrix[1][3] = 0.0f;
            g_afScratchMatrix[2][0] = 0.0f;
            g_afScratchMatrix[2][1] = 0.0f;
            g_afScratchMatrix[2][2] = 1.0f;
            g_afScratchMatrix[2][3] = 0.0f;
            g_afScratchMatrix[3][0] = SHADOW_CENTER;
            g_afScratchMatrix[3][1] = SHADOW_CENTER;
            g_afScratchMatrix[3][2] = 0.0f;
            g_afScratchMatrix[3][3] = 1.0f;
            Mtx4x4Multiply(g_afMvpMatrix, g_afMvpMatrix, g_afScratchMatrix);
#if TGR_BOSS_RALLY
            sum = (((long double)g_afMvpMatrix[0][3] + g_afMvpMatrix[1][3]) + g_afMvpMatrix[2][3]) +
                  g_afMvpMatrix[3][3];
            scale = !(sum < 0.0L || sum > 0.0L) ? 1.0f : 1.0L / sum;
#else
            sum = g_afMvpMatrix[3][3] +
                  ((g_afMvpMatrix[0][3] + g_afMvpMatrix[1][3]) + g_afMvpMatrix[2][3]);
            scale = sum == 0.0f ? 1.0f : 1.0f / sum;
#endif
            g_afMvpMatrix[0][0] = SHADOW_PRODUCT(g_afMvpMatrix[0][0], scale);
            g_afMvpMatrix[1][0] = SHADOW_PRODUCT(g_afMvpMatrix[1][0], scale);
            g_afMvpMatrix[2][0] = SHADOW_PRODUCT(g_afMvpMatrix[2][0], scale);
            g_afMvpMatrix[3][0] = SHADOW_PRODUCT(g_afMvpMatrix[3][0], scale);
            g_afMvpMatrix[0][1] = SHADOW_PRODUCT(g_afMvpMatrix[0][1], scale);
            g_afMvpMatrix[1][1] = SHADOW_PRODUCT(g_afMvpMatrix[1][1], scale);
            g_afMvpMatrix[2][1] = SHADOW_PRODUCT(g_afMvpMatrix[2][1], scale);
            g_afMvpMatrix[3][1] = SHADOW_PRODUCT(g_afMvpMatrix[3][1], scale);

            if (!g_bShadowCullDisabled) {
                saved = gDisplayListPtr;
                gDisplayListPtr = out;
                SetScissorRect(VEHICLE_GET(vehicle, ShadowCorners)[0][0],
                               VEHICLE_GET(vehicle, ShadowCorners)[1][1],
                               VEHICLE_GET(vehicle, ShadowCorners)[1][0] -
                                   VEHICLE_GET(vehicle, ShadowCorners)[0][0],
                               VEHICLE_GET(vehicle, ShadowCorners)[0][1] -
                                   VEHICLE_GET(vehicle, ShadowCorners)[1][1]);
                out = gDisplayListPtr;
                gDisplayListPtr = saved;
            }

            /* Copy the loads and the triangles of the model, with the vertices on the texture. */
            source = displayList;
            while (ShadowListHasRoom(out)) {
                w0 = source->words.w0;
                if ((w0 >> 24) == G_VTX) {
                    m00 = matrix[0];
                    m10 = matrix[4];
                    m20 = matrix[8];
                    m30 = matrix[12];
                    m01 = matrix[1];
                    m11 = matrix[5];
                    m21 = matrix[9];
                    m31 = matrix[13];
                    /* A load that no triangle used is overwritten. */
                    if (UnusedShadowLoad(out, lastLoadEnd)) {
                        vertex = savedVertex;
                        out--;
                    }
                    out->words.w0 = w0;
                    count = (w0 >> 10) & 0x3F;
                    if ((vertex + count) - g_pShadowVertexStart > SHADOW_VERTICES) {
                        goto nextVehicle;
                    }
                    savedVertex = vertex;
                    if (count > MAX_LOADED_VERTICES) {
                        SetDebugMessage("BAD VTX DL");
                    }
                    from = ShadowVertexSource(source++);
                    SetShadowVertexSource(out++, vertex);
                    lastLoadEnd = out;
                    for (flag = offTexture; count-- != 0; vertex++, from++) {
#if TGR_BOSS_RALLY
                        CopyConvertedModelVertex(vertex, from);
                        x = vertex->position.x;
                        y = vertex->position.y;
                        z = vertex->position.z;
                        rawS =
                            (((long double)x * m00 + (long double)z * m20) + (long double)y * m10) +
                            m30;
                        rawT =
                            (((long double)x * m01 + (long double)z * m21) + (long double)y * m11) +
                            m31;
                        vertex->texture.x = rawS;
                        vertex->texture.y = rawT;
                        if (!(rawS >= 0.0L)) {
                            bits = SHADOW_OFF_LOW_S;
                        } else if (vertex->texture.x >= SHADOW_TEXTURE_S) {
                            bits = SHADOW_OFF_HIGH_S;
                        } else {
                            bits = 0;
                        }
                        if (!(vertex->texture.y >= 0.0f)) {
                            bits |= SHADOW_OFF_LOW_T;
                        } else if (vertex->texture.y >= SHADOW_TEXTURE_T) {
                            bits |= SHADOW_OFF_HIGH_T;
                        }
#else
                        CopyVec4(vertex, from);
                        x = (f32)vertex->v.ob[0];
                        y = (f32)vertex->v.ob[1];
                        z = (f32)vertex->v.ob[2];
                        vertex->v.tc[0] = (s32)(((x * m00 + y * m10) + z * m20) + m30);
                        vertex->v.tc[1] = (s32)(((x * m01 + y * m11) + z * m21) + m31);
                        s = vertex->v.tc[0];
                        if (s < 0) {
                            bits = SHADOW_OFF_LOW_S;
                        } else if (s >= SHADOW_TEXTURE_S) {
                            bits = SHADOW_OFF_HIGH_S;
                        } else {
                            bits = 0;
                        }
                        t = vertex->v.tc[1];
                        if (t < 0) {
                            bits |= SHADOW_OFF_LOW_T;
                        } else if (t >= SHADOW_TEXTURE_T) {
                            bits |= SHADOW_OFF_HIGH_T;
                        }
#endif /* TGR_BOSS_RALLY */
                        *flag++ = bits;
                    }
                } else if ((w0 >> 24) == G_TRI2) {
                    if (g_bShadowCullDisabled) {
                        *out = *source;
                        out++;
                        source++;
#if TGR_BOSS_RALLY
                    } else if (g_bDisableCarShadow) {
                        if (!TriangleOffTexture(offTexture, w0)) {
                            ProjectShadowTriangleWord(instance, vehicle, savedVertex, w0);
                        }
                        if (!TriangleOffTexture(offTexture, source->words.w1)) {
                            ProjectShadowTriangleWord(
                                instance, vehicle, savedVertex, source->words.w1);
                        }
                        source++;
#endif /* TGR_BOSS_RALLY */
                    } else if (TriangleOffTexture(offTexture, w0)) {
                        if (!TriangleOffTexture(offTexture, source->words.w1)) {
                            out->words.w0 = _SHIFTL(G_TRI1, 24, 8);
                            out->words.w1 = source->words.w1 & 0xFFFFFF;
                            out++;
                        }
                        source++;
                    } else if (TriangleOffTexture(offTexture, source->words.w1)) {
                        out->words.w0 = _SHIFTL(G_TRI1, 24, 8);
                        out->words.w1 = source->words.w0 & 0xFFFFFF;
                        out++;
                        source++;
                    } else {
                        *out = *source;
                        out++;
                        source++;
                    }
                } else if ((w0 >> 24) == G_ENDDL) {
                    break;
                } else if ((w0 >> 24) == G_TRI1) {
#if TGR_BOSS_RALLY
                    if (!g_bShadowCullDisabled && g_bDisableCarShadow) {
                        if (!TriangleOffTexture(offTexture, source->words.w1)) {
                            ProjectShadowTriangleWord(
                                instance, vehicle, savedVertex, source->words.w1);
                        }
                    } else
#endif
                        if (g_bShadowCullDisabled ||
                            !TriangleOffTexture(offTexture, source->words.w1)) {
                        *out = *source;
                        out++;
                    }
                    source++;
                } else {
                    source++;
                }
            }
            if (UnusedShadowLoad(out, lastLoadEnd)) {
                vertex = savedVertex;
                out--;
            }
        nextVehicle:;
        }
        gSPEndDisplayList(out);
        out++;
#if TGR_BOSS_RALLY
        NullStub();
#else
        AddTimingMarker(0, 0xFF, 0x80, 0x80, 0xFF);
#endif
    }
    g_pShadowVertexWritePtr = vertex;
    g_pShadowDisplayListWritePtr = out;
}
