#include "intro.h"

#include <array>
#include <cmath>
#include <format>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

#include <SDL3/SDL.h>
#include <nlohmann/json.hpp>

#include "common/audio.h"
#include "common/audiomix.h"
#include "frontend.h"
#include "gpu.h"
#include "menutext.h"
#include "scenerenderer.h"

namespace coastline {

namespace {

// The time of each fade, in seconds.
constexpr float FADE_TIME = 0.2f;

// When each logo starts, and the times of a logo after its start: flying in, holding, leaving,
// and gone.
constexpr float FIRST_LOGO_START = 0.0f;
constexpr float PRODUCER_LOGO_START = 4.0f;
constexpr float DEVELOPER_LOGO_START = 8.0f;
constexpr float LOGO_SOUND_A_TIME = 0.35f;
constexpr float LOGO_SOUND_B_TIME = 0.5f;
constexpr float LOGO_LANDED_TIME = 2.0f;
constexpr float LOGO_LEAVE_TIME = 3.5f;
constexpr float LOGO_END_TIME = 6.0f;
constexpr float DEVELOPER_LOGO_END_TIME = 5.0f;

// The depth of a logo at rest, and its scale.
constexpr float LOGO_DEPTH = -50.0f;
constexpr float LOGO_SCALE = 0.03125f;

// The brightest a caption or the copyright gets, and how fast they fade.
constexpr int TEXT_ALPHA = 200;
constexpr double TEXT_FADE_RATE = 4.0;

// When the parts of the title show, and when the copyright does.
constexpr float TITLE_START = 12.0f;
constexpr float TITLE_A_END = 15.0f;
constexpr float TITLE_B_START = 13.0f;
constexpr float TITLE_B_END = 15.0f;
constexpr float TITLE_C_START = 15.2f;
constexpr float TITLE_C_END = 16.2f;
constexpr float TITLE_D_START = 15.0f;
constexpr float TITLE_D_END = 16.0f;
constexpr float COPYRIGHT_START = 17.2f;

// A skip before this jumps here; after the next time it ends the splash screens.
constexpr float SKIP_TIME = 17.2f;
constexpr float SKIP_END_TIME = 19.2f;
constexpr float SPLASH_END_TIME = 24.2f;

// The sound effect channels the splash screens silence when they end.
constexpr int SPLASH_SFX_CHANNELS = 5;

// The splash screens draw from 10 to 5000 away.
constexpr float SPLASH_NEAR = 10.0f;
constexpr float SPLASH_FAR = 5000.0f;

// The band of the screen the flag is cut to once it rests.
constexpr int FLAG_BAND_X = 40;
constexpr int FLAG_BAND_WIDTH = 250;
constexpr int FLAG_BAND_Y = 100;
constexpr int FLAG_BAND_HEIGHT = 50;

// The state of a logo: the voice of the sound that pans after it, and which sounds it played.
struct LogoSounds {
    s16 voice = -1;
    int soundA = 0;
    bool soundB = false;
};

} // namespace

struct Intro::State {
    std::unique_ptr<RenderModel> firstLogo;
    std::unique_ptr<RenderModel> producerLogo;
    std::unique_ptr<RenderModel> developerLogo;
    std::array<std::unique_ptr<RenderModel>, 4> title;
    std::unique_ptr<MenuText> text;
    Lighting splashLighting;
    Lighting menuLighting;
    std::vector<s8> soundA;
    std::vector<s8> soundB;

    float time = 0.0f;
    ScreenFade fade;
    // The volume of the sound effects. It fades out with the screen.
    ScreenFade sfxFade{.level = 1.0f, .target = 1.0f};
    bool skipped = false;
    bool fadingOut = false;
    bool buildDate = false;
    std::array<LogoSounds, 3> logos{};
    // The time of the vertex animation of each part of the title, from where it loads.
    std::array<float, 4> animationTime{};

