#ifndef COASTLINE_PLATFORM_WIN_GBICOMMAND_H
#define COASTLINE_PLATFORM_WIN_GBICOMMAND_H

#include <PR/gbi.h>

typedef struct GbiCommand GbiCommand;

/** A viewport of the console, in pixels with two bits of fraction. */
typedef struct GbiViewport {
    short anScale[4];     /*!< The scale of x, y, z, and an unused fourth value. */
    short anTranslate[4]; /*!< The translation of x, y, z, and an unused fourth value. */
} GbiViewport;

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
                float (*pMatrix)[4];          /*!< The floating-point matrix a command loads. */
                unsigned w1;                  /*!< The second word. */
                float f1;                     /*!< The second word as a float. */
                GbiCommand *pTarget;          /*!< The display list a branch or a call runs. */
                ModelVertexData vertexData;
            };
        };
        Gwords words;
    };
};

#endif
