#ifndef COASTLINE_PLATFORM_WIN_GBICOMMAND_H
#define COASTLINE_PLATFORM_WIN_GBICOMMAND_H

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

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_PLATFORM_WIN_GBICOMMAND_H */
