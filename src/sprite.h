#ifndef COASTLINE_SPRITE_H
#define COASTLINE_SPRITE_H

/**
 * @file
 * The sprites of the menus: pictures loaded from the cartridge and drawn in pieces.
 */

#include <PR/ultratypes.h>

/** A picture of the menus. Its fields join this structure as they are reconstructed. */
typedef struct {
    /** The texels, once loaded. */
    u8 *pixels;
    u32 romOffset;
    u8 unreconstructed8[0xC - 0x8];
    s32 bitsPerTexel;
    s32 width;
    s32 height;
    u8 unreconstructed18[0x1C - 0x18];
    /** Where the picture is drawn from. */
    s32 originX;
    s32 originY;
    u8 unreconstructed24[0x30 - 0x24];
} Sprite;

/**
 * Allocate memory for a sprite and decompress it there from the cartridge.
 *
 * @ghidraAddress USA: 0x80242b10
 */
void LoadTextureFromRom(Sprite *sprite);

/**
 * Draw a sprite at a position.
 *
 * @ghidraAddress USA: 0x80245604
 */
void DrawTexturedSprite(Sprite *sprite, s32 x, s32 y);

/**
 * Draw a sprite over the screen in tiles, with a byte level whose role is not reconstructed yet.
 * It does not read its last two arguments.
 *
 * @ghidraAddress USA: 0x80245798
 */
void DrawTiledSprite(Sprite *sprite, s32 level, s32 unused2, s32 unused3);

/**
 * Draw a rectangle of a sprite in a color.
 *
 * @ghidraAddress USA: 0x8024594c
 */
void DrawSpriteRegion(
    Sprite *sprite, s32 x, s32 y, s32 width, s32 height, s32 red, s32 green, s32 blue);

/**
 * Draw a bordered box. The five bytes after the border are its colors, whose roles are not
 * reconstructed yet.
 *
 * @ghidraAddress USA: 0x80246f90
 */
void DrawRectBorder(s32 x,
                    s32 y,
                    s32 width,
                    s32 height,
                    s32 border,
                    s32 color0,
                    s32 color1,
                    s32 color2,
                    s32 color3,
                    s32 color4);

#endif
