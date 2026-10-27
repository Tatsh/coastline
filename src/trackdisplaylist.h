#ifndef COASTLINE_TRACKDISPLAYLIST_H
#define COASTLINE_TRACKDISPLAYLIST_H

#include "displaylistops.h"

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#include "platform/win/combine.h"
#endif

/** Set the combiner of the track: the two levels of a texture mixed and then shaded. */
static inline void SetTrackCombiner(DisplayListCommand *pkt) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
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
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */
}

#endif /* COASTLINE_TRACKDISPLAYLIST_H */
