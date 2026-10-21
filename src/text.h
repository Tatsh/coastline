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
 * Start text at its position.
 *
 * @ghidraAddress USA: 0x8022f514
 */
void SetTextAlignLeft(void);

/**
 * End text at its position.
 *
 * @ghidraAddress USA: 0x8022f520
 */
void SetTextAlignRight(void);

/**
 * Set the size of the font.
 *
 * @ghidraAddress USA: 0x8022f5d0
 */
void SetTextSize(s32 size);

/**
 * Set the two colors of the text.
 *
 * @ghidraAddress USA: 0x8022f530
 */
void SetTextCustomColors(s32 red, s32 green, s32 blue, s32 red2, s32 green2, s32 blue2);

/**
 * Draw text at a position, aligned as the last alignment set.
 *
 * @ghidraAddress USA: 0x8022f5dc
 */
void DrawText(const char *text, s32 x, s32 y);

#endif
