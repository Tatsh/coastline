#ifndef COASTLINE_EAR_EARPLAYBACK_H
#define COASTLINE_EAR_EARPLAYBACK_H

#include <cstdint>

#include "ear/directionalmatrix.h"
#include "ear/directsoundstate.h"
#include "ear/playbacksmoothing.h"

class EarWaveSample;

/**
 * Common event playback state with overridable media, device, and stream operations.
 * ReleaseEarPlaybackResources() must run before destruction to release device resources.
 * The destructor is non-virtual and does not release registered playback by itself.
 */
class [[gnu::packed]] EarPlayback {
public:
    /**
     * Invoke the base finalisation hook without releasing playback resources or derived storage.
     * @ghidraAddress EAR: 0x1000a84e
     */
    ~EarPlayback();

    /**
     * Store source flags without validation or reinitialisation.
     * @param flags The source and resource-management flag bits for the media implementation.
     * @ghidraAddress EAR: 0x10008aa0
     */
    void SetEarPlaybackFlags(std::uint16_t flags);

    /**
     * @return The stored pause status, with zero for unpaused playback and nonzero for paused.
     * @ghidraAddress EAR: 0x1000ab77
     */
    int GetStatus();
    /**
     * @return The stored active marker; pausing does not clear the marker.
     * @ghidraAddress EAR: 0x1000ad34
     */
    int IsActive();
    /**
     * Request completion for the next completion check without stopping the device immediately.
     * @ghidraAddress EAR: 0x1000ad45
     */
    void MarkComplete();
    /**
     * @return The stored mute marker, with nonzero indicating mute.
     * @ghidraAddress EAR: 0x1000ab88
     */
    int GetEarPlaybackMute();
    /**
     * @return The stored streaming marker, with nonzero indicating buffered streaming.
     * @ghidraAddress EAR: 0x1000ab99
     */
    int IsEarPlaybackStreaming();

    /**
     * Withdraw a request to complete the playback.
     *
     * @return One, including when completion was not requested.
     * @ghidraAddress EAR: 0x10012530
     */
    int ResetPlaybackCompletion();
    /**
     * Withdraw a request to complete the playback.
     *
     * @ghidraAddress EAR: 0x1001254a
     */
    void ClearPlaybackCompletion();

private:
    /** @ghidraAddress EAR: 0x10012570 */
    virtual void InitializeDirectSoundPlaybackState();
    /** @ghidraAddress EAR: 0x10012596 */
    virtual void FinalizeDirectSoundPlaybackState();

public:
    /**
     * Release registered playback through the virtual closure hook.
     * Derived implementations also release their media storage here.
     * @ghidraAddress EAR: 0x1000a86a
     */
    virtual void ReleaseEarPlaybackResources();

    /**
     * Store directional matrix identifiers without resolving an entry or updating output.
     * @param direction The directional row index, normally 0..127.
     * @param matrix The built-in or registered matrix handle.
     * @ghidraAddress EAR: 0x10003a40
     */
    virtual void SetEarPlaybackDirectionalMatrix(std::uint16_t direction, std::uint16_t matrix);

    /**
     * Update cached directional gain for registered playback and restart active smoothing.
     * Output changes during ProcessEarPlaybackSmoothing(). Muted playback preserves cached gain.
     * Unregistered playback ignores the request. Negative gain is not clamped; NaN can be stored
     * when the direction or matrix changes but otherwise does not trigger an update.
     * @param direction The directional row index, normally 0..127.
     * @param gain The linear gain; values above one request the previously cached gain.
     * @param matrix The built-in or registered matrix handle.
     * @ghidraAddress EAR: 0x1000aa26
     */
    virtual void
    SetEarPlaybackDirectionalGain(std::uint16_t direction, double gain, std::uint16_t matrix);

private:
    /** @ghidraAddress EAR: 0x1001273a */
    virtual void UpdateDirectSoundPlaybackGainAndPan();

public:
    /**
     * Apply a frequency, gain, and pan smoothing pass to the current device buffer.
     * Inactive playback resets the pass counter. Global smoothing settings determine progression.
     * @ghidraAddress EAR: 0x1000ae0f
     */
    virtual void ProcessEarPlaybackSmoothing();

private:
    /** @ghidraAddress EAR: 0x1000aee1 */
    virtual void ResetEarPlaybackSmoothingPass();

public:
    /**
     * Store suppression controls without starting or stopping playback.
     * @param enabled The suppression-enable marker, with zero disabling suppression.
     * @param marker The suppression role; one suppresses base device starts when enabled.
     * @ghidraAddress EAR: 0x10003a70
     */
    virtual void SetPlaybackSuppressionParameters(std::uint32_t enabled, std::uint16_t marker);
    /**
     * Store an ordered nonzero start position and mark it specified; zero and NaN are ignored.
     * @param position The backend-specific position, usually a fraction of media length.
     * @ghidraAddress EAR: 0x1000ad81
     */
    virtual void SetEarPlaybackStartPosition(double position);
    /**
     * Store an absolute start offset for wave-stream preparation.
     * @param milliseconds The start offset in milliseconds; zero disables the offset.
     * @ghidraAddress EAR: 0x10003aa0
     */
    virtual void SetEarPlaybackStartMilliseconds(std::uint16_t milliseconds);
    /**
     * Store a backend-specific event control value without applying it immediately.
     * @param control The control value, used as a byte count for an in-memory MIDI source.
     * @ghidraAddress EAR: 0x10003ac0
     */
    virtual void SetReservedPlaybackEventControl(std::uint32_t control);

