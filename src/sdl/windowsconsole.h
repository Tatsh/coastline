#pragma once

namespace coastline {

/**
 * Attach to an existing parent console and reconnect unavailable standard streams.
 * Redirected streams are preserved. A console window is never allocated.
 */
void AttachParentConsole();

} // namespace coastline
