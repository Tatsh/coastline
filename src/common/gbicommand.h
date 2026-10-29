#ifndef COASTLINE_GBICOMMAND_H
#define COASTLINE_GBICOMMAND_H

#include <PR/gbi.h>

#include "common/ptr32.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct GbiCommand GbiCommand;

typedef Vp GbiViewport;

/** A row of the floating-point matrix a command loads. */
typedef float GbiMatrixRow[4];

struct ConvertedModelVertex;

typedef union ModelVertexData {
    TGR_PTR(Vtx) pSource;
    TGR_PTR(struct ConvertedModelVertex) pConverted;
} ModelVertexData;

/** A command of a display list of the game. */
struct GbiCommand {
    union {
        struct {
            unsigned w0; /*!< The first word, the opcode in its high byte. */
            union {
                TGR_PTR(const GbiViewport) pViewport; /*!< The viewport a command sets. */
                TGR_PTR(const void) pData;            /*!< The memory a command moves. */
                TGR_PTR(const Light) pLight; /*!< The light or look-at direction a command moves. */
                TGR_PTR(GbiMatrixRow) pMatrix; /*!< The floating-point matrix a command loads. */
                unsigned w1;                   /*!< The second word. */
                float f1;                      /*!< The second word as a float. */
                TGR_PTR(GbiCommand) pTarget;   /*!< The display list a branch or a call runs. */
                ModelVertexData vertexData;
            };
        };
        Gwords words;
    };
};

/** The opcodes of the commands of the display lists that have handlers of their own. */
enum GbiOpcode {
    GBI_OP_MATRIX = 0x01,                    /*!< HandleGbiMatrix(). */
    GBI_OP_MOVE_MEM = 0x03,                  /*!< HandleGbiMoveMem(). */
    GBI_OP_VERTEX = 0x04,                    /*!< A handler of vertices. */
    GBI_OP_DISPLAY_LIST = 0x06,              /*!< HandleGbiDisplayList(). */
    GBI_OP_TRI2 = 0xb1,                      /*!< A handler of two triangles. */
    GBI_OP_CLEAR_GEOMETRY_MODE = 0xb6,       /*!< HandleGbiClearGeometryMode(). */
    GBI_OP_SET_GEOMETRY_MODE = 0xb7,         /*!< HandleGbiSetGeometryMode(). */
    GBI_OP_END_DISPLAY_LIST = 0xb8,          /*!< HandleGbiEndDisplayList(). */
    GBI_OP_SET_OTHER_MODE_L = 0xb9,          /*!< HandleGbiSetOtherModeL(). */
    GBI_OP_MOVE_WORD = 0xbc,                 /*!< HandleGbiMoveWord(). */
    GBI_OP_POP_MATRIX = 0xbd,                /*!< HandleGbiPopMatrix(). */
    GBI_OP_TRI1 = 0xbf,                      /*!< A handler of one triangle. */
    GBI_OP_BIND_TEXTURE_ENTRY = 0xdc,        /*!< HandleGbiBindTextureEntry(). */
    GBI_OP_LOAD_PREPARED_TEXTURE = 0xdd,     /*!< HandleGbiLoadPreparedTexture(). */
    GBI_OP_SET_TEX_RECT_W = 0xde,            /*!< HandleGbiSetTexRectW(). */
    GBI_OP_SET_TEX_RECT_DEPTH = 0xdf,        /*!< HandleGbiSetTexRectDepth(). */
    GBI_OP_FILL_RECTANGLE_INT_COORDS = 0xe1, /*!< HandleGbiFillRectangleIntCoords(). */
    GBI_OP_SET_SCISSOR_INT_COORDS = 0xe2,    /*!< HandleGbiSetScissorIntCoords(). */
    GBI_OP_TEXTURE_RECTANGLE = 0xe3,         /*!< HandleGbiTextureRectangle(). */
    GBI_OP_TEXTURE_RECTANGLE_NATIVE = 0xe4,  /*!< HandleGbiTextureRectangleNative(). */
    GBI_OP_SET_SCISSOR = 0xed,               /*!< HandleGbiSetScissor(). */
    GBI_OP_SET_TILE_SIZE = 0xf2,             /*!< HandleGbiSetTileSize(). */
    GBI_OP_FILL_RECTANGLE = 0xf6,            /*!< HandleGbiFillRectangle(). */
    GBI_OP_SET_FILL_COLOR = 0xf7,            /*!< HandleGbiSetFillColor(). */
    GBI_OP_SET_FOG_COLOR = 0xf8,             /*!< HandleGbiSetFogColor(). */
    GBI_OP_SET_PRIM_COLOR = 0xfa,            /*!< HandleGbiSetPrimColor(). */
    GBI_OP_SET_ENV_COLOR = 0xfb,             /*!< HandleGbiSetEnvColor(). */
    GBI_OP_SET_COMBINE = 0xfc,               /*!< HandleGbiSetCombine(). */
};

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_GBICOMMAND_H */
