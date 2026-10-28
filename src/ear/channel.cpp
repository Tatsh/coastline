#include "channel.h"

#include <cmath>
#include <new>

#include "error.h"
#include "event.h"
#include "playbacksmoothing.h"
#include "runtime.h"
#include "scheduledevent.h"
#include "volume.h"
#include "win32/ear/musicdevice.h"
#include "win32/ear/timer.h"

uint32_t g_dwCdAudioChannelId;
uint32_t g_dwMidiChannelId;
uint32_t g_earChannelControlResultFlags;

// @ghidraAddress EAR: 0x1002ec00
static const double g_earChannelPercentScale = 0.01;
// @ghidraAddress EAR: 0x1002ec08
static const double g_earChannelMaximumGain = 1;
// @ghidraAddress EAR: 0x1002ebd8
static const double g_earChannelControlTimeUnitScale = 10000;
// @ghidraAddress EAR: 0x1002ec40
static const double g_earAttenuationLevelScale = 0.0001;

/** The library shuts down a channel with this in the high word of its argument. */
#define EAR_CHANNEL_LIBRARY_SHUTDOWN 0x10000

EarChannel::EarChannel() : recentEvents(10), events() {
    shutdownRequested = 0;
    targetGainChanged = 0;
    muted = 0;
    paused = 0;
    sequential = 0;
    flag01 = 0;
    gain = 1;
    targetGain = 1;
    gainStep = 1;
    maximumEvents = 0xffff;
    maximumStartedEvents = 0xffff;
    startedEventCount = 0;
}

EarChannel::~EarChannel() = default;

void EarChannel::SetChannelSmoothingStep(uint16_t control) {
    switch (control) {
    case 0x100:
        gainStep = 0.05;
        break;
    case 0x300:
        gainStep = 0.03;
        break;
    case 0x700:
        gainStep = 0.01;
        break;
    default:
        gainStep = 1;
        break;
    }
}

