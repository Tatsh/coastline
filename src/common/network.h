#ifndef COASTLINE_NETWORK_H
#define COASTLINE_NETWORK_H

#include <stdbool.h>

#include "common/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Whether the local player watches a network race without driving.
 *
 * @ghidraAddress BRD3D: 0x1022af1c
 * @ghidraAddress BRGlide: 0x10226a4c
 */
extern int g_bNetSpectator;

/**
 * Whether the other players of a network game ended the pause.
 *
 * @ghidraAddress BRD3D: 0x106909d8
 * @ghidraAddress BRGlide: 0x105ccb80
 */
extern int g_dwNetworkDataReceived;

/**
 * Send message 6 of the game naming the local player, when there is one.
 *
 * @ghidraAddress BRD3D: 0x1003dae0
 * @ghidraAddress BRGlide: 0x10037180
 */
void SendNetworkMessage6ToSession(void);

/**
 * Mark the network race started when its start frame has arrived.
 *
 * @ghidraAddress BRD3D: 0x10006160
 * @ghidraAddress BRGlide: 0x100064d0
 */
void CheckNetworkRaceStartTime(void);

/**
 * Clear the peer slots, the notifications, the race order, the player stacks, the pause requests,
 * and the clock of a network game.
 *
 * @return Always true.
 * @ghidraAddress BRD3D: 0x10005960
 * @ghidraAddress BRGlide: 0x10005cd0
 */
bool ResetNetworkState(void);

/**
 * Count the end of a pause requested by the other players, and end the pause after 100 frames.
 *
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
 * Process the messages received.
 *
 * @ghidraAddress BRD3D: 0x10006090
 * @ghidraAddress BRGlide: 0x10006400
 */
void EnableNetworkReceive(void);

/**
 * Stop processing the messages received.
 *
 * @ghidraAddress BRD3D: 0x100060c0
 * @ghidraAddress BRGlide: 0x10006430
 */
void DisableNetworkReceive(void);

/**
 * Get the slot of the local player of a network game.
 *
 * @return The slot, or -1 before one is assigned.
 * @ghidraAddress BRD3D: 0x10005d30
 * @ghidraAddress BRGlide: 0x100060a0
 */
int GetLocalPlayerIndex(void);

/**
 * Send join packets until the session assigns the local player an id.
 *
 * @return Whether the last join packet was sent.
 * @ghidraAddress BRD3D: 0x10004a90
 * @ghidraAddress BRGlide: 0x10004e00
 */
unsigned WaitForPlayerIdAssignment(void);

/**
 * Copy the name of a player into its peer slot, under the mutex of the slot.
 *
 * @param slotIndex The slot of the player.
 * @param name The name of the player.
 * @ghidraAddress BRD3D: 0x10005ee0
 * @ghidraAddress BRGlide: 0x10006250
 */
void CopyPlayerNameToSlotBuffer(int slotIndex, const char *name);

/**
 * Get an entry of the order of a network race.
 *
 * @param nIndex The index of the entry.
 * @return The entry.
 * @ghidraAddress BRD3D: 0x10005e40
 * @ghidraAddress BRGlide: 0x100061b0
 */
int GetNetworkRaceOrderEntry(int nIndex);

/**
 * The number of players in the network session.
 *
 * @return The number, or 0xffff when the description of the session is not available.
 * @ghidraAddress BRD3D: 0x1000c670
 * @ghidraAddress BRGlide: 0x10009ba0
 */
u32 GetDirectPlaySessionPlayerCount(void);

/**
 * Play the sound effect and show the status message a network thread left pending.
 *
 * @ghidraAddress BRD3D: 0x100060f0
 * @ghidraAddress BRGlide: 0x10006460
 */
void ProcessPendingNetworkNotifications(void);

/**
 * Add a vehicle for the player of a peer slot, with its colour, car, and name.
 *
 * @param slotIndex The slot of the player.
 * @ghidraAddress BRD3D: 0x1002f130
 * @ghidraAddress BRGlide: 0x1001c6a0
 */
void AddNetworkPlayerSlot(int slotIndex);

/**
 * Remove the driver of the vehicle of a peer slot from the race.
 *
 * @param slotIndex The slot of the player.
 * @ghidraAddress BRD3D: 0x1002f230
 * @ghidraAddress BRGlide: 0x1001c7a0
 */
void RemoveNetworkPlayerById(int slotIndex);

/**
 * Pop the slot of a player that joined, under the mutex of the stack.
 *
 * @return The slot, or -1 when the stack is empty.
 * @ghidraAddress BRD3D: 0x10005d40
 * @ghidraAddress BRGlide: 0x100060b0
 */
int PopJoinedPlayerSlot(void);

/**
 * Pop the slot of a player that departed, under the mutex of the stack.
 *
 * @return The slot, or -1 when the stack is empty.
 * @ghidraAddress BRD3D: 0x10005d90
 * @ghidraAddress BRGlide: 0x10006100
 */
int PopDepartedPlayerSlot(void);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_NETWORK_H */
