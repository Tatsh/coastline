#include <algorithm>
#include <array>
#include <cstdlib>
#include <iostream>
#include <span>
#include <vector>

#include "common/audio.h"
#include "music.h"

extern "C" {
u8 g_bMusicVolume = 255;
}

namespace {

void Check(bool condition, const char *message) {
    if (!condition) {
        std::cerr << "Failed. " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

std::vector<u8> Module() {
    std::vector<u8> bytes(0x2000);
    auto word = [&](std::size_t at, u32 value) {
        for (int shift = 0; shift < 32; shift += 8) {
            bytes[at++] = static_cast<u8>(value >> shift);
        }
    };
    word(0x3C, 0x114);
    bytes[0x40] = 1;
    bytes[0x44] = 6;
    bytes[0x46] = 1;
    bytes[0x48] = 1;
    bytes[0x4C] = 6;
    const std::array<u8, 16> rows{
        49, 1, 0x40, 0, 0, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80};
    std::size_t at = 0x150;
    word(at, 9);
    bytes[at + 5] = 2;
    bytes[at + 7] = rows.size();
    std::ranges::copy(rows, bytes.begin() + at + 9);
    at += 9 + rows.size();
    word(at, 0x20);
    bytes[at + 0x1B] = 1;
    at += 0x20;
    word(at, 4096);
    word(at + 8, 4096);
    bytes[at + 0xC] = 32;
    bytes[at + 0xE] = 1;
    std::fill_n(bytes.begin() + at + 0x28, 4096, 4);
    return bytes;
}

std::vector<std::int16_t> Render(coastline::N64Audio &audio, bool split) {
    if (!split) {
        return audio.Render(22000);
    }
    std::vector<std::int16_t> output;
    for (std::size_t frames : {1, 511, 441, 2048, 8192, 10807}) {
        auto samples = audio.Render(frames);
        output.insert(output.end(), samples.begin(), samples.end());
    }
    return output;
}

} // namespace

int main() {
    Check(SDL_SetHint(SDL_HINT_AUDIO_DRIVER, "dummy"), "the dummy audio driver is selected");
    Check(SDL_Init(SDL_INIT_AUDIO), "SDL audio starts");
    const auto module = Module();
    std::vector<std::int16_t> expected;
    for (int run = 0; run < 64; ++run) {
        coastline::N64MixerContext mixer;
        Check(g_nAudioChannelCount == MIXER_CHANNELS && g_bBgmVolume == 48 && g_bSfxVolume == 255 &&
                  g_bSfxMasterVolume == 255,
              "a fresh context restores default channel count and volumes");
        Check(g_MusicPlayer.orders == nullptr && g_apInstrumentData[127] == nullptr &&
                  g_apPatternData[255] == nullptr && g_MusicChannelStates[15].instrument == 0 &&
                  g_aSoundEffectSamples[5].start == nullptr && g_dwSoundEffectEchoOffset == 0 &&
                  g_aFreqScaleTable[9][11] == 0,
              "a fresh context resets playback and sample references");
        Check(std::ranges::all_of(g_abMusicModuleWorkBuffer, [](auto byte) { return byte == 0; }) &&
                  std::ranges::all_of(g_AudioDmaBuffer, [](auto sample) { return sample == 0; }) &&
                  std::ranges::all_of(g_SilenceSampleData, [](auto sample) { return sample == 0; }),
              "a fresh context clears its work and sample buffers");
        {
            coastline::N64Audio audio;
            audio.Play(module);
            auto samples = Render(audio, run % 2 != 0);
            Check(samples.size() == 44000 &&
                      std::ranges::any_of(samples, [](auto sample) { return sample != 0; }),
                  "the synthetic XM module produces stereo audio");
            if (run == 0) {
                expected = samples;
            } else {
                Check(samples == expected,
                      "fresh sessions and fragmented requests preserve rendered PCM");
            }
            audio.Stop();
            audio.Render(8192);
            samples = audio.Render(8192);
            Check(std::ranges::all_of(samples, [](auto sample) { return sample == 0; }),
                  "stopping the module drains the queued audio to silence");
        }
        g_bBgmVolume = 1;
        g_bSfxVolume = 2;
        g_bSfxMasterVolume = 3;
        g_apInstrumentData[127] = g_apInstrumentData[0];
        g_apPatternData[255] = g_abMusicModuleWorkBuffer;
        g_MusicChannelStates[15].instrument = 1;
        g_aSoundEffectSamples[5].start = g_SilenceSampleData;
        g_dwSoundEffectEchoOffset = 100;
        g_aFreqScaleTable[9][11] = 123;
        std::ranges::fill(g_abMusicModuleWorkBuffer, 0x7F);
        std::ranges::fill(g_SilenceSampleData, 1);
        std::ranges::fill(g_AudioDmaBuffer, 1);
    }
    {
        coastline::N64MixerContext mixer;
        std::array<s8, 10000> effect;
        effect.fill(8);
        coastline::N64Audio audio;
        N64InitSoundChannel(1, effect.data(), 8192, 8192);
        SetSoundChannelVolume(1, 0x00200020);
        const auto samples = audio.Render(8192);
        Check(std::ranges::any_of(samples, [](auto sample) { return sample != 0; }),
              "sound effects play through the context without a music module");
    }
    {
        coastline::N64MixerContext mixer;
        coastline::N64Audio audio;
        Check(audio.OpenDevice(), "the dummy playback device opens");
        audio.Play(module);
        SDL_Delay(30);
        Check(audio.Pause() && audio.Resume(), "the playback device pauses and resumes");
        audio.Stop();
    }
    SDL_Quit();
    return EXIT_SUCCESS;
}