int EarChannel::EnsureChannelPlaybackCapacity(uint16_t priority, uint16_t attenuation) {
    if (sequential) {
        return 1;
    }
    if (!priority && !attenuation) {
        return 0;
    }
    if (events.IsEmpty()) {
        return maximumEvents > 0;
    }
    uint16_t lowestPriority = priority;
    uint16_t lowestAttenuation = attenuation;
    POSITION position = events.GetStartPosition();
    uint16_t lowestPriorityEvent = 0;
    uint16_t lowestAttenuationEvent = 0;
    do {
        WORD key;
        void *value;
        events.GetNextAssoc(position, key, value);
        ScheduledEarEvent *event = static_cast<ScheduledEarEvent *>(value);
        if (lowestPriority > event->priority && event->priority) {
            lowestPriority = event->priority;
            lowestPriorityEvent = key;
        }
        if (lowestAttenuation > event->attenuation) {
            lowestAttenuation = event->attenuation;
            lowestAttenuationEvent = key;
        }
    } while (position);
    ScheduledEarEvent *event;
#ifdef ENABLE_PATCHES
    if (lowestPriority < priority && lowestPriorityEvent &&
        events.LookupWordPointerMapValue(lowestPriorityEvent, event)) {
#else
    if (lowestPriority < priority && lowestPriorityEvent) {
        // The lookup is not checked; the event came from the same map.
        events.LookupWordPointerMapValue(lowestPriorityEvent, event);
#endif /* ENABLE_PATCHES */
        event->ReleaseScheduledEventPlayback();
        return 1;
    }
#ifdef ENABLE_PATCHES
    if (lowestAttenuation < attenuation && lowestAttenuationEvent &&
        events.LookupWordPointerMapValue(lowestAttenuationEvent, event)) {
#else
    if (lowestAttenuation < attenuation && lowestAttenuationEvent) {
        events.LookupWordPointerMapValue(lowestAttenuationEvent, event);
#endif /* ENABLE_PATCHES */
        if (event->priority <= priority) {
            event->ReleaseScheduledEventPlayback();
            return 1;
        }
    }
    return 0;
}

int __stdcall EarRegisterChannelExportWrapper(uint32_t channel,
                                              uint32_t flags,
                                              uint32_t gainPercent,
                                              uint32_t playbackLimits) {
    return EAR_DLL_RegisterChannel(channel, flags, gainPercent, playbackLimits);
}

int __stdcall EAR_DLL_RegisterChannel(uint32_t channel,
                                      uint32_t flags,
                                      uint32_t gainPercent,
                                      uint32_t playbackLimits) {
    SetEarLastError(0);
    if (!IsEarReady()) {
        return 0;
    }
    if (channel == 0) {
        SetEarLastError(0x24);
        return 0;
    }
    g_ChannelRegistry.GrowWhenFull();
    EarChannel *registered;
    if (g_ChannelRegistry.LookupWordPointerMapValue(static_cast<uint16_t>(channel), registered)) {
        SetEarLastError(0x23);
        return 0;
    }
    void *storage = MfcAllocateWithNewHandler(sizeof(EarChannel));
    registered = storage ? new (storage) EarChannel : nullptr;
    if (registered == nullptr) {
        SetEarLastError(0x10);
        return 0;
    }
    registered->flag01 = flags & 1;
    registered->sequential = flags & 2;
    if ((flags & 6) == 6) {
        if (g_dwCdAudioChannelId) {
            SetEarLastError(0x1b);
            delete registered;
            return 0;
        }
        registered->flag01 = 1;
        registered->sequential = 1;
        OpenEarMusicDevice(1);
        if (!g_bCdAudioInitialized) {
            SetEarLastError(0x2b);
            delete registered;
            return 0;
        }
        g_dwCdAudioChannelId = channel;
    }
    if ((flags & 0xa) == 0xa) {
        if (g_dwMidiChannelId) {
            SetEarLastError(0x1b);
            delete registered;
            return 0;
        }
        registered->flag01 = 1;
        registered->sequential = 1;
        OpenEarMusicDevice(0);
        if (!g_bMidiSequencerInitialized) {
            SetEarLastError(0x2c);
            delete registered;
            return 0;
        }
        g_dwMidiChannelId = channel;
    }
    // The original multiplies the percentage as an unsigned 64-bit integer, twice.
    registered->gain =
        (long double)(uint64_t)gainPercent * g_earChannelPercentScale < g_earChannelMaximumGain ?
            (double)((long double)(uint64_t)gainPercent * g_earChannelPercentScale) :
            1.0;
    registered->targetGain = registered->gain;
    registered->maximumEvents = static_cast<uint16_t>(playbackLimits);
    if (registered->maximumEvents == 0) {
        registered->maximumEvents = 0xffff;
    }
    registered->maximumStartedEvents = static_cast<uint16_t>(playbackLimits >> 16);
    if (registered->maximumStartedEvents == 0) {
        registered->maximumStartedEvents = 0xffff;
    }
    if (flags & 0x10) {
        registered->priorityLevel = 1;
    } else if (flags & 0x30) {
        registered->priorityLevel = 50;
    } else if (flags & 0x70) {
        registered->priorityLevel = 100;
    } else {
        registered->priorityLevel = 0;
    }
    registered->SetChannelSmoothingStep(static_cast<uint16_t>(flags & 0x700));
    registered->events.InitHashTable(registered->maximumEvents + 5, TRUE);
    g_ChannelRegistry.SetWordPointerMapValue(static_cast<uint16_t>(channel), registered);
    return 1;
}

int __stdcall EarShutDownChannelExportWrapper(uint32_t channel) {
    return EAR_DLL_ShutDownChannel(channel);
}

int __stdcall EAR_DLL_ShutDownChannel(uint32_t channel) {
    SetEarLastError(0);
    if (!IsEarReady()) {
        return 0;
    }
    if (g_ChannelRegistry.IsEmpty()) {
        return 0;
    }
    uint16_t handle = static_cast<uint16_t>(channel);
    uint32_t mode = channel & 0xffff0000;
    int locked = 0;
    while (g_bPlaybackUpdateLocked) {
    }
    if (!g_bRegistryMutationLocked) {
        locked = 1;
        g_bRegistryMutationLocked = locked;
    }
    if (handle == 0) {
        POSITION position = g_ChannelRegistry.GetStartPosition();
        do {
            WORD key;
            void *value;
            g_ChannelRegistry.GetNextAssoc(position, key, value);
            EarShutDownChannelExportWrapper(key | mode);
        } while (position);
    } else {
        EarChannel *found;
        if (g_ChannelRegistry.LookupWordPointerMapValue(handle, found)) {
            if (found->shutdownRequested != 1 && mode != EAR_CHANNEL_LIBRARY_SHUTDOWN) {
                found->targetGainChanged = 1;
                found->shutdownRequested = 1;
                found->targetGain = 0;
            } else {
                EarClearChannelExportWrapper(handle, 0);
                if (handle == g_dwCdAudioChannelId) {
                    CloseCdAudioDevice();
                    g_dwCdAudioChannelId = 0;
                }
                if (handle == g_dwMidiChannelId) {
                    CloseMidiMusicDevice();
                    g_dwMidiChannelId = 0;
                }
                delete found;
                g_ChannelRegistry.RemoveKey(handle);
            }
        } else {
            SetEarLastError(0x1b);
        }
    }
    if (locked) {
        g_bRegistryMutationLocked = 0;
    }
    return 0;
}

int __stdcall EarClearChannelExportWrapper(uint32_t channel, int keepActive) {
    return EAR_DLL_ClearChannel(channel, keepActive);
}

int __stdcall EAR_DLL_ClearChannel(uint32_t channel, int keepActive) {
    SetEarLastError(0);
    if (!IsEarReady()) {
        return 0;
    }
    if (g_ChannelRegistry.IsEmpty()) {
        return 0;
    }
    int stopped = 0;
    int locked = 0;
    while (g_bPlaybackUpdateLocked) {
    }
    if (!g_bRegistryMutationLocked) {
        locked = 1;
        g_bRegistryMutationLocked = locked;
    }
    if (channel == 0) {
        POSITION position = g_ChannelRegistry.GetStartPosition();
        do {
            WORD key;
            void *value;
            g_ChannelRegistry.GetNextAssoc(position, key, value);
            EAR_DLL_ClearChannel(key, 0);
        } while (position);
    } else {
        EarChannel *found;
        if (g_ChannelRegistry.LookupWordPointerMapValue(static_cast<uint16_t>(channel), found)) {
            if (found->events.IsEmpty()) {
                stopped = 0;
            } else {
                POSITION position = found->events.GetStartPosition();
                found->recentEvents.RemoveAll();
                WORD key;
                void *value;
                if (found->sequential && keepActive) {
                    // This first association is only recorded; the loop below starts after it.
                    found->events.GetNextAssoc(position, key, value);
                    if (static_cast<ScheduledEarEvent *>(value)->IsScheduledEventPlaybackActive()) {
                        found->recentEvents.AddHead(key);
                    }
                }
                [[maybe_unused]] POSITION saved;
                do {
                    saved = position;
                    found->events.GetNextAssoc(position, key, value);
                    ScheduledEarEvent *event = static_cast<ScheduledEarEvent *>(value);
                    if (keepActive && event->IsScheduledEventPlaybackActive()) {
                        // The original passes the event itself where the export reads the
                        // parameters of an event.
                        EarShutDownEventExportWrapper(reinterpret_cast<uintptr_t>(event), 1);
                        stopped = 1;
                    } else {
                        delete event;
                        found->events.RemoveKey(key);
                    }
                } while (position);
            }
        } else {
            stopped = 0;
            SetEarLastError(0x1b);
        }
    }
    if (locked) {
        g_bRegistryMutationLocked = 0;
    }
    return stopped;
}

void __stdcall EarSetAttenuationLevelExportWrapper(uint32_t channel, uint32_t level) {
    EAR_DLL_SetAttenuationLevel(channel, level);
}

void __stdcall EAR_DLL_SetAttenuationLevel(uint32_t channel, uint32_t level) {
    SetEarLastError(0);
    if (!IsEarReady()) {
        return;
    }
    if (level > 10000) {
        SetEarLastError(0x24);
        level = 10000;
    }
#ifndef ENABLE_PATCHES
    else if (level < 0) {
        // The original tests the unsigned level against 0 as well, which never holds.
        SetEarLastError(0x24);
        level = 0;
    }
#endif /* ENABLE_PATCHES */
    while (g_bPlaybackUpdateLocked) {
    }
    g_bPlaybackUpdateLocked = 1;
    if (channel == 0) {
        if (level == 0) {
            g_dMasterAttenuationLevel = 0;
        } else {
            g_dMasterAttenuationLevel =
                (double)((long double)(uint64_t)level * g_earAttenuationLevelScale);
        }
    } else {
        EarChannel *found;
        if (!g_ChannelRegistry.IsEmpty() &&
            g_ChannelRegistry.LookupWordPointerMapValue(static_cast<uint16_t>(channel), found)) {
            // The x87 comparison leaves an unordered gain as it is.
            if (std::islessgreater((long double)(uint64_t)level * g_earAttenuationLevelScale,
                                   (long double)found->targetGain)) {
                found->targetGain =
                    (double)((long double)(uint64_t)level * g_earAttenuationLevelScale);
                found->targetGainChanged = 1;
            }
        } else {
            SetEarLastError(0x1b);
        }
    }
    g_bPlaybackUpdateLocked = 0;
}

uint32_t __stdcall EarChangeChannelControlExportWrapper(uint32_t channel, uint32_t control) {
    return EAR_DLL_ChangeChannelControl(channel, control);
}

/** Answer a control with 0x80000000, a query of an event of a channel. */
static uint32_t QueryChannelEvent(uint32_t channel, uint32_t control, EarChannel *found) {
    uint16_t event = static_cast<uint16_t>(control);
    if (event == 0 && found->sequential && !found->recentEvents.IsEmpty()) {
        event = found->recentEvents.GetHead();
    }
    if ((control & 0x80010000) == 0x80010000) {
        return event;
    }
    if ((control & 0x80020000) == 0x80020000) {
        return EarGetEventStatusExportWrapper(channel, event);
    }
    if ((control & 0x80040000) == 0x80040000) {
        ScheduledEarEvent *scheduled;
        if (found->events.LookupWordPointerMapValue(event, scheduled)) {
            return scheduled->GetScheduledEventMediaLength();
        }
        return 0;
    }
    return 0;
}

/** Answer a control with 0x10000000, a control of the CD audio. */
static int ControlCdAudio(uint32_t control, uint32_t *result) {
    if ((control & 0x10000010) == 0x10000010) {
        *result = GetCdTrackCount();
    } else if ((control & 0x10000001) == 0x10000001) {
        *result = OpenCdAudioDoor();
    } else if ((control & 0x10000002) == 0x10000002) {
        *result = CloseCdAudioDoorAndWait();
    } else if ((control & 0x10000004) == 0x10000004) {
        *result = IsCdAudioMediaPresent();
    } else if ((control & 0x10000008) == 0x10000008) {
        *result = LoadCdTrackPositions();
    } else if ((control & 0x10000020) == 0x10000020) {
        *result = g_wFirstCdAudioTrack;
    } else if ((control & 0x10000040) == 0x10000040) {
        *result = g_wLastCdAudioTrack;
    } else {
        return 0;
    }
    return 1;
}

uint32_t __stdcall EAR_DLL_ChangeChannelControl(uint32_t channel, uint32_t control) {
    SetEarLastError(0);
    if (!IsEarReady()) {
        return 0;
    }
    if (g_ChannelRegistry.IsEmpty()) {
        return 0;
    }
    while (g_bPlaybackUpdateLocked) {
    }
    if (channel == 0 && (control & 0x20000000) != 0x20000000) {
        if ((control & 0xc) == 0xc) {
            EarStartTimerExportWrapper();
        } else if (control & 4) {
            EarShutDownTimerExportWrapper();
        }
        POSITION position = g_ChannelRegistry.GetStartPosition();
        do {
            WORD key;
            void *value;
            g_ChannelRegistry.GetNextAssoc(position, key, value);
            EAR_DLL_ChangeChannelControl(key, control);
        } while (position);
        return g_earChannelControlResultFlags;
    }
    int muteChanged = 0;
    int pauseChanged = 0;
    g_earChannelControlResultFlags = 0;
    if (channel == 0 && (control & 0x20000000) == 0x20000000) {
        if ((control & 0x20010000) == 0x20010000) {
            return g_earPlaybackSmoothingEnabled == 0;
        }
        if (control & 0x20020000) {
            g_earPlaybackSmoothingEnabled = control & 1;
        }
        return 1;
    }
    EarChannel *found;
    if (!g_ChannelRegistry.LookupWordPointerMapValue(static_cast<uint16_t>(channel), found)) {
        SetEarLastError(0x1b);
        return 0;
    }
    if ((control & 0x80000000) == 0x80000000) {
        return QueryChannelEvent(channel, control, found);
    }
    if ((control & 0x40000000) == 0x40000000) {
        uint16_t value = static_cast<uint16_t>(control);
        control &= 0xffff0000;
        if ((control & 0x40010000) == 0x40010000) {
            if (value == 0) {
                return found->events.GetCount();
            }
            found->maximumEvents = value;
        }
        if ((control & 0x40040000) == 0x40040000) {
            if (value == 0) {
                return found->startedEventCount;
            }
            found->maximumStartedEvents = value;
        }
        if ((control & 0x40020000) == 0x40020000) {
            return (uint32_t)(int64_t)((long double)found->gain * g_earChannelControlTimeUnitScale);
        }
    } else if ((control & 0x10000000) == 0x10000000) {
        uint32_t result;
        if (ControlCdAudio(control, &result)) {
            return result;
        }
    }
    if ((control & 0xc) == 0xc) {
        pauseChanged = found->paused == 1;
        found->paused = 0;
    } else if (control & 4) {
        pauseChanged = found->paused == 0;
        found->paused = 1;
    }
    if (found->paused) {
        g_earChannelControlResultFlags |= 4;
    }
    if ((control & 3) == 3) {
        muteChanged = found->muted == 1;
        found->muted = 0;
    } else if (control & 1) {
        muteChanged = found->muted == 0;
        found->muted = 1;
    }
    if (found->muted) {
        g_earChannelControlResultFlags |= 1;
    }
    if ((control & 0xf0) == 0xf0) {
        found->priorityLevel = 0;
    } else if ((control & 0x70) == 0x70) {
        found->priorityLevel = 100;
    } else if ((control & 0x30) == 0x30) {
        found->priorityLevel = 50;
    } else if (control & 0x10) {
        found->priorityLevel = 1;
    }
    if (found->priorityLevel == 100) {
        g_earChannelControlResultFlags |= 0x70;
    } else if (found->priorityLevel == 50) {
        g_earChannelControlResultFlags |= 0x30;
    } else if (found->priorityLevel == 1) {
        g_earChannelControlResultFlags |= 0x10;
    }
    if (control & 0x1000) {
        found->SetChannelSmoothingStep(static_cast<uint16_t>((control & 0xf000) >> 4));
    }
    if ((control & 0x300) == 0x300) {
        found->flag01 = 0;
    } else if (control & 0x100) {
        found->flag01 = 1;
    }
    if (found->flag01) {
        g_earChannelControlResultFlags |= 0x100;
    }
    if ((control & 0xc00) == 0xc00) {
        found->sequential = 0;
    } else if (control & 0x400) {
        found->sequential = 1;
    }
    if (found->sequential) {
        g_earChannelControlResultFlags |= 0x400;
    }
    if (!found->events.IsEmpty()) {
        POSITION position = found->events.GetStartPosition();
        do {
            WORD key;
            void *value;
            found->events.GetNextAssoc(position, key, value);
            ScheduledEarEvent *event = static_cast<ScheduledEarEvent *>(value);
            if (pauseChanged) {
                if (found->paused) {
                    event->PauseScheduledEventPlayback();
                } else {
                    event->ResumeScheduledEventPlayback();
                }
            }
            if (muteChanged) {
                if (found->muted) {
                    event->MuteScheduledEventPlayback();
                } else {
                    event->UnmuteScheduledEventPlayback();
                }
            }
        } while (position);
    }
    return g_earChannelControlResultFlags;
}
