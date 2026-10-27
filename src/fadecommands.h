#ifndef COASTLINE_FADE_COMMANDS_H
#define COASTLINE_FADE_COMMANDS_H

#include "render.h"

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#include "platform/win/combine.h"

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
#endif

#endif
