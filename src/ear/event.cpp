#include "event.h"

#include <cstdlib>
#include <cstring>
#include <new>

#include "channel.h"
#include "direction.h"
#include "directionalmatrix.h"
#include "distance.h"
#include "environment.h"
#include "error.h"
#include "runtime.h"
#include "scheduledevent.h"
#include "win32/ear/earplayback.h"
#include "win32/ear/registry.h"

namespace {
// @ghidraAddress EAR: 0x1002ebf0
const double g_earMixEventAttenuationScale = 0.0001;
// @ghidraAddress EAR: 0x1002ebf8
const double g_earMoveEventAttenuationScale = 0.0001;

// The frequency scale, in units of its parameter, that a pitch varies around.
constexpr int EAR_EVENT_UNIT_PITCH = 10000;
// The value of EarEventParams::backendParameter that stands for -1.
constexpr std::uint16_t EAR_EVENT_NEGATIVE_BACKEND_PARAMETER = 0xffff;
// The distance unit of EarEventParams::distanceUnit that is already centimetres.
constexpr std::uint32_t EAR_EVENT_CENTIMETERS = 4;
// The source type whose reserved control comes from the parameters.
constexpr std::uint16_t EAR_EVENT_RESERVED_CONTROL_SOURCE = 6;
// The source type that plays without a source.
constexpr std::uint16_t EAR_EVENT_SOURCELESS = 0x1e;
} // namespace

/** The size EarEventParams::size must give. */
#define EAR_EVENT_PARAMS_SIZE 0x80

/** The kind of the target of EAR_DLL_ShutDownEvent in the high word of its control. */
enum EarShutDownEventTarget {
    EAR_SHUT_DOWN_EVENT_PARAMS = 0, /*!< The target is the parameters of the event. */
    EAR_SHUT_DOWN_EVENT_HANDLE = 1, /*!< The target is the handle of the event. */
};

uint16_t GenerateEarObjectHandle(void) {
    // @ghidraAddress EAR: 0x1003b8c8
    static WORD channel;
    for (;;) {
        ++g_wEarObjectHandleCounter;
        if (g_wEarObjectHandleCounter == 0) {
            ++g_wEarObjectHandleCounter;
        }
        if (g_ActiveEventRegistry.IsEmpty()) {
            return g_wEarObjectHandleCounter;
        }
        if (!g_ActiveEventRegistry.Lookup(g_wEarObjectHandleCounter, channel)) {
            return g_wEarObjectHandleCounter;
        }
    }
}

/**
 * Copy the parameters of an event once their size is right.
 *
 * @return Whether the size was right.
 */
static int CopyEarEventParams(const EarEventParams *params, EarEventParams *copy) {
    uint32_t size;
    // Only the low word of the size is read; the high word stays as the stack left it.
    std::memmove(&size, params, sizeof(params->size));
    if ((size & 0xffff) != EAR_EVENT_PARAMS_SIZE) {
        SetEarLastError(0x1d);
        return 0;
    }
    std::memmove(copy, params, EAR_EVENT_PARAMS_SIZE);
    return 1;
}

uint32_t __stdcall EarStartEventExportWrapper(const EarEventParams *params) {
    return EAR_DLL_StartEvent(params);
}

uint32_t __stdcall EAR_DLL_StartEvent(const EarEventParams *params) {
    SetEarLastError(0);
    if (!IsEarReady()) {
        return 0;
    }
    EarEventParams copy;
    if (!CopyEarEventParams(params, &copy)) {
        return 0;
    }
    if (copy.eventHandle == 0) {
        return 0;
    }
    EarChannel *channel;
    if (!g_ChannelRegistry.LookupWordPointerMapValue(copy.channelHandle, channel)) {
        SetEarLastError(0x1b);
        return 0;
    }
    ScheduledEarEvent *event;
    if (!channel->events.LookupWordPointerMapValue(static_cast<uint16_t>(copy.eventHandle),
                                                   event)) {
        return 0;
    }
    event->UpdateScheduledEventPlayback(channel->gain);
    if (!event->StartScheduledEarEvent()) {
        SetEarLastError(0x14);
        return 0;
    }
    ++channel->startedEventCount;
    return copy.eventHandle;
}

