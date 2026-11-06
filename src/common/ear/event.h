#ifndef COASTLINE_EAR_EVENT_H
#define COASTLINE_EAR_EVENT_H

#include <stdint.h>

#include "eventparameters.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Take the next handle of an event that no active event has, skipping 0.
 *
 * @return The handle.
 * @ghidraAddress EAR: 0x1000d730
 */
uint16_t GenerateEarObjectHandle(void);

/**
 * @param params The parameters of the event.
 * @return The handle of the event, or 0.
 * @ghidraAddress EAR: 0x1000cc20
 */
uint32_t __stdcall EarMixEventExportWrapper(const EarEventParams *params);

/**
 * Make an event from its parameters and add it to its channel, by its distance, direction, gain,
 * timing, and queue mode, without starting it. An event out of its distance is dropped.
 *
 * @param params The parameters of the event.
 * @return The handle of the event, or 0.
 * @ghidraAddress EAR: 0x1000cc30
 */
uint32_t __stdcall EAR_DLL_MixEvent(const EarEventParams *params);

/**
 * @param params The parameters of the event.
 * @return The handle of the event, or 0.
 * @ghidraAddress EAR: 0x1000d950
 */
uint32_t __stdcall EarMoveEventExportWrapper(const EarEventParams *params);

/**
 * Update an event of a channel from its parameters: its matrix, its pause and mute, its pitch,
 * priority, and times, and its distance, delay, gain, and direction.
 *
 * @param params The parameters of the event.
 * @return The handle of the event, or 0.
 * @ghidraAddress EAR: 0x1000d960
 */
uint32_t __stdcall EAR_DLL_MoveEvent(const EarEventParams *params);

/**
 * @param params The parameters of the event.
 * @return The handle of the event, or 0.
 * @ghidraAddress EAR: 0x1000f950
 */
uint32_t __stdcall EarStartEventExportWrapper(const EarEventParams *params);

/**
 * Start the playback of an event the game mixed, at the gain of its channel.
 *
 * @param params The parameters of the event.
 * @return The handle of the event, or 0.
 * @ghidraAddress EAR: 0x1000f960
 */
uint32_t __stdcall EAR_DLL_StartEvent(const EarEventParams *params);

/**
 * @param target The parameters of the event, or its handle when the high word of the control is
 * 1.
 * @param control The stop control in the low word, and the kind of the target in the high word.
 * @return The handle of the event for a stop control, else 0.
 * @ghidraAddress EAR: 0x1000f450
 */
uint32_t __stdcall EarShutDownEventExportWrapper(uintptr_t target, uint32_t control);

/**
 * Stop an event, with a stop control or at once. An event stopped at once leaves the registry of
 * the active events.
 *
 * @param target The parameters of the event, or its handle when the high word of the control is
 * 1.
 * @param control The stop control in the low word, and the kind of the target in the high word.
 * @return The handle of the event for a stop control, else 0.
 * @ghidraAddress EAR: 0x1000f464
 */
uint32_t __stdcall EAR_DLL_ShutDownEvent(uintptr_t target, uint32_t control);

/**
 * @param channel The channel.
 * @param event The event, 0 for the most recent of a sequential channel.
 * @return The status of the event.
 * @ghidraAddress EAR: 0x1000c6a0
 */
uint32_t __stdcall EarGetEventStatusExportWrapper(uint32_t channel, uint32_t event);

/**
 * Report the state of an event as flags: 0x1 muted, 0x2 playing, 0x4 active, 0x8 on a sequential
 * channel, 0x10 with a frequency override, 0x20 streaming, 0x40 with its expiry disabled, and
 * 0x80 stopping.
 *
 * @param channel The channel.
 * @param event The event, 0 for the most recent of a sequential channel.
 * @return The status of the event.
 * @ghidraAddress EAR: 0x1000c6b4
 */
uint32_t __stdcall EAR_DLL_GetEventStatus(uint32_t channel, uint32_t event);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_EAR_EVENT_H */
