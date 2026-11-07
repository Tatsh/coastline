#ifndef COASTLINE_NETWORKRACE_H
#define COASTLINE_NETWORKRACE_H

#include <stdbool.h>

#include <minwindef.h>

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

/**
 * Mark the network race started when its start frame has arrived.
 * @ghidraAddress BRD3D: 0x10006160
 * @ghidraAddress BRGlide: 0x100064d0
 */
void CheckNetworkRaceStartTime(void);

/**
 * Process pending notifications from the network session.
 * @ghidraAddress BRD3D: 0x100060f0
 * @ghidraAddress BRGlide: 0x10006460
 */
void ProcessPendingNetworkNotifications(void);

/**
 * Remove a departed network player from its racer slot.
 * @param slotIndex The slot of the departed player.
 * @ghidraAddress BRD3D: 0x1002f230
 * @ghidraAddress BRGlide: 0x1001c7a0
 */
void RemoveNetworkPlayerById(int slotIndex);

/**
 * Pop the slot of a player that departed, under the mutex of the stack.
 * @return The slot, or -1 when the stack is empty.
 * @ghidraAddress BRD3D: 0x10005d90
 * @ghidraAddress BRGlide: 0x10006100
 */
int PopDepartedPlayerSlot(void);

/**
 * Reset the network session state.
 * @return Always true.
 * @ghidraAddress BRD3D: 0x10005960
 * @ghidraAddress BRGlide: 0x10005cd0
 */
bool ResetNetworkState(void);

/**
 * Enable processing of messages received from the network session.
 * @ghidraAddress BRD3D: 0x10006090
 * @ghidraAddress BRGlide: 0x10006400
 */
void EnableNetworkReceive(void);

/**
 * Get the slot of the local player of a network game.
 * @return The slot, or -1 before one is assigned.
 * @ghidraAddress BRD3D: 0x10005d30
 * @ghidraAddress BRGlide: 0x100060a0
 */
int GetLocalPlayerIndex(void);

/**
 * Wait until the local player receives a slot.
 * @return Whether the last player-join packet was sent successfully.
 * @ghidraAddress BRD3D: 0x10004a90
 * @ghidraAddress BRGlide: 0x10004e00
 */
unsigned WaitForPlayerIdAssignment(void);

/**
 * Copy a player's name into the buffer of its network slot.
 * @param slotIndex The slot.
 * @param name The player name.
 * @ghidraAddress BRD3D: 0x10005ee0
 * @ghidraAddress BRGlide: 0x10006250
 */
void CopyPlayerNameToSlotBuffer(int slotIndex, const char *name);

/**
 * Count players in the network session.
 * @return The count, or 0xffff when the session description is unavailable.
 * @ghidraAddress BRD3D: 0x1000c670
 * @ghidraAddress BRGlide: 0x10009ba0
 */
DWORD GetDirectPlaySessionPlayerCount(void);

/**
 * Add a network player to its racer slot.
 * @param slotIndex The slot.
 * @ghidraAddress BRD3D: 0x1002f130
 * @ghidraAddress BRGlide: 0x1001c6a0
 */
void AddNetworkPlayerSlot(int slotIndex);

/**
 * Pop the slot of a player that joined, under the mutex of the stack.
 * @return The slot, or -1 when the stack is empty.
 * @ghidraAddress BRD3D: 0x10005d40
 * @ghidraAddress BRGlide: 0x100060b0
 */
int PopJoinedPlayerSlot(void);

#ifdef __cplusplus
}
#endif

#endif
