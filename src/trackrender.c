#include "trackrender.h"

#include <PR/gbi.h>
#include <PR/gu.h>
#include <PR/os.h>
#include <PR/os_libc.h>

#include "audiomix.h"
#include "camera.h"
#include "debugtext.h"
#include "game.h"
#include "input.h"
#include "mathutil.h"
#include "race.h"
#include "render.h"
#include "system.h"
#include "timing.h"
#include "track.h"
#include "trackselect.h"
#include "util.h"
#include "visgrid.h"
#include "weather.h"

/* The physical address of a matrix in the cached segment, which wraps past the top of memory. */
#define PHYSICAL(address) ((u32)(address) + 0x80000000)

/* The words of the display list of the shadows a placed model may fill. */
#define SHADOW_LIST_WORDS 969

/* The texture coordinates past which a vertex is off the texture of a shadow, in s and t. */
#define SHADOW_TEXTURE_S 0x400
#define SHADOW_TEXTURE_T 0x1000

/* The bits of a vertex off the texture: to the left, the right, above, and below. */
#define SHADOW_OFF_LOW_S 0x1
#define SHADOW_OFF_HIGH_S 0x2
#define SHADOW_OFF_LOW_T 0x4
#define SHADOW_OFF_HIGH_T 0x8

/* The most vertices a load of the display list of a placed model may hold. */
#define MAX_LOADED_VERTICES 32

/* The texture of a shadow per unit along and across the car, for its length and width. */
#define SHADOW_LENGTH_SCALE 235.52f
#define SHADOW_WIDTH_SCALE 161.28f

/* The centre of the texture of a shadow, in its coordinates. */
#define SHADOW_CENTER 512.0f

/* The minimum level of detail of the primitive colour, and the colour itself. */
#define PRIM_MIN_LEVEL 0x17

/* The size of a cell of the grid of the track. */
#define GRID_CELL_SIZE 32.0f

/* The cells the grid of the track has on a side. */
#define GRID_CELLS 64

/* The squared distance in cells from which the cells count as near. */
#define NEAR_CELL_DISTANCE 9

/* The ranges a fixed point matrix holds. */
#define FIXED_MATRIX_MAX 32767.0f

/* g_nTrackTransparentStartIdx before it is known. */
#define TRANSPARENT_START_UNKNOWN 0x1000

/** The bits of the texture a vertex of a triangle is off, which all of its vertices share. */
static inline u8 TriangleOffTexture(u8 *offTexture, u32 word) {
    u8 bits;

    word >>= 1;
    bits = offTexture[word & 0x1F];
    word >>= 8;
    bits &= offTexture[word & 0x1F];
    word >>= 8;
    bits &= offTexture[word & 0x1F];
    return bits;
}

/** The geometry modes of the track, with its culling, shading and fog. */
static inline u32 TrackGeometryMode(void) {
    u32 fog = g_dwWeatherActive ? G_FOG : 0;
    u32 shading = g_bSmoothShading ? G_SHADING_SMOOTH : 0;
    u32 cull = g_dwTrackMirrored != g_dwViewportMirrored ? G_CULL_FRONT : G_CULL_BACK;

    return (cull | G_ZBUFFER | G_SHADE | G_LIGHTING | G_TEXTURE_GEN_LINEAR) | shading | fog;
}