    // A logo flies in with a spin from the distance and two sounds, holds, then slides off to the
    // right with a sound that pans after it.
    void UpdateLogo(float start, float end, LogoSounds &sounds) {
        float t = time;
        if (t < start + LOGO_LANDED_TIME) {
            if (t > start + LOGO_SOUND_A_TIME && sounds.soundA == 0) {
                PlaySoundEffect(soundA, SfxVolume(32, 32));
                sounds.soundA = 1;
            }
            if (t > start + LOGO_SOUND_B_TIME && !sounds.soundB) {
                PlaySoundEffect(soundB, SfxVolume(32, 32));
                sounds.soundB = true;
            }
            return;
        }
        if (t >= start + end || !(t > start + LOGO_LEAVE_TIME)) {
            return;
        }
        if (sounds.soundA < 2) {
            sounds.voice = PlaySoundEffect(soundA, SfxVolume(20, 20));
            sounds.soundA = 2;
        } else if (sounds.voice != -1) {
            int pan = std::min(static_cast<int>((t - (start + LOGO_LEAVE_TIME)) * 32.0f), 20);
            g_aSoundEffectChannels[sounds.voice].volume = static_cast<s32>(SfxVolume(
                static_cast<std::uint32_t>(20 - pan), static_cast<std::uint32_t>(20 + pan)));
        }
    }

    // The place of a logo at a time: flying in, holding, or leaving, or none once gone.
    std::optional<Matrix4> LogoMatrix(float start, float end) const {
        float t = time;
        if (t < start) {
            return std::nullopt;
        }
        if (t < start + LOGO_LANDED_TIME) {
            float left = start + LOGO_LANDED_TIME - t;
            float since = t - start;
            return ModelAt(-left * 32.0f,
                           0.0f,
                           LOGO_DEPTH - left * 750.0f,
                           since * HALF_PI * since * since * since * 0.0625f + -PI,
                           LOGO_SCALE);
        }
        if (t >= start + end) {
            return std::nullopt;
        }
        float x = 0.0f;
        if (t > start + LOGO_LEAVE_TIME) {
            float since = t - (start + LOGO_LEAVE_TIME);
            x = since * since * 256.0f;
        }
        return ModelAt(x, 0.0f, LOGO_DEPTH, -HALF_PI, LOGO_SCALE);
    }

    // DrawModelAt() of the console: turned about its own axis, scaled, stood up, and placed, in
    // the space the splash screens move to the middle of the screen and 100 forward.
    static Matrix4 ModelAt(float x, float y, float z, float angle, float scale) {
        return MultiplyMatrices(Translation(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, 100.0f),
                                ScreenModelAt(x, y, z, angle, scale));
    }

    // The depth of a part of the title, which slows as it comes forward and rests at 0 by its end.
    float TitleDepth(float start, float end, float speed) const {
        if (time < end) {
            return (std::sqrt(end - start) - std::sqrt(time - start)) * speed;
        }
        return 0.0f;
    }

    // The pose of the vertex animation of a part of the title at its time.
    std::optional<MorphPose> TitlePose(std::size_t part) const {
        const auto &animations = title[part]->Animations();
        if (animations.empty() || animations[0].keyTimes.size() < 2) {
            return std::nullopt;
        }
        const VertexAnimationInfo &animation = animations[0];
        float t =
            std::clamp(animationTime[part], animation.keyTimes.front(), animation.keyTimes.back());
        std::size_t k = 1;
        while (k + 1 < animation.keyTimes.size() && !(t < animation.keyTimes[k])) {
            ++k;
        }
        float from = animation.keyTimes[k - 1];
        float to = animation.keyTimes[k];
        float share = to > from ? (t - from) / (to - from) : 0.0f;
        // The console blends in 4096ths.
        share = std::floor(share * 4096.0f) / 4096.0f;
        return MorphPose{animation.firstTarget + static_cast<int>(k) - 1,
                         animation.firstTarget + static_cast<int>(k),
                         share};
    }

    // AdvanceForwardAnimation() and AdvanceReversingAnimation() of the console, for an animation
    // that repeats back and forth: the time after a frame.
    static float AdvanceBouncing(const VertexAnimationInfo &animation,
                                 float time,
                                 float seconds,
                                 bool &reversing) {
        if (reversing) {
            time -= seconds;
            if (animation.startTime <= time) {
                return time;
            }
            reversing = false;
            return animation.startTime * 2.0f - time;
        }
        time += seconds;
        if (time < animation.endTime) {
            return time;
        }
        float span = (animation.endTime - animation.startTime) * 2.0f;
        float limit = animation.endTime + span;
        while (limit < time) {
            time -= span;
        }
        limit -= span * 0.5f;
        if (!(limit < time)) {
            reversing = true;
            return animation.endTime * 2.0f - time;
        }
        return time - (animation.endTime - animation.startTime);
    }

