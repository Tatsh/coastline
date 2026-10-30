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
#include "gpu.h"
#include "menutext.h"
#include "scenerenderer.h"

namespace coastline {

namespace {

constexpr float HALF_PI = 1.570796371f;
constexpr float PI = 3.141592741f;

// The time of each fade, in seconds.
constexpr float FADE_TIME = 0.2f;

// The silence after each sound, for the mixer to read zeros past its end.
constexpr std::size_t SOUND_PADDING = 0x100;

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

// The levels of a sound effect channel, the right in the upper half and the left in the lower.
constexpr std::uint32_t SfxVolume(std::uint32_t right, std::uint32_t left) {
    return (right << 16) + left;
}

// The 32.32 step that plays a sample at its own rate.
constexpr std::uint64_t SFX_STEP_ONE = 1ull << 32;

// The sound effect channels the splash screens silence when they end.
constexpr int SPLASH_SFX_CHANNELS = 5;

// The camera of the menus looks at the middle of the 320 by 240 screen from 290 in front of it,
// with a field of 45 degrees, and the splash screens draw from 10 to 5000 away.
constexpr float MENU_CENTER_X = 160.0f;
constexpr float MENU_CENTER_Y = 120.0f;
constexpr float MENU_DISTANCE = 290.0f;
constexpr float SCREEN_FOV = 45.0f;
constexpr float SPLASH_NEAR = 10.0f;
constexpr float SPLASH_FAR = 5000.0f;
constexpr float SCREEN_WIDTH = 320.0f;
constexpr float SCREEN_HEIGHT = 240.0f;

Matrix4 Translation(float x, float y, float z) {
    return {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, x, y, z, 1};
}

Matrix4 Scale(float s) {
    return {s, 0, 0, 0, 0, s, 0, 0, 0, 0, s, 0, 0, 0, 0, 1};
}

// A turn by an angle in radians about the x or the z axis, as guRotateF() turns it.
Matrix4 RotationX(float angle) {
    float c = std::cos(angle);
    float s = std::sin(angle);
    return {1, 0, 0, 0, 0, c, s, 0, 0, -s, c, 0, 0, 0, 0, 1};
}

Matrix4 RotationZ(float angle) {
    float c = std::cos(angle);
    float s = std::sin(angle);
    return {c, s, 0, 0, -s, c, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
}

// The projection of the menus over the whole frame, its height the height of the 4:3 screen, to
// a depth from 0 to 1.
Matrix4 MenuProjection(float aspect) {
    float focal = 1.0f / std::tan(SCREEN_FOV * PI / 360.0f);
    float range = SPLASH_FAR - SPLASH_NEAR;
    Matrix4 projection{focal / aspect,
                       0,
                       0,
                       0,
                       0,
                       focal,
                       0,
                       0,
                       0,
                       0,
                       -SPLASH_FAR / range,
                       -1,
                       0,
                       0,
                       -SPLASH_NEAR * SPLASH_FAR / range,
                       0};
    return MultiplyMatrices(projection,
                            Translation(-MENU_CENTER_X, -MENU_CENTER_Y, -MENU_DISTANCE));
}

// The flat draws of the 640 by 480 screen, centred in the frame at 4:3 with y down.
Matrix4 FlatProjection(float aspect) {
    float width = (4.0f / 3.0f) / aspect;
    return {2.0f * width / 640.0f, 0, 0, 0, 0, -2.0f / 480.0f, 0, 0, 0, 0, 1, 0, -width, 1, 0, 1};
}

// The glTF of a model of the console stands it up from z up to y up, which the matrices of the
// console do themselves: this turns it back.
const Matrix4 CONSOLE_SPACE = RotationX(HALF_PI);

// The lights of the splash screens or the menus, in the space of the console.
Lighting ReadLighting(const nlohmann::json &light) {
    Lighting lighting;
    auto direction = light.at("direction").get<std::array<float, 3>>();
    float length = std::sqrt(direction[0] * direction[0] + direction[1] * direction[1] +
                             direction[2] * direction[2]);
    for (std::size_t i = 0; i < 3; ++i) {
        lighting.direction[i] = length > 0.0f ? direction[i] / length : 0.0f;
        lighting.ambient[i] = light.at("ambient")[i].get<float>() / 255.0f;
        lighting.directional[i] = light.at("directional")[i].get<float>() / 255.0f;
    }
    return lighting;
}

// The signed 8-bit samples of a sound of the import, with silence after them.
std::optional<std::vector<s8>> ReadSound(const std::filesystem::path &path) {
    SDL_AudioSpec spec;
    Uint8 *read = nullptr;
    Uint32 length = 0;
    if (!SDL_LoadWAV(path.string().c_str(), &spec, &read, &length) || spec.format != SDL_AUDIO_U8) {
        SDL_free(read);
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s cannot be read.", path.string().c_str());
        return std::nullopt;
    }
    std::vector<s8> samples(length + SOUND_PADDING, 0);
    for (Uint32 i = 0; i < length; ++i) {
        samples[i] = static_cast<s8>(read[i] ^ 0x80);
    }
    SDL_free(read);
    return samples;
}

// The fade of the screen: a wipe whose level goes to its target at a rate, and the edges of the
// picture it shows, in pixels of the 320 by 240 screen.
struct ScreenFade {
    float level = 0.0f;
    float target = 0.0f;
    float rate = 0.0f;
    bool autoReverse = false;
    bool skipFrame = false;
    int left = 0;
    int right = 0;

    void SetTarget(float to, float seconds) {
        skipFrame = true;
        if (to < level || to == 0.0f) {
            if (level == 1.0f || rate <= 0.0f) {
                rate = -1.0f / seconds;
                target = to;
            } else {
                // Still fading in: fade out once the picture is whole.
                autoReverse = true;
            }
            return;
        }
        target = to;
        rate = 1.0f / seconds;
    }

    bool FadingIn() const {
        return rate > 0.0f && !autoReverse;
    }

    bool FadingOut() const {
        return rate < 0.0f || autoReverse;
    }

    bool Complete() const {
        return target == level && !autoReverse;
    }

    void Update(float seconds) {
        if (!skipFrame) {
            if (level != target) {
                level += rate * seconds;
                if (rate < 0.0f) {
                    level = std::max(level, target);
                } else if (target <= level) {
                    level = target;
                    if (autoReverse) {
                        target = 0.0f;
                        rate = -rate;
                        autoReverse = false;
                    }
                }
            }
        } else {
            skipFrame = false;
        }
        constexpr int WIDTH = static_cast<int>(SCREEN_WIDTH);
        if (rate > 0.0f) {
            // Fading in, the picture grows from the left.
            left = 0;
            right = (static_cast<int>(WIDTH * level) + 3) & ~3;
        } else if (rate < 0.0f) {
            // Fading out, black comes in from the left.
            int move = (WIDTH - static_cast<int>(WIDTH * level) - left + 3) & ~3;
            left += move;
            right = std::min(right + move, WIDTH);
        } else {
            left = 0;
            right = WIDTH;
        }
    }
};

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
    std::uint32_t soundALength = 0;
    std::uint32_t soundBLength = 0;

    float time = 0.0f;
    ScreenFade fade;
    bool skipped = false;
    bool fadingOut = false;
    bool buildDate = false;
    std::array<LogoSounds, 3> logos{};
    // The time of the vertex animation of each part of the title, from where it loads.
    std::array<float, 4> animationTime{};

    s16 PlaySound(std::vector<s8> &sample, std::uint32_t length, std::uint32_t volume) {
        s16 channel = FindFreeSoundChannel();
        if (channel != -1) {
            N64InitSoundChannel(channel, sample.data(), length, 0);
            g_aSoundEffectChannels[channel].volume = static_cast<s32>(volume);
            g_aSoundEffectChannels[channel].step = SFX_STEP_ONE;
        }
        return channel;
    }

    // A logo flies in with a spin from the distance and two sounds, holds, then slides off to the
    // right with a sound that pans after it.
    void UpdateLogo(float start, float end, LogoSounds &sounds) {
        float t = time;
        if (t < start + LOGO_LANDED_TIME) {
            if (t > start + LOGO_SOUND_A_TIME && sounds.soundA == 0) {
                PlaySound(soundA, soundALength, SfxVolume(32, 32));
                sounds.soundA = 1;
            }
            if (t > start + LOGO_SOUND_B_TIME && !sounds.soundB) {
                PlaySound(soundB, soundBLength, SfxVolume(32, 32));
                sounds.soundB = true;
            }
            return;
        }
        if (t >= start + end || !(t > start + LOGO_LEAVE_TIME)) {
            return;
        }
        if (sounds.soundA < 2) {
            sounds.voice = FindFreeSoundChannel();
            if (sounds.voice != -1) {
                N64InitSoundChannel(sounds.voice, soundA.data(), soundALength, 0);
                g_aSoundEffectChannels[sounds.voice].volume = static_cast<s32>(SfxVolume(20, 20));
                g_aSoundEffectChannels[sounds.voice].step = SFX_STEP_ONE;
            }
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
        Matrix4 place =
            MultiplyMatrices(Translation(x, y, z),
                             MultiplyMatrices(RotationX(-HALF_PI),
                                              MultiplyMatrices(Scale(scale), RotationZ(angle))));
        return MultiplyMatrices(Translation(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, 100.0f),
                                MultiplyMatrices(place, CONSOLE_SPACE));
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
    state->soundALength = static_cast<std::uint32_t>(soundA->size() - SOUND_PADDING);
    state->soundBLength = static_cast<std::uint32_t>(soundB->size() - SOUND_PADDING);
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
    return IntroEnd::Running;
}

void Intro::Draw(SceneRenderer &renderer) const {
    const State &s = *m_state;
    GpuWindow &window = renderer.Window();
    float aspect = static_cast<float>(window.FrameWidth()) /
                   static_cast<float>(std::max<Uint32>(window.FrameHeight(), 1));
    Matrix4 projection = MenuProjection(aspect);
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
        // ponytail: the flag is not cut to the band the console scissors it to after TITLE_A_END;
        // a scissor rectangle on the renderer's draws would add it.
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
        renderer.DrawModel(*s.title[0], flag, projection, s.menuLighting, s.TitlePose(0), false);
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

    // The wipe of the fade: black left of the picture and right of it, across the whole frame.
    if (s.fade.level != 1.0f) {
        constexpr float WIDTH = SCREEN_WIDTH;
        Matrix4 whole{2.0f, 0, 0, 0, 0, -2.0f, 0, 0, 0, 0, 1, 0, -1.0f, 1.0f, 0, 1};
        std::vector<FlatVertex> black;
        auto cover = [&](float from, float to) {
            if (to <= from) {
                return;
            }
            std::array<std::uint8_t, 4> color{0, 0, 0, 255};
            black.insert(black.end(),
                         {{from, 0.0f, 0.0f, 0.0f, color},
                          {to, 0.0f, 0.0f, 0.0f, color},
                          {to, 1.0f, 0.0f, 0.0f, color},
                          {from, 1.0f, 0.0f, 0.0f, color}});
        };
        cover(0.0f, static_cast<float>(s.fade.left) / WIDTH);
        cover(static_cast<float>(s.fade.right) / WIDTH, 1.0f);
        if (!black.empty()) {
            renderer.DrawQuads(black, FlatMaterial{}, whole);
        }
    }
}

} // namespace coastline