/** Load the matrices and the lights of the track. */
static inline void LoadTrackMatricesAndLights(u32 identity) {
    gSPMatrix(gDisplayListPtr++,
              PHYSICAL(g_pProjectionMtx),
              G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
    gSPPerspNormalize(gDisplayListPtr++, g_wPerspNorm);
    gSPMatrix(gDisplayListPtr++, identity, G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_PUSH);
    gSPNumLights(gDisplayListPtr++, NUMLIGHTS_1);
    gSPLight(gDisplayListPtr++, &g_aTrackLights[g_nTrackLightBufferIndex].l[0], 1);
    gSPLight(gDisplayListPtr++, &g_aTrackLights[g_nTrackLightBufferIndex].a, 2);
    gSPLightColor(gDisplayListPtr++, LIGHT_1, g_dwDirLightColor);
    gSPLightColor(gDisplayListPtr++, LIGHT_2, g_dwAmbientLightColor);
}

/** Set the modes of the textures of the track. */
static inline void SetTrackTextureModes(void) {
    gDPSetTextureDetail(gDisplayListPtr++, G_TD_CLAMP);
    gDPSetTextureLOD(gDisplayListPtr++, g_bTrackTextureLod ? G_TL_LOD : G_TL_TILE);
    gDPSetTextureLUT(gDisplayListPtr++, G_TT_NONE);
    gDPSetTextureFilter(gDisplayListPtr++, g_dwTextureFilterMode);
}

/** Set the clipping ratio for the views on the screen. */
static inline void SetTrackClipRatio(void) {
    if (g_dwNumViewports == 1) {
        gSPClipRatio(gDisplayListPtr++, FRUSTRATIO_1);
    } else {
        gSPClipRatio(gDisplayListPtr++, FRUSTRATIO_6);
    }
}

/** Set the combiner of the track: the two levels of a texture mixed and then shaded. */
static inline void SetTrackCombiner(Gfx *pkt) {
    gDPSetCombineLERP(pkt,
                      TEXEL1,
                      TEXEL0,
                      LOD_FRACTION,
                      TEXEL0,
                      TEXEL1,
                      TEXEL0,
                      LOD_FRACTION,
                      TEXEL0,
                      COMBINED,
                      0,
                      SHADE,
                      0,
                      0,
                      0,
                      0,
                      COMBINED);
}

/** Draw a placed model without shadows, and count it. */
static inline void DrawPlainInstance(TrackInstance *instance) {
    gSPTexture(gDisplayListPtr++, 0xFFFF, 0xFFFF, 2, G_TX_RENDERTILE, G_ON);
    gDPTileSync(gDisplayListPtr++);
    gSPDisplayList(gDisplayListPtr++, instance->displayList);
    g_dwStatTrianglesDrawn += instance->triangleCount;
    g_dwStatVerticesDrawn += instance->vertexCount;
    g_dwStatPolygonsDrawn += instance->polygonCount;
}

void ProjectWorldToScreenRect(Vec3f *point, s32 size, s16 low[2], s16 high[2]) {
    ViewportRect *view = &g_aViewportRects[g_dwCurrentSplitScreenIndex];
    f32 clip[4];
    f32 upper[2];
    f32 inverse, radius, halfWidth, halfHeight;
    s32 halfWidthPixels = view->width >> 1;
    s32 halfHeightPixels = view->height >> 1;
    s32 centerX = view->x + halfWidthPixels;
    s32 centerY = view->y + halfHeightPixels;

    Vec3fTransformByMatrix(clip, point, g_afViewProjMatrix);
    if (clip[3] <= 0.001f && clip[3] >= -0.001f) {
        return;
    }
    inverse = 1.0f / clip[3];
    if (g_dwTrackMirrored != g_dwViewportMirrored) {
        clip[0] *= -inverse;
    } else {
        clip[0] *= inverse;
    }
    clip[1] *= inverse;
    radius = (f32)size * inverse;
    /* The lower corner overwrites the point on the stack, and the upper one has its own pair. */
    upper[0] = clip[0] + radius;
    clip[0] -= radius;
    upper[1] = clip[1] + radius;
    clip[1] -= radius;
    halfWidth = (f32)halfWidthPixels;
    halfHeight = (f32)halfHeightPixels;
    low[0] = (s32)(clip[0] * halfWidth) + centerX;
    low[1] = centerY - (s32)(clip[1] * halfHeight);
    high[0] = (s32)(upper[0] * halfWidth) + centerX;
    high[1] = centerY - (s32)(upper[1] * halfHeight);
}

void RenderTrackInstance(s32 index, s32 vehicles, s32 deferred) {
    TrackInstance *instance = &g_LoadedTrack.instances[index];
    Gfx *displayList = instance->displayList;
    /* The list of the shadows is walked a word at a time. */
    u32 *out = (u32 *)g_pShadowDisplayListWritePtr;
    Vtx *vertex = g_pShadowVertexWritePtr;
    u32 *source, *lastLoadEnd;
    Gfx *saved;
    Vtx *from, *savedVertex;
    Vehicle *vehicle;
    f32 *matrix;
    /* The original kept room for a full load of 63 vertices. */
    u8 offTexture[64];
    u8 *flag;
    f32 lengthScale, widthScale, alongX, alongY, acrossX, acrossY, sum, scale, x, y, z;
    f32 m00, m10, m20, m30, m01, m11, m21, m31;
    s32 i, count, s, t;
    u32 w0;
    u8 bits;

    if (vehicles == 0 ||
        g_pActiveCamera == &g_pActiveCameraVehicle->cameras[VEHICLE_CAMERA_MIRROR] ||
        (instance->flags & TRACK_INSTANCE_NO_SHADOW)) {
        gSPTexture(gDisplayListPtr++, 0xFFFF, 0xFFFF, 2, G_TX_RENDERTILE, G_ON);
        gDPTileSync(gDisplayListPtr++);
        gSPDisplayList(gDisplayListPtr++, displayList);
        g_dwStatTrianglesDrawn += instance->triangleCount;
        g_dwStatVerticesDrawn += instance->vertexCount;
        g_dwStatPolygonsDrawn += instance->polygonCount;
    } else {
        AddTimingMarker(0, 0x50, 0xFA, 0x50, 0xFF);
        gSPTexture(gDisplayListPtr++, 0xFFFF, 0xFFFF, 2, G_TX_RENDERTILE, G_ON);
        gDPTileSync(gDisplayListPtr++);
        gDPSetPrimColor(gDisplayListPtr++, PRIM_MIN_LEVEL, 0, 0xFF, 0, 0, 0xFF);
        gSPDisplayList(gDisplayListPtr++, displayList);

        /* The shadows: the particle texture, clamped and doubled, in the colour of the fog. */
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
            SetScissorRect(g_aViewportRects[g_dwCurrentSplitScreenIndex].x,
                           g_aViewportRects[g_dwCurrentSplitScreenIndex].y,
                           g_aViewportRects[g_dwCurrentSplitScreenIndex].width,
                           g_aViewportRects[g_dwCurrentSplitScreenIndex].height);
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
            if ((g_pActiveCamera == &g_pActiveCameraVehicle->cameras[VEHICLE_CAMERA_COCKPIT] ||
                 g_pActiveCamera == &g_pActiveCameraVehicle->cameras[VEHICLE_CAMERA_BUMPER]) &&
                i == g_pActiveCameraVehicle->index) {
                continue;
            }
            vehicle = &g_aVehicles[i];
            gDPPipeSync((Gfx *)out);
            out += 2;
            gDPSetEnvColor(
                (Gfx *)out, g_bFogR, g_bFogG, g_bFogB, FloatToInt(vehicle->fogFactor * 255.0f));
            out += 2;

            /* The texture lies along the vehicle on the ground, centred under it. */
            g_vShadowForward = vehicle->frame.forward;
            if (g_vShadowForward.x == 0.0f && g_vShadowForward.y == 0.0f) {
                g_vShadowForward = vehicle->frame.up;
            }
            g_vShadowForward.z = 0.0f;
            lengthScale = Vec2fLength((Vec2f *)&g_vShadowForward);
            if (lengthScale < 0.5f) {
                lengthScale = 0.5f;
            }
            widthScale = Vec2fLength((Vec2f *)&vehicle->frame.side);
            if (widthScale < 0.5f) {
                widthScale = 0.5f;
            }
            /* The first wheel of the model sizes the shadow. */
            lengthScale = SHADOW_LENGTH_SCALE / vehicle->model->wheels[0].x / lengthScale;
            widthScale = SHADOW_WIDTH_SCALE / vehicle->model->wheels[0].y / widthScale;
            Vec3fNormalize(&g_vShadowForward);
            alongX = g_vShadowForward.x * lengthScale;
            alongY = -g_vShadowForward.y * lengthScale;
            acrossX = g_vShadowForward.x * widthScale;
            acrossY = -g_vShadowForward.y * widthScale;
            memcpy(g_afMvpMatrix, instance->matrix, sizeof(g_afMvpMatrix));
            g_afMvpMatrix[3][0] -= vehicle->frame.position.x;
            g_afMvpMatrix[3][1] -= vehicle->frame.position.y;
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
            sum = g_afMvpMatrix[3][3] +
                  ((g_afMvpMatrix[0][3] + g_afMvpMatrix[1][3]) + g_afMvpMatrix[2][3]);
            scale = sum == 0.0f ? 1.0f : 1.0f / sum;
            g_afMvpMatrix[0][0] *= scale;
            g_afMvpMatrix[1][0] *= scale;
            g_afMvpMatrix[2][0] *= scale;
            g_afMvpMatrix[3][0] *= scale;
            g_afMvpMatrix[0][1] *= scale;
            g_afMvpMatrix[1][1] *= scale;
            g_afMvpMatrix[2][1] *= scale;
            g_afMvpMatrix[3][1] *= scale;

            if (!g_bShadowCullDisabled) {
                saved = gDisplayListPtr;
                gDisplayListPtr = (Gfx *)out;
                SetScissorRect(vehicle->shadowCorners[0][0],
                               vehicle->shadowCorners[1][1],
                               vehicle->shadowCorners[1][0] - vehicle->shadowCorners[0][0],
                               vehicle->shadowCorners[0][1] - vehicle->shadowCorners[1][1]);
                out = (u32 *)gDisplayListPtr;
                gDisplayListPtr = saved;
            }

            /* Copy the loads and the triangles of the model, with the vertices on the texture. */
            source = (u32 *)displayList;
            while (out - (u32 *)g_pShadowDisplayListStart < SHADOW_LIST_WORDS) {
                w0 = source[0];
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
                    if (out == lastLoadEnd) {
                        vertex = savedVertex;
                        out -= 2;
                    }
                    *out++ = w0;
                    source++;
                    count = (w0 >> 10) & 0x3F;
                    if ((vertex + count) - g_pShadowVertexStart > SHADOW_VERTICES) {
                        /* The first word of the load is stepped over and back. */
                        out--;
                        goto nextVehicle;
                    }
                    savedVertex = vertex;
                    if (count > MAX_LOADED_VERTICES) {
                        SetDebugMessage("BAD VTX DL");
                    }
                    from = (Vtx *)*source++;
                    *out++ = (u32)vertex;
                    lastLoadEnd = out;
                    for (flag = offTexture; count-- != 0; vertex++, from++) {
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
                        *flag++ = bits;
                    }
                } else if ((w0 >> 24) == G_TRI2) {
                    if (g_bShadowCullDisabled) {
                        out[0] = w0;
                        out[1] = source[1];
                        out += 2;
                        source += 2;
                    } else if (TriangleOffTexture(offTexture, w0)) {
                        if (!TriangleOffTexture(offTexture, source[1])) {
                            out[0] = _SHIFTL(G_TRI1, 24, 8);
                            out[1] = source[1] & 0xFFFFFF;
                            out += 2;
                        }
                        source += 2;
                    } else if (TriangleOffTexture(offTexture, source[1])) {
                        out[0] = _SHIFTL(G_TRI1, 24, 8);
                        out[1] = source[0] & 0xFFFFFF;
                        out += 2;
                        source += 2;
                    } else {
                        out[0] = w0;
                        out[1] = source[1];
                        out += 2;
                        source += 2;
                    }
                } else if ((w0 >> 24) == G_ENDDL) {
                    break;
                } else if ((w0 >> 24) == G_TRI1) {
                    if (g_bShadowCullDisabled || !TriangleOffTexture(offTexture, source[1])) {
                        out[0] = w0;
                        out[1] = source[1];
                        out += 2;
                    }
                    source += 2;
                } else {
                    source += 2;
                }
            }
            if (out == lastLoadEnd) {
                vertex = savedVertex;
                out -= 2;
            }
        nextVehicle:;
        }
        gSPEndDisplayList((Gfx *)out);
        out += 2;
        AddTimingMarker(0, 0xFF, 0x80, 0x80, 0xFF);
    }
    g_pShadowVertexWritePtr = vertex;
    g_pShadowDisplayListWritePtr = (Gfx *)out;
}

