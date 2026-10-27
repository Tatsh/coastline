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

private:
    [[maybe_unused]] std::uint8_t unknown0[0xc]; /* +0x00 */
    EarPlayback *playback;
    int playbackInitialized;
    [[maybe_unused]] std::uint8_t unknown14[0x18]; /* +0x14 */
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
