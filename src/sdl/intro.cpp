#include "intro.h"

#include <array>
#include <cmath>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

#include <SDL3/SDL.h>
#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include "common/audio.h"
#include "common/audiomix.h"
#include "frontend.h"
#include "gpu.h"
#include "introsequence.h"
#include "language.h"
#include "menuscreen.h"
#include "menutext.h"
#include "music.h"
#include "scenerenderer.h"

namespace coastline {

namespace {

// The time of each fade, in seconds.
constexpr float FADE_TIME = 0.2f;

// When each logo starts, and the times of a logo after its start: flying in, holding, leaving,
// and gone.
constexpr float FIRST_LOGO_START = 0.0f;
constexpr float PRODUCER_LOGO_START = 4.0f;
constexpr float LOGO_SOUND_A_TIME = 0.35f;
constexpr float LOGO_SOUND_B_TIME = 0.5f;
constexpr float LOGO_LANDED_TIME = 2.0f;
constexpr float LOGO_LEAVE_TIME = 3.5f;
constexpr float LOGO_END_TIME = 6.0f;

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
    std::unique_ptr<IntroSequence> sequence;
    bool sequenceOnce = false;
    bool sequencePlayed = false;
    std::unique_ptr<RenderModel> firstLogo;
    std::unique_ptr<RenderModel> producerLogo;
    std::unique_ptr<RenderModel> developerLogo;
    std::array<std::unique_ptr<RenderModel>, 4> title;
    std::unique_ptr<RenderTexture> titleImage;
    std::unique_ptr<MenuText> text;
    Lighting splashLighting;
    Lighting menuLighting;
    std::vector<s8> soundA;
    std::vector<s8> soundB;
    bool usa = true;
    float developerStart = 8.0f;
    float developerEnd = 5.0f;
    float titleOffset = 0.0f;
    float copyrightStart = 17.2f;
    float skipEnd = 19.2f;
    float splashEnd = 24.2f;

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
            N64MixerLock lock;
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
        if (time + titleOffset < end) {
            return (std::sqrt(end - start) - std::sqrt(time + titleOffset - start)) * speed;
        }
        return 0.0f;
    }

