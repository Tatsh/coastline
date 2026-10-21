#ifndef COASTLINE_SPRITE_H
#define COASTLINE_SPRITE_H

/**
 * @file
 * The sprites of the menus: pictures loaded from the cartridge and drawn in strips, and the
 * rectangles of solid colour drawn with them.
 */

#include <PR/ultratypes.h>

/** The style of an 8-bit sprite that draws it in the primitive colour times its texels. */
#define SPRITE_STYLE_TINTED 99

/**
 * A picture of the menus. The texels are stored in strips of rows, from the bottom row up, and
 * each strip is loaded and drawn on its own.
 */
typedef struct {
    u8 *pixels;      /*!< The texels, once loaded. */
    u32 romOffset;   /*!< Where the compressed picture starts on the cartridge. */
    u32 romEnd;      /*!< The start of the next asset on the cartridge. */
    u8 bitsPerTexel; /*!< The bits of a texel, 4 for intensity, 8 for intensity and alpha, and 16
                          for colour. */
    u32 width;       /*!< The width in texels. */
    u32 height;      /*!< The height in texels. */
    u32 stripHeight; /*!< The rows of a strip. */
    s32 originX;     /*!< The x the picture is drawn from on the screen. */
    s32 originY;     /*!< The y the picture is drawn from on the screen. */
    s32 drawWidth;   /*!< The width the picture is drawn at on the screen, negative to flip it. */
    s32 drawHeight;  /*!< The height the picture is drawn at on the screen. */
    u8 style; /*!< The style of a paint tool on the pictures of the tools, or SPRITE_STYLE_TINTED
                   for a sprite drawn in a colour. */
} Sprite;

/**
 * Allocate memory for a sprite and decompress it there from the cartridge. A picture whose size
 * does not match its dimensions is reported on the debug output, and the game then halts.
 *
 * @param sprite The sprite to load.
 * @ghidraAddress USA: 0x80242b10
 * @ghidraAddress JPN: 0x80243b20 (stub)
 * @ghidraAddress EUR: 0x80244020
 */
void LoadTextureFromRom(Sprite *sprite);

/**
 * Draw a sprite at its origin, at its size on the screen.
 *
 * @param sprite The sprite to draw.
 * @ghidraAddress USA: 0x80245470
 * @ghidraAddress JPN: 0x802464cc (stub)
 * @ghidraAddress EUR: 0x80246994 (stub)
 */
void DrawSpriteAtOrigin(Sprite *sprite);

/**
 * Draw a sprite at a position.
 *
 * @param sprite The sprite to draw.
 * @param x The X coordinate on the screen.
 * @param y The Y coordinate on the screen.
 * @ghidraAddress USA: 0x80245604
 * @ghidraAddress JPN: 0x80246674 (stub)
 * @ghidraAddress EUR: 0x80246b3c (stub)
 */
void DrawTexturedSprite(Sprite *sprite, s32 x, s32 y);

/**
 * Draw a sprite at its origin in a colour. Only a 4-bit sprite and a tinted 8-bit sprite take
 * the colour.
 *
 * @param sprite The sprite to draw.
 * @param red The red component of the colour.
 * @param green The green component of the colour.
 * @param blue The blue component of the colour.
 * @ghidraAddress USA: 0x80245798
 * @ghidraAddress JPN: 0x80246810 (stub)
 * @ghidraAddress EUR: 0x80246cd8 (stub)
 */
void DrawSpriteTinted(Sprite *sprite, u8 red, u8 green, u8 blue);

/**
 * Draw a sprite stretched over a rectangle in a colour.
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
 * @ghidraAddress JPN: 0x802469cc (stub)
 * @ghidraAddress EUR: 0x80246e94 (stub)
 */
void DrawSpriteRegion(
    Sprite *sprite, s32 x, s32 y, s32 width, s32 height, u8 red, u8 green, u8 blue);

/**
 * Draw one strip of a sprite stretched over a rectangle, upside down to match the bottom-up rows.
 * The depth of the sprite picks the format and the combiner. A depth other than 4, 8, or 16 loads
 * nothing and draws the rectangle with the texture memory as it is.
 *
 * @param sprite The sprite the strip belongs to.
 * @param pixels The texels of the strip.
 * @param width The width of the strip in texels.
 * @param height The height of the strip in texels.
 * @param x The left edge on the screen.
 * @param y The top edge on the screen.
 * @param drawWidth The width on the screen, negative to mirror the strip.
 * @param drawHeight The height on the screen.
 * @param red The red component of the primitive colour.
 * @param green The green component of the primitive colour.
 * @param blue The blue component of the primitive colour.
 * @ghidraAddress USA: 0x80245b00
 * @ghidraAddress JPN: 0x80246b88 (stub)
 * @ghidraAddress EUR: 0x80247050 (stub)
 */
