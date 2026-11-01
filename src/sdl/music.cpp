#include "music.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cassert>
#include <cstring>
#include <fstream>
#include <stdexcept>

extern "C" {
#include "common/audio.h"
}

extern "C" {
void osSyncPrintf(const char *format, ...) {
    va_list arguments;
    va_start(arguments, format);
    SDL_LogMessageV(SDL_LOG_CATEGORY_AUDIO, SDL_LOG_PRIORITY_VERBOSE, format, arguments);
    va_end(arguments);
}
}

namespace {

constexpr u32 RING_BYTES = AUDIO_RING_BYTES;
// The constants of the mixing thread in audio.c.
constexpr s32 MUSIC_LEAD = 0x1000;
constexpr u32 SOUND_EFFECT_LEAD = 0x1000;
constexpr s32 FIRST_PAIRED_CHANNEL = 6;
constexpr u32 TICKS_PER_SECOND = 100;
constexpr u64 SOUND_EFFECT_SAMPLE_MASK = 0xFFFEFFFEFFFEFFFEULL;
constexpr s32 MUSIC_CHANNEL_VOLUME = 0x20;
constexpr s32 SOUND_LEVELS_START = 0x20;
constexpr const char *MIXER_PROPERTY = "coastline.n64-mixer";
constexpr std::array<f32, MUSIC_NOTES> NOTE_FREQUENCIES{
    std::bit_cast<f32>(0x4382D99Au),
    std::bit_cast<f32>(0x438A999Au),
    std::bit_cast<f32>(0x4392D99Au),
    std::bit_cast<f32>(0x439B8CCDu),
    std::bit_cast<f32>(0x43A4CCCDu),
    std::bit_cast<f32>(0x43AE999Au),
    std::bit_cast<f32>(0x43B90000u),
    std::bit_cast<f32>(0x43C40000u),
    std::bit_cast<f32>(0x43CFA666u),
    std::bit_cast<f32>(0x43DC0000u),
    std::bit_cast<f32>(0x43E9199Au),
    std::bit_cast<f32>(0x43F6F333u),
};

// The mixers store a 32.32 position whose upper word is the address of the sample. The step is
// added before each read, with the carry of the fraction into the address.
inline s32 NextSample(MixerChannel &channel) {
    u64 sum = static_cast<u64>(channel.fraction) + channel.step;
    channel.sample += static_cast<s64>(sum >> 32);
    channel.fraction = static_cast<u32>(sum);
    return *channel.sample;
}

// MULT and ADD of the processor wrap at 32 bits.
inline s32 Mul(s32 a, s32 b) {
    return static_cast<s32>(static_cast<u32>(a) * static_cast<u32>(b));
}

inline s32 Add(s32 a, s32 b) {
    return static_cast<s32>(static_cast<u32>(a) + static_cast<u32>(b));
}

inline u8 *RingBytes(s16 *ring) {
    return reinterpret_cast<u8 *>(ring);
}

// The console's mixers loop on a byte count: the first part runs to the end of the ring, the rest
// from its start. Each pass of the body runs while the count is above zero.
template <typename Body>
u32 MixRing(s16 *ring, u32 start, u32 bytes, u32 frameBytes, Body body) {
    s32 over = static_cast<s32>(start + bytes) - static_cast<s32>(RING_BYTES);
    if (over <= 0) {
        over = 0;
    }
    s32 count = static_cast<s32>(bytes) - over;
    u32 offset = start;
    for (int pass = 0; pass < 2; ++pass) {
        if (count != 0) {
            do {
                count -= static_cast<s32>(frameBytes);
                body(RingBytes(ring) + offset);
                offset += frameBytes;
            } while (count > 0);
        }
        if (over == 0) {
            break;
        }
        count = over;
        over = 0;
        offset -= RING_BYTES;
    }
    return offset;
}

inline void StoreHalf(u8 *at, s32 value) {
    s16 half = static_cast<s16>(value);
    std::memcpy(at, &half, sizeof(half));
}

// The music clips at the range of a sample rather than wrapping as the console does, as a gain on
// the music can raise it past the levels of the console.
inline void StoreHalfClipped(u8 *at, s32 value) {
    StoreHalf(at, std::clamp(value, -32768, 32767));
}

inline s32 LoadHalf(const u8 *at) {
    s16 half;
    std::memcpy(&half, at, sizeof(half));
    return half;
}

// The volumes of the game as the mixers read them, copied by PublishN64Volumes().
struct MixerVolumes {
    u8 bgm;       /*!< g_bBgmVolume. */
    u8 music;     /*!< g_bMusicVolume. */
    u8 sfxMaster; /*!< g_bSfxMasterVolume. */
    u8 sfx;       /*!< g_bSfxVolume. */
};
} // namespace