    /**
     * Initialise implementation-specific media; the base implementation does not change state.
     * @param source The implementation-specific source representation, unused by the base.
     * @param streaming The requested streaming marker, unused by the base.
     * @param looping The requested looping marker, unused by the base.
     * @return One in the base implementation; derived implementations return zero on failure.
     * @ghidraAddress EAR: 0x1000aa14
     */
    virtual int InitializeEarPlayback(const void *source, int streaming, int looping);

private:
    /** @ghidraAddress EAR: 0x10012715 */
    virtual std::uint16_t AllocateEarEventId();
    /** @ghidraAddress EAR: 0x100125fd */
    virtual void UpdateDirectSoundPlaybackFrequency();

public:
    /**
     * Post an enabled start notification, attempt device playback, and mark successful starts
     * active.
     * Enabled suppression with role one reports success without starting the base device buffer.
     * @return One on success; zero on a missing buffer or device error, with an EAR error recorded.
     * @ghidraAddress EAR: 0x1000a9bf
     */
    virtual int StartEarPlayback();

private:
    /** @ghidraAddress EAR: 0x10012860 */
    virtual int PlayDirectSoundEventBuffer();

public:
    /**
     * Mark playback paused and attempt to stop the device buffer without clearing active state.
     * @return One on success; zero when the buffer is missing or a device stop fails.
     * @ghidraAddress EAR: 0x1000a99f
     */
    virtual int PauseEarPlayback();

private:
    /** @ghidraAddress EAR: 0x100128f8 */
    virtual int StopDirectSoundEventBuffer();

public:
    /**
     * Attempt a restart when paused and active, then clear paused status.
     * @return One, including when a restart fails or playback was not paused.
     * @ghidraAddress EAR: 0x1000add8
     */
    virtual int ResumeEarPlayback();

protected:
    /**
     * Stop and release a registered buffer, post an enabled closure notification, and unregister
     * playback.
     * An absent playback handle suppresses all closure work.
     * @ghidraAddress EAR: 0x1000a88b
     */
    virtual void CloseRegisteredEarPlayback();

private:
    /** @ghidraAddress EAR: 0x1001294f */
    virtual void ReleaseDirectSoundEventBuffer();
    /** @ghidraAddress EAR: 0x1000a880 */
    virtual void RefillEarPlaybackStream();
    /** @ghidraAddress EAR: 0x100125a1 */
    virtual void CopyEarPlaybackSamplesToOutput();

public:
    /**
     * Perform implementation-specific completion; the base implementation does not change state.
     * @ghidraAddress EAR: 0x1000a8e5
     */
    virtual void CompleteEarPlayback();

private:
    /** @ghidraAddress EAR: 0x100129a3 */
    virtual int CheckEarPlaybackLoopCompletion();

public:
    /**
     * Apply implementation-specific preset data; the base implementation ignores the request.
     *
     * @param preset The preset, unused.
     * @ghidraAddress EAR: 0x10006710
     */
    virtual void ApplyPreset(EarWaveSample *preset);

protected:
    /**
     * Respond to sample attachment; the base implementation ignores the notification.
     * @param sample The borrowed sample pointer, unused by the base.
     * @ghidraAddress EAR: 0x100129bc
     */
    virtual void OnPlaybackWaveSampleAttached(EarWaveSample *sample);

public:
    /**
     * @return Zero in the base implementation; media length in backend-specific units otherwise.
     * @ghidraAddress EAR: 0x100094e0
     */
    virtual std::uint32_t GetEarPlaybackMediaLength();

public:
    /**
     * Store mute state and refresh cached directional gain without stopping playback or timers.
     * @param muted The mute marker, with nonzero indicating mute.
     * @ghidraAddress EAR: 0x1000ab3f
     */
    virtual void SetEarPlaybackMute(int muted);
    /**
     * Store the implementation-specific playback override marker.
     * @param enabled The override marker, with nonzero indicating enabled.
     * @ghidraAddress EAR: 0x1000abaa
     */
    virtual void SetBackendPlaybackOverrideEnabled(int enabled);
    /**
     * Clamp an ordered changed scale to 0..6.5535 and recalculate frequency from the base rate.
     * Unordered comparisons preserve the existing value. Output updates during smoothing.
     * @param scale The dimensionless frequency multiplier; base frequency must be initialised.
     * @ghidraAddress EAR: 0x1000ac36
     */
    virtual void SetEarPlaybackFrequencyScale(double scale);
    /**
     * Store the initial frequency multiplier without validation or device updates.
     * @param scale The dimensionless multiplier applied during derived media initialisation.
     * @ghidraAddress EAR: 0x1000abd1
     */
    virtual void SetEarPlaybackBaseFrequencyScale(double scale);
    /**
     * Store an ordered changed backend parameter and invoke the parameter-change hook.
     * Unordered comparisons preserve the existing value.
     * @param value The implementation-specific parameter, without validation.
     * @ghidraAddress EAR: 0x1000abf3
     */
    virtual void SetCachedBackendPlaybackParameter(double value);

private:
    /** @ghidraAddress EAR: 0x10012e2b */
    virtual void OnBackendPlaybackParameterChanged();

public:
    /**
     * Check requested completion or active device state and refill a stream half when necessary.
     * Non-streaming looping playback and inactive playback do not complete without a request.
     * @return Nonzero when completion is requested or an active non-looping buffer has stopped.
     * @ghidraAddress EAR: 0x1000a8f0
     */
    virtual int IsBufferedEarPlaybackComplete();

protected:
    /**
     * Query the device buffer; a non-null buffer is required.
     * @return Nonzero when the buffer is not playing; otherwise zero.
     * @ghidraAddress EAR: 0x100126dd
     */
    virtual int IsDirectSoundBufferStopped();

public:
    /**
     * Store looping state without changing the currently playing buffer's device flags.
     * @param enabled The looping marker, with nonzero indicating enabled.
     * @ghidraAddress EAR: 0x1000ad6b
     */
    virtual void SetEarPlaybackLooping(int enabled);
    /**
     * Store notification controls for later playback transitions.
     * @param flags Bit zero enables start messages; bit one enables registered-closure messages.
     * @param event The event identifier included in notification messages.
     * @ghidraAddress EAR: 0x1000adb7
     */
    virtual void SetEarPlaybackNotification(std::uint32_t flags, std::uint16_t event);

protected:
    /**
     * Initialise inactive playback and smoothing defaults without allocating media or a buffer.
     * Derived initialisation must set source flags and base frequency before reading them.
     * @ghidraAddress EAR: 0x1000a6a0
     */
    EarPlayback();

