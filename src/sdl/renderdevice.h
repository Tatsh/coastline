#pragma once

/**
 * @file
 * The renderer the device of common/renderdevice.h draws with in the SDL game.
 */

#include "sdl/tlrenderer.h"

namespace coastline {

/**
 * Draw the render states and the triangles of the device with a renderer.
 *
 * @param renderer The renderer, or null before there is one.
 */
void SetDeviceRenderer(TlRenderer *renderer);

} // namespace coastline
