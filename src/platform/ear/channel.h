#ifndef COASTLINE_EAR_CHANNEL_H
#define COASTLINE_EAR_CHANNEL_H

#include <stdint.h>

#ifdef __cplusplus
#include "mfc.h"
#include "registry.h"

class ScheduledEarEvent;

/** A channel the game registers, which plays and groups its events. */
class EarChannel {
public:
    /** @ghidraAddress EAR: 0x1000e330 */
    EarChannel();
    /** @ghidraAddress EAR: 0x1000e440 */
    ~EarChannel();

    /**
     * Set the step the gain of the channel moves by each update, from the smoothing bits of a
     * control.
     *
     * @param control The smoothing bits: 0x100, 0x300, or 0x700 for steps of 0.05, 0.03, and
     * 0.01, and any other value for an immediate change.
     * @ghidraAddress EAR: 0x1000c270
     */
    void SetChannelSmoothingStep(uint16_t control);

    /**
     * Make room for an event of a priority and an attenuation, releasing the playback of an event
     * below them when the channel is full.
     *
     * @param priority The priority of the new event.
     * @param attenuation The attenuation of the new event.
     * @return Whether the new event can play.
     * @ghidraAddress EAR: 0x100075f0
     */
    int EnsureChannelPlaybackCapacity(uint16_t priority, uint16_t attenuation);

    int32_t flag01;     /*!< Registration flag 0x1, set by control 0x100 and cleared by 0x300. */
    int32_t sequential; /*!< Whether the events play one after another, as music does. */
    int32_t paused;     /*!< Whether the events of the channel are paused. */
    int32_t muted;      /*!< Whether the events of the channel are muted. */
    int32_t targetGainChanged;     /*!< Whether EarChannel::targetGain changed since the update. */
    int32_t shutdownRequested;     /*!< Whether the channel shuts down once its gain is 0. */
    double gain;                   /*!< The gain of the channel, from 0 to 1. */
    double targetGain;             /*!< The gain the channel moves towards. */
    double gainStep;               /*!< The change of the gain at each update. */
    uint16_t maximumEvents;        /*!< The events the channel can hold, 0xffff for any number. */
    uint16_t maximumStartedEvents; /*!< The events the channel can start, 0xffff for any. */
    uint16_t startedEventCount;    /*!< The events the channel has started. */
    uint16_t priorityLevel;        /*!< 1, 50, or 100 from the priority bits, or 0. */
    CWordList recentEvents;        /*!< The handles of the events, the most recent first. */
    EarWordPointerRegistry<ScheduledEarEvent> events; /*!< The events by handle. */
};

/** @ghidraAddress EAR: 0x1003b6e0 */
extern uint32_t g_dwCdAudioChannelId;
/** @ghidraAddress EAR: 0x1003b6e4 */
extern uint32_t g_dwMidiChannelId;
/**
 * The state of the last channel a control changed, as flags of the control.
 *
 * @ghidraAddress EAR: 0x1003b8c4
 */
extern uint32_t g_earChannelControlResultFlags;

extern "C" {
#endif

/**
 * @param channel The channel.
 * @param flags The flags of the channel.
 * @param gainPercent The gain of the channel, in percent.
 * @param playbackLimits The events the channel holds in the low word, and the events it starts in
 * the high word.
 * @return Whether the channel was registered.
 * @ghidraAddress EAR: 0x1000de80
 */
int __stdcall EarRegisterChannelExportWrapper(uint32_t channel,
                                              uint32_t flags,
                                              uint32_t gainPercent,
                                              uint32_t playbackLimits);

/**
 * Register a channel. Flag 0x1 sets EarChannel::flag01 and flag 0x2 makes the channel
 * sequential, with 0x6 for the CD audio and 0xa for the MIDI music. Flags 0x10, 0x30, and 0x70
 * give the priority level, and flags 0x700 the smoothing of the gain.
 *
 * @param channel The channel, not 0.
 * @param flags The flags of the channel.
 * @param gainPercent The gain of the channel, in percent, at most 100.
 * @param playbackLimits The events the channel holds in the low word, and the events it starts in
 * the high word, 0 for any number.
 * @return Whether the channel was registered.
 * @ghidraAddress EAR: 0x1000de9c
 */
int __stdcall EAR_DLL_RegisterChannel(uint32_t channel,
                                      uint32_t flags,
                                      uint32_t gainPercent,
                                      uint32_t playbackLimits);

/**
 * @param channel The channel, 0 for every channel, with 0x10000 for the shutdown of the library.
 * @return 0.
 * @ghidraAddress EAR: 0x1000f0a0
 */
int __stdcall EarShutDownChannelExportWrapper(uint32_t channel);

/**
 * Shut down a channel and its events. A channel that is not yet shutting down only starts to fade
 * out, unless the library is shutting down.
 *
 * @param channel The channel, 0 for every channel, with 0x10000 for the shutdown of the library.
 * @return 0.
 * @ghidraAddress EAR: 0x1000f0b0
 */
int __stdcall EAR_DLL_ShutDownChannel(uint32_t channel);

/**
 * @param channel The channel, 0 for every channel.
 * @param keepActive Whether the active events stop rather than being deleted.
 * @return Whether an active event was stopped.
 * @ghidraAddress EAR: 0x1000c320
 */
int __stdcall EarClearChannelExportWrapper(uint32_t channel, int keepActive);

/**
 * Delete the events of a channel, or stop the active ones.
 *
 * @param channel The channel, 0 for every channel.
 * @param keepActive Whether the active events stop rather than being deleted.
 * @return Whether an active event was stopped.
 * @ghidraAddress EAR: 0x1000c334
 */
int __stdcall EAR_DLL_ClearChannel(uint32_t channel, int keepActive);

/**
 * @param channel The channel, 0 for every channel.
 * @param control The control.
 * @return The result of the control.
 * @ghidraAddress EAR: 0x1000bb50
 */
uint32_t __stdcall EarChangeChannelControlExportWrapper(uint32_t channel, uint32_t control);

/**
 * Change or query a channel. A control with 0x80000000 queries an event, with 0x40000000 sets or
 * queries the limits and the gain of the channel, and with 0x10000000 controls the CD audio.
 * Otherwise the bits pause and resume (0x4, 0xc), mute and unmute (0x1, 0x3), and set the
 * priority level, the smoothing, and the flags of the channel.
 *
 * @param channel The channel, 0 for every channel.
 * @param control The control.
 * @return The result of a query, or g_earChannelControlResultFlags.
 * @ghidraAddress EAR: 0x1000bb64
 */
uint32_t __stdcall EAR_DLL_ChangeChannelControl(uint32_t channel, uint32_t control);

/**
 * @param channel The channel, 0 for every channel.
 * @param level The attenuation level, in ten-thousandths.
 * @ghidraAddress EAR: 0x1000ef30
 */
void __stdcall EarSetAttenuationLevelExportWrapper(uint32_t channel, uint32_t level);

/**
 * Set the gain a channel moves towards, or the master attenuation of every channel.
 *
 * @param channel The channel, 0 for the master attenuation.
 * @param level The attenuation level, in ten-thousandths, at most 10000.
 * @ghidraAddress EAR: 0x1000ef44
 */
void __stdcall EAR_DLL_SetAttenuationLevel(uint32_t channel, uint32_t level);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_EAR_CHANNEL_H */
