#ifndef COASTLINE_EAR_EARPLAYBACK_H
#define COASTLINE_EAR_EARPLAYBACK_H

#include <cstdint>

#include "ear/directionalmatrix.h"
#include "ear/directsoundstate.h"
#include "ear/playbacksmoothing.h"

class EarWaveSample;

// The virtual routines are declared in the order of the slots of the native table.
class [[gnu::packed]] EarPlayback {
public:
    /** @ghidraAddress EAR: 0x1000a84e */
    ~EarPlayback();

    /** @ghidraAddress EAR: 0x10008aa0 */
    void SetEarPlaybackFlags(std::uint16_t flags);

    /** @ghidraAddress EAR: 0x1000ab77 */
    int GetStatus();
    /** @ghidraAddress EAR: 0x1000ad34 */
    int IsActive();
    /** @ghidraAddress EAR: 0x1000ad45 */
    void MarkComplete();
    /** @ghidraAddress EAR: 0x1000ab88 */
    int GetEarPlaybackMute();
    /** @ghidraAddress EAR: 0x1000ab99 */
    int IsEarPlaybackStreaming();

    /**
     * Withdraw a request to complete the playback.
     *
     * @return Whether the request was withdrawn, which is always.
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
    /** @ghidraAddress EAR: 0x1000a86a */
    virtual void ReleaseEarPlaybackResources();

    /** @ghidraAddress EAR: 0x10003a40 */
    virtual void SetEarPlaybackDirectionalMatrix(std::uint16_t direction, std::uint16_t matrix);

    /** @ghidraAddress EAR: 0x1000aa26 */
    virtual void
    SetEarPlaybackDirectionalGain(std::uint16_t direction, double gain, std::uint16_t matrix);

private:
    /** @ghidraAddress EAR: 0x1001273a */
    virtual void UpdateDirectSoundPlaybackGainAndPan();

public:
    /** @ghidraAddress EAR: 0x1000ae0f */
    virtual void ProcessEarPlaybackSmoothing();

private:
    /** @ghidraAddress EAR: 0x1000aee1 */
    virtual void ResetEarPlaybackSmoothingPass();

public:
    /** @ghidraAddress EAR: 0x10003a70 */
    virtual void SetPlaybackSuppressionParameters(std::uint32_t enabled, std::uint16_t marker);
    /** @ghidraAddress EAR: 0x1000ad81 */
    virtual void SetEarPlaybackStartPosition(double position);
    /** @ghidraAddress EAR: 0x10003aa0 */
    virtual void SetEarPlaybackStartMilliseconds(std::uint16_t milliseconds);
    /** @ghidraAddress EAR: 0x10003ac0 */
    virtual void SetReservedPlaybackEventControl(std::uint32_t control);

    /** @ghidraAddress EAR: 0x1000aa14 */
    virtual int InitializeEarPlayback(const void *source, int streaming, int looping);

private:
    /** @ghidraAddress EAR: 0x10012715 */
    virtual std::uint16_t AllocateEarEventId();
    /** @ghidraAddress EAR: 0x100125fd */
    virtual void UpdateDirectSoundPlaybackFrequency();

public:
    /** @ghidraAddress EAR: 0x1000a9bf */
    virtual int StartEarPlayback();

private:
    /** @ghidraAddress EAR: 0x10012860 */
    virtual int PlayDirectSoundEventBuffer();

public:
    /** @ghidraAddress EAR: 0x1000a99f */
    virtual int PauseEarPlayback();

private:
    /** @ghidraAddress EAR: 0x100128f8 */
    virtual int StopDirectSoundEventBuffer();

public:
    /** @ghidraAddress EAR: 0x1000add8 */
    virtual int ResumeEarPlayback();

protected:
    /** @ghidraAddress EAR: 0x1000a88b */
    virtual void CloseRegisteredEarPlayback();

private:
    /** @ghidraAddress EAR: 0x1001294f */
    virtual void ReleaseDirectSoundEventBuffer();
    /** @ghidraAddress EAR: 0x1000a880 */
    virtual void RefillEarPlaybackStream();
    /** @ghidraAddress EAR: 0x100125a1 */
    virtual void CopyEarPlaybackSamplesToOutput();

public:
    /** @ghidraAddress EAR: 0x1000a8e5 */
    virtual void CompleteEarPlayback();

private:
    /** @ghidraAddress EAR: 0x100129a3 */
    virtual int CheckEarPlaybackLoopCompletion();

public:
    /**
     * Native slot shares the empty CObject serialization body.
     *
     * @param preset The preset, unused.
     * @ghidraAddress EAR: 0x10006710
     */
    virtual void ApplyPreset(EarWaveSample *preset);

protected:
    /** @ghidraAddress EAR: 0x100129bc */
    virtual void OnPlaybackWaveSampleAttached(EarWaveSample *sample);

public:
    /** @ghidraAddress EAR: 0x100094e0 */
    virtual std::uint32_t GetEarPlaybackMediaLength();

public:
    /** @ghidraAddress EAR: 0x1000ab3f */
    virtual void SetEarPlaybackMute(int muted);
    /** @ghidraAddress EAR: 0x1000abaa */
    virtual void SetBackendPlaybackOverrideEnabled(int enabled);
    /** @ghidraAddress EAR: 0x1000ac36 */
    virtual void SetEarPlaybackFrequencyScale(double scale);
    /** @ghidraAddress EAR: 0x1000abd1 */
    virtual void SetEarPlaybackBaseFrequencyScale(double scale);
    /** @ghidraAddress EAR: 0x1000abf3 */
    virtual void SetCachedBackendPlaybackParameter(double value);

private:
    /** @ghidraAddress EAR: 0x10012e2b */
    virtual void OnBackendPlaybackParameterChanged();

public:
    /** @ghidraAddress EAR: 0x1000a8f0 */
    virtual int IsBufferedEarPlaybackComplete();

protected:
    /** @ghidraAddress EAR: 0x100126dd */
    virtual int IsDirectSoundBufferStopped();

public:
    /** @ghidraAddress EAR: 0x1000ad6b */
    virtual void SetEarPlaybackLooping(int enabled);
    /** @ghidraAddress EAR: 0x1000adb7 */
    virtual void SetEarPlaybackNotification(std::uint32_t flags, std::uint16_t event);

protected:
    /** @ghidraAddress EAR: 0x1000a6a0 */
    EarPlayback();

