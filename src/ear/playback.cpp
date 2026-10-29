#include "playback.h"

#include <cmath>

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#include <windows.h>
// Multimedia declarations require the core Windows SDK types first.
#include <mmsystem.h>
#endif

#include "channel.h"
#include "ear/registry.h"
#include "ear/timer.h"
#include "error.h"
#include "runtime.h"
#include "scheduledevent.h"

namespace {
// @ghidraAddress EAR: 0x1003b49c
DWORD g_dwPlaybackElapsedTimeMs;
// @ghidraAddress EAR: 0x1003b644
POSITION g_pCurrentChannelMapPosition;
// @ghidraAddress EAR: 0x1003b3c4
WORD g_wCurrentUpdateChannelId;
// @ghidraAddress EAR: 0x1003b4d8
POSITION g_pCurrentEventMapPosition;

// The x87 compares of the original treat an unordered gain as equal to its target, so a gain that
// is not a number stops the ramp, and a gain that turns into one after a step is set to the target
// only when it falls.
void RampChannelGain(EarChannel *channel) {
    if (!channel->targetGainChanged) {
        return;
    }
    if (!std::islessgreater(channel->gain, channel->targetGain)) {
        channel->targetGainChanged = 0;
        return;
    }
    if (channel->gain < channel->targetGain) {
        channel->gain += channel->gainStep;
        if (std::isgreater(channel->gain, channel->targetGain)) {
            channel->gain = channel->targetGain;
        }
    } else {
        channel->gain -= channel->gainStep;
        if (!std::isgreaterequal(channel->gain, channel->targetGain)) {
            channel->gain = channel->targetGain;
        }
    }
}
} // namespace

void __stdcall UpdateEarPlaybackEngine(void) {
    if (g_bRegistryMutationLocked || g_bPlaybackUpdateLocked) {
        return;
    }
    g_bPlaybackUpdateLocked = 1;
    WORD eventHandle = 0;
    // The original clears a second handle that it never reads.
    [[maybe_unused]] WORD unusedHandle = 0;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    g_dwPlaybackUpdateTimeMs = timeGetTime();
#else
    g_dwPlaybackUpdateTimeMs = GetEarTimeMilliseconds();
#endif
    g_dwPlaybackElapsedTimeMs = g_dwPlaybackUpdateTimeMs - g_dwPreviousPlaybackUpdateTimeMs;
    g_dwPreviousPlaybackUpdateTimeMs = g_dwPlaybackUpdateTimeMs;
    if (g_ChannelRegistry.IsEmpty()) {
        g_bPlaybackUpdateLocked = 0;
        return;
    }
    g_pCurrentChannelMapPosition = g_ChannelRegistry.GetStartPosition();
    do {
        void *channelEntry;
        g_ChannelRegistry.GetNextAssoc(
            g_pCurrentChannelMapPosition, g_wCurrentUpdateChannelId, channelEntry);
        auto *channel = static_cast<EarChannel *>(channelEntry);
        RampChannelGain(channel);
        if (!channel->events.IsEmpty() && !channel->paused) {
            g_pCurrentEventMapPosition = channel->events.GetStartPosition();
            do {
                void *eventEntry;
                channel->events.GetNextAssoc(g_pCurrentEventMapPosition, eventHandle, eventEntry);
                auto *event = static_cast<ScheduledEarEvent *>(eventEntry);
                if (event->GetScheduledEventPlaybackStatus()) {
                    continue;
                }
                event->AdvanceScheduledEventTimers(g_dwPlaybackElapsedTimeMs);
                if (event->IsScheduledEventPlaybackComplete()) {
                    ScheduledEarEvent *twin;
                    if (event->suppressionEnabled &&
                        channel->events.LookupWordPointerMapValue(event->twinEventHandle, twin)) {
                        twin->RequestScheduledEventStop();
                    }
                    if (event->playbackStarted) {
                        --channel->startedEventCount;
                    }
                    delete event;
                    channel->events.RemoveKey(eventHandle);
                    if (channel->sequential && !channel->recentEvents.IsEmpty()) {
                        int remaining = channel->recentEvents.GetCount();
                        POSITION position = channel->recentEvents.GetHeadPosition();
                        while (remaining > 0 && position) {
                            POSITION current = position;
                            WORD handle = channel->recentEvents.GetNext(position);
                            // A removed handle does not count against the handles left.
                            if (eventHandle == handle) {
                                channel->recentEvents.RemoveAt(current);
                            } else {
                                --remaining;
                            }
                        }
                    }
                } else if (event->CanStartScheduledEarEvent()) {
                    if (channel->startedEventCount >= channel->maximumStartedEvents &&
                        !channel->EnsureChannelPlaybackCapacity(event->priority,
                                                                event->attenuation)) {
                        // The event is released but still started below.
                        SetEarLastError(0x26);
                        event->ReleaseScheduledEventPlayback();
                    }
                    if (!channel->sequential ||
                        (channel->sequential && !channel->recentEvents.IsEmpty() &&
                         eventHandle == channel->recentEvents.GetHead())) {
                        if (event->InitializeScheduledEarEventPlayback()) {
                            event->UpdateScheduledEventPlayback(channel->gain);
                            if (!event->lifetimeCountdownSuppressed) {
                                if (event->StartScheduledEarEvent()) {
                                    ++channel->startedEventCount;
                                } else {
                                    event->RequestScheduledEventStop();
                                }
                            }
                        }
                    }
                } else if (!channel->flag01 || channel->targetGainChanged || event->fadingOut) {
                    event->UpdateScheduledEventPlayback(channel->gain);
                    if (event->fadingOut && !event->attenuation) {
                        event->RequestScheduledEventStop();
                    }
                }
            } while (g_pCurrentEventMapPosition);
        }
        if (channel->shutdownRequested && !channel->targetGainChanged) {
            g_bPlaybackUpdateLocked = 0;
            EarShutDownChannelExportWrapper(g_wCurrentUpdateChannelId);
            g_bPlaybackUpdateLocked = 1;
        }
    } while (g_pCurrentChannelMapPosition);
    g_bPlaybackUpdateLocked = 0;
}