    std::array<bool, 4> reversing{};
};

Intro::Intro(std::unique_ptr<State> state) : m_state(std::move(state)) {
}

Intro::~Intro() = default;

std::unique_ptr<Intro> Intro::Load(SceneRenderer &renderer, const std::filesystem::path &release) {
    auto state = std::make_unique<State>();
    auto constants = release / "constants";
    state->firstLogo = RenderModel::Load(renderer, constants / "splash_first_logo.gltf");
    state->producerLogo = RenderModel::Load(renderer, constants / "splash_producer_logo.gltf");
    state->developerLogo = RenderModel::Load(renderer, constants / "splash_developer_logo.gltf");
    for (std::size_t part = 0; part < state->title.size(); ++part) {
        state->title[part] = RenderModel::Load(
            renderer,
            constants / std::format("splash_title_{}.gltf", static_cast<char>('a' + part)));
        if (!state->title[part]) {
            return nullptr;
        }
    }
    state->text = MenuText::Load(renderer, release / "fonts");
    auto soundA = ReadSound(constants / "splash_sound_a.wav");
    auto soundB = ReadSound(constants / "splash_sound_b.wav");
    if (!state->firstLogo || !state->producerLogo || !state->developerLogo || !state->text ||
        !soundA || !soundB) {
        return nullptr;
    }
    state->soundA = std::move(*soundA);
    state->soundB = std::move(*soundB);
    try {
        std::ifstream file(release / "lights.json");
        auto lights = nlohmann::json::parse(file);
        state->splashLighting = ReadLighting(lights.at("splash"));
        state->menuLighting = ReadLighting(lights.at("menu"));
    } catch (const nlohmann::json::exception &error) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "%s is not the lights of the release: %s",
                     (release / "lights.json").string().c_str(),
                     error.what());
        return nullptr;
    }
    std::unique_ptr<Intro> intro(new Intro(std::move(state)));
    intro->Restart();
    return intro;
}

void Intro::Restart() {
    State &s = *m_state;
    s.fade = {};
    s.fade.SetTarget(1.0f, FADE_TIME);
    s.sfxFade = {.level = 1.0f, .target = 1.0f};
    g_bSfxVolume = MAX_SFX_VOLUME;
    s.time = 0.0f;
    s.skipped = false;
    s.fadingOut = false;
    s.logos = {};
    for (std::size_t part = 0; part < s.title.size(); ++part) {
        const auto &animations = s.title[part]->Animations();
        s.animationTime[part] = animations.empty() ? 0.0f : animations[0].time;
        s.reversing[part] = false;
    }
}

IntroEnd Intro::Update(float seconds, const IntroButtons &buttons) {
    State &s = *m_state;
    s.buildDate = buttons.buildDate;
    s.fade.Update(seconds);
    s.sfxFade.Update(seconds);
    g_bSfxVolume = static_cast<u8>(static_cast<int>(s.sfxFade.level * MAX_SFX_VOLUME));
    if (!s.fade.FadingIn() || s.fade.Complete()) {
        s.time += seconds;
    }
    s.UpdateLogo(FIRST_LOGO_START, LOGO_END_TIME, s.logos[0]);
    if (s.time >= PRODUCER_LOGO_START) {
        s.UpdateLogo(PRODUCER_LOGO_START, LOGO_END_TIME, s.logos[1]);
    }
    if (s.time >= DEVELOPER_LOGO_START) {
        s.UpdateLogo(DEVELOPER_LOGO_START, DEVELOPER_LOGO_END_TIME, s.logos[2]);
    }
    // The flag and the second part of the title wave, and the other parts, which do not move,
    // advance with them.
    for (std::size_t part = 0; part < s.title.size(); ++part) {
        const auto &animations = s.title[part]->Animations();
        if (!animations.empty()) {
            s.animationTime[part] = State::AdvanceBouncing(
                animations[0], s.animationTime[part], seconds, s.reversing[part]);
        }
    }

    // Fade out on a skip or at the end, and leave for the main menu or the demo once faded.
    if (s.fade.FadingOut()) {
        if (s.fade.Complete()) {
            for (int i = 0; i < SPLASH_SFX_CHANNELS; ++i) {
                g_aSoundEffectChannels[i].step = 0;
                g_aSoundEffectChannels[i].volume = 0;
            }
            // The console leaves the sound effects silent for the next screen to fade in. The
            // screens after the intro do not fade them yet, and they return to full volume here.
            g_bSfxVolume = MAX_SFX_VOLUME;
            return s.skipped ? IntroEnd::MainMenu : IntroEnd::AttractDemo;
        }
        return IntroEnd::Running;
    }
    bool leave = false;
    if (!s.skipped || !(s.time > SKIP_END_TIME)) {
        if (buttons.skip) {
            s.skipped = true;
            if (s.fadingOut) {
                leave = true;
            } else if (s.time < SKIP_TIME) {
                s.time = SKIP_TIME;
            } else if (s.time > SKIP_END_TIME) {
                leave = true;
            }
        }
        if (!leave && !(s.time >= SPLASH_END_TIME)) {
            return IntroEnd::Running;
        }
    }
    s.fadingOut = true;
    s.fade.SetTarget(0.0f, FADE_TIME);
    s.sfxFade.SetTarget(0.0f, FADE_TIME);
    return IntroEnd::Running;
}

