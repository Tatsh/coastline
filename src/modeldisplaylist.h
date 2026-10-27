#ifndef COASTLINE_MODELDISPLAYLIST_H
#define COASTLINE_MODELDISPLAYLIST_H

#include "displaylistops.h"
#include "render.h"

/** Set up the tiles of the textures of a model: the load tile and the two render tiles. */
static inline void SetModelTiles(void) {
    gDPSetTile(
        gDisplayListPtr++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 0, 0, G_TX_LOADTILE, 0, 0, 0, 0, 0, 0, 0);
    gDPSetTile(gDisplayListPtr++, G_IM_FMT_RGBA, G_IM_SIZ_4b, 0, 0x1F0, 6, 0, 0, 0, 0, 0, 0, 0);
    gDPSetTile(gDisplayListPtr++, G_IM_FMT_RGBA, G_IM_SIZ_4b, 0, 0x100, 5, 0, 0, 0, 0, 0, 0, 0);
}

/** Turn the textures on at full scale and set up the tiles. */
static inline void SetModelTextures(void) {
    gSPTexture(gDisplayListPtr++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
    gSPClearGeometryMode(gDisplayListPtr++, G_TEXTURE_GEN | G_TEXTURE_GEN_LINEAR);
    gDPTileSync(gDisplayListPtr++);
    SetModelTiles();
}

/** Draw a display list of the current model when it has one. */
static inline void DrawModelDisplayList(DisplayListCommand *displayList) {
    if (displayList != NULL) {
        gSPDisplayList(gDisplayListPtr++, displayList);
    }
}

#endif
