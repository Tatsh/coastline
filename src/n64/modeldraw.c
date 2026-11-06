#include <PR/gbi.h>
#include <PR/gu.h>

#include "common/model.h"
#include "common/render.h"
#include "common/system.h"
#include "common/util.h"

void DrawModelList(Model *model, float m[4][4]) {
    LookAt *lookAt;
    Mtx *matrixBlock;
    unsigned int i;

    gDPPipeSync(gDisplayListPtr++);
    gDPSetCycleType(gDisplayListPtr++, 1 << G_MDSFT_CYCLETYPE);
    if (g_dwDrawModelListNoZBuffer != 0) {
        gDPSetRenderMode(gDisplayListPtr++, 0x0C192008, 0);
        g_dwDrawModelListNoZBuffer = 0;
    } else {
        gDPSetRenderMode(gDisplayListPtr++, 0x0C192038, 0);
    }
    gDisplayListWords(gDisplayListPtr++, 0xfc26a004, 0x1ffc93f8);
    gDPSetBlendColor(gDisplayListPtr++, 0, 0, 0, 0);
    gDPSetTextureDetail(gDisplayListPtr++, 0);
    gDPSetTextureLOD(gDisplayListPtr++, 0);
    gDPSetTextureLUT(gDisplayListPtr++, 0);
    gDPSetTextureFilter(gDisplayListPtr++, g_dwTextureFilterMode);
    gDPSetColorDither(gDisplayListPtr++, g_dwColorDitherMode);
    gDPSetAlphaDither(gDisplayListPtr++, g_dwAlphaDitherMode);
    gSPSetGeometryMode(gDisplayListPtr++, 1);
    gDPSetAlphaCompare(gDisplayListPtr++, 1);
    gDPSetTextureDetail(gDisplayListPtr++, 0);
    gDPSetTextureLOD(gDisplayListPtr++, 1 << G_MDSFT_TEXTLOD);
    gDPSetTextureLUT(gDisplayListPtr++, 0);
    gDPSetTextureFilter(gDisplayListPtr++, g_dwTextureFilterMode);
    {
        Gfx *command = (Gfx *)(gDisplayListPtr++);

        command->words.w0 = 0xb6000000;
        command->words.w1 = 0x853200;
    }
    gSPSetGeometryMode(gDisplayListPtr++,
                       (g_dwViewportMirrored - g_dwTrackMirrored ? 0x1000 : 0x2000) | 0xA0005 |
                           0x200);
    lookAt = AllocLightBlock();
    guLookAtReflectF(
        g_afScratchMatrix, lookAt, 0.0f, -1.0f, 15.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
    gDisplayListWords(gDisplayListPtr++, 0x03840010, GBI_WORD1(lookAt));
    gDisplayListWords(gDisplayListPtr++, 0x03820010, GBI_WORD1((char *)lookAt + 16));
    matrixBlock = (Mtx *)AllocDisplayListBlock();
    guMtxF2L(m, matrixBlock);
    gSPMatrix(gDisplayListPtr++, matrixBlock, G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_PUSH);
    gSPTexture(gDisplayListPtr++, 0xFFFF, 0xFFFF, 0, 0, 1);
    gSPClearGeometryMode(gDisplayListPtr++, 0xC0000);
    gDPTileSync(gDisplayListPtr++);
    gDisplayListWords(gDisplayListPtr++, 0xF5100000, 0x07000000);
    gDisplayListWords(gDisplayListPtr++, 0xF50001F0, 0x06000000);
    gDisplayListWords(gDisplayListPtr++, 0xF5000100, 0x05000000);
    {
        int pass;

        for (pass = 0; pass != 2; pass++) {
            for (i = 0; i < model->partCount; i++) {
                if (!(model->parts[i].flags & MODEL_PART_TRANSLUCENT) - !pass) {
                    continue;
                }
                if (model->parts[i].displayList != 0) {
                    if (model->parts[i].flags & MODEL_PART_RAMPED) {
                        {
                            Gfx *command = (Gfx *)(gDisplayListPtr++);

                            command->words.w0 = 0xbc00000a;
                            command->words.w1 = 0;
                        }
                        {
                            Gfx *command = (Gfx *)(gDisplayListPtr++);

                            command->words.w0 = 0xbc00040a;
                            command->words.w1 = 0;
                        }
                        {
                            Gfx *command = (Gfx *)(gDisplayListPtr++);

                            command->words.w0 = 0xbc00200a;
                            command->words.w1 = g_adwModelShadeColors[model->parts[i].flags &
                                                                      MODEL_PART_LIGHT_RAMP];
                        }
                        {
                            Gfx *command = (Gfx *)(gDisplayListPtr++);

                            command->words.w0 = 0xbc00240a;
                            command->words.w1 = g_adwModelShadeColors[model->parts[i].flags &
                                                                      MODEL_PART_LIGHT_RAMP];
                        }
                    }
                    if (model->parts[i].flags & MODEL_PART_TWO_SIDED) {
                        gSPClearGeometryMode(gDisplayListPtr++, 0x3000);
                    }
                    if ((model->parts[i].flags & MODEL_PART_FLAT) && g_bSmoothShading != 0) {
                        gSPClearGeometryMode(gDisplayListPtr++, 0x200);
                    }
                    gSPTexture(gDisplayListPtr++, 0xFFFF, 0xFFFF, 0, 0, 1);
                    gDPTileSync(gDisplayListPtr++);
                    matrixBlock = (Mtx *)AllocDisplayListBlock();
                    guTranslateF(g_afMvpMatrix,
                                 model->parts[i].position.x,
                                 model->parts[i].position.y,
                                 model->parts[i].position.z);
                    guMtxF2L(g_afMvpMatrix, matrixBlock);
                    gSPMatrix(
                        gDisplayListPtr++, matrixBlock, G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_PUSH);
                    gSPDisplayList(gDisplayListPtr++, model->parts[i].displayList);
                    gSPPopMatrix(gDisplayListPtr++, 0);
                    if ((model->parts[i].flags & MODEL_PART_FLAT) && g_bSmoothShading != 0) {
                        gSPSetGeometryMode(gDisplayListPtr++, 0x200);
                    }
                    if (model->parts[i].flags & MODEL_PART_TWO_SIDED) {
                        gSPSetGeometryMode(gDisplayListPtr++,
                                           g_dwViewportMirrored - g_dwTrackMirrored ? 0x1000 :
                                                                                      0x2000);
                    }
                    if (model->parts[i].flags & MODEL_PART_RAMPED) {
                        {
                            Gfx *command = (Gfx *)(gDisplayListPtr++);

                            command->words.w0 = 0xbc00000a;
                            command->words.w1 = 0xFFFFFF00;
                        }
                        {
                            Gfx *command = (Gfx *)(gDisplayListPtr++);

                            command->words.w0 = 0xbc00040a;
                            command->words.w1 = 0xFFFFFF00;
                        }
                        {
                            Gfx *command = (Gfx *)(gDisplayListPtr++);

                            command->words.w0 = 0xbc00200a;
                            command->words.w1 = 0x40404000;
                        }
                        {
                            Gfx *command = (Gfx *)(gDisplayListPtr++);

                            command->words.w0 = 0xbc00240a;
                            command->words.w1 = 0x40404000;
                        }
                    }
                }
            }
        }
    }
    gSPPopMatrix(gDisplayListPtr++, 0);
}