namespace coastline {

class N64Mixer {
public:
    N64Mixer() {
        if (!mutex) {
            throw std::runtime_error("The audio mixer mutex could not be created.");
        }
        music.channelCount = MIXER_CHANNELS;
        music.bgmVolume = 0x30;
        music.sfxVolume = 0xFF;
        music.sfxMasterVolume = 0xFF;
        std::ranges::copy(NOTE_FREQUENCIES, music.noteFrequencies);
    }

    static N64Mixer &Get() {
        auto *mixer = static_cast<N64Mixer *>(
            SDL_GetPointerProperty(SDL_GetGlobalProperties(), MIXER_PROPERTY, nullptr));
        assert(mixer != nullptr);
        return *mixer;
    }

    SdlMixerState channels{};
    SdlMusicState music{};
    MixerVolumes volumes{};
    std::unique_ptr<SDL_Mutex, decltype(&SDL_DestroyMutex)> mutex{SDL_CreateMutex(),
                                                                  SDL_DestroyMutex};
};

N64MixerContext::N64MixerContext() : m_mixer(std::make_unique<N64Mixer>()) {
    const auto properties = SDL_GetGlobalProperties();
    assert(SDL_GetPointerProperty(properties, MIXER_PROPERTY, nullptr) == nullptr);
    if (!SDL_SetPointerProperty(properties, MIXER_PROPERTY, m_mixer.get())) {
        throw std::runtime_error("The audio mixer context could not be registered.");
    }
}

N64MixerContext::~N64MixerContext() {
    SDL_ClearProperty(SDL_GetGlobalProperties(), MIXER_PROPERTY);
}

} // namespace coastline