void Intro::Draw(SceneRenderer &renderer) const {
    const State &s = *m_state;
    GpuWindow &window = renderer.Window();
    float aspect = static_cast<float>(window.FrameWidth()) /
                   static_cast<float>(std::max<Uint32>(window.FrameHeight(), 1));
    Matrix4 projection = MenuProjection(aspect, SPLASH_NEAR, SPLASH_FAR);
    Matrix4 flat = FlatProjection(aspect);

    // The captions fade in as a logo lands and out as it leaves.
    auto caption = [&](const char *line, float start, int y) {
        float t = s.time;
        int alpha = TEXT_ALPHA;
        if (t < start + LOGO_LANDED_TIME) {
            alpha =
                TEXT_ALPHA - static_cast<int>(static_cast<double>(start + LOGO_LANDED_TIME - t) *
                                              255.0 * TEXT_FADE_RATE);
        } else if (t > start + LOGO_LEAVE_TIME) {
            alpha =
                TEXT_ALPHA - static_cast<int>(static_cast<double>(t - (start + LOGO_LEAVE_TIME)) *
                                              255.0 * TEXT_FADE_RATE);
        }
        if (alpha > 0) {
            TextStyle style;
            style.size = 10;
            style.align = TextStyle::Align::Center;
            style.colors = std::array<int, 6>{alpha, alpha, alpha, alpha, alpha, alpha};
            s.text->Draw(renderer, line, 160, y, style, flat);
        }
    };

    if (s.time >= FIRST_LOGO_START) {
        if (auto place = s.LogoMatrix(FIRST_LOGO_START, LOGO_END_TIME)) {
            renderer.DrawModel(
                *s.firstLogo, *place, projection, s.splashLighting, std::nullopt, true);
        }
    }
    if (s.time >= PRODUCER_LOGO_START) {
        caption("Produced by", PRODUCER_LOGO_START, 70);
        if (auto place = s.LogoMatrix(PRODUCER_LOGO_START, LOGO_END_TIME)) {
            renderer.DrawModel(
                *s.producerLogo, *place, projection, s.splashLighting, std::nullopt, true);
        }
    }
    if (s.time >= DEVELOPER_LOGO_START) {
        caption("Developed by", DEVELOPER_LOGO_START, 60);
        if (auto place = s.LogoMatrix(DEVELOPER_LOGO_START, DEVELOPER_LOGO_END_TIME)) {
            renderer.DrawModel(
                *s.developerLogo, *place, projection, s.splashLighting, std::nullopt, true);
        }
    }

    // The title, tilted, its parts coming forward one after another, lit by the lights of the
    // menus. The flag and the second part draw without the depth.
    if (s.time > TITLE_START) {
        // Once the flag rests it is cut to a band of the screen, inside the edges of the fade.
        std::optional<SDL_Rect> band;
        if (s.time > TITLE_A_END) {
            int left = std::max(FLAG_BAND_X, s.fade.left);
            int right = std::min(FLAG_BAND_X + FLAG_BAND_WIDTH, s.fade.right);
            band = FrameRect(window.FrameWidth(),
                             window.FrameHeight(),
                             left,
                             FLAG_BAND_Y,
                             std::max(right - left, 0),
                             FLAG_BAND_HEIGHT);
        }
        float z =
            s.time < TITLE_A_END ?
                (std::sqrt(TITLE_A_END - TITLE_START) - std::sqrt(s.time - TITLE_START)) * 300.0f :
                -0.0f;
        Matrix4 tilt =
            MultiplyMatrices(RotationZ(7.0f * PI / 180.0f), RotationX(-7.0f * PI / 180.0f));
        Matrix4 place = State::ModelAt(
            0.0f, 0.0f, z, std::sqrt(s.time - TITLE_START) * 0.01f + -HALF_PI, 0.05f);
        // The tilt is between the move to the middle of the screen and the place of the flag.
        Matrix4 middle = Translation(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, 100.0f);
        Matrix4 flag = MultiplyMatrices(
            middle,
            MultiplyMatrices(
                tilt,
                MultiplyMatrices(Translation(-SCREEN_WIDTH / 2, -SCREEN_HEIGHT / 2, -100.0f),
                                 place)));
        renderer.DrawModel(
            *s.title[0], flag, projection, s.menuLighting, s.TitlePose(0), false, band);
    }
    if (s.time > TITLE_B_START) {
        renderer.DrawModel(*s.title[1],
                           State::ModelAt(0.0f,
                                          0.0f,
                                          s.TitleDepth(TITLE_B_START, TITLE_B_END, 500.0f),
                                          -HALF_PI,
                                          1.0f / 12.0f),
                           projection,
                           s.menuLighting,
                           s.TitlePose(1),
                           false);
    }
    if (s.time > TITLE_C_START) {
        renderer.DrawModel(*s.title[2],
                           State::ModelAt(0.0f,
                                          0.0f,
                                          s.TitleDepth(TITLE_C_START, TITLE_C_END, 500.0f),
                                          -HALF_PI,
                                          1.0f / 12.0f),
                           projection,
                           s.menuLighting,
                           s.TitlePose(2),
                           true);
    }
    if (s.time > TITLE_D_START) {
        renderer.DrawModel(*s.title[3],
                           State::ModelAt(0.0f,
                                          0.0f,
                                          s.TitleDepth(TITLE_D_START, TITLE_D_END, 500.0f),
                                          -HALF_PI,
                                          1.0f / 12.0f),
                           projection,
                           s.menuLighting,
                           s.TitlePose(3),
                           true);
    }

    // The copyright fades in, and the date of the build shows while every C button is held.
    if (s.time > COPYRIGHT_START) {
        int alpha = std::min(static_cast<int>(static_cast<double>(s.time - COPYRIGHT_START) *
                                              255.0 * TEXT_FADE_RATE),
                             TEXT_ALPHA);
        TextStyle style;
        style.align = TextStyle::Align::Center;
        style.colors = std::array<int, 6>{alpha, alpha, alpha, alpha, alpha, alpha};
        if (s.buildDate) {
            style.size = 10;
            s.text->Draw(renderer, "25Aug97 11:46:52", 160, 40, style, flat);
        }
        style.size = 7;
        s.text->Draw(renderer,
                     "O 1997 Boss Game Studios, Inc.  All Rights Reserved.",
                     160,
                     190,
                     style,
                     flat);
        s.text->Draw(renderer, "Top Gear Rally is a trademark of Kemco.", 160, 197, style, flat);
        s.text->Draw(renderer,
                     "Distributed by Midway Home Entertainment under license.",
                     160,
                     204,
                     style,
                     flat);
        s.text->Draw(renderer,
                     "Midway is a trademark of Midway Games Inc.  Used by permission.",
                     160,
                     211,
                     style,
                     flat);
        s.text->Draw(renderer, "LICENSED BY NINTENDO", 160, 224, style, flat);
        // A small c inside the O of the first line makes the copyright sign.
        style.size = 3;
        s.text->Draw(renderer, "c", 78, 190, style, flat);
    }

    s.fade.Draw(renderer);
}

} // namespace coastline
