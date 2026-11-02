#include "introsequence.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include <SDL3/SDL.h>
#include <nlohmann/json.hpp>

#include "common/audio.h"
#include "common/settings.h"
#include "common/soundsystem.h"
#include "frontend.h"
#include "gpu.h"
#include "menuscreen.h"
#include "scenerenderer.h"
#include "videoplayer.h"

namespace coastline {

struct IntroSequence::State {
    struct Entry {
        std::string type;
        std::filesystem::path file;
        float duration = 0;
        bool skippable = true;
        float scale = 0.03125f;
        float depth = -50;
    };
    SceneRenderer &renderer;
    std::vector<Entry> entries;
    std::size_t index = 0;
    double time = 0;
    std::unique_ptr<RenderTexture> image;
    std::unique_ptr<RenderModel> model;
    std::unique_ptr<VideoPlayer> video;

    void Open() {
        image.reset();
        model.reset();
        video.reset();
        time = 0;
        while (index < entries.size()) {
            const auto &entry = entries[index];
            if (entry.type == "image") {
                image = RenderTexture::Load(renderer, entry.file, true);
            } else if (entry.type == "model") {
                model = RenderModel::Load(renderer, entry.file);
            } else {
                video = VideoPlayer::Open(renderer, entry.file);
            }
            if (image || model || video) {
                return;
            }
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                        "Skipping unavailable intro asset %s.",
                        entry.file.filename().string().c_str());
            ++index;
        }
    }
};

IntroSequence::IntroSequence(std::unique_ptr<State> state) : m_state(std::move(state)) {
}

IntroSequence::~IntroSequence() = default;

std::unique_ptr<IntroSequence> IntroSequence::Load(SceneRenderer &renderer,
                                                   const std::filesystem::path &assets,
                                                   const nlohmann::json &entries) {
    if (!entries.is_array() || entries.size() > 64) {
        return nullptr;
    }
    auto state = std::make_unique<State>(renderer);
    try {
        for (const auto &value : entries) {
            State::Entry entry;
            entry.type = value.at("type").get<std::string>();
            const auto file = std::filesystem::path(value.at("file").get<std::string>());
            entry.duration = value.value("duration", entry.type == "video" ? 0.0f : 2.0f);
            entry.skippable = value.value("skippable", true);
            entry.scale = value.value("scale", 0.03125f);
            entry.depth = value.value("depth", -50.0f);
            if ((entry.type != "image" && entry.type != "video" && entry.type != "model") ||
                file.empty() || file.is_absolute() || file.has_root_name() ||
                std::ranges::find(file, "..") != file.end() || !std::isfinite(entry.duration) ||
                entry.duration < 0 || entry.duration > 3600 ||
                (entry.type != "video" && entry.duration == 0) || !std::isfinite(entry.scale) ||
                entry.scale <= 0 || entry.scale > 1000 || !std::isfinite(entry.depth) ||
                entry.depth >= -1 || entry.depth < -100000) {
                return nullptr;
            }
            entry.file = assets / file;
            state->entries.push_back(std::move(entry));
        }
    } catch (const nlohmann::json::exception &) {
        return nullptr;
    }
    auto sequence = std::unique_ptr<IntroSequence>(new IntroSequence(std::move(state)));
    sequence->Restart();
    return sequence;
}

void IntroSequence::Restart() {
    m_state->index = 0;
    m_state->Open();
}

bool IntroSequence::Update(float seconds, bool skip) {
    auto &s = *m_state;
    if (s.index >= s.entries.size()) {
        return false;
    }
    if (std::isfinite(seconds)) {
        s.time += std::clamp(seconds, 0.0f, 0.25f);
    }
    const auto &entry = s.entries[s.index];
    const bool playing =
        !s.video ||
        s.video->Update(s.time,
                        g_dwPlayMusicMode ? static_cast<float>(g_bBgmVolume) /
                                                g_adwBgmVolumeLevels[VOLUME_LEVEL_COUNT - 1] :
                                            0.0f);
    if ((skip && entry.skippable) || !playing || (entry.duration > 0 && s.time >= entry.duration)) {
        ++s.index;
        s.Open();
    }
    return s.index < s.entries.size();
}

void IntroSequence::Draw() const {
    const auto &s = *m_state;
    const auto *image = s.video ? s.video->Texture() : s.image.get();
    if (image) {
        const float aspect = static_cast<float>(image->Width()) / image->Height();
        const int width = std::min(320, static_cast<int>(240 * aspect));
        const int height = std::min(240, static_cast<int>(320 / aspect));
        MenuScreen::DrawTexture(
            s.renderer, *image, (320 - width) / 2, (240 - height) / 2, width, height);
    } else if (s.model) {
        const auto &entry = s.entries[s.index];
        const auto &window = s.renderer.Window();
        const float aspect = static_cast<float>(window.FrameWidth()) / window.FrameHeight();
        auto place = Translation(0, 0, entry.depth);
        auto scale = Scale(entry.scale);
        s.renderer.DrawModel(*s.model,
                             MultiplyMatrices(place, scale),
                             MenuProjection(aspect, 1, 100000, s.renderer.FrontendCamera()),
                             Lighting{},
                             std::nullopt,
                             true);
    }
}

bool IntroSequence::PlayingVideo() const {
    return m_state->video != nullptr;
}

} // namespace coastline
