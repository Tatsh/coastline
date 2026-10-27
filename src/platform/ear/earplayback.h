#ifndef COASTLINE_EAR_EARPLAYBACK_H
#define COASTLINE_EAR_EARPLAYBACK_H

#include <cstdint>

class EarWaveSample;
class EarPreset;

class EarPlayback {
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

protected:
    /**
     * @ghidraAddress EAR: 0x1000a6a0
     * @stub
     */
    EarPlayback();

private:
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
    /**
     * @ghidraAddress EAR: 0x10003a40
     * @stub
     */
    virtual void SetEarPlaybackDirectionalMatrix(std::uint16_t direction, std::uint16_t matrix);
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
    /**
     * @ghidraAddress EAR: 0x10003a70
     * @stub
     */
    virtual void SetPlaybackSuppressionParameters(std::uint32_t enabled, std::uint16_t marker);
    /**
     * @ghidraAddress EAR: 0x1000ad81
     * @stub
     */
    virtual void SetEarPlaybackStartPosition(double position);
    /**
     * @ghidraAddress EAR: 0x10003aa0
     * @stub
     */
    virtual void SetEarPlaybackStartMilliseconds(std::uint16_t milliseconds);
    /**
     * @ghidraAddress EAR: 0x10003ac0
     * @stub
     */
    virtual void SetReservedPlaybackEventControl(std::uint32_t control);
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
    /**
     * @ghidraAddress EAR: 0x1000ab3f
     * @stub
     */
    virtual void SetEarPlaybackMute(int muted);
    /**
     * @ghidraAddress EAR: 0x1000abaa
     * @stub
     */
    virtual void SetBackendPlaybackOverrideEnabled(int enabled);
    /**
     * @ghidraAddress EAR: 0x1000ac36
     * @stub
     */
    virtual void SetEarPlaybackFrequencyScale(double scale);
    /**
     * @ghidraAddress EAR: 0x1000abd1
     * @stub
     */
    virtual void SetEarPlaybackBaseFrequencyScale(double scale);
    /**
     * @ghidraAddress EAR: 0x1000abf3
     * @stub
     */
    virtual void SetCachedBackendPlaybackParameter(double value);
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
    /**
     * @ghidraAddress EAR: 0x1000ad6b
     * @stub
     */
    virtual void SetEarPlaybackLooping(int enabled);
    /**
     * @ghidraAddress EAR: 0x1000adb7
     * @stub
     */
    virtual void SetEarPlaybackNotification(std::uint32_t flags, std::uint16_t event);

    std::uint8_t unknown4[0x18]; /* +0x04 */
    int completionRequested;
    std::uint8_t unknown20[4]; /* +0x20 */
    int active;
    std::uint8_t unknown28[4]; /* +0x28 */
    int status;
    std::uint8_t unknown30[0x7c]; /* +0x30 */
};

#endif
