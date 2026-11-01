#include "soundeffects.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstring>
#include <limits>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace coastline {

struct SoundEffects::State {
    static constexpr const char *PROPERTY = "coastline.sound-effects";
    static constexpr std::array<const char *, 25> RACE_SOUNDS{"engine",
                                                              "vehicle-impact",
                                                              "impact",
                                                              "landing",
                                                              "skid-surface-2",
                                                              "skid-surface-1",
                                                              "6",
                                                              "skid-surface-0",
                                                              "roll-surface-1",
                                                              "roll-surface-2",
                                                              "roll-surface-0",
                                                              "roll-snow",
                                                              "slow-surface",
                                                              "beep",
                                                              "go",
                                                              "waterfall",
                                                              "vehicle-impact",
                                                              "vehicle-impact",
                                                              "impact",
                                                              "impact",
                                                              nullptr,
                                                              nullptr,
                                                              nullptr,
                                                              nullptr,
                                                              "engine"};

    struct Wave {
        SDL_AudioSpec spec{};
        std::vector<Uint8> bytes;
    };

    std::filesystem::path assets;
    Source source;
    bool enabled;
    std::map<std::string, Wave> cache;

    static State *Active() {
        return static_cast<State *>(
            SDL_GetPointerProperty(SDL_GetGlobalProperties(), PROPERTY, nullptr));
    }

    static std::string Utf8(const std::filesystem::path &path) {
        const auto text = path.u8string();
        return {text.begin(), text.end()};
    }

    static std::filesystem::path FromUtf8(const char *text) {
        return std::u8string(text, text + std::strlen(text));
    }

    static std::optional<Wave> Read(const std::filesystem::path &path) {
        SDL_AudioSpec spec{};
        Uint8 *data = nullptr;
        Uint32 bytes = 0;
        const auto file = Utf8(path);
        bool loaded = SDL_LoadWAV(file.c_str(), &spec, &data, &bytes);
        if (!loaded) {
            int count = 0;
            const auto directory = path.has_parent_path() ? Utf8(path.parent_path()) : ".";
            auto **matches = SDL_GlobDirectory(
                directory.c_str(), Utf8(path.filename()).c_str(), SDL_GLOB_CASEINSENSITIVE, &count);
            loaded = matches && count > 0 &&
                     SDL_LoadWAV(Utf8(FromUtf8(directory.c_str()) / FromUtf8(matches[0])).c_str(),
                                 &spec,
                                 &data,
                                 &bytes);
            SDL_free(matches);
        }
        if (!loaded || bytes == 0) {
            SDL_free(data);
            return std::nullopt;
        }
        Wave wave{spec, std::vector<Uint8>(data, data + bytes)};
        SDL_free(data);
        return wave;
    }

    static bool Convert(Wave &wave, SDL_AudioSpec target) {
        if (wave.bytes.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
            return false;
        }
        Uint8 *converted = nullptr;
        int bytes = 0;
        if (!SDL_ConvertAudioSamples(&wave.spec,
                                     wave.bytes.data(),
                                     static_cast<int>(wave.bytes.size()),
                                     &target,
                                     &converted,
                                     &bytes)) {
            return false;
        }
        wave.spec = target;
        wave.bytes.assign(converted, converted + bytes);
        SDL_free(converted);
        return true;
    }

    static bool Match(Wave &wave, Wave reference, SDL_AudioSpec target) {
        const SDL_AudioSpec analysis{SDL_AUDIO_F32, target.channels, target.freq};
        if (!Convert(wave, analysis) || !Convert(reference, analysis)) {
            return false;
        }
        auto levels = [](const Wave &input) {
            double energy = 0;
            float peak = 0;
            for (std::size_t at = 0; at + sizeof(float) <= input.bytes.size();
                 at += sizeof(float)) {
                float value;
                std::memcpy(&value, input.bytes.data() + at, sizeof(value));
                energy += static_cast<double>(value) * value;
                peak = std::max(peak, std::abs(value));
            }
            const auto count = input.bytes.size() / sizeof(float);
            return std::pair{count ? std::sqrt(energy / count) : 0.0, peak};
        };
        const auto [rms, peak] = levels(wave);
        const auto referenceRms = levels(reference).first;
        const float gain =
            rms > 0 && peak > 0 ? static_cast<float>(std::min(referenceRms / rms, 0.98 / peak)) : 0;
        for (std::size_t at = 0; at + sizeof(float) <= wave.bytes.size(); at += sizeof(float)) {
            float value;
            std::memcpy(&value, wave.bytes.data() + at, sizeof(value));
            value *= gain;
            std::memcpy(wave.bytes.data() + at, &value, sizeof(value));
        }
        return Convert(wave, target);
    }

