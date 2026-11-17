#ifndef COASTLINE_EAR_SCHEDULEDEVENT_H
#define COASTLINE_EAR_SCHEDULEDEVENT_H

/**
 * @file
 * The scheduled event of an EAR channel. An event delays, fades, positions, and retires one
 * playback implementation.
 */

#include <cstdint>

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#include <windows.h>
#else
#include "common/types.h"
#endif

#include "event.h"
#include "playback.h"

#pragma pack(push, 1)

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
class ScheduledEarEvent {
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
     * Read the paused status of the playback implementation.
     *
     * @return The backend status value, with zero for unpaused playback and nonzero for paused.
     * @ghidraAddress EAR: 0x10007310
     */
    int GetScheduledEventPlaybackStatus();
    /**
     * Test whether the event is ready for retirement by the playback engine.
     *
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
     * Test whether the playback engine may initialise and start a pending event.
     *
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
     * Read the media length reported by the playback implementation.
     *
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
     * Read the active flag of the playback implementation.
     *
     * @return Nonzero while the backend reports the playback active; otherwise zero.
     * @ghidraAddress EAR: 0x1000c520
     */
    int IsScheduledEventPlaybackActive();
    /**
     * Read the mute flag of the playback implementation.
     *
     * @return Nonzero while the backend playback is muted; otherwise zero.
     * @ghidraAddress EAR: 0x1000c820
     */
    int IsScheduledEventPlaybackMuted();
    /**
     * Read the frequency-scale override flag set by mixing or moving the event.
     *
     * @return Nonzero when the event applies frequencyScale; otherwise zero.
     * @ghidraAddress EAR: 0x1000c840
     */
    int HasScheduledEventFrequencyOverride();
    /**
     * Read the flag that stops the countdown of the pending lifetime.
     *
     * @return Nonzero when the lifetime countdown is suppressed; otherwise zero.
     * @ghidraAddress EAR: 0x1000c860
     */
    int IsScheduledEventPendingExpiryDisabled();
    /**
     * Read the streaming flag of the playback implementation, not the cached event flag.
     *
     * @return Nonzero when the backend streams its media; otherwise zero.
     * @ghidraAddress EAR: 0x1000c880
     */
    int IsScheduledEventPlaybackStreaming();
    /**
     * Read the flag set by a stop control that selects a fade.
     *
     * @return Nonzero while the event fades out; otherwise zero.
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

    // Pass the scheduling fields to the playback implementation and initialise its media.
    // An event mixed with a twin takes the environment's initial gain, matrix 1, and looping
    // streaming playback. The original uses direction 4, and the twin uses direction 8, a start
    // position of 0.5, and a base frequency scale of 0.96. A preset source requires a registered
    // preset. Doppler adds 50 milliseconds to the start delay and records the current time.
    // Returns one on success, and zero after an unregistered preset (EAR error 0x29) or a backend
    // initialisation failure. Both failures deactivate the event.
    // @ghidraAddress EAR: 0x10007dfc
    int InitializeScheduledEarEventPlayback();
    // Apply the result of CalculateScheduledEventDopplerScale() to the playback when the event
    // overrides the frequency scale or uses Doppler.
    // @ghidraAddress EAR: 0x100080e8
    void UpdateScheduledEventDopplerScale();
    // Calculate the frequency scale from the change of distance over time and the sound speed of
    // the environment. A distance within the environment's crossover distance ramps the radial
    // speed down linearly through zero instead of reversing it at once. Returns frequencyScale
    // divided by one minus the radial speed over the sound speed. frequencyScale is returned
    // unchanged without Doppler, on the first sample, for an unregistered environment (EAR error
    // 0x1f), for a zero average time, or for a zero or NaN denominator.
    // @ghidraAddress EAR: 0x100081ec
    long double CalculateScheduledEventDopplerScale();
    // Remove the event from the active registry when the registry is not empty, and mark the
    // playback complete. Initialised streaming playback completes through the backend instead.
    // @ghidraAddress EAR: 0x1000854e
    void DeactivateScheduledEarEvent();
    // Free the playback of a silent event for a later restart, or stop an event that does not
    // require attenuation. An event that requires attenuation is changed only when its
    // attenuation is zero and its playback is initialised. Its playback is then destroyed and
    // replaced by a new unstarted implementation. Allocation failure records EAR error 0x10 and
    // destroys the event.
    // @ghidraAddress EAR: 0x10008832
    void ReleaseScheduledEventPlayback();
    // Allocate the playback implementation for sourceType into playback. Type 0 is a PCM wave
    // stream, 2 a memory wave stream, 3 a preset sample, 31 an IMA ADPCM wave stream, and on
    // Windows 6 is MIDI and 30 is CD audio. Returns one on success, and zero for an unsupported
    // type (with playback cleared) and in patched builds for an allocation failure.
    // @ghidraAddress EAR: 0x100085bb
    int CreateEarPlaybackImplementation(std::uint16_t flags);

    EarEventSource source;
    std::uint16_t sourceType;  // Low byte of the construction flags.
    std::uint16_t sourceFlags; // Playback flags, the high byte of the construction flags.
    // 260-byte copy of a file path, freed by the destructor except in a twin.
    char *allocatedSource;
    EarPlayback *playback;   // Destroyed with the event.
    int playbackInitialized; // Nonzero once the playback initialised its media.
    int playbackStarted;     // Nonzero once the backend started; counted by the channel.
    int looping;
    int streaming;
    // Nonzero when frequencyScale replaces the frequency scale of the playback.
    int frequencyScaleOverrideEnabled;
    int dopplerEnabled; // Nonzero when the frequency scale follows the radial speed.
    int fadingOut;      // Nonzero after a stop control selected a fade.
    // Nonzero to stop the countdown of remainingLifetimeMs and to defer the engine's start.
    int lifetimeCountdownSuppressed;
    // Nonzero when remainingActiveTimeMs limits the time the playback remains active.
    int activeDurationLimitEnabled;
    // Nonzero when a zero attenuation frees the playback in place of stopping the event.
    int attenuationRequired;
    int distanceLimitEnabled; // Nonzero when maximumDistance blocks the start.
    int trackElapsedTime;     // Nonzero to accumulate elapsedTimeMs while pending.
    // Nonzero for an event mixed with a twin; read with suppressionMarker by the playback.
    std::uint32_t suppressionEnabled;
    std::uint16_t suppressionMarker; // One in a twin and zero in its original.
    std::uint16_t twinEventHandle;   // Channel handle of the paired twin event.
    std::uint16_t attenuation;       // Gain from 0 to 10000, where 10000 is unity.
    std::uint16_t directionIndex;    // Entry of the directional matrix for the gain.
    std::uint16_t matrixId;          // Handle of the directional matrix, 16 by default.
    std::int32_t startDelayMs;       // Milliseconds before the event may start.
    // Milliseconds before a pending event expires, or -1000 for no lifetime.
    std::int32_t remainingLifetimeMs;
    // Milliseconds of active playback remaining when activeDurationLimitEnabled is set.
    std::int32_t remainingActiveTimeMs;
    // Milliseconds spent pending, passed to the playback as its start time.
    std::int32_t elapsedTimeMs;
    // Attenuation removed by each playback update during a fade.
    std::uint16_t attenuationDecrement;
    std::int32_t distance;             // Distance from the listener at the last update.
    std::int32_t maximumDistance;      // Distance at or beyond which the event cannot start.
    std::int32_t previousDistance;     // Distance of the previous Doppler sample.
    double previousRadialSpeed;        // Radial speed of the previous Doppler sample.
    DWORD previousDistanceTime;        // Time of the previous Doppler sample, in ms.
    std::uint32_t accumulatedDistance; // Sum of the distance changes of every sample.
    std::uint32_t accumulatedTime;     // Sum of the sample intervals in milliseconds.
    std::uint32_t distanceSampleCount; // Divisor of the Doppler averages.
    int crossingActive;           // Nonzero while the radial speed ramps through the crossover.
    double crossingInitialSpeed;  // Radial speed when the crossover ramp began.
    double crossingSpeedStep;     // Radial speed removed at each step of the ramp.
    double crossingStep;          // Step count of the ramp, from one.
    std::int32_t radialDirection; // -1 when the distance grows, 1 when it shrinks.
    std::uint16_t environmentId;
    // The two bytes after the environment handle of the mix parameters, copied by mixing and
    // by the twin constructor and never read.
    [[maybe_unused]] std::uint8_t unknownA6[2]; /* +0xa6 */
    double frequencyScale;                      // Frequency scale of an override, before Doppler.
    double baseFrequencyScale;                  // Given to the playback.
    double startPosition;                       // Given to the playback.
    double backendParameter;                    // Given to the playback, or -1.
    // Bit 0 for the start and bit 1 for the stop.
    std::uint32_t notificationFlags;
    // Cleared by the main constructor and never read.
    [[maybe_unused]] std::uint8_t unknownCc[4]; /* +0xcc */
    // Reserved control of a source of type 6, given to the playback.
    std::uint32_t reservedEventControl;
    // A lower nonzero value is released first for capacity.
    std::uint16_t priority;
    // Handle of the event in its channel, also passed to the playback for notification.
    std::uint16_t notificationEvent;
};

#pragma pack(pop)

#endif /* COASTLINE_EAR_SCHEDULEDEVENT_H */
