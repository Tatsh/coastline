#include "frontend.h"

#include <algorithm>
#include <cmath>

namespace coastline {

namespace {

// The camera of the menus looks at the middle of the screen from 290 in front of it, with a field
// of 45 degrees.
constexpr float MENU_CENTER_X = 160.0f;
constexpr float MENU_CENTER_Y = 120.0f;
constexpr float MENU_DISTANCE = 290.0f;
constexpr float SCREEN_FOV = 45.0f;

// The glTF of a model of the console stands it up from z up to y up, which the matrices of the
// console do themselves: this turns it back.
const Matrix4 CONSOLE_SPACE = RotationX(HALF_PI);

} // namespace

Matrix4 Translation(float x, float y, float z) {
    return {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, x, y, z, 1};
}

Matrix4 Scale(float s) {
    return {s, 0, 0, 0, 0, s, 0, 0, 0, 0, s, 0, 0, 0, 0, 1};
}

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

Matrix4 MenuProjection(float aspect, float nearPlane, float farPlane) {
    float focal = 1.0f / std::tan(SCREEN_FOV * PI / 360.0f);
    float range = farPlane - nearPlane;
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
                       -farPlane / range,
                       -1,
                       0,
                       0,
                       -nearPlane * farPlane / range,
                       0};
    return MultiplyMatrices(projection,
                            Translation(-MENU_CENTER_X, -MENU_CENTER_Y, -MENU_DISTANCE));
}

Matrix4 FlatProjection(float aspect) {
    float width = (4.0f / 3.0f) / aspect;
    return {2.0f * width / 640.0f, 0, 0, 0, 0, -2.0f / 480.0f, 0, 0, 0, 0, 1, 0, -width, 1, 0, 1};
}

Matrix4 ScreenModelAt(float x, float y, float z, float angle, float scale) {
    Matrix4 place = MultiplyMatrices(
        Translation(x, y, z),
        MultiplyMatrices(RotationX(-HALF_PI), MultiplyMatrices(Scale(scale), RotationZ(angle))));
    return MultiplyMatrices(place, CONSOLE_SPACE);
}

Matrix4
StageCamera(float aspect, const std::array<float, 3> &eye, int x, int y, int width, int height) {
    // The field of view of the camera of the console, its near plane, and its far plane, a fifth
    // of the projection scale of 400.
    constexpr float CAMERA_FOV = 0.5235987901687622f;
    constexpr float ASPECT_4_3 = 1.3333334f;
    constexpr float CAMERA_NEAR = 2.4f;
    constexpr float CAMERA_FAR = 400.0f * 0.2f;
    using Vector3 = std::array<float, 3>;
    auto normalized = [](const Vector3 &v) {
        float length = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
        return Vector3{v[0] / length, v[1] / length, v[2] / length};
    };
    auto cross = [](const Vector3 &a, const Vector3 &b) {
        return Vector3{
            a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
    };
    auto dot = [](const Vector3 &a, const Vector3 &b) {
        return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
    };
    Vector3 forward = normalized({-eye[0], -eye[1], -eye[2]});
    Vector3 side = normalized(cross(forward, {0.0f, 0.0f, 1.0f}));
    Vector3 up = cross(side, forward);
    Matrix4 view{side[0],
                 up[0],
                 -forward[0],
                 0,
                 side[1],
                 up[1],
                 -forward[1],
                 0,
                 side[2],
                 up[2],
                 -forward[2],
                 0,
                 -dot(side, eye),
                 -dot(up, eye),
                 dot(forward, eye),
                 1};
    float w = static_cast<float>(width);
    float h = static_cast<float>(height);
    float fovy = CAMERA_FOV * ASPECT_4_3 * (h / w);
    float focal = 1.0f / std::tan(fovy * 0.5f);
    Matrix4 projection{focal / (w / h),
                       0,
                       0,
                       0,
                       0,
                       focal,
                       0,
                       0,
                       0,
                       0,
                       CAMERA_FAR / (CAMERA_NEAR - CAMERA_FAR),
                       -1,
                       0,
                       0,
                       CAMERA_NEAR * CAMERA_FAR / (CAMERA_NEAR - CAMERA_FAR),
                       0};
    float across = ASPECT_4_3 / aspect;
    float left = (static_cast<float>(x) / SCREEN_WIDTH * 2.0f - 1.0f) * across;
    float right = (static_cast<float>(x + width) / SCREEN_WIDTH * 2.0f - 1.0f) * across;
    float top = 1.0f - static_cast<float>(y) / SCREEN_HEIGHT * 2.0f;
    float bottom = 1.0f - static_cast<float>(y + height) / SCREEN_HEIGHT * 2.0f;
    Matrix4 viewport{(right - left) / 2.0f,
                     0,
                     0,
                     0,
                     0,
                     (top - bottom) / 2.0f,
                     0,
                     0,
                     0,
                     0,
                     1,
                     0,
                     (right + left) / 2.0f,
                     (top + bottom) / 2.0f,
                     0,
                     1};
    return MultiplyMatrices(viewport, MultiplyMatrices(projection, view));
}

Matrix4 StageCarAt(float x, float y, float angle) {
    constexpr float STAGE_HEIGHT = 0.254f;
    float c = std::cos(angle);
    float s = std::sin(angle);
    Matrix4 frame{c, s, 0, 0, -s, c, 0, 0, 0, 0, 1, 0, x, y, STAGE_HEIGHT, 1};
    return MultiplyMatrices(frame, CONSOLE_SPACE);
}

SDL_Rect FrameRect(Uint32 frameWidth, Uint32 frameHeight, int x, int y, int width, int height) {
    float scale = static_cast<float>(frameHeight) / SCREEN_HEIGHT;
    float offset = (static_cast<float>(frameWidth) - SCREEN_WIDTH * scale) / 2.0f;
    return {static_cast<int>(offset + static_cast<float>(x) * scale),
            static_cast<int>(static_cast<float>(y) * scale),
            static_cast<int>(static_cast<float>(width) * scale),
            static_cast<int>(static_cast<float>(height) * scale)};
}

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

s16 PlaySoundEffect(std::vector<s8> &sample, std::uint32_t volume) {
    s16 channel = FindFreeSoundChannel();
    if (channel != -1) {
        N64InitSoundChannel(
            channel, sample.data(), static_cast<u32>(sample.size() - SOUND_PADDING), 0);
        g_aSoundEffectChannels[channel].volume = static_cast<s32>(volume);
        g_aSoundEffectChannels[channel].step = SFX_STEP_ONE;
    }
    return channel;
}

void ScreenFade::SetTarget(float to, float seconds) {
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

bool ScreenFade::FadingIn() const {
    return rate > 0.0f && !autoReverse;
}

bool ScreenFade::FadingOut() const {
    return rate < 0.0f || autoReverse;
}

bool ScreenFade::Complete() const {
    return target == level && !autoReverse;
}

void ScreenFade::Update(float seconds) {
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

void ScreenFade::Draw(SceneRenderer &renderer) const {
    if (level == 1.0f) {
        return;
    }
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
    cover(0.0f, static_cast<float>(left) / SCREEN_WIDTH);
    cover(static_cast<float>(right) / SCREEN_WIDTH, 1.0f);
    if (!black.empty()) {
        renderer.DrawQuads(black, FlatMaterial{}, whole);
    }
}

} // namespace coastline
