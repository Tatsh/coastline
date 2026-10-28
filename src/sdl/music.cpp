#include "music.h"

#include <algorithm>
#include <bit>
#include <cstring>
#include <fstream>

#include "assetchain.h"

extern "C" {
#include "audio.h"
}

// The storage of the music player and the mixers. The console stores it in its BSS and data
// segments, and the values are the initial values of the USA release.
extern "C" {
u16 g_wAudioBufferWritePos;
MixerChannel g_aMixerChannels[MIXER_CHANNELS];
u64 g_qwSoundEffectSampleMask;
MixerChannel g_aSoundEffectChannels[SOUND_EFFECT_CHANNELS];
s32 g_dwSoundEffectEchoOffset;
s32 g_nAudioChannelCount;
u8 g_bBgmVolume = 0x24;
#ifndef COASTLINE_PC_GAME
// The game code of the PC release defines it in numericdata.c.
u8 g_bMusicVolume = 0xFF;
#endif
u8 g_bSfxMasterVolume = 0xFF;
u8 g_bSfxVolume = 0xFF;
f32 g_afNoteFrequencies[MUSIC_NOTES] = {
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
s32 g_dwMusicPatternRow;
s8 g_SilenceSampleData[4];
s32 g_dwAudioTickToggle;
s32 g_dwAiDmaPrevLength;
u32 g_dwSoundEffectMixOffset;
s16 g_AudioDmaBuffer[AUDIO_RING_BYTES / sizeof(s16)];
// The console reserves 0x200 and 0x400 bytes of pointers for these.
XmSample *g_apInstrumentData[128];
u8 *g_apPatternData[256];
MusicChannel g_MusicChannelStates[MIXER_CHANNELS];
SoundEffectSample g_aSoundEffectSamples[SOUND_EFFECT_CHANNELS];
s32 g_bMusicPlaybackEnabled;
MusicPlayer g_MusicPlayer;
u64 g_aFreqScaleTable[MUSIC_OCTAVES][MUSIC_NOTES];
s32 g_dwAudioMixBytes;
s32 g_dwAiDmaCurrentLength;
s32 g_dwAudioChannelLoopIdx;
// The largest module, the menu music, expands to under 300 KiB with its sample tails.
u8 g_abMusicModuleWorkBuffer[0x100000];

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
constexpr u32 SOUND_EFFECT_LEAD = 0x5000;
constexpr s32 FIRST_PAIRED_CHANNEL = 6;
constexpr u32 TICKS_PER_SECOND = 100;
constexpr u64 SOUND_EFFECT_SAMPLE_MASK = 0xFFFEFFFEFFFEFFFEULL;
constexpr s32 MUSIC_CHANNEL_VOLUME = 0x20;
constexpr s32 SOUND_LEVELS_START = 0x20;

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

inline s32 LoadHalf(const u8 *at) {
    s16 half;
    std::memcpy(&half, at, sizeof(half));
    return half;
}

} // namespace

extern "C" {

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
    MixerChannel *c = g_aMixerChannels;
    u32 end = MixRing(ring, g_wAudioBufferWritePos, bytes, 4, [c](u8 *frame) {
        s32 left = NextSample(c[0]);
        s32 left2 = NextSample(c[2]);
        s32 right = NextSample(c[1]);
        s32 right2 = NextSample(c[3]);
        s32 left3 = NextSample(c[4]);
        s32 right3 = NextSample(c[5]);
        StoreHalf(frame,
                  Add(Add(Mul(c[0].mixVolume, left), Mul(c[2].mixVolume, left2)),
                      Mul(c[4].mixVolume, left3)));
        StoreHalf(frame + 2,
                  Add(Add(Mul(c[3].mixVolume, right2), Mul(c[1].mixVolume, right)),
                      Mul(c[5].mixVolume, right3)));
    });
    g_wAudioBufferWritePos = static_cast<u16>(end);
}

void MixAudioStereoSamples(s16 *ring, u32 bytes, u32 pairOffset) {
    MixerChannel *c = &g_aMixerChannels[FIRST_PAIRED_CHANNEL + pairOffset / sizeof(MixerChannel)];
    u32 end = MixRing(ring, g_wAudioBufferWritePos, bytes, 4, [c](u8 *frame) {
        s32 left = Mul(c[0].mixVolume, NextSample(c[0]));
        s32 right = Mul(c[1].mixVolume, NextSample(c[1]));
        StoreHalf(frame, Add(LoadHalf(frame), left));
        StoreHalf(frame + 2, Add(LoadHalf(frame + 2), right));
    });
    g_wAudioBufferWritePos = static_cast<u16>(end);
}

void MixSoundEffectChannels(s16 *ring, u32 bytes, u32 offset) {
    MixerChannel *c = g_aSoundEffectChannels;
    MixRing(ring, offset, bytes, 8, [c](u8 *at) {
        for (int i = 0; i < 5; ++i) {
            u64 sum = static_cast<u64>(c[i].fraction) + c[i].step;
            c[i].sample += static_cast<s64>(sum >> 32);
            c[i].fraction = static_cast<u32>(sum);
        }
        // Channel 0 adds its echo, a second sample at a fixed distance, when one is set.
        s32 echo = g_dwSoundEffectEchoOffset != 0 ? c[0].sample[g_dwSoundEffectEchoOffset] : 0;
        s32 mixed = Mul(c[0].mixVolume, echo + c[0].sample[0]);
        for (int i = 1; i < 5; ++i) {
            mixed = Add(mixed, Mul(c[i].mixVolume, c[i].sample[0]));
        }
        // The processor has the 32-bit sum sign-extended in a 64-bit register, and the mask
        // preserves its upper bits.
        u64 mask = g_qwSoundEffectSampleMask;
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

std::expected<std::vector<std::uint8_t>, std::string>
ReadRomModule(std::span<const std::uint8_t> rom, std::uint32_t offset) {
    if (offset >= rom.size()) {
        return std::unexpected("The module is past the end of the image.");
    }
    auto data = Tools::Common::decompress(rom.subspan(offset));
    if (!data) {
        return std::unexpected(data.error().message);
    }
    return std::move(*data);
}

N64Audio::N64Audio() {
    // InitAudioSystem() of the console, without the interface and the thread.
    g_qwSoundEffectSampleMask = SOUND_EFFECT_SAMPLE_MASK;
    for (int i = 0; i < MIXER_CHANNELS; ++i) {
        g_aMixerChannels[i].sample = reinterpret_cast<s8 *>(g_AudioDmaBuffer);
        g_aMixerChannels[i].step = 0;
        g_aMixerChannels[i].volume = MUSIC_CHANNEL_VOLUME;
    }
    for (auto &channel : g_aSoundEffectChannels) {
        channel.sample = g_SilenceSampleData;
        channel.step = 0;
        channel.volume = SOUND_LEVELS_START;
    }
    g_wAudioBufferWritePos = 0;
    MixAudioSamples(g_AudioDmaBuffer, RING_BYTES);
    BuildFrequencyScaleTable();
    // osAiSetNextBuffer() queues the whole ring, and the interface plays it from its start.
    m_aiLength = RING_BYTES;
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
    SDL_ResumeAudioStreamDevice(m_stream);
    return true;
}

void N64Audio::Play(std::span<const std::uint8_t> module) {
    if (m_stream != nullptr) {
        SDL_LockAudioStream(m_stream);
    }
    // LoadMusicModule() reads the module in place, so it is retained.
    m_module.assign(module.begin(), module.end());
    InitMusicPlayback(m_module.data(), g_abMusicModuleWorkBuffer);
    if (m_stream != nullptr) {
        SDL_UnlockAudioStream(m_stream);
    }
}

void N64Audio::Tick(std::vector<std::int16_t> &output) {
    // The interface plays a tick's worth of the ring. Its length register counts down in
    // doublewords, and the thread queues the ring again before it runs dry.
    m_aiRemainder += static_cast<u64>(N64_AUDIO_RATE) * 4;
    u32 played = static_cast<u32>(m_aiRemainder / TICKS_PER_SECOND) & ~7u;
    m_aiRemainder -= static_cast<u64>(played) * TICKS_PER_SECOND;
    u32 start = RING_BYTES - m_aiLength;
    for (u32 i = 0; i < played; i += 2) {
        output.push_back(g_AudioDmaBuffer[((start + i) % RING_BYTES) / 2]);
    }
    m_aiLength = m_aiLength > played ? m_aiLength - played : m_aiLength + RING_BYTES - played;

    // AudioMixingThreadMain() of the console, one message of the timer.
    g_dwAudioTickToggle ^= 1;
    for (int i = 0; i < g_nAudioChannelCount; ++i) {
        MixerChannel &channel = g_aMixerChannels[i];
        channel.mixVolume = static_cast<s32>(
            (static_cast<u32>(channel.volume) * g_bBgmVolume * g_bMusicVolume) >> 16);
    }
    for (auto &channel : g_aSoundEffectChannels) {
        u32 levels = static_cast<u32>(channel.volume);
        channel.mixVolume =
            static_cast<s32>(((((levels >> 16) * g_bSfxMasterVolume * g_bSfxVolume) >> 16) << 16) +
                             (((levels & 0xFFFF) * g_bSfxMasterVolume * g_bSfxVolume) >> 16));
    }
    u32 length = m_aiLength;
    s32 position = static_cast<s32>(length) - MUSIC_LEAD;
    if (position < 0) {
        position += RING_BYTES;
    }
    s32 mixBytes = g_dwAiDmaPrevLength - position;
    g_dwAiDmaPrevLength = position;
    g_dwAiDmaCurrentLength = position;
    if (mixBytes < 0) {
        mixBytes += RING_BYTES;
    }
    g_dwAudioMixBytes = mixBytes;
    CheckMusicLoopPoints();
    if (g_dwAudioTickToggle && g_bMusicPlaybackEnabled) {
        UpdateMusicPlayback();
    }
    u16 writePos = g_wAudioBufferWritePos;
    MixAudioSamples(g_AudioDmaBuffer, static_cast<u32>(g_dwAudioMixBytes));
    for (s32 i = FIRST_PAIRED_CHANNEL; i < g_nAudioChannelCount; i += 2) {
        g_wAudioBufferWritePos = writePos;
        MixAudioStereoSamples(g_AudioDmaBuffer,
                              static_cast<u32>(g_dwAudioMixBytes),
                              (i - FIRST_PAIRED_CHANNEL) * sizeof(MixerChannel));
    }
    u32 offset = g_dwSoundEffectMixOffset;
    u32 bytes = (((SOUND_EFFECT_LEAD - length) >> 3) << 3) - offset;
    if (bytes > RING_BYTES) {
        bytes -= RING_BYTES;
    }
    MixSoundEffectChannels(g_AudioDmaBuffer, bytes, offset);
    CheckSoundEffectLoopPoints();
    g_dwSoundEffectMixOffset += bytes;
    if (g_dwSoundEffectMixOffset > RING_BYTES) {
        g_dwSoundEffectMixOffset -= RING_BYTES;
    }
}

std::vector<std::int16_t> N64Audio::Render(std::size_t frames) {
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
    auto samples = audio->Render(static_cast<std::size_t>(additional) / 4 + 1);
    SDL_PutAudioStreamData(
        stream, samples.data(), static_cast<int>(samples.size() * sizeof(std::int16_t)));
}

bool WriteWav(const std::filesystem::path &path, std::span<const std::int16_t> samples, int rate) {
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    auto put32 = [&](std::uint32_t value) {
        for (int i = 0; i < 4; ++i) {
            file.put(static_cast<char>(value >> (i * 8)));
        }
    };
    auto put16 = [&](std::uint16_t value) {
        file.put(static_cast<char>(value));
        file.put(static_cast<char>(value >> 8));
    };
    auto dataBytes = static_cast<std::uint32_t>(samples.size() * 2);
    file.write("RIFF", 4);
    put32(36 + dataBytes);
    file.write("WAVEfmt ", 8);
    put32(16);
    put16(1);
    put16(2);
    put32(static_cast<std::uint32_t>(rate));
    put32(static_cast<std::uint32_t>(rate) * 4);
    put16(4);
    put16(16);
    file.write("data", 4);
    put32(dataBytes);
    for (auto sample : samples) {
        put16(static_cast<std::uint16_t>(sample));
    }
    return static_cast<bool>(file);
}

} // namespace coastline
