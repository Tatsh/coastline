#include "event.h"

#include <cstring>

#include "channel.h"
#include "earplayback.h"
#include "error.h"
#include "registry.h"
#include "runtime.h"
#include "scheduledevent.h"

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