    /**
     * Store buffer capacity without reallocating storage.
     * @param bytes The total decoded sample-buffer capacity in bytes.
     */
    void SetPlaybackBufferBytes(std::uint32_t bytes) {
        bufferBytes = bytes;
    }
    /**
     * Store the refill-half capacity without changing the buffer.
     * @param bytes The decoded byte count in each streaming half.
     */
    void SetStreamHalfBytes(std::uint32_t bytes) {
        streamHalfBytes = bytes;
    }
    /** Clear the streaming marker without changing device looping flags. */
    void DisablePlaybackStreaming() {
        streaming = 0;
    }
    /** @return The stored wave-stream start offset in milliseconds, or zero when disabled. */
    std::uint16_t GetPlaybackStartMilliseconds() const {
        return startMilliseconds;
    }
    /** @return Whether a backend-specific start position is specified. */
    bool HasPlaybackStartPosition() const {
        return startPositionSpecified != 0;
    }
    /** Mark the stored start position specified without changing its value. */
    void SpecifyPlaybackStartPosition() {
        startPositionSpecified = 1;
    }
    /**
     * Store stream and loop markers without validation or device changes.
     * @param isStreaming The streaming marker, with nonzero indicating enabled.
     * @param isLooping The looping marker, with nonzero indicating enabled.
     */
    void SetPlaybackStreamingAndLooping(int isStreaming, int isLooping) {
        streaming = isStreaming;
        looping = isLooping;
    }
    /** @return The dimensionless initial frequency multiplier. */
    double GetPlaybackBaseFrequencyScale() const {
        return baseFrequencyScale;
    }
    /**
     * Store base frequency without updating the current device frequency.
     * @param value The base sample rate in hertz.
     */
    void SetPlaybackBaseFrequency(std::uint32_t value) {
        baseFrequency = value;
    }
    /**
     * Store a registration handle without modifying the playback registry.
     * @param handle The playback handle, or zero for unregistered playback.
     */
    void SetPlaybackHandle(std::uint32_t handle) {
        playbackHandle = handle;
    }
    /** @return The stored playback handle, or zero for unregistered playback. */
    std::uint32_t GetPlaybackHandle() const {
        return playbackHandle;
    }

#if TGR_PLATFORM == TGR_PLATFORM_WIN
    /**
     * Create a Windows device buffer; an existing buffer must be released before replacement.
     * @param device The non-null borrowed DirectSound device used for creation.
     * @param description The non-null buffer descriptor required by DirectSound.
     * @return The HRESULT from CreateSoundBuffer(); DS_OK indicates success.
     */
    HRESULT CreateDirectSoundPlaybackBuffer(IDirectSound *device, _DSBUFFERDESC *description);
#else
    /**
     * Create an SDL mixer buffer; an existing buffer must be released before replacement.
     * @param format The decoded PCM sample format copied into the voice.
     * @param bytes The requested buffer capacity in bytes.
     * @return Whether creation succeeded, with the new buffer stored on success.
     */
    bool CreateDirectSoundPlaybackBuffer(const EarWaveFormat &format, DWORD bytes);
#endif
    /** Reset current frequency to the initialised base frequency without updating the device. */
    void ResetPlaybackBufferFrequency() {
        frequency = baseFrequency;
    }
    /** @return The stored decoded sample-buffer capacity in bytes. */
    std::uint32_t GetPlaybackBufferBytes() const {
        return bufferBytes;
    }
    /** @return Whether a start position is specified and streaming is disabled. */
    bool HasNonStreamingStartPosition() const {
        return startPositionSpecified && !streaming;
    }
    /** @return The backend-specific start position, meaningful when marked specified. */
    double GetPlaybackStartPosition() const {
        return startPosition;
    }
    /** Clear the specified marker and reset the start position to zero. */
    void ClearPlaybackStartPosition() {
        startPositionSpecified = 0;
        startPosition = 0;
    }
    /** @return The borrowed buffer, or null; releasing the buffer invalidates the pointer. */
    EarDirectSoundPlaybackState::Buffer *GetDirectSoundBuffer() const {
        return directSound.GetBuffer();
    }
    /** @return The incremented stream-drain counter, wrapping at 32 bits. */
    std::uint32_t IncrementLoopDrainCount() {
        return ++loopDrainCount;
    }
    /** Clear both loop and stream markers without stopping the device immediately. */
    void EndStreamingPlaybackLoop() {
        looping = 0;
        streaming = 0;
    }
    /** @return The stored source flags; derived construction must initialise the value first. */
    std::uint16_t GetPlaybackSourceFlags() const {
        return flags;
    }
    /** @return Whether the stored looping marker is nonzero. */
    bool IsPlaybackLooping() const {
        return looping != 0;
    }
    /** @return Whether a nonzero playback handle is stored. */
    bool HasPlaybackHandle() const {
        return playbackHandle != 0;
    }
    /** Clear the looping marker without changing device playback flags. */
    void DisablePlaybackLooping() {
        looping = 0;
    }
    /** @return The next refill-half index, zero or one under normal streaming progression. */
    std::uint32_t GetStreamHalfIndex() const {
        return streamHalfIndex;
    }
    /** @return The stored decoded byte count in each streaming half. */
    std::uint32_t GetStreamHalfBytes() const {
        return streamHalfBytes;
    }
    /** Toggle the refill-half index between zero and one. */
    void FlipStreamHalfIndex() {
        streamHalfIndex = !streamHalfIndex;
    }

private:
    /** @ghidraAddress EAR: 0x1000abc0 */
    int GetBackendPlaybackOverrideEnabled();
    /** @ghidraAddress EAR: 0x1000ad5a */
    int GetEarPlaybackLooping();

