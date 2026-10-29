#ifndef COASTLINE_EAR_SDLMIXER_H
#define COASTLINE_EAR_SDLMIXER_H

#include <cstdint>

#include "ear/wavechunks.h"

/** The frames a second EAR mixes at, the format it gives the primary buffer of DirectSound. */
inline constexpr int EAR_MIX_RATE = 22050;
/** The channels EAR mixes, the format it gives the primary buffer of DirectSound. */
inline constexpr int EAR_MIX_CHANNELS = 2;

/**
 * A buffer of samples the mixer plays, as a secondary buffer of DirectSound plays in the original.
 * The volume and the pan take the units of DirectSound, hundredths of a decibel, and a value
 * outside the range of DirectSound is rejected as DirectSound rejects it.
 */
class EarSdlVoice {
public:
    /**
     * Create a silent, stopped buffer and add it to the mix.
     *
     * @param format The format of the samples, 8-bit or 16-bit PCM of one or two channels.
     * @param bytes The bytes of the buffer.
     * @return The buffer, or a null pointer for a format DirectSound does not play.
     */
    static EarSdlVoice *Create(const EarWaveFormat &format, std::uint32_t bytes);

    /** Remove the buffer from the mix and delete it. */
    void Release();

    /**
     * Copy samples into the buffer, continuing at its start past its end.
     *
     * @param offset The offset in the buffer, in bytes.
     * @param data The samples.
     * @param bytes The bytes of the samples.
     */
    void Write(std::uint32_t offset, const void *data, std::uint32_t bytes);

    /**
     * Start the buffer from its play cursor.
     *
     * @param looping Whether the buffer starts again at its end.
     */
    void Play(bool looping);

    /** Stop the buffer, leaving its play cursor where it is. */
    void Stop();

    /** @return Whether the buffer is playing. */
    bool IsPlaying() const;

    /** @return The offset of the frame that plays next, in bytes. */
    std::uint32_t GetPlayCursor() const;

    /**
     * Move the play cursor.
     *
     * @param offset The offset, in bytes.
     * @return Whether the offset is in the buffer.
     */
    bool SetPlayCursor(std::uint32_t offset);

    /**
     * @param frequency The frames a second the buffer plays at, from 100 to 100000.
     */
    void SetFrequency(std::uint32_t frequency);

    /**
     * @param volume The attenuation, from -10000 to 0 hundredths of a decibel.
     */
    void SetVolume(LONG volume);

    /**
     * @param pan The attenuation of the right side when negative, or of the left side when
     * positive, from -10000 to 10000 hundredths of a decibel.
     */
    void SetPan(LONG pan);

    /**
     * Add the frames the buffer plays next to a mix.
     *
     * @param mix The interleaved stereo mix.
     * @param frames The frames of the mix.
     */
    void MixInto(float *mix, int frames);

private:
    EarSdlVoice() = default;
    float ReadSample(std::uint32_t frame, int channel) const;
    void UpdateGains();

    EarWaveFormat format = {};       /*!< The format of the samples. */
    std::uint8_t *samples = nullptr; /*!< The samples. */
    std::uint32_t bytes = 0;         /*!< The bytes of @ref samples. */
    std::uint32_t frames = 0;        /*!< The whole frames of @ref samples. */
    std::uint64_t position = 0;      /*!< The frame that plays next, in 32.32 fixed point. */
    std::uint32_t frequency = 0;     /*!< The frames a second the buffer plays at. */
    LONG volume = 0;                 /*!< The attenuation of both sides. */
    LONG pan = 0;                    /*!< The attenuation of one side. */
    float leftGain = 1;              /*!< The amplitude of the left side. */
    float rightGain = 1;             /*!< The amplitude of the right side. */
    bool playing = false;            /*!< Whether the buffer is playing. */
    bool looping = false;            /*!< Whether the buffer starts again at its end. */
};

/**
 * Select whether the mixer renders on request, through EarRenderSdlMixer(), rather than to the
 * default playback device. The choice applies when the mixer opens.
 *
 * @param offline Whether to render on request.
 */
void EarSetSdlMixerOffline(bool offline);

/**
 * Open the mixer, and the default playback device unless it renders on request.
 *
 * @return Whether the mixer opened.
 */
bool EarOpenSdlMixer();

/** Close the mixer and its device, and delete every buffer. */
void EarCloseSdlMixer();

/**
 * Mix frames of a mixer that renders on request, and advance its clock by their time.
 *
 * @param samples The interleaved stereo 16-bit samples, two for each frame.
 * @param frames The frames to mix.
 */
void EarRenderSdlMixer(std::int16_t *samples, int frames);

#endif /* COASTLINE_EAR_SDLMIXER_H */
