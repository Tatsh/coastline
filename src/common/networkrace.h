#ifndef COASTLINE_NETWORKRACE_H
#define COASTLINE_NETWORKRACE_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Send message 6 of the game naming the local player, when there is one.
 * @ghidraAddress BRD3D: 0x1003dae0
 * @ghidraAddress BRGlide: 0x10037180
 */
void SendNetworkMessage6ToSession(void);

/**
 * Tick a pending request to end the pause of a network race.
 * @ghidraAddress BRD3D: 0x10005090
 * @ghidraAddress BRGlide: 0x10005400
 */
void TickNetworkUnpauseRequest(void);

/**
 * Request the end of a pause from the other players of a network game, when no request waits.
 *
 * @return Always true.
 * @ghidraAddress BRD3D: 0x10004c20
 * @ghidraAddress BRGlide: 0x10004f90
 */
bool RequestNetworkUnpause(void);

/**
 * Stop processing the messages received.
 * @ghidraAddress BRD3D: 0x100060c0
 * @ghidraAddress BRGlide: 0x10006430
 */
void DisableNetworkReceive(void);

/**
 * Whether network data was received since the last pause-menu update.
 * @ghidraAddress BRD3D: 0x106909d8
 * @ghidraAddress BRGlide: 0x105ccb80
 */
extern int g_dwNetworkDataReceived;

#ifdef __cplusplus
}
#endif

#endif