extern "C" {

SdlMixerState *GetSdlMixerState(void) {
    return &coastline::N64Mixer::Get().channels;
}

SdlMusicState *GetSdlMusicState(void) {
    return &coastline::N64Mixer::Get().music;
}

void CopyVec4(void *dst, const void *src) {
    std::memcpy(dst, src, 16);
}

void MemFill64Aligned(void *dst, u32 size, s32 value) {
    // The processor ORs the sign-extended value with itself shifted up a word, and stores the
    // doubleword big-endian in 64-byte blocks.
    u64 pattern = static_cast<u64>(static_cast<s64>(value)) |
                  (static_cast<u64>(static_cast<s64>(value)) << 32);
    u32 bytes = (size + 0x3F) & 0xFFC0;
    auto *out = static_cast<u8 *>(dst);
    for (u32 i = 0; i < bytes; ++i) {
        out[i] = static_cast<u8>(pattern >> (56 - (i % 8) * 8));
    }
}

void MixAudioSamples(s16 *ring, u32 bytes) {
    auto &mixer = *GetSdlMixerState();
    MixerChannel *c = mixer.musicChannels;
    u32 end = MixRing(ring, mixer.writePosition, bytes, 4, [c](u8 *frame) {
        s32 left = NextSample(c[0]);
        s32 left2 = NextSample(c[2]);
        s32 right = NextSample(c[1]);
        s32 right2 = NextSample(c[3]);
        s32 left3 = NextSample(c[4]);
        s32 right3 = NextSample(c[5]);
        StoreHalfClipped(frame,
                         Add(Add(Mul(c[0].mixVolume, left), Mul(c[2].mixVolume, left2)),
                             Mul(c[4].mixVolume, left3)));
        StoreHalfClipped(frame + 2,
                         Add(Add(Mul(c[3].mixVolume, right2), Mul(c[1].mixVolume, right)),
                             Mul(c[5].mixVolume, right3)));
    });
    mixer.writePosition = static_cast<u16>(end);
}

void MixAudioStereoSamples(s16 *ring, u32 bytes, u32 pairOffset) {
    auto &mixer = *GetSdlMixerState();
    MixerChannel *c =
        &mixer.musicChannels[FIRST_PAIRED_CHANNEL + pairOffset / sizeof(MixerChannel)];
    u32 end = MixRing(ring, mixer.writePosition, bytes, 4, [c](u8 *frame) {
        s32 left = Mul(c[0].mixVolume, NextSample(c[0]));
        s32 right = Mul(c[1].mixVolume, NextSample(c[1]));
        StoreHalfClipped(frame, Add(LoadHalf(frame), left));
        StoreHalfClipped(frame + 2, Add(LoadHalf(frame + 2), right));
    });
    mixer.writePosition = static_cast<u16>(end);
}

void MixSoundEffectChannels(s16 *ring, u32 bytes, u32 offset) {
    auto &mixer = *GetSdlMixerState();
    MixerChannel *c = mixer.soundEffectChannels;
    const auto echoOffset = mixer.soundEffectEchoOffset;
    const auto mask = mixer.soundEffectSampleMask;
    MixRing(ring, offset, bytes, 8, [c, echoOffset, mask](u8 *at) {
        for (int i = 0; i < 5; ++i) {
            u64 sum = static_cast<u64>(c[i].fraction) + c[i].step;
            c[i].sample += static_cast<s64>(sum >> 32);
            c[i].fraction = static_cast<u32>(sum);
        }
        // Channel 0 adds its echo, a second sample at a fixed distance, when one is set.
        s32 echo = echoOffset != 0 ? c[0].sample[echoOffset] : 0;
        s32 mixed = Mul(c[0].mixVolume, echo + c[0].sample[0]);
        for (int i = 1; i < 5; ++i) {
            mixed = Add(mixed, Mul(c[i].mixVolume, c[i].sample[0]));
        }
        // The processor has the 32-bit sum sign-extended in a 64-bit register, and the mask
        // preserves its upper bits.
        u64 frame = static_cast<u64>(static_cast<s64>(mixed)) & mask;
        u64 doubled = (frame << 32) + frame;
        // The doubleword of the ring is big-endian on the console: four halfwords in order.
        u64 existing = 0;
        for (int i = 0; i < 4; ++i) {
            existing = (existing << 16) | static_cast<u16>(LoadHalf(at + i * 2));
        }
        u64 result = (doubled + (existing & mask)) & mask;
        for (int i = 0; i < 4; ++i) {
            StoreHalf(at + i * 2, static_cast<s16>(result >> (48 - i * 16)));
        }
    });
}

} // extern "C"

namespace coastline {

N64MixerLock::N64MixerLock() : m_mutex(N64Mixer::Get().mutex.get()) {
    SDL_LockMutex(m_mutex);
}

N64MixerLock::~N64MixerLock() {
    SDL_UnlockMutex(m_mutex);
}

void PublishN64Volumes() {
    N64MixerLock lock;
    auto &mixer = N64Mixer::Get();
    mixer.volumes = {
        mixer.music.bgmVolume, g_bMusicVolume, mixer.music.sfxMasterVolume, mixer.music.sfxVolume};
}

N64Audio::N64Audio() : m_mixer(N64Mixer::Get()) {
    // Another player may be mixing the same channels.
    N64MixerLock lock;
    PublishN64Volumes();
    // InitAudioSystem() of the console, without the interface and the thread.
    auto &music = m_mixer.music;
    auto &mixer = m_mixer.channels;
    std::ranges::fill(music.ring, 0);
    mixer.soundEffectSampleMask = SOUND_EFFECT_SAMPLE_MASK;
    for (int i = 0; i < MIXER_CHANNELS; ++i) {
        mixer.musicChannels[i].sample = reinterpret_cast<s8 *>(music.ring);
        mixer.musicChannels[i].step = 0;
        mixer.musicChannels[i].volume = MUSIC_CHANNEL_VOLUME;
    }
    for (auto &channel : mixer.soundEffectChannels) {
        channel.sample = music.silence;
        channel.step = 0;
        channel.volume = SOUND_LEVELS_START;
    }
    mixer.writePosition = 0;
    MixAudioSamples(music.ring, RING_BYTES);
    BuildFrequencyScaleTable();
}

N64Audio::~N64Audio() {
    if (m_stream != nullptr) {
        SDL_DestroyAudioStream(m_stream);
    }
}

bool N64Audio::OpenDevice() {
    SDL_AudioSpec spec{SDL_AUDIO_S16, 2, N64_AUDIO_RATE};
    m_stream =
        SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, StreamCallback, this);
    if (m_stream == nullptr) {
        SDL_LogWarn(SDL_LOG_CATEGORY_AUDIO, "The audio device did not open: %s", SDL_GetError());
        return false;
    }
    return UpdateVolumeProcessing();
}