s32 CompareVisibleCellDistance(const void *a, const void *b) {
    s16 second = ((const VisibleCell *)b)->distance;
    s16 first = ((const VisibleCell *)a)->distance;

    if (second < first) {
        return 1;
    }
    if (first < second) {
        return -1;
    }
    return 0;
}

void BuildVisibleInstanceList(void) {
    VisibleCell *cell;
    TrackGridCursor cursor;
    Vehicle *vehicle;
    Vec3f point;
    f32 *position;
    f32 dot, forwardX, forwardY;
    s32 count, row, column, cameraColumn, cameraRow, farDistance, midDistance, i, j, bit;
    s32 columnDistance, rowDistance;
    u32 found;
    u16 index;

    g_bVisibleCellListOverflow = FALSE;
    count = 0;
    for (row = g_dwVisibleGridMinRow; row <= g_dwVisibleGridMaxRow; row++) {
        for (column = g_adwVisibleGridRowMinCol[row]; column <= g_adwVisibleGridRowMaxCol[row];
             column++) {
            if (count == VISIBLE_CELLS) {
                g_bVisibleCellListOverflow = TRUE;
                goto terminate;
            }
            g_aVisibleCellTuples[count].column = column;
            g_aVisibleCellTuples[count].row = row;
            count++;
        }
    }
terminate:
    g_aVisibleCellTuples[count].column = 0xFF;
    g_aVisibleCellTuples[count].row = 0xFF;

    /* The cell of the camera, kept on the grid at its far edges. */
    cameraColumn = (s32)(g_pActiveCamera->position.x * (1.0f / GRID_CELL_SIZE));
    if (cameraColumn == GRID_CELLS) {
        cameraColumn--;
    }
    cameraRow = (s32)(g_pActiveCamera->position.y * (1.0f / GRID_CELL_SIZE));
    if (cameraRow == GRID_CELLS) {
        cameraRow--;
    }
    for (i = 0; i < count; i++) {
        columnDistance = g_aVisibleCellTuples[i].column - cameraColumn;
        rowDistance = g_aVisibleCellTuples[i].row - cameraRow;
        g_aVisibleCellTuples[i].distance =
            columnDistance * columnDistance + rowDistance * rowDistance;
    }
    QuickSort(g_aVisibleCellTuples, count, sizeof(VisibleCell), CompareVisibleCellDistance);

    MemFill64Aligned(g_abInstanceVisibilityMask, g_LoadedTrack.instanceCount, -1);
    g_nVisibleInstanceCount = 0;
    g_nVisibleCellIdxNear = -1;
    g_nVisibleCellIdxFar = -1;
    g_nVisibleCellIdxMid = -1;
    farDistance = (s32)(gFrustumFar / GRID_CELL_SIZE);
    farDistance -= 3;
    farDistance *= farDistance;
    if (g_pActiveCameraVehicle->input->playbackBuffer != NULL &&
        g_pActiveCamera == &g_pActiveCameraVehicle->cameras[VEHICLE_CAMERA_TRACKSIDE]) {
        midDistance = NEAR_CELL_DISTANCE;
    } else {
        midDistance = 1;
    }

    /* The models of the cells, the nearest cells first, each model once. */
    for (cell = g_aVisibleCellTuples; cell->column != 0xFF; cell++) {
        if (cell->distance >= NEAR_CELL_DISTANCE && g_nVisibleCellIdxNear == -1) {
            g_nVisibleCellIdxNear = g_nVisibleInstanceCount;
        }
        if (midDistance < cell->distance && g_nVisibleCellIdxMid == -1) {
            g_nVisibleCellIdxMid = g_nVisibleInstanceCount;
        }
        if (farDistance < cell->distance && g_nVisibleCellIdxFar == -1) {
            g_nVisibleCellIdxFar = g_nVisibleInstanceCount;
        }
        /* The model an animation flies is listed with every cell while it is in view. */
        /* The original takes the address of the position, and only the call reads it. */
        if (g_dwAirplaneInstance != 0 && g_abInstanceVisibilityMask[g_dwAirplaneInstance] != 0 &&
            (position = g_LoadedTrack.instances[g_dwAirplaneInstance].matrix[3],
             IsPointVisible(position[0], position[1]))) {
            g_awVisibleInstanceList[g_nVisibleInstanceCount++] = g_dwAirplaneInstance;
            g_abInstanceVisibilityMask[g_dwAirplaneInstance] = 0;
        }
        found = LookupInstanceGridCell(cell->column, cell->row);
        cursor.start = found;
        cursor.count = found >> 16;
        if (found != 0) {
            while ((index = NextGridCellInstance(&cursor)) != 0) {
                if (g_abInstanceVisibilityMask[index] != 0 && g_dwAirplaneInstance != index) {
                    g_awVisibleInstanceList[g_nVisibleInstanceCount++] = index;
                    g_abInstanceVisibilityMask[index] = 0;
                }
            }
        }
    }

    /* The vehicles in view ahead of the camera, the nearest first, cast shadows. */
    g_nNearbyVehicleCount = 0;
    for (i = 0; i < g_dwNumActiveVehicles; i++) {
        if (!g_adwVehicleVisible[i]) {
            continue;
        }
        Vec3fSubtract(
            &g_vVehicleVisibilityDelta, &g_aVehicles[i].frame.position, &g_pActiveCamera->position);
        dot = Vec3fDot(&g_pActiveCamera->forward, &g_vVehicleVisibilityDelta);
        if (dot < 2.0f) {
            continue;
        }
        for (j = g_nNearbyVehicleCount - 1; j >= 0 && dot < g_afNearbyVehicleDot[j]; j--) {
            g_afNearbyVehicleDot[j + 1] = g_afNearbyVehicleDot[j];
            g_anNearbyVehicleIdx[j + 1] = g_anNearbyVehicleIdx[j];
        }
        g_afNearbyVehicleDot[j + 1] = dot;
        g_anNearbyVehicleIdx[j + 1] = i;
        g_nNearbyVehicleCount++;
    }
    if (g_nNearbyVehicleCount >= 5) {
        g_nNearbyVehicleCount = 4;
    }
    for (i = 0; i < g_nNearbyVehicleCount; i++) {
        vehicle = &g_aVehicles[g_anNearbyVehicleIdx[i]];
        bit = 1 << g_anNearbyVehicleIdx[i];
        for (j = 0; j < vehicle->nearbyInstanceCount; j++) {
            g_abInstanceVisibilityMask[vehicle->nearbyInstances[j]] |= bit;
        }
    }

    g_wActiveInstanceHideMask = 0;
    for (i = 0; i < g_dwCameraRayHitCount; i++) {
        g_wActiveInstanceHideMask |=
            g_LoadedTrack.instances[g_awCameraRayHitInstances[i]].hidesGroups;
    }

    /* The reflections look along the camera on the ground, which must not be straight down. */
    forwardX = g_pActiveCamera->forward.x;
    forwardY = g_pActiveCamera->forward.y;
    if (forwardX == 0.0 && forwardY == 0.0) {
        forwardX = 0.0001f;
    }
    g_pTrackLookAt = AllocLightBlock();
    guLookAtReflectF(g_afMvpMatrix,
                     g_pTrackLookAt,
                     forwardX * 50.0f,
                     forwardY * 50.0f,
                     0.0f,
                     0.0f,
                     0.0f,
                     0.0f,
                     0.0f,
                     0.0f,
                     1.0f);

    /* At night the light of the track comes from the headlights of the car in view. */
    if (g_dwNightMode) {
        g_vTrackLightDirection.x =
            g_pActiveCameraVehicle->frame.forward.x * -4.0f + g_pActiveCameraVehicle->frame.up.x;
        g_vTrackLightDirection.y =
            g_pActiveCameraVehicle->frame.forward.y * -4.0f + g_pActiveCameraVehicle->frame.up.y;
        g_vTrackLightDirection.z =
            g_pActiveCameraVehicle->frame.forward.z * -4.0f + g_pActiveCameraVehicle->frame.up.z;
    } else {
        g_vTrackLightDirection.x = g_afLightDirection[0];
        g_vTrackLightDirection.y = g_afLightDirection[1];
        g_vTrackLightDirection.z = g_afLightDirection[2];
    }
    Vec3fNormalize(&g_vTrackLightDirection);
    Vec3fScaleInPlace(&g_vTrackLightDirection, 120.0f);
    g_nTrackLightBufferIndex = (g_nTrackLightBufferIndex + 1) % 4;
    g_aTrackLights[g_nTrackLightBufferIndex] = g_DefaultLights;
    g_aTrackLights[g_nTrackLightBufferIndex].l[0].l.dir[0] = (s32)g_vTrackLightDirection.x;
    g_aTrackLights[g_nTrackLightBufferIndex].l[0].l.dir[1] = (s32)g_vTrackLightDirection.y;
    g_aTrackLights[g_nTrackLightBufferIndex].l[0].l.dir[2] = (s32)g_vTrackLightDirection.z;

    /* The rectangle of the screen the shadow of each vehicle covers, kept to the view. */
    for (i = 0; i < g_dwNumVehicles; i++) {
        vehicle = g_aRacerSlots[i].vehicle;
        if (vehicle == NULL) {
            continue;
        }
        point.x = vehicle->frame.position.x;
        point.y = vehicle->frame.position.y;
        point.z = vehicle->frame.position.z - vehicle->groundDistance;
        ProjectWorldToScreenRect(&point, 8, vehicle->shadowCorners[0], vehicle->shadowCorners[1]);
        /* The view is indexed afresh for each bound, and read again after each store. */
        if (vehicle->shadowCorners[0][0] < g_aViewportRects[g_dwCurrentSplitScreenIndex].x) {
            vehicle->shadowCorners[0][0] = g_aViewportRects[g_dwCurrentSplitScreenIndex].x;
        }
        if (vehicle->shadowCorners[1][0] < g_aViewportRects[g_dwCurrentSplitScreenIndex].x) {
            vehicle->shadowCorners[1][0] = g_aViewportRects[g_dwCurrentSplitScreenIndex].x;
        }
        if (vehicle->shadowCorners[0][1] < g_aViewportRects[g_dwCurrentSplitScreenIndex].y) {
            vehicle->shadowCorners[0][1] = g_aViewportRects[g_dwCurrentSplitScreenIndex].y;
        }
        if (vehicle->shadowCorners[1][1] < g_aViewportRects[g_dwCurrentSplitScreenIndex].y) {
            vehicle->shadowCorners[1][1] = g_aViewportRects[g_dwCurrentSplitScreenIndex].y;
        }
        if (g_aViewportRects[g_dwCurrentSplitScreenIndex].x +
                g_aViewportRects[g_dwCurrentSplitScreenIndex].width <
            vehicle->shadowCorners[0][0]) {
            vehicle->shadowCorners[0][0] = g_aViewportRects[g_dwCurrentSplitScreenIndex].x +
                                           g_aViewportRects[g_dwCurrentSplitScreenIndex].width;
        }
        if (g_aViewportRects[g_dwCurrentSplitScreenIndex].x +
                g_aViewportRects[g_dwCurrentSplitScreenIndex].width <
            vehicle->shadowCorners[1][0]) {
            vehicle->shadowCorners[1][0] = g_aViewportRects[g_dwCurrentSplitScreenIndex].x +
                                           g_aViewportRects[g_dwCurrentSplitScreenIndex].width;
        }
        if (g_aViewportRects[g_dwCurrentSplitScreenIndex].y +
                g_aViewportRects[g_dwCurrentSplitScreenIndex].height <
            vehicle->shadowCorners[0][1]) {
            vehicle->shadowCorners[0][1] = g_aViewportRects[g_dwCurrentSplitScreenIndex].y +
                                           g_aViewportRects[g_dwCurrentSplitScreenIndex].height;
        }
        if (g_aViewportRects[g_dwCurrentSplitScreenIndex].y +
                g_aViewportRects[g_dwCurrentSplitScreenIndex].height <
            vehicle->shadowCorners[1][1]) {
            vehicle->shadowCorners[1][1] = g_aViewportRects[g_dwCurrentSplitScreenIndex].y +
                                           g_aViewportRects[g_dwCurrentSplitScreenIndex].height;
        }
    }
}