uint32_t __stdcall EarShutDownEventExportWrapper(uintptr_t target, uint32_t control) {
    return EAR_DLL_ShutDownEvent(target, control);
}

uint32_t __stdcall EAR_DLL_ShutDownEvent(uintptr_t target, uint32_t control) {
    SetEarLastError(0);
    if (!IsEarReady()) {
        return 0;
    }
    uint16_t stopControl = static_cast<uint16_t>(control);
    uint16_t kind = static_cast<uint16_t>(control >> 16);
    uint32_t handle = 0;
    WORD channelHandle = 0;
    if (kind == EAR_SHUT_DOWN_EVENT_HANDLE) {
        handle = static_cast<uint32_t>(target);
        if (g_ActiveEventRegistry.IsEmpty() ||
            !g_ActiveEventRegistry.Lookup(handle, channelHandle)) {
            return 0;
        }
    } else if (kind == EAR_SHUT_DOWN_EVENT_PARAMS) {
        EarEventParams copy;
        if (!CopyEarEventParams(reinterpret_cast<const EarEventParams *>(target), &copy)) {
            return 0;
        }
        handle = copy.eventHandle;
        channelHandle = copy.channelHandle;
    }
    EarChannel *channel;
    if (!g_ChannelRegistry.LookupWordPointerMapValue(channelHandle, channel)) {
        SetEarLastError(0x1b);
        return 0;
    }
    ScheduledEarEvent *event;
    if (!channel->events.LookupWordPointerMapValue(static_cast<uint16_t>(handle), event)) {
        return 0;
    }
    if (stopControl) {
        event->ApplyScheduledEventStopControl(stopControl);
    } else {
        event->RequestScheduledEventStop();
    }
    if (stopControl) {
        return handle;
    }
    g_ActiveEventRegistry.RemoveKey(handle);
    return 0;
}

uint32_t __stdcall EarGetEventStatusExportWrapper(uint32_t channel, uint32_t event) {
    return EAR_DLL_GetEventStatus(channel, event);
}

uint32_t __stdcall EAR_DLL_GetEventStatus(uint32_t channel, uint32_t event) {
    SetEarLastError(0);
    uint32_t status = 0;
    while (g_bPlaybackUpdateLocked) {
    }
    g_bPlaybackUpdateLocked = 1;
    if (IsEarReady()) {
        EarChannel *found;
        if (g_ChannelRegistry.IsEmpty() ||
            !g_ChannelRegistry.LookupWordPointerMapValue(static_cast<uint16_t>(channel), found)) {
            SetEarLastError(0x1b);
        } else {
            if (event == 0 && !found->recentEvents.IsEmpty()) {
                event = found->recentEvents.GetHead();
            }
            ScheduledEarEvent *scheduled;
            if (found->events.LookupWordPointerMapValue(static_cast<uint16_t>(event), scheduled)) {
                if (scheduled->IsScheduledEventPlaybackActive()) {
                    status |= 4;
                }
                if (scheduled->IsScheduledEventPlaybackMuted()) {
                    status |= 1;
                }
                if (scheduled->GetScheduledEventPlaybackStatus()) {
                    status |= 2;
                }
                if (scheduled->IsScheduledEventPlaybackStreaming()) {
                    status |= 0x20;
                }
                if (scheduled->HasScheduledEventFrequencyOverride()) {
                    status |= 0x10;
                }
                if (scheduled->IsScheduledEventPendingExpiryDisabled()) {
                    status |= 0x40;
                }
                if (found->sequential) {
                    status |= 8;
                }
                if (scheduled->IsScheduledEventStopping()) {
                    status |= 0x80;
                }
            }
        }
    }
    g_bPlaybackUpdateLocked = 0;
    return status;
}

uint32_t __stdcall EarMixEventExportWrapper(const EarEventParams *params) {
    return EAR_DLL_MixEvent(params);
}