    // The pose of the vertex animation of a part of the title at its time.
    std::optional<MorphPose> TitlePose(std::size_t part) const {
        const auto &animations = title[part]->Animations();
        if (animations.empty()) {
            return std::nullopt;
        }
        return PoseAtTime(animations[0], animationTime[part]);
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

std::unique_ptr<Intro> Intro::Load(SceneRenderer &renderer,
                                   const std::filesystem::path &release,
                                   std::string_view selectedRegion,
                                   const std::filesystem::path &assets,
                                   const nlohmann::json &settings) {
    auto state = std::make_unique<State>();
    if (settings.contains("intro")) {
        state->sequence = IntroSequence::Load(renderer, assets, settings.at("intro"), settings);
        state->sequenceOnce = settings.contains("introOnce") &&
                              settings.at("introOnce").is_boolean() &&
                              settings.at("introOnce").get<bool>();
        if (state->sequence) {
            return std::unique_ptr<Intro>(new Intro(std::move(state)));
        }
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Invalid asset-pack intro sequence.");
    }
    const auto region =
        selectedRegion.empty() ? CartridgeRegion(release) : std::string(selectedRegion);
    state->usa = region == "usa";
    const bool japanese = region == "jpn";
    if (!state->usa) {
        state->developerStart = 4.0f;
        state->developerEnd = japanese ? 4.5f : 5.0f;
        state->titleOffset = 4.0f;
        state->copyrightStart = japanese ? 10.5f : 12.95f;
        state->skipEnd = japanese ? 12.5f : 14.95f;
        state->splashEnd = japanese ? 16.8f : 18.2f;
    }
    auto constants = release / "constants";
    state->firstLogo = RenderModel::Load(renderer, constants / "splash_first_logo.gltf");
    const auto producer = constants / "splash_producer_logo.gltf";
    if (state->usa) {
        state->producerLogo = RenderModel::Load(renderer, producer);
    }
    state->developerLogo = RenderModel::Load(renderer, constants / "splash_developer_logo.gltf");
    if (japanese) {
        state->titleImage =
            RenderTexture::Load(renderer, release / "data" / "g_SplashTitleLogoTexture.png", true);
        if (!state->titleImage) {
            return nullptr;
        }
    }
    for (std::size_t part = 0; !japanese && part < state->title.size(); ++part) {
        state->title[part] = RenderModel::Load(
            renderer,
            constants / fmt::format("splash_title_{}.gltf", static_cast<char>('a' + part)));
        if (!state->title[part]) {
            return nullptr;
        }
    }
    state->text = MenuText::Load(
        renderer, FontDirectory(release.parent_path().parent_path(), SelectedLanguage()));
    auto soundA = ReadSound(constants / "splash_sound_a.wav");
    auto soundB = ReadSound(constants / "splash_sound_b.wav");
    if (!state->firstLogo || (!state->producerLogo && state->usa) || !state->developerLogo ||
        !state->text || !soundA || !soundB) {
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
                     "Invalid lighting data %s: %s",
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
    if (s.sequence) {
        if (!s.sequenceOnce || !s.sequencePlayed) {
            s.sequence->Restart();
        }
        return;
    }
    s.fade = {};
    s.fade.SetTarget(1.0f, FADE_TIME);
    s.sfxFade = {.level = 1.0f, .target = 1.0f};
    g_bSfxVolume = MAX_SFX_VOLUME;
    s.time = 0.0f;
    s.skipped = false;
    s.fadingOut = false;
    s.logos = {};
    for (std::size_t part = 0; part < s.title.size(); ++part) {
        if (!s.title[part]) {
            continue;
        }
        const auto &animations = s.title[part]->Animations();
        s.animationTime[part] = animations.empty() ? 0.0f : animations[0].time;
        s.reversing[part] = false;
    }
}

IntroEnd Intro::Update(float seconds, const IntroButtons &buttons) {
    State &s = *m_state;
    if (s.sequence) {
        if (s.sequenceOnce && s.sequencePlayed) {
            return IntroEnd::MainMenu;
        }
        s.sequence->ControlVideo(buttons.pauseVideo, buttons.seekVideo, buttons.restartVideo);
        if (s.sequence->Update(seconds, buttons.skip)) {
            return IntroEnd::Running;
        }
        s.sequencePlayed = true;
        return IntroEnd::MainMenu;
    }
    s.buildDate = buttons.buildDate;
    s.fade.Update(seconds);
    s.sfxFade.Update(seconds);
    g_bSfxVolume = static_cast<u8>(static_cast<int>(s.sfxFade.level * MAX_SFX_VOLUME));
    if (!s.fade.FadingIn() || s.fade.Complete()) {
        s.time += seconds;
    }
    s.UpdateLogo(FIRST_LOGO_START, LOGO_END_TIME, s.logos[0]);
    if (s.producerLogo && s.time >= PRODUCER_LOGO_START) {
        s.UpdateLogo(PRODUCER_LOGO_START, LOGO_END_TIME, s.logos[1]);
    }
    if (s.time >= s.developerStart) {
        s.UpdateLogo(s.developerStart, s.developerEnd, s.logos[2]);
    }
    // The flag and the second part of the title wave, and the other parts, which do not move,
    // advance with them.
    for (std::size_t part = 0; part < s.title.size(); ++part) {
        if (!s.title[part]) {
            continue;
        }
        const auto &animations = s.title[part]->Animations();
        if (!animations.empty()) {
            s.animationTime[part] = State::AdvanceBouncing(
                animations[0], s.animationTime[part], seconds, s.reversing[part]);
        }
    }

    // Fade out on a skip or at the end, and leave for the main menu or the demo once faded.
    if (s.fade.FadingOut()) {
        if (s.fade.Complete()) {
            {
                N64MixerLock lock;
                for (int i = 0; i < SPLASH_SFX_CHANNELS; ++i) {
                    g_aSoundEffectChannels[i].step = 0;
                    g_aSoundEffectChannels[i].volume = 0;
                }
            }
            // The console leaves the sound effects silent for the next screen to fade in. The
            // screens after the intro do not fade them yet, and they return to full volume here.
            g_bSfxVolume = MAX_SFX_VOLUME;
            return s.skipped ? IntroEnd::MainMenu : IntroEnd::AttractDemo;
        }
        return IntroEnd::Running;
    }
    bool leave = false;
    if (!s.skipped || !(s.time > s.skipEnd)) {
        if (buttons.skip) {
            s.skipped = true;
            if (s.fadingOut) {
                leave = true;
            } else if (s.time < s.copyrightStart) {
                s.time = s.copyrightStart;
            } else if (s.time > s.skipEnd) {
                leave = true;
            }
        }
        if (!leave && !(s.time >= s.splashEnd)) {
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
    if (s.sequence) {
        s.sequence->Draw();
        return;
    }
    GpuWindow &window = renderer.Window();
    float aspect = static_cast<float>(window.FrameWidth()) /
                   static_cast<float>(std::max<Uint32>(window.FrameHeight(), 1));
    Matrix4 projection = MenuProjection(aspect, SPLASH_NEAR, SPLASH_FAR, renderer.FrontendCamera());
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
    if (s.producerLogo && s.time >= PRODUCER_LOGO_START) {
        caption("Produced by", PRODUCER_LOGO_START, 70);
        if (auto place = s.LogoMatrix(PRODUCER_LOGO_START, LOGO_END_TIME)) {
            renderer.DrawModel(
                *s.producerLogo, *place, projection, s.splashLighting, std::nullopt, true);
        }
    }
    if (s.time >= s.developerStart) {
        caption("Developed by", s.developerStart, 60);
        if (auto place = s.LogoMatrix(s.developerStart, s.developerEnd)) {
            renderer.DrawModel(
                *s.developerLogo, *place, projection, s.splashLighting, std::nullopt, true);
        }
    }

    // The title, tilted, its parts coming forward one after another, lit by the lights of the
    // menus. The flag and the second part draw without the depth.
    const float titleTime = s.time + s.titleOffset;
    if (s.titleImage && s.time > 8.5f) {
        MenuScreen::DrawTexture(renderer, *s.titleImage, 60, 36, 200, 150);
        if (s.time < 9.5f) {
            MenuScreen::DrawFadeOverlay(renderer, s.time - 8.5f, 0, 0, 320, 240);
        }
    }
    if (s.title[0] && titleTime > TITLE_START) {
        // Once the flag rests it is cut to a band of the screen, inside the edges of the fade.
        std::optional<SDL_Rect> band;
        if (titleTime > TITLE_A_END) {
            int left = std::max(FLAG_BAND_X, s.fade.left);
            int right = std::min(FLAG_BAND_X + FLAG_BAND_WIDTH, s.fade.right);
            band = FrameRect(window.FrameWidth(),
                             window.FrameHeight(),
                             left,
                             FLAG_BAND_Y,
                             std::max(right - left, 0),
                             FLAG_BAND_HEIGHT);
        }
        float z = titleTime < TITLE_A_END ?
                      (std::sqrt(TITLE_A_END - TITLE_START) - std::sqrt(titleTime - TITLE_START)) *
                          300.0f :
                      -0.0f;
        Matrix4 tilt =
            MultiplyMatrices(RotationZ(7.0f * PI / 180.0f), RotationX(-7.0f * PI / 180.0f));
        Matrix4 place = State::ModelAt(
            0.0f, 0.0f, z, std::sqrt(titleTime - TITLE_START) * 0.01f + -HALF_PI, 0.05f);
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
    if (s.title[1] && titleTime > TITLE_B_START) {
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
    if (s.title[2] && titleTime > TITLE_C_START) {
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
    if (s.title[3] && titleTime > TITLE_D_START) {
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
    if (s.time > s.copyrightStart) {
        int alpha = std::min(static_cast<int>(static_cast<double>(s.time - s.copyrightStart) *
                                              255.0 * TEXT_FADE_RATE),
                             TEXT_ALPHA);
        TextStyle style;
        style.align = TextStyle::Align::Center;
        style.colors = std::array<int, 6>{alpha, alpha, alpha, alpha, alpha, alpha};
        if (s.buildDate) {
            style.size = 10;
#ifdef ENABLE_PATCHES
            // The patched cartridge shows the project, the commit, and the time of the build.
            s.text->Draw(renderer,
                         "github.com/Tatsh/coastline " COASTLINE_GIT_SHA " " __DATE__ " " __TIME__,
                         160,
                         40,
                         style,
                         flat);
#else
            s.text->Draw(renderer, "25Aug97 11:46:52", 160, 40, style, flat);
#endif /* ENABLE_PATCHES */
        }
        style.size = 7;
        s.text->Draw(renderer,
                     "O 1997 Boss Game Studios, Inc.  All Rights Reserved.",
                     160,
                     s.usa ? 190 : 195,
                     style,
                     flat);
        s.text->Draw(renderer,
                     "Top Gear Rally is a trademark of Kemco.",
                     160,
                     s.usa ? 197 : 202,
                     style,
                     flat);
        if (s.usa) {
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
        }
        s.text->Draw(renderer, "LICENSED BY NINTENDO", 160, s.usa ? 224 : 214, style, flat);
        // A small c inside the O of the first line makes the copyright sign.
        style.size = 3;
        s.text->Draw(renderer, "c", 78, s.usa ? 190 : 195, style, flat);
    }

    s.fade.Draw(renderer);
}

bool Intro::PlayingVideo() const {
    return m_state->sequence && m_state->sequence->PlayingVideo();
}

} // namespace coastline
