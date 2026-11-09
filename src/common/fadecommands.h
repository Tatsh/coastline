#ifndef COASTLINE_FADE_COMMANDS_H
#define COASTLINE_FADE_COMMANDS_H

#include "common/render.h"

#if TGR_BOSS_RALLY
#include "common/combine.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Set primitive-colour blending for a fade's second cycle.
 * @param secondCycle The second-cycle colour and alpha source.
 */
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
/** Set primitive-colour blending for a fade's second cycle.
 * @param secondCycle The second-cycle colour and alpha source.
 */
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
#endif /* TGR_BOSS_RALLY */

#endif /* COASTLINE_FADE_COMMANDS_H */