void N64Audio::Play(std::span<const std::uint8_t> module) {
    N64MixerLock lock;
    // LoadMusicModule() reads the module in place, so it is retained.
    m_module.assign(module.begin(), module.end());
    // LoadAndPlayTrackBgm() of the console silences the channels before a new module. The
    // channels otherwise play the notes of the old module on from their samples.
    ResetMusicPlaybackState();
    InitMusicPlayback(m_module.data(), m_mixer.music.moduleWork);
}

void N64Audio::Stop() {
    N64MixerLock lock;
    m_mixer.music.playbackEnabled = FALSE;
    ResetMusicChannels();
    // The channels otherwise loop the samples of their last notes. A channel stopped on the ring
    // would read the mixed output back as a level that steps each time the ring is mixed again.
    for (int i = 0; i < m_mixer.music.channelCount; ++i) {
        m_mixer.channels.musicChannels[i].sample = m_mixer.music.silence;
        m_mixer.channels.musicChannels[i].step = 0;
    }
}

bool N64Audio::Pause() {
    m_paused = true;
    return UpdateVolumeProcessing();
}

bool N64Audio::Resume() {
    m_paused = false;
    return UpdateVolumeProcessing();
}

bool N64Audio::UpdateVolumeProcessing() {
    bool paused;
    {
        N64MixerLock lock;
        const bool music =
            m_musicEnabled && m_mixer.volumes.bgm != 0 && m_mixer.music.playbackEnabled;
        const bool sfx = m_sfxEnabled && m_mixer.volumes.sfxMaster != 0;
        paused = m_paused || (!music && !sfx);
    }
    if (m_stream == nullptr || paused == m_devicePaused) {
        return true;
    }
    const bool changed =
        paused ? SDL_PauseAudioStreamDevice(m_stream) : SDL_ResumeAudioStreamDevice(m_stream);
    if (changed) {
        m_devicePaused = paused;
    }
    return changed;
}

void N64Audio::SetProcessingEnabled(bool music, bool soundEffects) {
    {
        N64MixerLock lock;
        m_musicEnabled = music;
        m_sfxEnabled = soundEffects;
    }
    UpdateVolumeProcessing();
}

void N64Audio::SetMusicGain(float gain) {
    N64MixerLock lock;
    m_musicGain = static_cast<std::uint32_t>(std::clamp(gain, 0.0f, 8.0f) * 256.0f + 0.5f);
}

