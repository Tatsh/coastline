#ifndef COASTLINE_TRACKDISPLAYLIST_H
#define COASTLINE_TRACKDISPLAYLIST_H

#include "displaylistops.h"

#if TGR_PC
#include "win32/combine.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** Set the combiner of the track: the two levels of a texture mixed and then shaded. */
static inline void SetTrackCombiner(DisplayListCommand *pkt) {
#if TGR_PC
    PackCombineMode(pkt,
                    1000 + G_CCMUX_TEXEL1,
                    1000 + G_CCMUX_TEXEL0,
                    1000 + G_CCMUX_LOD_FRACTION,
                    1000 + G_CCMUX_TEXEL0,
                    1000 + G_ACMUX_TEXEL1,
                    1000 + G_ACMUX_TEXEL0,
                    1000 + G_CCMUX_LOD_FRACTION,
                    1000 + G_ACMUX_TEXEL0,
                    1000 + G_CCMUX_COMBINED,
                    0,
                    1000 + G_CCMUX_SHADE,
                    0,
                    0,
                    0,
                    0,
                    1000 + G_ACMUX_COMBINED);
#else
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
#endif /* TGR_PC */
}

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_TRACKDISPLAYLIST_H */
