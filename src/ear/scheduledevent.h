#ifndef COASTLINE_EAR_SCHEDULEDEVENT_H
#define COASTLINE_EAR_SCHEDULEDEVENT_H

#include <cstdint>

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#include <windows.h>
#else
#include "common/types.h"
#endif

#include "event.h"
#include "playback.h"

class EarChannel;
class EarPlayback;

/** Source representation selected by the low byte of the scheduled event flags. */
union EarEventSource {
    /** Borrowed filename or media pointer for the selected playback implementation. */
    const void *address;
    /** Registered wave-sample handle when the source type is 3. */
    std::uint16_t presetHandle;
};

/**
 * Schedule playback with delayed starts, attenuation, spatial updates, and completion timers.
 * Playback methods require successful construction and an allocated playback implementation.
 * A twin event borrows source storage from its original event.
 */
class [[gnu::packed]] ScheduledEarEvent {
public:
    /**
     * Allocate playback for the source type and initialise scheduling defaults.
     * File-backed PCM and ADPCM paths without source flags 0x6000 are copied to 260-byte storage
     * and truncated to 259 bytes. Other source storage remains borrowed until destruction.
     * Failed allocation or an unsupported type records an EAR error and releases event resources.
     *
     * @param flags The source type in the low byte and playback flags in the high byte.
     * @param source The source representation required by the type, including a preset handle
     * encoded in pointer bits for type 3. File paths must be non-null, terminated strings.
     * @ghidraAddress EAR: 0x100077e0
     */
    ScheduledEarEvent(std::uint16_t flags, const void *source);
    /**
     * Create a twin with separate playback and borrowed source storage.
     * The original source storage must remain valid until the twin is destroyed.
     * Native construction does not initialise every scheduling field or report playback failure.
     *
     * @param other The original event with source storage valid throughout the twin's lifetime.
     * @ghidraAddress EAR: 0x10007b56
     */
    ScheduledEarEvent(const ScheduledEarEvent &other);
    /**
     * Release playback resources and source storage copied by this event.
     * Twins do not extend the lifetime of source storage allocated by the original event.
     * @ghidraAddress EAR: 0x10007d6a
     */
    ~ScheduledEarEvent();

    /**
     * @return The backend status value, with zero for unpaused playback and nonzero for paused.
     * @ghidraAddress EAR: 0x10007310
     */
    int GetScheduledEventPlaybackStatus();
    /**
     * @return Nonzero when the backend reports playback complete; otherwise zero.
     * @ghidraAddress EAR: 0x10007330
     */
    int IsScheduledEventPlaybackComplete();
    /**
     * Disable backend looping and mark playback complete for subsequent event retirement.
     * @ghidraAddress EAR: 0x10007350
     */
    void RequestScheduledEventStop();

    /**
     * @return Nonzero when playback is uninitialised, the delay has elapsed, and attenuation
     * and distance requirements permit a start; otherwise zero.
     * @ghidraAddress EAR: 0x10007380
     */
    int CanStartScheduledEarEvent();
    /**
     * Advance pending timers or the active duration limit and mark expired playback complete.
     * Timer arithmetic wraps at 32 bits. A pending lifetime of -1000 disables lifetime processing.
     * Other pending lifetimes expire below -500 milliseconds, even with countdown suppression.
     *
     * @param elapsedMilliseconds The unsigned elapsed time in milliseconds.
     * @ghidraAddress EAR: 0x10007400
     */
    void AdvanceScheduledEventTimers(std::uint32_t elapsedMilliseconds);
    /**
     * Initialise playback if necessary, apply smoothing and frequency, and start the backend.
     * Initialisation failure records EAR error 0x1a and deactivates the event.
     *
     * @return One on success; zero on initialisation failure, paused status, or backend failure.
     * @ghidraAddress EAR: 0x10008126
     */
    int StartScheduledEarEvent();
    /**
     * Advance fade attenuation and apply directional gain, frequency, and backend parameters.
     * Zero attenuation releases initialised playback resources when attenuation is required
     * and no stop fade is active.
     *
     * @param gain The linear gain multiplier applied after the event's 0..10000 attenuation.
     * @ghidraAddress EAR: 0x100074c0
     */
    void UpdateScheduledEventPlayback(double gain);
    /**
     * Select a stop fade, or disable looping and attempt a start when bit zero is set.
     * Fade attenuation decreases once per playback update rather than per millisecond.
     *
     * @param control Bit zero selects the non-looping start. Otherwise the low nibble selects
     * decrements of 500 (2), 250 (4), 125 (8), or 10000 / ((control >> 4) * 0.02) (14).
     * Other low nibbles select a zero decrement.
     * @ghidraAddress EAR: 0x100088ea
     */
    void ApplyScheduledEventStopControl(std::uint16_t control);

    /**
     * @return The media length in backend-specific units, without conversion to milliseconds.
     * @ghidraAddress EAR: 0x1000c1d0
     */
    std::uint32_t GetScheduledEventMediaLength();
    /**
     * Set backend mute without pausing event timers.
     * @ghidraAddress EAR: 0x1000c1f0
     */
    void MuteScheduledEventPlayback();
    /**
     * Clear backend mute without changing event timers.
     * @ghidraAddress EAR: 0x1000c210
     */
    void UnmuteScheduledEventPlayback();
    /**
     * Pause backend playback and discard the backend stop result.
     * @ghidraAddress EAR: 0x1000c230
     */
    void PauseScheduledEventPlayback();
    /**
     * Clear paused status and attempt to restart an active paused backend.
     *
     * @return One, including when the backend restart fails.
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
    // Mixing and moving an event set its fields from the parameters directly.
    friend std::uint32_t __stdcall EAR_DLL_MixEvent(const EarEventParams *params);
    friend std::uint32_t __stdcall EAR_DLL_MoveEvent(const EarEventParams *params);
    // The engine starts, updates, and retires the events of each channel.
    friend void __stdcall UpdateEarPlaybackEngine(void);

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
    std::uint16_t twinEventHandle;
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