/**
 * Load the product of the model view and the projection of a placed model whose matrix holds only
 * its position and a scale: the view and projection scaled by it, put at its position, and scaled
 * again into the range of fixed point.
 */
static inline void ForceUnrotatedMatrix(TrackInstance *instance, s32 index, s32 listIndex) {
    f32(*vp)[4] = g_afViewProjMatrix;
    f32(*mvp)[4] = g_afMvpMatrix;
    f32(*m)[4] = instance->matrix;
    Mtx *block;
    f32 divisor, scale, high, low;
    s32 row, column;

    divisor = m[0][0] * g_fViewMatrixMaxAbs + 0.375f;
    if (divisor == 0.0f) {
        divisor = m[0][0] * (1.1 * g_fViewMatrixMaxAbs) + 0.375;
    }
    scale = 1.999755859375f / divisor;
    for (column = 0; column < 4; column++) {
        mvp[3][column] =
            (((m[3][0] * vp[0][column] + m[3][1] * vp[1][column]) + m[3][2] * vp[2][column]) +
             vp[3][column] * m[3][3]) *
            scale;
    }
    scale *= m[0][0];
    for (row = 0; row < 3; row++) {
        for (column = 0; column < 4; column++) {
            mvp[row][column] = vp[row][column] * scale;
        }
    }
    block = (Mtx *)AllocDisplayListBlock();

    high = 0.0f;
    low = 0.0f;
    for (column = 0; column < 4; column++) {
        if (high <= mvp[3][column]) {
            high = mvp[3][column];
        }
        if (mvp[3][column] <= low) {
            low = mvp[3][column];
        }
    }
    if (FIXED_MATRIX_MAX < (f64)high || (f64)low < -FIXED_MATRIX_MAX) {
#if TGR_REGION != TGR_REGION_JP
        /* Only the second model in view reports its overflow. */
        if (listIndex == 1) {
            osSyncPrintf("Bad Final Matrix: in=%d (%s), s=%f\n-------------\n"
                         "%f %f %f %f\n%f %f %f %f\n%f %f %f %f\n%f %f %f %f\n-------------\n"
                         "%f %f %f %f\n%f %f %f %f\n%f %f %f %f\n%f %f %f %f\n-------------\n"
                         "%f %f %f %f\n%f %f %f %f\n%f %f %f %f\n%f %f %f %f\n-------------\n",
                         index,
                         g_LoadedTrack.instanceNames != NULL ? g_LoadedTrack.instanceNames[index] :
                                                               "???",
                         scale,
                         m[0][0],
                         m[0][1],
                         m[0][2],
                         m[0][3],
                         m[1][0],
                         m[1][1],
                         m[1][2],
                         m[1][3],
                         m[2][0],
                         m[2][1],
                         m[2][2],
                         m[2][3],
                         m[3][0],
                         m[3][1],
                         m[3][2],
                         m[3][3],
                         vp[0][0],
                         vp[0][1],
                         vp[0][2],
                         vp[0][3],
                         vp[1][0],
                         vp[1][1],
                         vp[1][2],
                         vp[1][3],
                         vp[2][0],
                         vp[2][1],
                         vp[2][2],
                         vp[2][3],
                         vp[3][0],
                         vp[3][1],
                         vp[3][2],
                         vp[3][3],
                         mvp[0][0],
                         mvp[0][1],
                         mvp[0][2],
                         mvp[0][3],
                         mvp[1][0],
                         mvp[1][1],
                         mvp[1][2],
                         mvp[1][3],
                         mvp[2][0],
                         mvp[2][1],
                         mvp[2][2],
                         mvp[2][3],
                         mvp[3][0],
                         mvp[3][1],
                         mvp[3][2],
                         mvp[3][3]);
        }
#endif
        scale = -low < high ? FIXED_MATRIX_MAX / high : -FIXED_MATRIX_MAX / low;
        for (row = 0; row < 4; row++) {
            for (column = 0; column < 4; column++) {
                mvp[row][column] *= scale;
            }
        }
    }
    guMtxF2L(g_afMvpMatrix, block);
    gSPForceMatrix(gDisplayListPtr++, block);
}