void DrawSpriteStrip(Sprite *sprite,
                     u8 *pixels,
                     s32 width,
                     s32 height,
                     s32 x,
                     s32 y,
                     s32 drawWidth,
                     s32 drawHeight,
                     u8 red,
                     u8 green,
                     u8 blue);

/**
 * Draw a rectangle of the texels of a 4-bit sprite stretched over a rectangle of the screen in a
 * colour, upside down. The texels loaded extend one column and one row past the source rectangle.
 *
 * @param sprite The sprite to draw from.
 * @param srcX The left edge of the source rectangle in texels.
 * @param srcY The top edge of the source rectangle in texels.
 * @param srcWidth The width of the source rectangle in texels.
 * @param srcHeight The height of the source rectangle in texels.
 * @param x The left edge on the screen.
 * @param y The top edge on the screen.
 * @param width The width on the screen.
 * @param height The height on the screen.
 * @param red The red component of the colour.
 * @param green The green component of the colour.
 * @param blue The blue component of the colour.
 * @ghidraAddress USA: 0x802465f0
 * @ghidraAddress JPN: 0x802477d8
 * @ghidraAddress EUR: 0x80247ca0
 */
void DrawScaledTexturedRect(Sprite *sprite,
                            s32 srcX,
                            s32 srcY,
                            s32 srcWidth,
                            s32 srcHeight,
                            s32 x,
                            s32 y,
                            s32 width,
                            s32 height,
                            u8 red,
                            u8 green,
                            u8 blue);

/**
 * Fill one pixel in a grey.
 *
 * @param x The X coordinate on the screen.
 * @param y The Y coordinate on the screen.
 * @param shade The level of the grey.
 * @ghidraAddress USA: 0x80246a80
 * @ghidraAddress JPN: 0x80247c68
 * @ghidraAddress EUR: 0x80248130
 */
void DrawPixel(s32 x, s32 y, u8 shade);

/**
 * Draw the outline of a rectangle in a colour, inside its edges.
 *
 * @param x The left edge on the screen.
 * @param y The top edge on the screen.
 * @param width The width.
 * @param height The height.
 * @param thickness The thickness of the outline, at least 1 once the screen scale applies.
 * @param red The red component of the colour.
 * @param green The green component of the colour.
 * @param blue The blue component of the colour.
 * @ghidraAddress USA: 0x80246bcc
 * @ghidraAddress JPN: 0x80247db4
 * @ghidraAddress EUR: 0x8024827c
 */
void DrawRectOutline(s32 x, s32 y, s32 width, s32 height, s32 thickness, u8 red, u8 green, u8 blue);

/**
 * Fill a rectangle in a colour. A width or height under 1 once the screen scale applies is 1.
 *
 * @param x The left edge on the screen.
 * @param y The top edge on the screen.
 * @param width The width.
 * @param height The height.
 * @param red The red component of the colour.
 * @param green The green component of the colour.
 * @param blue The blue component of the colour.
 * @ghidraAddress USA: 0x80246e10
 * @ghidraAddress EUR: 0x802484c0
 */
void DrawFilledRectHiRes(s32 x, s32 y, s32 width, s32 height, u8 red, u8 green, u8 blue);

/**
 * Draw a box with a bevelled border outside its edges, light at the top and the left and dark at
 * the bottom and the right, or the other way round when sunken.
 *
 * @param x The left edge of the box.
 * @param y The top edge of the box.
 * @param width The width of the box.
 * @param height The height of the box.
 * @param border The width of the border, at least 1 once the screen scale applies.
 * @param transparent Whether the inside of the box is not filled.
 * @param sunken 1 to swap the light and the dark shades of the border.
 * @param red The red component of the colour inside the box.
 * @param green The green component of the colour inside the box.
 * @param blue The blue component of the colour inside the box.
 * @ghidraAddress USA: 0x80246f90
 * @ghidraAddress JPN: 0x80248178
 * @ghidraAddress EUR: 0x80248640
 */
void DrawRectBorder(s32 x,
                    s32 y,
                    s32 width,
                    s32 height,
                    s32 border,
                    u8 transparent,
                    u8 sunken,
                    u8 red,
                    u8 green,
                    u8 blue);

#endif
