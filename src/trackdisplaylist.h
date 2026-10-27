#ifndef COASTLINE_TRACKDISPLAYLIST_H
#define COASTLINE_TRACKDISPLAYLIST_H

#include "displaylistops.h"

/** Set the combiner of the track: the two levels of a texture mixed and then shaded. */
static inline void SetTrackCombiner(DisplayListCommand *pkt) {
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
}

#endif
