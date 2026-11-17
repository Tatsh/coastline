#pragma once

/**
 * @file
 * The attachment of the Windows game to the console of the process that started it.
 */

namespace coastline {

/**
 * Attach to an existing parent console and reconnect unavailable standard streams.
 * Redirected streams are preserved. A console window is never allocated.
 */
void AttachParentConsole();

} // namespace coastline