    /** @ghidraAddress EAR: 0x100125ac */
    DWORD GetDirectSoundPlaybackCursor();

protected:
    /** Playback registry key, or zero when unregistered. */
    std::uint32_t playbackHandle;
    /** Implementation-specific event control, including in-memory MIDI source length in bytes. */
    std::uint32_t reservedEventControl;
    /** Notification bits for starts (1) and registered closures (2). */
    std::uint32_t notificationFlags;
    /** Event identifier included in window notifications. */
    std::uint16_t notificationEvent;
    /** Source flags interpreted by derived media implementations; base construction omits them. */
    std::uint16_t flags;
    /** Nonzero while looping is requested. */
    int looping;
    /** Nonzero while media requires half-buffer refills. */
    int streaming;
    /** Nonzero after completion is requested or detected. */
    int completionRequested;
    /** Next streaming half to refill, initially one. */
    std::uint32_t streamHalfIndex;
    /** Nonzero after successful playback starts, including during a pause. */
    int active;
    /** Nonzero when muted; cached gain remains available for unmuting. */
    int muted;
    /** Pause status, zero when unpaused. */
    int status;
    /** Nonzero when implementation-specific position or frequency overrides are enabled. */
    int backendOverrideEnabled;
    /** Nonzero when spatial suppression processing is enabled. */
    std::uint32_t suppressionEnabled;
    /** Nonzero when startPosition is specified rather than merely stored. */
    int startPositionSpecified;
    /** Suppression role, with one suppressing base device starts. */
    std::uint16_t suppressionMarker;
    /** Wave-stream initial offset in milliseconds, or zero when disabled. */
    std::uint16_t startMilliseconds;
    /** Selected directional matrix handle, initially 16. */
    std::uint16_t matrixId;
    /** Selected directional row index, normally 0..127. */
    std::uint16_t directionIndex;
    /** Linear gain at the start of the current smoothing transition. */
    double previousGain;
    /** Target linear gain before directional and master attenuation. */
    double targetGain;
    /** Directional pan and gain at the start of the current smoothing transition. */
    EarDirectionalMatrixEntry previousMatrixEntry;
    /** Target directional pan and gain resolved from the selected matrix. */
    EarDirectionalMatrixEntry targetMatrixEntry;
    /** Target sample frequency in hertz before device range clamping. */
    std::uint32_t frequency;
    /** Sample frequency in hertz at the start of the smoothing transition. */
    std::uint32_t previousFrequency;
    /** Unrecovered bytes retained at the original object offset. */
    [[maybe_unused]] std::uint8_t unknown6c[2]; /* +0x6c */
    /** Smoothing pass index; zero selects the target directly. */
    std::uint16_t smoothingPass;
    /** Decoded device sample-buffer capacity in bytes. */
    std::uint32_t bufferBytes;
    /** Decoded byte count in each streaming half. */
    std::uint32_t streamHalfBytes;
    /** Implementation-specific start position, usually a fraction of media length. */
    double startPosition;
    /** Current dimensionless playback frequency multiplier. */
    double frequencyScale;
    /** Initial frequency multiplier applied while preparing media. */
    double baseFrequencyScale;
    /** Initial sample frequency in hertz; derived initialisation must assign it. */
    std::uint32_t baseFrequency;
    /** Cached implementation-specific parameter, initially -1. */
    double cachedBackendParameter;
    /** Number of post-data streaming refills used while draining non-looping playback. */
    std::uint32_t loopDrainCount;
    /** Device buffer reference and cached byte cursors; resource release is explicit. */
    EarDirectSoundPlaybackState directSound;
};

/**
 * Next EAR event identifier, initially one and incremented with 16-bit wraparound.
 * @ghidraAddress EAR: 0x10036144
 */
/** @cond */
extern std::uint16_t g_wEarObjectHandleCounter;
/** @endcond */

#endif /* COASTLINE_EAR_EARPLAYBACK_H */