uint32_t __stdcall EAR_DLL_MixEvent(const EarEventParams *params) {
    SetEarLastError(0);
    if (!IsEarReady()) {
        return 0;
    }
    EarEventParams copy;
    if (!CopyEarEventParams(params, &copy)) {
        return 0;
    }
    if (copy.maximumDistance == 0) {
        ++copy.maximumDistance;
    }
    uint16_t sourceType = copy.sourceType & 0xff;
    // The original splits off the flags of the source and never reads them.
    [[maybe_unused]] uint16_t sourceFlags = copy.sourceType & 0xff00;
    if (!copy.source && sourceType != EAR_EVENT_SOURCELESS) {
        SetEarLastError(0x1d);
        return 0;
    }
    EarChannel *channel;
    if (!g_ChannelRegistry.LookupWordPointerMapValue(copy.channelHandle, channel)) {
        SetEarLastError(0x1b);
        return 0;
    }
    void *storage = MfcAllocateWithNewHandler(sizeof(ScheduledEarEvent));
    ScheduledEarEvent *event =
        storage ? new (storage) ScheduledEarEvent(copy.sourceType, copy.source) : nullptr;
    if (!event) {
        SetEarLastError(0x10);
        return 0;
    }
    event->notificationEvent = GenerateEarObjectHandle();
    if (copy.flags & EAR_EVENT_OMNIDIRECTIONAL) {
        copy.matrixHandle = 1;
        copy.direction = 4;
    }
    EarPresetTable *matrix;
    EarEnvironment *environment;
    uint32_t distance;
    if (!g_MatrixRegistry.LookupWordPointerMapValue(copy.matrixHandle, matrix)) {
        SetEarLastError(0x27);
        goto fail;
    }
    event->matrixId = copy.matrixHandle;
    if (!g_EnvironmentRegistry.LookupWordPointerMapValue(copy.environmentHandle, environment)) {
        SetEarLastError(0x1f);
        goto fail;
    }
    // The handle is copied with the two bytes after it, as one word.
    event->environmentId = copy.environmentHandle;
    std::memcpy(event->unknownA6, copy.unknown3a, sizeof(event->unknownA6));
    if (copy.distanceUnit != EAR_EVENT_CENTIMETERS) {
        ConvertPlayDistancesToCentimeters(&copy);
    }
    distance = static_cast<uint32_t>(CalculateRoundedPlanarDistance(&copy));
    event->attenuationRequired = (copy.flags & EAR_EVENT_ATTENUATION_REQUIRED) != 0;
    event->trackElapsedTime = (copy.flags & EAR_EVENT_TRACK_ELAPSED) != 0;
    event->distanceLimitEnabled = (copy.flags & EAR_EVENT_DISTANCE_LIMIT) != 0;
    if (event->distanceLimitEnabled) {
        event->maximumDistance = copy.limitDistance;
    }
    if (copy.flags & EAR_EVENT_DELAY_BY_DISTANCE) {
        // An event beyond its distance with no gain there is dropped without an error.
        if (distance > copy.maximumDistance && !copy.minimumGain && !event->attenuationRequired) {
            goto fail;
        }
        event->startDelayMs =
            static_cast<int32_t>(distance / environment->GetEnvironmentSoundSpeed());
        event->distance = static_cast<int32_t>(distance);
    }
    // Without its own delay, the delay of the distance is dropped.
    if (copy.flags & EAR_EVENT_START_DELAY) {
        event->startDelayMs += copy.startDelay;
    } else {
        event->startDelayMs = 0;
    }
    if (copy.flags & EAR_EVENT_LIFETIME) {
        event->remainingLifetimeMs = copy.lifetime + event->startDelayMs;
    } else {
        event->remainingLifetimeMs = -1000;
    }
    if (copy.flags & EAR_EVENT_ACTIVE_TIME) {
        event->remainingActiveTimeMs = copy.activeTime;
        event->activeDurationLimitEnabled = 1;
    }
    if (copy.flags & EAR_EVENT_NOTIFY_START) {
        event->notificationFlags |= 1;
    }
    if (copy.flags & EAR_EVENT_NOTIFY_STOP) {
        event->notificationFlags |= 2;
    }
    event->attenuation = CalculateDistanceAttenuation(distance, &copy);
    event->directionIndex = CalculateDirectionalPresetIndex(distance, event->matrixId, &copy);
    event->looping = (copy.flags & EAR_EVENT_LOOPING) != 0;
    event->streaming = (copy.flags & EAR_EVENT_NOT_STREAMING) == 0;
    event->dopplerEnabled = (copy.flags & EAR_EVENT_DOPPLER) != 0;
    event->frequencyScaleOverrideEnabled = (copy.flags & EAR_EVENT_FREQUENCY_SCALE) != 0;
    if (event->frequencyScaleOverrideEnabled) {
        event->frequencyScale =
            static_cast<long double>(copy.frequencyScale) * g_earMixEventAttenuationScale;
    }
    if (copy.flags & EAR_EVENT_BASE_FREQUENCY) {
        event->baseFrequencyScale =
            static_cast<long double>(copy.baseFrequencyScale) * g_earMixEventAttenuationScale;
    }
    if (copy.flags & EAR_EVENT_PITCH_VARIATION) {
        if (!copy.pitchVariation) {
            copy.pitchVariation = environment->GetEnvironmentPitchVariationRange();
        }
        int pitch =
            std::rand() % (copy.pitchVariation * 2) + (EAR_EVENT_UNIT_PITCH - copy.pitchVariation);
        event->baseFrequencyScale = static_cast<long double>(pitch) * g_earMixEventAttenuationScale;
    }
    if (copy.flags & EAR_EVENT_BACKEND_PARAMETER) {
        if (copy.backendParameter == EAR_EVENT_NEGATIVE_BACKEND_PARAMETER) {
            event->backendParameter = -1;
        } else {
            event->backendParameter =
                static_cast<long double>(copy.backendParameter) * g_earMixEventAttenuationScale;
        }
    } else {
        event->backendParameter = 0;
    }
    if (copy.flags & EAR_EVENT_START_POSITION) {
        event->startPosition =
            static_cast<long double>(copy.startPosition) * g_earMixEventAttenuationScale;
    }
    event->lifetimeCountdownSuppressed = (copy.flags & EAR_EVENT_SUPPRESS_LIFETIME) != 0;
    if (sourceType == EAR_EVENT_RESERVED_CONTROL_SOURCE) {
        event->lifetimeCountdownSuppressed = 1;
        event->reservedEventControl = copy.reservedControl;
    }
    if (channel->muted || (copy.flags & EAR_EVENT_MUTED)) {
        event->MuteScheduledEventPlayback();
    }
    if (channel->paused || (copy.flags & EAR_EVENT_PAUSED)) {
        event->PauseScheduledEventPlayback();
    }
    event->suppressionEnabled = (copy.flags & EAR_EVENT_TWIN) != 0;
    if (copy.flags & EAR_EVENT_PRIORITY) {
        event->priority = copy.priority;
    }
    while (g_bPlaybackUpdateLocked) {
    }
    g_bPlaybackUpdateLocked = 1;
    if (channel->events.GetCount() >= channel->maximumEvents &&
        !channel->EnsureChannelPlaybackCapacity(event->priority, event->attenuation)) {
        SetEarLastError(0x26);
        g_bPlaybackUpdateLocked = 0;
        goto fail;
    }
    channel->events.SetWordPointerMapValue(event->notificationEvent, event);
    if (event->suppressionEnabled) {
        void *twinStorage = MfcAllocateWithNewHandler(sizeof(ScheduledEarEvent));
        // The original does not check the allocation of the twin.
        ScheduledEarEvent *twin =
            twinStorage ? new (twinStorage) ScheduledEarEvent(*event) : nullptr;
        twin->notificationEvent = GenerateEarObjectHandle();
        twin->twinEventHandle = event->notificationEvent;
        event->twinEventHandle = twin->notificationEvent;
        channel->events.SetWordPointerMapValue(event->notificationEvent, event);
        channel->events.SetWordPointerMapValue(twin->notificationEvent, twin);
    }
    if (channel->sequential) {
        uint16_t mode = static_cast<uint16_t>(copy.queueControl);
        uint16_t target = static_cast<uint16_t>(copy.queueControl >> 16);
        if (channel->recentEvents.IsEmpty()) {
            channel->recentEvents.AddHead(event->notificationEvent);
        } else if (mode == EAR_EVENT_QUEUE_NEXT || mode == EAR_EVENT_QUEUE_REPLACE) {
            ScheduledEarEvent *current;
#ifdef ENABLE_PATCHES
            if (!channel->events.LookupWordPointerMapValue(channel->recentEvents.GetHead(),
                                                           current) ||
                !current->IsScheduledEventPlaybackActive()) {
#else
            // The result of the lookup is not read.
            channel->events.LookupWordPointerMapValue(channel->recentEvents.GetHead(), current);
            if (!current->IsScheduledEventPlaybackActive()) {
#endif /* ENABLE_PATCHES */
                channel->recentEvents.InsertBefore(channel->recentEvents.GetHeadPosition(),
                                                   event->notificationEvent);
            } else {
                channel->recentEvents.InsertAfter(channel->recentEvents.GetHeadPosition(),
                                                  event->notificationEvent);
                if (mode == EAR_EVENT_QUEUE_REPLACE) {
                    current->RequestScheduledEventStop();
                }
            }
        } else if (mode == EAR_EVENT_QUEUE_BEFORE_EVENT || EAR_EVENT_QUEUE_AFTER_EVENT != 0) {
            // The original tests the constant of the last mode, so every other mode searches.
            POSITION position = channel->recentEvents.GetHeadPosition();
            POSITION previous = position;
            int found = 0;
            do {
                uint16_t handle = channel->recentEvents.GetNext(position);
                if (handle == target) {
                    found = 1;
                    ScheduledEarEvent *targetEvent;
#ifdef ENABLE_PATCHES
                    int targetFound =
                        channel->events.LookupWordPointerMapValue(handle, targetEvent);
#else
                    channel->events.LookupWordPointerMapValue(handle, targetEvent);
#endif /* ENABLE_PATCHES */
                    if (mode == EAR_EVENT_QUEUE_BEFORE_EVENT) {
#ifdef ENABLE_PATCHES
                        if (targetFound && targetEvent->IsScheduledEventPlaybackActive()) {
#else
                        if (targetEvent->IsScheduledEventPlaybackActive()) {
#endif /* ENABLE_PATCHES */
                            mode = EAR_EVENT_QUEUE_AFTER_EVENT;
                        } else {
                            channel->recentEvents.InsertBefore(previous, event->notificationEvent);
                        }
                    }
                    if (mode == EAR_EVENT_QUEUE_AFTER_EVENT) {
                        channel->recentEvents.InsertAfter(previous, event->notificationEvent);
                    }
                }
                previous = position;
            } while (!found && position);
            if (!found) {
                mode = EAR_EVENT_QUEUE_TAIL;
            }
        }
        // The first event of an empty list is added a second time in this mode.
        if (mode == EAR_EVENT_QUEUE_TAIL) {
            channel->recentEvents.AddTail(event->notificationEvent);
        }
    }
    g_ActiveEventRegistry.SetAt(event->notificationEvent, copy.channelHandle);
    g_bPlaybackUpdateLocked = 0;
    return event->notificationEvent;

fail:
    if (event) {
        event->~ScheduledEarEvent();
        ::operator delete(event);
    }
    return 0;
}

uint32_t __stdcall EarMoveEventExportWrapper(const EarEventParams *params) {
    return EAR_DLL_MoveEvent(params);
}

uint32_t __stdcall EAR_DLL_MoveEvent(const EarEventParams *params) {
    // @ghidraAddress EAR: 0x1003b8d8
    static EarEventParams copy;
    // @ghidraAddress EAR: 0x1003b958
    static EarChannel *channel;
    // @ghidraAddress EAR: 0x1003b8d0
    static ScheduledEarEvent *event;
    // @ghidraAddress EAR: 0x1003b960
    static EarEnvironment *environment;
    // @ghidraAddress EAR: 0x1003b968
    static int32_t distance;
    SetEarLastError(0);
    if (!IsEarReady()) {
        return 0;
    }
    if (!CopyEarEventParams(params, &copy)) {
        return 0;
    }
    if (copy.eventHandle == 0) {
        SetEarLastError(0x1c);
        return 0;
    }
    if (copy.maximumDistance == 0) {
        ++copy.maximumDistance;
    }
    if (!g_ChannelRegistry.LookupWordPointerMapValue(copy.channelHandle, channel)) {
        SetEarLastError(0x1b);
        return 0;
    }
    if (copy.flags & EAR_EVENT_OMNIDIRECTIONAL) {
        copy.matrixHandle = 1;
        copy.direction = 4;
    }
    while (g_bPlaybackUpdateLocked) {
    }
    g_bPlaybackUpdateLocked = 1;
    if (!channel->events.LookupWordPointerMapValue(static_cast<uint16_t>(copy.eventHandle),
                                                   event)) {
        g_bPlaybackUpdateLocked = 0;
        return 0;
    }
    if (event->matrixId != copy.matrixHandle) {
        EarPresetTable *matrix;
        if (!g_MatrixRegistry.LookupWordPointerMapValue(copy.matrixHandle, matrix)) {
            SetEarLastError(0x27);
            g_bPlaybackUpdateLocked = 0;
            return 0;
        }
        event->matrixId = copy.matrixHandle;
    }
    if (!g_EnvironmentRegistry.LookupWordPointerMapValue(copy.environmentHandle, environment)) {
        SetEarLastError(0x1f);
        g_bPlaybackUpdateLocked = 0;
        return 0;
    }
    if (copy.flags & EAR_EVENT_PAUSED) {
        event->PauseScheduledEventPlayback();
    } else if (event->GetScheduledEventPlaybackStatus() && !channel->paused) {
        event->ResumeScheduledEventPlayback();
    }
    if ((copy.flags & EAR_EVENT_MUTED) || channel->muted) {
        event->MuteScheduledEventPlayback();
    } else if (event->IsScheduledEventPlaybackMuted() && !channel->muted) {
        event->UnmuteScheduledEventPlayback();
    }
    if (copy.flags & EAR_EVENT_PITCH_VARIATION) {
        if (!copy.pitchVariation) {
            copy.pitchVariation = environment->GetEnvironmentPitchVariationRange();
        }
        int pitch =
            std::rand() % (copy.pitchVariation * 2) + (EAR_EVENT_UNIT_PITCH - copy.pitchVariation);
        event->baseFrequencyScale =
            static_cast<long double>(pitch) * g_earMoveEventAttenuationScale;
    }
    if (copy.flags & EAR_EVENT_BACKEND_PARAMETER) {
        if (copy.backendParameter == EAR_EVENT_NEGATIVE_BACKEND_PARAMETER) {
            event->backendParameter = -1;
        } else {
            event->backendParameter =
                static_cast<long double>(copy.backendParameter) * g_earMoveEventAttenuationScale;
        }
    } else {
        event->backendParameter = 0;
    }
    if (copy.flags & EAR_EVENT_PRIORITY) {
        event->priority = copy.priority;
    }
    if (copy.flags & EAR_EVENT_ACTIVE_TIME) {
        if (!event->activeDurationLimitEnabled || copy.activeTime < event->remainingActiveTimeMs) {
            event->remainingActiveTimeMs = copy.activeTime;
            event->activeDurationLimitEnabled = 1;
        }
    } else {
        event->activeDurationLimitEnabled = 0;
    }
    if (copy.distanceUnit != EAR_EVENT_CENTIMETERS) {
        ConvertPlayDistancesToCentimeters(&copy);
    }
    distance = CalculateRoundedPlanarDistance(&copy);
    if ((copy.flags & EAR_EVENT_DELAY_BY_DISTANCE) && event->distance != distance &&
        event->distance) {
        // The delay follows the time the sound takes over the change of the distance.
        event->startDelayMs -= (event->distance - distance) /
                               static_cast<int>(environment->GetEnvironmentSoundSpeed());
    }
    event->frequencyScaleOverrideEnabled = (copy.flags & EAR_EVENT_FREQUENCY_SCALE) != 0;
    if (event->frequencyScaleOverrideEnabled) {
        event->frequencyScale =
            static_cast<long double>(copy.frequencyScale) * g_earMoveEventAttenuationScale;
    }
    event->dopplerEnabled = (copy.flags & EAR_EVENT_DOPPLER) != 0;
    event->distance = distance;
    if (!event->fadingOut) {
        event->attenuation = CalculateDistanceAttenuation(static_cast<uint32_t>(distance), &copy);
    }
    event->directionIndex =
        CalculateDirectionalPresetIndex(static_cast<uint32_t>(distance), event->matrixId, &copy);
    g_bPlaybackUpdateLocked = 0;
    return copy.eventHandle;
}
