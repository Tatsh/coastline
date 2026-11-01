#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <vector>

#include "soundeffects.h"

namespace {

void WriteWave(const std::filesystem::path &path,
               int rate,
               int amplitude,
               int frames = 4096,
               bool pulse = false) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::binary);
    auto word = [&](std::uint32_t value, int size) {
        for (int index = 0; index < size; ++index) {
            file.put(static_cast<char>(value >> (index * 8)));
        }
    };
    file.write("RIFF", 4);
    word(36 + frames * 2, 4);
    file.write("WAVEfmt ", 8);
    word(16, 4);
    word(1, 2);
    word(1, 2);
    word(rate, 4);
    word(rate * 2, 4);
    word(2, 2);
    word(16, 2);
    file.write("data", 4);
    word(frames * 2, 4);
    for (int index = 0; index < frames; ++index) {
        const int value =
            pulse ? (index == 0 ? amplitude : 1) : ((index / 64) % 2 ? amplitude : -amplitude);
        word(static_cast<std::uint16_t>(value), 2);
    }
}

struct Wave {
    SDL_AudioSpec spec{};
    Uint8 *data = nullptr;
    Uint32 bytes = 0;
    ~Wave() {
        SDL_free(data);
    }
    double Rms() const {
        double energy = 0;
        assert(spec.format == SDL_AUDIO_S16LE);
        for (Uint32 at = 0; at < bytes; at += 2) {
            const auto sample = static_cast<std::int16_t>(data[at] | (data[at + 1] << 8));
            energy += static_cast<double>(sample) * sample;
        }
        return std::sqrt(energy / (bytes / 2));
    }
};

} // namespace

int main() {
    assert(SDL_Init(0));
    const auto root = std::filesystem::temp_directory_path() /
                      ("coastline-sounds-" +
                       std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    const auto pc = root / "sfx/Beep.wav";
    const auto assets = root / std::filesystem::path(u8"音声");
    const auto native = assets / "n64/base/sounds";
    WriteWave(pc, 22050, 4096);
    WriteWave(native / "beep.wav", 10999, 16384);
    WriteWave(native / "roll-surface-1.wav", 10999, 8192);
    WriteWave(native / "roll-surface-2.wav", 10999, 8192, 2048);
    WriteWave(native / "impact.wav", 10999, 32767, 4096, true);
    WriteWave(assets / "sfx/front-end5.wav", 22050, 12000);
    WriteWave(root / "n64/base/segments/scrollSound.wav", 10999, 3000);
    using Effects = coastline::SoundEffects;
    {
        Effects effects(assets, Effects::Source::Pc, true);
        Wave wave;
        assert(Effects::LoadRaceWave(
            (root / "sfx/beep.wav").string().c_str(), 13, &wave.spec, &wave.data, &wave.bytes));
        assert(wave.spec.freq == 22050 && wave.bytes == 8192);
        assert(std::abs(wave.Rms() - 4096) < 1);
    }
    {
        Effects effects(assets, Effects::Source::N64, true);
        Wave wave;
        assert(Effects::LoadRaceWave(pc.string().c_str(), 13, &wave.spec, &wave.data, &wave.bytes));
        assert(wave.spec.freq == 22050 && wave.spec.channels == 1);
        assert(wave.bytes > 16384 && wave.bytes < 16500);
        assert(std::abs(wave.Rms() - 4096) < 4);
        std::filesystem::remove(native / "beep.wav");
        Wave cached;
        assert(Effects::LoadRaceWave(
            pc.string().c_str(), 13, &cached.spec, &cached.data, &cached.bytes));
        assert(cached.bytes == wave.bytes && std::memcmp(cached.data, wave.data, wave.bytes) == 0);
        Wave first;
        Wave second;
        assert(
            Effects::LoadRaceWave(pc.string().c_str(), 8, &first.spec, &first.data, &first.bytes));
        assert(Effects::LoadRaceWave(
            pc.string().c_str(), 9, &second.spec, &second.data, &second.bytes));
        assert(first.bytes > second.bytes * 1.99 && first.bytes < second.bytes * 2.01);
        Wave missing;
        assert(Effects::LoadRaceWave(
            pc.string().c_str(), 14, &missing.spec, &missing.data, &missing.bytes));
        assert(missing.bytes == 8192 && std::abs(missing.Rms() - 4096) < 1);
        Wave taunt;
        assert(
            Effects::LoadRaceWave(pc.string().c_str(), 20, &taunt.spec, &taunt.data, &taunt.bytes));
        assert(taunt.Rms() == 0);
        Wave impulse;
        assert(Effects::LoadRaceWave(
            pc.string().c_str(), 2, &impulse.spec, &impulse.data, &impulse.bytes));
        int peak = 0;
        for (Uint32 at = 0; at < impulse.bytes; at += 2) {
            const auto sample =
                static_cast<std::int16_t>(impulse.data[at] | (impulse.data[at + 1] << 8));
            peak = std::max(peak, std::abs(static_cast<int>(sample)));
        }
        assert(peak <= 32113 && peak > 30000);
        assert(impulse.Rms() < 4096);
    }
    {
        Effects effects(assets, Effects::Source::Pc, true);
        Wave menu;
        assert(Effects::LoadMenuWave(
            root / "n64/base/segments/scrollSound.wav", &menu.spec, &menu.data, &menu.bytes));
        assert(menu.spec.format == SDL_AUDIO_U8 && menu.spec.freq == 10999);
        assert(menu.bytes > 2000 && menu.bytes < 2100);
    }
    {
        Effects effects(assets, Effects::Source::Pc, false);
        Wave menu;
        assert(Effects::LoadMenuWave(
            root / "n64/base/segments/scrollSound.wav", &menu.spec, &menu.data, &menu.bytes));
        for (Uint32 at = 0; at < menu.bytes; ++at) {
            assert(menu.data[at] == 128);
        }
        Wave race;
        assert(Effects::LoadRaceWave(pc.string().c_str(), 13, &race.spec, &race.data, &race.bytes));
        assert(race.Rms() == 0);
    }
    Wave raw;
    assert(Effects::LoadRaceWave(pc.string().c_str(), 13, &raw.spec, &raw.data, &raw.bytes));
    assert(raw.bytes == 8192);
    std::filesystem::remove_all(root);
    SDL_Quit();
}