void N64Audio::Tick(std::vector<std::int16_t> &output) {
    auto &music = m_mixer.music;
    auto &mixer = m_mixer.channels;
    const auto &volumes = m_mixer.volumes;
    // The mixers process pairs of stereo frames. Fractional frames accumulate between ticks.
    m_frameRemainder += N64_AUDIO_RATE;
    const u32 frames = static_cast<u32>(m_frameRemainder / TICKS_PER_SECOND) & ~1u;
    m_frameRemainder -= static_cast<u64>(frames) * TICKS_PER_SECOND;
    const u32 played = frames * 2 * sizeof(s16);
    for (u32 i = 0; i < played; i += 2) {
        output.push_back(music.ring[((m_outputReadOffset + i) % RING_BYTES) / 2]);
    }
    m_outputReadOffset = (m_outputReadOffset + played) % RING_BYTES;

    // AudioMixingThreadMain() of the console, one message of the timer.
    if (m_musicProcessing) {
        m_musicTick = !m_musicTick;
    }
    for (int i = 0; m_musicProcessing && i < music.channelCount; ++i) {
        MixerChannel &channel = mixer.musicChannels[i];
        channel.mixVolume = static_cast<s32>((static_cast<std::uint64_t>(channel.volume) *
                                              volumes.bgm * volumes.music * m_musicGain) >>
                                             24);
    }
    if (m_sfxProcessing) {
        for (auto &channel : mixer.soundEffectChannels) {
            u32 levels = static_cast<u32>(channel.volume);
            u8 master = volumes.sfxMaster;
            u8 sfx = volumes.sfx;
            channel.mixVolume = static_cast<s32>(((((levels >> 16) * master * sfx) >> 16) << 16) +
                                                 (((levels & 0xFFFF) * master * sfx) >> 16));
        }
    }
    // Prime the lookahead once, then replenish only the PCM consumed by each tick.
    const u32 mixBytes = played + (m_primed ? 0 : MUSIC_LEAD);
    u16 writePos = mixer.writePosition;
    if (m_musicProcessing) {
        CheckMusicLoopPoints();
        if (m_musicTick && music.playbackEnabled) {
            UpdateMusicPlayback();
        }
        MixAudioSamples(music.ring, mixBytes);
        for (s32 i = FIRST_PAIRED_CHANNEL; i < music.channelCount; i += 2) {
            mixer.writePosition = writePos;
            MixAudioStereoSamples(
                music.ring, mixBytes, (i - FIRST_PAIRED_CHANNEL) * sizeof(MixerChannel));
        }
    } else {
        mixer.writePosition =
            static_cast<u16>(MixRing(music.ring, writePos, mixBytes, 4, [](u8 *frame) {
                StoreHalf(frame, 0);
                StoreHalf(frame + 2, 0);
            }));
    }
    const u32 bytes = played + (m_primed ? 0 : SOUND_EFFECT_LEAD);
    if (m_sfxProcessing) {
        MixSoundEffectChannels(music.ring, bytes, m_soundEffectMixOffset);
        CheckSoundEffectLoopPoints();
    }
    m_soundEffectMixOffset += bytes;
    if (m_soundEffectMixOffset > RING_BYTES) {
        m_soundEffectMixOffset -= RING_BYTES;
    }
    m_primed = true;
}

std::vector<std::int16_t> N64Audio::Render(std::size_t frames) {
    N64MixerLock lock;
    PublishN64Volumes();
    return RenderLocked(frames);
}

std::vector<std::int16_t> N64Audio::RenderLocked(std::size_t frames) {
    const bool music = m_musicEnabled && m_mixer.volumes.bgm != 0;
    const bool sfx = m_sfxEnabled && m_mixer.volumes.sfxMaster != 0;
    if (!music && !sfx && (m_musicProcessing || m_sfxProcessing)) {
        m_pending.clear();
        std::ranges::fill(m_mixer.music.ring, 0);
        m_mixer.channels.writePosition = 0;
        m_outputReadOffset = 0;
        m_soundEffectMixOffset = 0;
        m_primed = false;
    }
    m_musicProcessing = music;
    m_sfxProcessing = sfx;
    if (!music && !sfx) {
        return std::vector<std::int16_t>(frames * 2, 0);
    }
    std::vector<std::int16_t> output(m_pending);
    m_pending.clear();
    while (output.size() < frames * 2) {
        Tick(output);
    }
    m_pending.assign(output.begin() + static_cast<std::ptrdiff_t>(frames * 2), output.end());
    output.resize(frames * 2);
    return output;
}

void SDLCALL N64Audio::StreamCallback(void *userData,
                                      SDL_AudioStream *stream,
                                      int additional,
                                      [[maybe_unused]] int total) {
    auto *audio = static_cast<N64Audio *>(userData);
    if (additional <= 0) {
        return;
    }
    std::vector<std::int16_t> samples;
    {
        // The stream of SDL is locked around the callback, and the mixers after it. The game
        // thread does not lock a stream while it has the mixers locked.
        N64MixerLock lock;
        samples = audio->RenderLocked(static_cast<std::size_t>(additional) / 4 + 1);
    }
    SDL_PutAudioStreamData(
        stream, samples.data(), static_cast<int>(samples.size() * sizeof(std::int16_t)));
}

} // namespace coastline
