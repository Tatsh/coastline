#ifndef COASTLINE_PLATFORM_WIN_GBICOMMAND_H
#define COASTLINE_PLATFORM_WIN_GBICOMMAND_H

#include <PR/gbi.h>

typedef struct GbiCommand GbiCommand;

typedef Vp GbiViewport;

struct ConvertedModelVertex;

typedef union ModelVertexData {
    Vtx *pSource;
    struct ConvertedModelVertex *pConverted;
} ModelVertexData;

/** A command of a display list of the game. */
struct GbiCommand {
    union {
        struct {
            unsigned w0; /*!< The first word, the opcode in its high byte. */
            union {
                const GbiViewport *pViewport; /*!< The viewport a command sets. */
                const void *pData;            /*!< The memory a command moves. */
                const Light *pLight; /*!< The light or look-at direction a command moves. */
                float (*pMatrix)[4]; /*!< The floating-point matrix a command loads. */
                unsigned w1;         /*!< The second word. */
                float f1;            /*!< The second word as a float. */
                GbiCommand *pTarget; /*!< The display list a branch or a call runs. */
                ModelVertexData vertexData;
            };
        };
        Gwords words;
    };
};

#endif /* COASTLINE_PLATFORM_WIN_GBICOMMAND_H */
