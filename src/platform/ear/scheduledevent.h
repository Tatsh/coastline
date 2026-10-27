#ifndef COASTLINE_EAR_SCHEDULEDEVENT_H
#define COASTLINE_EAR_SCHEDULEDEVENT_H

#include <cstdint>

class EarPlayback;

class [[gnu::packed]] ScheduledEarEvent {
public:
    /**
     * @ghidraAddress EAR: 0x100077e0
     * @stub
     */
    ScheduledEarEvent(std::uint16_t flags, const void *source);
    /**
     * @ghidraAddress EAR: 0x10007d6a
     * @stub
     */
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

private:
    /**
     * @ghidraAddress EAR: 0x10007dfc
     * @stub
     */
    int InitializeScheduledEarEventPlayback();
    /** @ghidraAddress EAR: 0x100080e8 */
    void UpdateScheduledEventDopplerScale();
    /**
     * @ghidraAddress EAR: 0x100081ec
     * @stub
     */
    long double CalculateScheduledEventDopplerScale();
    /**
     * @ghidraAddress EAR: 0x1000854e
     * @stub
     */
    void DeactivateScheduledEarEvent();

    [[maybe_unused]] std::uint8_t unknown0[0xc]; /* +0x00 */
    EarPlayback *playback;
    int playbackInitialized;
    int playbackStarted;
    [[maybe_unused]] std::uint8_t unknown18[8]; /* +0x18 */
    int frequencyScaleOverrideEnabled;
    int dopplerEnabled;
    [[maybe_unused]] std::uint8_t unknown28[4]; /* +0x28 */
    int lifetimeCountdownSuppressed;
    int activeDurationLimitEnabled;
    int attenuationRequired;
    int repeatLimitEnabled;
    int trackElapsedTime;
    [[maybe_unused]] std::uint8_t unknown40[8]; /* +0x40 */
    std::uint16_t attenuation;
    [[maybe_unused]] std::uint8_t unknown4a[4]; /* +0x4a */
    std::int32_t startDelayMs;
    std::int32_t remainingLifetimeMs;
    std::int32_t remainingActiveTimeMs;
    std::int32_t elapsedTimeMs;
    [[maybe_unused]] std::uint8_t unknown5e[2]; /* +0x5e */
    std::int32_t repeatCount;
    std::int32_t repeatLimit;
    [[maybe_unused]] std::uint8_t unknown68[0x70]; /* +0x68 */
};

#endif
