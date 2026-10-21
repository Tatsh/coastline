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
    /** The size the picture is drawn at on the screen. A negative width flips it. */
    s32 drawWidth;
    s32 drawHeight;
    /** The style a paint tool is set to, on the pictures of the tools. */
    u8 style;
} Sprite;

/**
 * Allocate memory for a sprite and decompress it there from the cartridge.
 *
 * @param sprite The sprite to load.
 * @ghidraAddress USA: 0x80242b10
 */
void LoadTextureFromRom(Sprite *sprite);

/**
 * Draw a sprite at a position.
 *
 * @param sprite The sprite to draw.
 * @param x The X coordinate on the screen.
 * @param y The Y coordinate on the screen.
 * @ghidraAddress USA: 0x80245604
 */
void DrawTexturedSprite(Sprite *sprite, s32 x, s32 y);

/**
 * Draw a sprite over the screen in tiles, with a byte level whose role is not reconstructed yet.
 * It does not read its last two arguments.
 *
 * @param sprite The sprite to draw.
 * @param level The byte level.
 * @param unused2 Not read.
 * @param unused3 Not read.
 * @ghidraAddress USA: 0x80245798
 */
void DrawSpriteTinted(Sprite *sprite, s32 level, s32 unused2, s32 unused3);

/**
 * Draw a rectangle of a sprite in a color.
 *
 * @param sprite The sprite to draw.
 * @param x The left edge of the rectangle on the screen.
 * @param y The top edge of the rectangle on the screen.
 * @param width The width of the rectangle.
 * @param height The height of the rectangle.
 * @param red The red component of the colour.
 * @param green The green component of the colour.
 * @param blue The blue component of the colour.
 * @ghidraAddress USA: 0x8024594c
 */
void DrawSpriteRegion(
    Sprite *sprite, s32 x, s32 y, s32 width, s32 height, s32 red, s32 green, s32 blue);

/**
 * Draw a bordered box. The five bytes after the border are its colors, whose roles are not
 * reconstructed yet.
 *
 * @param x The left edge of the box.
 * @param y The top edge of the box.
 * @param width The width of the box.
 * @param height The height of the box.
 * @param border The width of the border, at least 1.
 * @param color0 The first colour byte.
 * @param color1 The second colour byte.
 * @param color2 The third colour byte.
 * @param color3 The fourth colour byte.
 * @param color4 The fifth colour byte.
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