void RenderTrackGeometry(s32 deferred) {
    TrackInstance *instance;
    Mtx *block;
    u32 identity;
    s32 clear, deferredLast, opaqueCount, matrixLoaded, i, index;

    clear = !g_dwNightMode && !g_dwRainMode;
    matrixLoaded = FALSE;
    if (!deferred) {
        BuildVisibleInstanceList();
        g_nDeferredInstanceLastIdx = -1;
        deferredLast = -1;
        g_nOpaqueVisibleInstanceCount = opaqueCount = g_nVisibleInstanceCount;
        g_fViewMatrixMaxAbs = GetMaxAbsValue12(g_afViewProjMatrix);
        g_dwInstanceHideFlags = 0;
        if (clear) {
            g_dwInstanceHideFlags = TRACK_INSTANCE_DARK_OR_WET;
        }
        if (g_dwGameMode != GAME_MODE_ARCADE &&
            (g_dwGameMode != GAME_MODE_AFTER_SEASON || g_pActiveProfile->season != 0)) {
            g_dwInstanceHideFlags |= TRACK_INSTANCE_ARCADE;
        }
        if (g_dwWeatherActive && (g_dwSnowMode || g_dwRainMode || g_dwNightMode) &&
            g_dwSelectedTrackId != 2 && g_dwSelectedTrackId != 7) {
            g_dwInstanceHideFlags |= TRACK_INSTANCE_CLEAR_SKY;
        }
        g_nTrackTransparentStartIdxFar = TRANSPARENT_START_UNKNOWN;
        g_nTrackTransparentStartIdxNear = TRANSPARENT_START_UNKNOWN;
        g_nTrackTransparentStartIdx = TRANSPARENT_START_UNKNOWN;
    }
    AddTimingMarker(0, 0xFF, 0x80, 0x80, 0xFF);
    /* The blender mixes in the fog in fog, and passes the colour through otherwise. */
    if (g_dwWeatherActive) {
        g_dwTrackOtherModeLBlend = GBL_c1(G_BL_CLR_FOG, G_BL_A_SHADE, G_BL_CLR_IN, G_BL_1MA);
    } else {
        g_dwTrackOtherModeLBlend = GBL_c1(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1);
    }
    g_dwTrackOtherModeLFlags = AA_EN | Z_CMP | Z_UPD | CVG_DST_CLAMP | ZMODE_OPA | ALPHA_CVG_SEL |
                               GBL_c2(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM);

    identity = PHYSICAL(&g_mtxIdentity);
    gSPMatrix(gDisplayListPtr++,
              PHYSICAL(g_pProjectionMtx),
              G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
    gSPMatrix(gDisplayListPtr++, identity, G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_PUSH);
    gSPPerspNormalize(gDisplayListPtr++, g_wPerspNorm);
    gSPLookAt(gDisplayListPtr++, g_pTrackLookAt);
    gSPNumLights(gDisplayListPtr++, NUMLIGHTS_1);
    gSPLight(gDisplayListPtr++, &g_aTrackLights[g_nTrackLightBufferIndex].l[0], 1);
    gSPLight(gDisplayListPtr++, &g_aTrackLights[g_nTrackLightBufferIndex].a, 2);
    gSPLightColor(gDisplayListPtr++, LIGHT_1, g_dwDirLightColor);
    gSPLightColor(gDisplayListPtr++, LIGHT_2, g_dwAmbientLightColor);
    gDPPipeSync(gDisplayListPtr++);
    gDPSetTexturePersp(gDisplayListPtr++, G_TP_PERSP);
    gDPSetTextureConvert(gDisplayListPtr++, G_TC_FILT);
    gDPSetCombineKey(gDisplayListPtr++, G_CK_NONE);
    gDPSetAlphaCompare(gDisplayListPtr++, G_AC_THRESHOLD);
    gDPSetColorDither(gDisplayListPtr++, g_dwColorDitherMode);
    gDPSetAlphaDither(gDisplayListPtr++, g_dwAlphaDitherMode);
    gDPSetCycleType(gDisplayListPtr++, G_CYC_1CYCLE);
    gDPSetBlendColor(gDisplayListPtr++, 0, 0, 0, 0);
    gDPSetCycleType(gDisplayListPtr++, G_CYC_2CYCLE);
    gDPSetRenderMode(gDisplayListPtr++, g_dwTrackOtherModeLBlend, g_dwTrackOtherModeLFlags);
    SetTrackCombiner(gDisplayListPtr++);
    SetTrackTextureModes();
    gSPSegment(gDisplayListPtr++, 0, 0);
    gSPClearGeometryMode(gDisplayListPtr++,
                         G_CLIPPING | G_TEXTURE_GEN | G_FOG | G_CULL_BACK | G_CULL_FRONT |
                             G_SHADING_SMOOTH);
    gSPSetGeometryMode(gDisplayListPtr++, TrackGeometryMode());
    RestoreCurrentViewport();
    LoadTrackMatricesAndLights(identity);
    /* The fog is set up twice over. */
    SetupFog();
    SetupFog();
    gDPSetBlendColor(gDisplayListPtr++, 0, 0, 0, 0);
    gSPClearGeometryMode(gDisplayListPtr++,
                         G_TEXTURE_GEN | G_FOG | G_CULL_BACK | G_CULL_FRONT | G_SHADING_SMOOTH);
    gSPSetGeometryMode(gDisplayListPtr++, TrackGeometryMode());
    gDPPipeSync(gDisplayListPtr++);
    gDPSetCycleType(gDisplayListPtr++, G_CYC_2CYCLE);
    gDPSetRenderMode(gDisplayListPtr++, g_dwTrackOtherModeLBlend, g_dwTrackOtherModeLFlags);
    SetTrackCombiner(gDisplayListPtr++);
    gDPSetPrimColor(gDisplayListPtr++, PRIM_MIN_LEVEL, 0, 0xFF, 0, 0, 0xFF);
    SetTrackTextureModes();
    SetTrackClipRatio();
    gSPTexture(gDisplayListPtr++, 0xFFFF, 0xFFFF, 2, G_TX_RENDERTILE, G_ON);
    gSPClearGeometryMode(gDisplayListPtr++, G_TEXTURE_GEN | G_TEXTURE_GEN_LINEAR);
    gDPTileSync(gDisplayListPtr++);
    gDPSetTile(
        gDisplayListPtr++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 0, 0, G_TX_LOADTILE, 0, 0, 0, 0, 0, 0, 0);
    gDPSetTile(gDisplayListPtr++, G_IM_FMT_RGBA, G_IM_SIZ_4b, 0, 0x1F0, 6, 0, 0, 0, 0, 0, 0, 0);
    gDPSetTile(gDisplayListPtr++, G_IM_FMT_RGBA, G_IM_SIZ_4b, 0, 0x100, 5, 0, 0, 0, 0, 0, 0, 0);

    if (deferred) {
        deferredLast = g_nDeferredInstanceLastIdx;
        i = opaqueCount = g_nOpaqueVisibleInstanceCount;
        g_nVisibleInstanceCount = opaqueCount + deferredLast + 1;
    } else {
        StepDebugHiddenInstance();
        i = 0;
    }
    for (; i < g_nVisibleInstanceCount; i++) {
        /* The near and far cells set the clipping again, and from the far ones on it clips. */
        if (i == g_nVisibleCellIdxNear || i == g_nVisibleCellIdxFar) {
            SetTrackClipRatio();
            if (i == g_nVisibleCellIdxFar) {
                gSPSetGeometryMode(gDisplayListPtr++, G_CLIPPING);
            }
        }
        if (i < opaqueCount) {
            index = g_awVisibleInstanceList[i];
            instance = &g_LoadedTrack.instances[index];
            if (instance->groups & g_wActiveInstanceHideMask) {
                continue;
            }
            if (g_bDebugHideInstance && g_nDebugHiddenInstanceIndex == index) {
                continue;
            }
            if (instance->flags & g_dwInstanceHideFlags) {
                continue;
            }
            /* The translucent models wait for the second pass, at the start of the list. */
            if (instance->flags & TRACK_INSTANCE_TRANSLUCENT) {
                if (g_nTrackTransparentStartIdx == TRANSPARENT_START_UNKNOWN &&
                    g_nVisibleCellIdxMid < i) {
                    g_nTrackTransparentStartIdx = deferredLast + 1;
                }
                g_awVisibleInstanceList[++deferredLast] = index;
                continue;
            }
        } else {
            /* The second pass draws the translucent models the farthest first. */
            index = g_awVisibleInstanceList[deferredLast - i + opaqueCount];
            instance = &g_LoadedTrack.instances[index];
        }

        if (instance->flags & TRACK_INSTANCE_UNROTATED) {
            if (!matrixLoaded) {
                gSPMatrix(gDisplayListPtr++, identity, G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
                matrixLoaded = TRUE;
            }
            ForceUnrotatedMatrix(instance, index, i);
        } else {
            matrixLoaded = FALSE;
            block = (Mtx *)AllocDisplayListBlock();
            guMtxF2L(instance->matrix, block);
            gSPMatrix(
                gDisplayListPtr++, PHYSICAL(block), G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
        }

        /* The flags are read again after each command written. */
        if (instance->flags & TRACK_INSTANCE_MODE_FLAGS) {
            if (instance->flags & TRACK_INSTANCE_RAMPED) {
                if (g_dwNightMode && (instance->flags & TRACK_INSTANCE_NIGHT_LIT)) {
                    gSPLightColor(
                        gDisplayListPtr++, LIGHT_1, (g_dwDirLightColor >> 1) & 0x7F7F7F00);
                } else {
                    gSPLightColor(gDisplayListPtr++, LIGHT_1, 0);
                }
                gSPLightColor(gDisplayListPtr++,
                              LIGHT_2,
                              g_adwLightRampColors[instance->flags & TRACK_INSTANCE_LIGHT_RAMP]);
            }
            if (instance->flags & TRACK_INSTANCE_TWO_SIDED) {
                gSPClearGeometryMode(gDisplayListPtr++, G_CULL_BACK | G_CULL_FRONT);
            }
            if ((instance->flags & TRACK_INSTANCE_CLEAR_SKY) && g_dwWeatherActive &&
                !g_dwNightMode && !g_dwSnowMode && !g_dwRainMode) {
                gSPClearGeometryMode(gDisplayListPtr++, G_FOG);
            }
            if ((instance->flags & TRACK_INSTANCE_FLAT) && g_bSmoothShading) {
                gSPClearGeometryMode(gDisplayListPtr++, G_SHADING_SMOOTH);
            }
        }

        if (g_nVisibleCellIdxMid < i && i < g_nTrackTransparentStartIdx) {
            DrawPlainInstance(instance);
        } else {
            RenderTrackInstance(index, g_abInstanceVisibilityMask[index], deferred);
        }

        if (instance->flags & TRACK_INSTANCE_MODE_FLAGS) {
            if ((instance->flags & TRACK_INSTANCE_FLAT) && g_bSmoothShading) {
                gSPSetGeometryMode(gDisplayListPtr++, G_SHADING_SMOOTH);
            }
            if (instance->flags & TRACK_INSTANCE_RAMPED) {
                gSPLightColor(gDisplayListPtr++, LIGHT_1, g_dwDirLightColor);
                gSPLightColor(gDisplayListPtr++, LIGHT_2, g_dwAmbientLightColor);
            }
            if (instance->flags & TRACK_INSTANCE_TWO_SIDED) {
                gSPSetGeometryMode(gDisplayListPtr++,
                                   g_dwTrackMirrored != g_dwViewportMirrored ? G_CULL_FRONT :
                                                                               G_CULL_BACK);
            }
            if (g_dwWeatherActive && !g_dwNightMode && !g_dwSnowMode && !g_dwRainMode &&
                (instance->flags & TRACK_INSTANCE_CLEAR_SKY)) {
                gSPSetGeometryMode(gDisplayListPtr++, G_FOG);
            }
        }
    }

    /* After the first pass, where the second starts the translucent models past the middle. */
    if (!deferred) {
        if (g_nTrackTransparentStartIdx == TRANSPARENT_START_UNKNOWN) {
            g_nTrackTransparentStartIdx = deferredLast;
        }
        g_nTrackTransparentStartIdx = deferredLast - g_nTrackTransparentStartIdx + opaqueCount;
    }
    gSPMatrix(gDisplayListPtr++, identity, G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
    gSPClearGeometryMode(gDisplayListPtr++, G_FOG);
    gDPPipeSync(gDisplayListPtr++);
    AddTimingMarker(0, 0, 0xB4, 0, 0xFF);
    g_nDeferredInstanceLastIdx = deferredLast;
}
