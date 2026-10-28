#ifndef COASTLINE_EAR_CHANNEL_H
#define COASTLINE_EAR_CHANNEL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ghidraAddress EAR: 0x1000de80
 * @stub
 */
int __stdcall EarRegisterChannelExportWrapper(uint32_t channel,
                                              uint32_t flags,
                                              uint32_t gainPercent,
                                              uint32_t playbackLimits);

/**
 * Shut down a channel and its events.
 *
 * @param channel The channel, 0 for every channel, with 0x10000 for the shutdown of the library.
 * @return Whether the channel was shut down.
 * @ghidraAddress EAR: 0x1000f0a0
 * @stub
 */
int __stdcall EarShutDownChannelExportWrapper(uint32_t channel);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_EAR_CHANNEL_H */
