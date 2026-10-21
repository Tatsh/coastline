#ifndef COASTLINE_TEXT_H
#define COASTLINE_TEXT_H

/**
 * @file
 * The drawing of text with the game's font.
 */

#include <PR/ultratypes.h>

/**
 * Draw text in the normal style.
 *
 * @ghidraAddress USA: 0x8022f4f8
 */
void SetTextStyleNormal(void);

/**
 * Centre text on its position.
 *
 * @ghidraAddress USA: 0x8022f504
 */
void SetTextAlignCenter(void);

/**
 * Set the size of the font.
 *
 * @ghidraAddress USA: 0x8022f5d0
 */
void SetTextSize(s32 size);

/**
 * Draw text at a position, aligned as the last alignment set.
 *
 * @ghidraAddress USA: 0x8022f5dc
 */
void DrawText(const char *text, s32 x, s32 y);

#endif