    static bool Copy(const Wave &wave, SDL_AudioSpec *spec, Uint8 **data, Uint32 *bytes) {
        *data = static_cast<Uint8 *>(SDL_malloc(wave.bytes.size()));
        if (!*data) {
            return false;
        }
        std::memcpy(*data, wave.bytes.data(), wave.bytes.size());
        *spec = wave.spec;
        *bytes = static_cast<Uint32>(wave.bytes.size());
        return true;
    }
};

SoundEffects::SoundEffects(const std::filesystem::path &assets, Source source, bool enabled)
    : m_state(std::make_unique<State>(std::filesystem::absolute(assets), source, enabled)) {
    assert(State::Active() == nullptr);
    SDL_SetPointerProperty(SDL_GetGlobalProperties(), State::PROPERTY, m_state.get());
    SDL_Log("Using %s sound effects.", source == Source::Pc ? "PC" : "N64");
}

SoundEffects::~SoundEffects() {
    SDL_ClearProperty(SDL_GetGlobalProperties(), State::PROPERTY);
}

bool SoundEffects::LoadRaceWave(
    const char *path, int sample, SDL_AudioSpec *spec, Uint8 **data, Uint32 *bytes) {
    State *state = State::Active();
    if (!state || state->source == Source::Pc || sample < 0 ||
        sample >= static_cast<int>(State::RACE_SOUNDS.size())) {
        auto wave = State::Read(State::FromUtf8(path));
        if (wave && state && !state->enabled) {
            std::fill(wave->bytes.begin(),
                      wave->bytes.end(),
                      wave->spec.format == SDL_AUDIO_U8 ? 128 : 0);
        }
        return wave && State::Copy(*wave, spec, data, bytes);
    }
    const auto key = std::string(path) + '#' + std::to_string(sample);
    if (const auto found = state->cache.find(key); found != state->cache.end()) {
        return State::Copy(found->second, spec, data, bytes);
    }
    auto reference = State::Read(State::FromUtf8(path));
    if (!reference) {
        return false;
    }
    auto wave = *reference;
    if (const char *name = State::RACE_SOUNDS[sample]) {
        const auto file = state->assets / "n64/base/sounds" / (std::string(name) + ".wav");
        auto console = State::Read(file);
        const SDL_AudioSpec target{SDL_AUDIO_S16LE, reference->spec.channels, reference->spec.freq};
        if (console && State::Match(*console, *reference, target)) {
            wave = std::move(*console);
        } else {
            SDL_LogWarn(SDL_LOG_CATEGORY_AUDIO,
                        "Missing %s; using PC sound %s.",
                        State::Utf8(file).c_str(),
                        path);
        }
    } else {
        // The console release does not include matching taunts.
        std::fill(wave.bytes.begin(), wave.bytes.end(), wave.spec.format == SDL_AUDIO_U8 ? 128 : 0);
    }
    if (!state->enabled) {
        std::fill(wave.bytes.begin(), wave.bytes.end(), wave.spec.format == SDL_AUDIO_U8 ? 128 : 0);
    }
    return State::Copy(state->cache.emplace(key, std::move(wave)).first->second, spec, data, bytes);
}

bool SoundEffects::LoadMenuWave(const std::filesystem::path &path,
                                SDL_AudioSpec *spec,
                                Uint8 **data,
                                Uint32 *bytes) {
    State *state = State::Active();
    auto wave = State::Read(path);
    if (!wave) {
        return false;
    }
    if ((wave->spec.format != SDL_AUDIO_U8 || wave->spec.channels != 1 ||
         wave->spec.freq != 10999) &&
        !State::Convert(*wave, {SDL_AUDIO_U8, 1, 10999})) {
        return false;
    }
    if (state && !state->enabled) {
        std::fill(wave->bytes.begin(), wave->bytes.end(), 128);
    }
    return State::Copy(*wave, spec, data, bytes);
}

} // namespace coastline
