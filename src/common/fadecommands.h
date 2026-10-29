#ifndef COASTLINE_FADE_COMMANDS_H
#define COASTLINE_FADE_COMMANDS_H

#include "common/render.h"

#if TGR_PC
#include "win32/combine.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WriteFadeCombineMode(secondCycle)                                                          \
    PackCombineMode(gDisplayListPtr++,                                                             \
                    0,                                                                             \
                    0,                                                                             \
                    0,                                                                             \
                    1000 + G_CCMUX_PRIMITIVE,                                                      \
                    0,                                                                             \
                    0,                                                                             \
                    0,                                                                             \
                    1000 + G_ACMUX_PRIMITIVE,                                                      \
                    0,                                                                             \
                    0,                                                                             \
                    0,                                                                             \
                    1000 + G_CCMUX_##secondCycle,                                                  \
                    0,                                                                             \
                    0,                                                                             \
                    0,                                                                             \
                    1000 + G_ACMUX_##secondCycle)

#ifdef __cplusplus
}
#endif

#else
#define WriteFadeCombineMode(secondCycle)                                                          \
    gDPSetCombineLERP(gDisplayListPtr++,                                                           \
                      0,                                                                           \
                      0,                                                                           \
                      0,                                                                           \
                      PRIMITIVE,                                                                   \
                      0,                                                                           \
                      0,                                                                           \
                      0,                                                                           \
                      PRIMITIVE,                                                                   \
                      0,                                                                           \
                      0,                                                                           \
                      0,                                                                           \
                      secondCycle,                                                                 \
                      0,                                                                           \
                      0,                                                                           \
                      0,                                                                           \
                      secondCycle)
#endif /* TGR_PC */

#endif /* COASTLINE_FADE_COMMANDS_H */
