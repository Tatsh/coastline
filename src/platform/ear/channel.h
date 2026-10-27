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

#ifdef __cplusplus
}
#endif

#endif
