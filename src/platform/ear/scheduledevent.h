#ifndef COASTLINE_EAR_SCHEDULEDEVENT_H
#define COASTLINE_EAR_SCHEDULEDEVENT_H

#include <cstdint>

#include <windows.h>

class EarChannel;
class EarPlayback;

union EarEventSource {
    const void *address;
    std::uint16_t presetHandle;
};

class [[gnu::packed]] ScheduledEarEvent {
public:
    /** @ghidraAddress EAR: 0x100077e0 */
    ScheduledEarEvent(std::uint16_t flags, const void *source);
    /** @ghidraAddress EAR: 0x10007d6a */
    ~ScheduledEarEvent();

    /** @ghidraAddress EAR: 0x10007310 */
    int GetScheduledEventPlaybackStatus();
    /** @ghidraAddress EAR: 0x10007330 */
    int IsScheduledEventPlaybackComplete();
    /** @ghidraAddress EAR: 0x10007350 */
    void RequestScheduledEventStop();

    /** @ghidraAddress EAR: 0x10007380 */
    int CanStartScheduledEarEvent();
    /** @ghidraAddress EAR: 0x10007400 */
    void AdvanceScheduledEventTimers(std::uint32_t elapsedMilliseconds);
    /** @ghidraAddress EAR: 0x10008126 */
    int StartScheduledEarEvent();
    /** @ghidraAddress EAR: 0x100074c0 */
    void UpdateScheduledEventPlayback(double gain);
    /** @ghidraAddress EAR: 0x100088ea */
    void ApplyScheduledEventStopControl(std::uint16_t control);

    /**
     * @return The length of the media of the playback, in its own units.
     * @ghidraAddress EAR: 0x1000c1d0
     */
    std::uint32_t GetScheduledEventMediaLength();
    /** @ghidraAddress EAR: 0x1000c1f0 */
    void MuteScheduledEventPlayback();
    /** @ghidraAddress EAR: 0x1000c210 */
    void UnmuteScheduledEventPlayback();
    /** @ghidraAddress EAR: 0x1000c230 */
    void PauseScheduledEventPlayback();
    /**
     * @return Whether the playback resumed, which is always.
     * @ghidraAddress EAR: 0x1000c250
     */
    int ResumeScheduledEventPlayback();
    /**
     * @return Whether the playback is active.
     * @ghidraAddress EAR: 0x1000c520
     */
    int IsScheduledEventPlaybackActive();
    /**
     * @return Whether the playback is muted.
     * @ghidraAddress EAR: 0x1000c820
     */
    int IsScheduledEventPlaybackMuted();
    /**
     * @return Whether the event overrides the frequency scale.
     * @ghidraAddress EAR: 0x1000c840
     */
    int HasScheduledEventFrequencyOverride();
    /**
     * @return Whether the countdown of the lifetime is suppressed.
     * @ghidraAddress EAR: 0x1000c860
     */
    int IsScheduledEventPendingExpiryDisabled();
    /**
     * @return Whether the playback streams.
     * @ghidraAddress EAR: 0x1000c880
     */
    int IsScheduledEventPlaybackStreaming();
    /**
     * @return Whether the event is fading out.
     * @ghidraAddress EAR: 0x1000c8a0
     */
    int IsScheduledEventStopping();

private:
    // A channel compares the priority and the attenuation of its events directly.
    friend class EarChannel;

    /** @ghidraAddress EAR: 0x10007dfc */
    int InitializeScheduledEarEventPlayback();
    /** @ghidraAddress EAR: 0x100080e8 */
    void UpdateScheduledEventDopplerScale();
    /** @ghidraAddress EAR: 0x100081ec */
    long double CalculateScheduledEventDopplerScale();
    /** @ghidraAddress EAR: 0x1000854e */
    void DeactivateScheduledEarEvent();
    /** @ghidraAddress EAR: 0x10008832 */
    void ReleaseScheduledEventPlayback();
    /** @ghidraAddress EAR: 0x100085bb */
    int CreateEarPlaybackImplementation(std::uint16_t flags);

    EarEventSource source;
    std::uint16_t sourceType;
    std::uint16_t sourceFlags;
    char *allocatedSource;
    EarPlayback *playback;
    int playbackInitialized;
    int playbackStarted;
    int looping;
    int streaming;
    int frequencyScaleOverrideEnabled;
    int dopplerEnabled;
    int fadingOut;
    int lifetimeCountdownSuppressed;
    int activeDurationLimitEnabled;
    int attenuationRequired;
    int distanceLimitEnabled;
    int trackElapsedTime;
    std::uint32_t suppressionEnabled;
    std::uint16_t suppressionMarker;
    [[maybe_unused]] std::uint8_t unknown46[2]; /* +0x46 */
    std::uint16_t attenuation;
    std::uint16_t directionIndex;
    std::uint16_t matrixId;
    std::int32_t startDelayMs;
    std::int32_t remainingLifetimeMs;
    std::int32_t remainingActiveTimeMs;
    std::int32_t elapsedTimeMs;
    std::uint16_t attenuationDecrement;
    std::int32_t distance;
    std::int32_t maximumDistance;
    std::int32_t previousDistance;
    double previousRadialSpeed;
    DWORD previousDistanceTime;
    std::uint32_t accumulatedDistance;
    std::uint32_t accumulatedTime;
    std::uint32_t distanceSampleCount;
    int crossingActive;
    double crossingInitialSpeed;
    double crossingSpeedStep;
    double crossingStep;
    std::int32_t radialDirection;
    std::uint16_t environmentId;
    [[maybe_unused]] std::uint8_t unknownA6[2]; /* +0xa6 */
    double frequencyScale;
    double baseFrequencyScale;
    double startPosition;
    double backendParameter;
    std::uint32_t notificationFlags;
    [[maybe_unused]] std::uint8_t unknownCc[4]; /* +0xcc */
    std::uint32_t reservedEventControl;
    std::uint16_t priority;
    std::uint16_t notificationEvent;
};

#endif /* COASTLINE_EAR_SCHEDULEDEVENT_H */
