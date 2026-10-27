#ifndef COASTLINE_EAR_EARPLAYBACK_H
#define COASTLINE_EAR_EARPLAYBACK_H

#include <cstdint>

#include "directionalmatrix.h"
#include "directsoundstate.h"

class EarWaveSample;
class EarPreset;

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

protected:
    /** @ghidraAddress EAR: 0x1000a6a0 */
    EarPlayback();

private:
    /** @ghidraAddress EAR: 0x1000abc0 */
    int GetBackendPlaybackOverrideEnabled();
    /** @ghidraAddress EAR: 0x1000ad5a */
    int GetEarPlaybackLooping();

    /** @ghidraAddress EAR: 0x100125ac */
    DWORD GetDirectSoundPlaybackCursor();

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
    /**
     * @ghidraAddress EAR: 0x10012715
     * @stub
     */
    virtual std::uint16_t AllocateEarEventId();
    /** @ghidraAddress EAR: 0x100125fd */
    virtual void UpdateDirectSoundPlaybackFrequency();

public:
    /** @ghidraAddress EAR: 0x1000a9bf */
    virtual int StartEarPlayback();

private:
    /** @ghidraAddress EAR: 0x10012860 */
    virtual int PlayDirectSoundEventBuffer();
    /** @ghidraAddress EAR: 0x1000a99f */
    virtual void PauseEarPlayback();
    /** @ghidraAddress EAR: 0x100128f8 */
    virtual int StopDirectSoundEventBuffer();
    /** @ghidraAddress EAR: 0x1000add8 */
    virtual int ResumeEarPlayback();

protected:
    /**
     * @ghidraAddress EAR: 0x1000a88b
     * @stub
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
    /** @ghidraAddress EAR: 0x1000a8e5 */
    virtual void CompleteEarPlayback();

private:
    /** @ghidraAddress EAR: 0x100129a3 */
    virtual int CheckEarPlaybackLoopCompletion();

public:
    /**
     * Native slot shares the empty CObject serialization body.
     * @ghidraAddress EAR: 0x10006710
     * @stub
     */
    virtual void ApplyPreset(const EarPreset *preset);

private:
    /** @ghidraAddress EAR: 0x100129bc */
    virtual void OnPlaybackWaveSampleAttached(EarWaveSample *sample);
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
    IDirectSoundBuffer *GetDirectSoundBuffer() const {
        return directSound.buffer;
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
    bool HasPlaybackWaveSample() const {
        return sample != nullptr;
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
    EarWaveSample *sample;
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
    [[maybe_unused]] std::uint8_t unknown70[4]; /* +0x70 */
    std::uint32_t streamHalfBytes;
    double startPosition;
    double frequencyScale;
    double baseFrequencyScale;
    std::uint32_t baseFrequency;
    double cachedBackendParameter;
    std::uint32_t loopDrainCount;
    EarDirectSoundPlaybackState directSound;
};

/** @ghidraAddress EAR: 0x100360b8 */
extern std::uint16_t g_wPlaybackSmoothingPassCount;

/** @ghidraAddress EAR: 0x1003b7b8 */
extern int g_earPlaybackSmoothingEnabled;

#endif