    void SetPlaybackBufferBytes(std::uint32_t bytes) {
        bufferBytes = bytes;
    }
    void SetStreamHalfBytes(std::uint32_t bytes) {
        streamHalfBytes = bytes;
    }
    void DisablePlaybackStreaming() {
        streaming = 0;
    }
    std::uint16_t GetPlaybackStartMilliseconds() const {
        return startMilliseconds;
    }
    bool HasPlaybackStartPosition() const {
        return startPositionSpecified != 0;
    }
    void SpecifyPlaybackStartPosition() {
        startPositionSpecified = 1;
    }
    void SetPlaybackStreamingAndLooping(int isStreaming, int isLooping) {
        streaming = isStreaming;
        looping = isLooping;
    }
    double GetPlaybackBaseFrequencyScale() const {
        return baseFrequencyScale;
    }
    void SetPlaybackBaseFrequency(std::uint32_t value) {
        baseFrequency = value;
    }
    void SetPlaybackHandle(std::uint32_t handle) {
        playbackHandle = handle;
    }
    std::uint32_t GetPlaybackHandle() const {
        return playbackHandle;
    }

#if TGR_PLATFORM == TGR_PLATFORM_WIN
    HRESULT CreateDirectSoundPlaybackBuffer(IDirectSound *device, _DSBUFFERDESC *description);
#else
    bool CreateDirectSoundPlaybackBuffer(const EarWaveFormat &format, DWORD bytes);
#endif
    void ResetPlaybackBufferFrequency() {
        frequency = baseFrequency;
    }
    std::uint32_t GetPlaybackBufferBytes() const {
        return bufferBytes;
    }
    bool HasNonStreamingStartPosition() const {
        return startPositionSpecified && !streaming;
    }
    double GetPlaybackStartPosition() const {
        return startPosition;
    }
    void ClearPlaybackStartPosition() {
        startPositionSpecified = 0;
        startPosition = 0;
    }
    EarDirectSoundPlaybackState::Buffer *GetDirectSoundBuffer() const {
        return directSound.GetBuffer();
    }
    std::uint32_t IncrementLoopDrainCount() {
        return ++loopDrainCount;
    }
    void EndStreamingPlaybackLoop() {
        looping = 0;
        streaming = 0;
    }
    std::uint16_t GetPlaybackSourceFlags() const {
        return flags;
    }
    bool IsPlaybackLooping() const {
        return looping != 0;
    }
    bool HasPlaybackHandle() const {
        return playbackHandle != 0;
    }
    void DisablePlaybackLooping() {
        looping = 0;
    }
    std::uint32_t GetStreamHalfIndex() const {
        return streamHalfIndex;
    }
    std::uint32_t GetStreamHalfBytes() const {
        return streamHalfBytes;
    }
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
    std::uint32_t playbackHandle;
    std::uint32_t reservedEventControl;
    std::uint32_t notificationFlags;
    std::uint16_t notificationEvent;
    std::uint16_t flags;
    int looping;
    int streaming;
    int completionRequested;
    std::uint32_t streamHalfIndex;
    int active;
    int muted;
    int status;
    int backendOverrideEnabled;
    std::uint32_t suppressionEnabled;
    int startPositionSpecified;
    std::uint16_t suppressionMarker;
    std::uint16_t startMilliseconds;
    std::uint16_t matrixId;
    std::uint16_t directionIndex;
    double previousGain;
    double targetGain;
    EarDirectionalMatrixEntry previousMatrixEntry;
    EarDirectionalMatrixEntry targetMatrixEntry;
    std::uint32_t frequency;
    std::uint32_t previousFrequency;
    [[maybe_unused]] std::uint8_t unknown6c[2]; /* +0x6c */
    std::uint16_t smoothingPass;
    std::uint32_t bufferBytes;
    std::uint32_t streamHalfBytes;
    double startPosition;
    double frequencyScale;
    double baseFrequencyScale;
    std::uint32_t baseFrequency;
    double cachedBackendParameter;
    std::uint32_t loopDrainCount;
    EarDirectSoundPlaybackState directSound;
};

/** @ghidraAddress EAR: 0x10036144 */
extern std::uint16_t g_wEarObjectHandleCounter;

#endif /* COASTLINE_EAR_EARPLAYBACK_H */
