#pragma once

/**
 * Query DirectDraw for the video memory left for textures.
 *
 * @return The free bytes of texture memory.
 * @ghidraAddress BRD3D: 0x1003e220
 */
unsigned GetAvailableTextureMemory(void);
