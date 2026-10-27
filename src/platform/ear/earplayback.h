#ifndef COASTLINE_EAR_EARPLAYBACK_H
#define COASTLINE_EAR_EARPLAYBACK_H

#include <cstdint>

class EarWaveSample;
class EarPreset;

class [[gnu::packed]] EarPlayback {
public:
    /**
     * @ghidraAddress EAR: 0x1000a84e
     * @stub
     */
    ~EarPlayback();

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
    /**
     * @ghidraAddress EAR: 0x1000a6a0
     * @stub
     */
    EarPlayback();

private:
    /** @ghidraAddress EAR: 0x1000abc0 */
    int GetBackendPlaybackOverrideEnabled();
    /** @ghidraAddress EAR: 0x1000ad5a */
    int GetEarPlaybackLooping();

    /**
     * @ghidraAddress EAR: 0x10012570
     * @stub
     */
    virtual void InitializeDirectSoundPlaybackState();
    /**
     * @ghidraAddress EAR: 0x10012596
     * @stub
     */
    virtual void FinalizeDirectSoundPlaybackState();
    /**
     * @ghidraAddress EAR: 0x1000a86a
     * @stub
     */
    virtual void ReleaseEarPlaybackResources();

public:
    /** @ghidraAddress EAR: 0x10003a40 */
    virtual void SetEarPlaybackDirectionalMatrix(std::uint16_t direction, std::uint16_t matrix);

private:
    /**
     * @ghidraAddress EAR: 0x1000aa26
     * @stub
     */
    virtual void
    SetEarPlaybackDirectionalGain(std::uint16_t direction, double gain, std::uint16_t matrix);
    /**
     * @ghidraAddress EAR: 0x1001273a
     * @stub
     */
    virtual void UpdateDirectSoundPlaybackGainAndPan();
    /**
     * @ghidraAddress EAR: 0x1000ae0f
     * @stub
     */
    virtual void ProcessEarPlaybackSmoothing();
    /**
     * @ghidraAddress EAR: 0x1000aee1
     * @stub
     */
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

private:
    /**
     * @ghidraAddress EAR: 0x1000aa14
     * @stub
     */
    virtual int InitializeEarPlayback(const void *source, int streaming, int looping);
    /**
     * @ghidraAddress EAR: 0x10012715
     * @stub
     */
    virtual std::uint16_t AllocateEarEventId();
    /**
     * @ghidraAddress EAR: 0x100125fd
     * @stub
     */
    virtual void UpdateDirectSoundPlaybackFrequency();
    /**
     * @ghidraAddress EAR: 0x1000a9bf
     * @stub
     */
    virtual int StartEarPlayback();
    /**
     * @ghidraAddress EAR: 0x10012860
     * @stub
     */
    virtual int PlayDirectSoundEventBuffer();
    /**
     * @ghidraAddress EAR: 0x1000a99f
     * @stub
     */
    virtual void PauseEarPlayback();
    /**
     * @ghidraAddress EAR: 0x100128f8
     * @stub
     */
    virtual int StopDirectSoundEventBuffer();
    /**
     * @ghidraAddress EAR: 0x1000add8
     * @stub
     */
    virtual int ResumeEarPlayback();
    /**
     * @ghidraAddress EAR: 0x1000a88b
     * @stub
     */
    virtual void CloseRegisteredEarPlayback();
    /**
     * @ghidraAddress EAR: 0x1001294f
     * @stub
     */
    virtual void ReleaseDirectSoundEventBuffer();
    /**
     * @ghidraAddress EAR: 0x1000a880
     * @stub
     */
    virtual void RefillEarPlaybackStream();
    /**
     * @ghidraAddress EAR: 0x100125a1
     * @stub
     */
    virtual void CopyEarPlaybackSamplesToOutput();
    /**
     * @ghidraAddress EAR: 0x1000a8e5
     * @stub
     */
    virtual void CompleteEarPlayback();
    /**
     * @ghidraAddress EAR: 0x100129a3
     * @stub
     */
    virtual int CheckEarPlaybackLoopCompletion();
    /**
     * Native slot shares the empty CObject serialization body.
     * @ghidraAddress EAR: 0x10006710
     * @stub
     */
    virtual void ApplyPreset(const EarPreset *preset);
    /**
     * @ghidraAddress EAR: 0x100129bc
     * @stub
     */
    virtual void OnPlaybackWaveSampleAttached(EarWaveSample *sample);
    /**
     * @ghidraAddress EAR: 0x100094e0
     * @stub
     */
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
    /**
     * @ghidraAddress EAR: 0x10012e2b
     * @stub
     */
    virtual void OnBackendPlaybackParameterChanged();
    /**
     * @ghidraAddress EAR: 0x1000a8f0
     * @stub
     */
    virtual int IsBufferedEarPlaybackComplete();
    /**
     * @ghidraAddress EAR: 0x100126dd
     * @stub
     */
    virtual int IsDirectSoundBufferStopped();

public:
    /** @ghidraAddress EAR: 0x1000ad6b */
    virtual void SetEarPlaybackLooping(int enabled);
    /** @ghidraAddress EAR: 0x1000adb7 */
    virtual void SetEarPlaybackNotification(std::uint32_t flags, std::uint16_t event);

private:
    [[maybe_unused]] std::uint8_t unknown4[4]; /* +0x04 */
    std::uint32_t reservedEventControl;
    std::uint32_t notificationFlags;
    std::uint16_t notificationEvent;
    [[maybe_unused]] std::uint8_t unknown12[2]; /* +0x12 */
    int looping;
    int streaming;
    int completionRequested;
    [[maybe_unused]] std::uint8_t unknown20[4]; /* +0x20 */
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
    [[maybe_unused]] std::uint8_t unknown44[0x20]; /* +0x44 */
    std::uint32_t frequency;
    std::uint32_t previousFrequency;
    [[maybe_unused]] std::uint8_t unknown6c[0xc]; /* +0x6c */
    double startPosition;
    double frequencyScale;
    double baseFrequencyScale;
    std::uint32_t baseFrequency;
    double cachedBackendParameter;
    [[maybe_unused]] std::uint8_t unknown9c[0x10]; /* +0x9c */
};

/** @ghidraAddress EAR: 0x1002ea78 */
extern const double g_earPlaybackScaleZero;

/** @ghidraAddress EAR: 0x1002ea80 */
extern const double g_earPlaybackMaximumFrequencyScale;

#endif
