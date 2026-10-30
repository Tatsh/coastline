#pragma once

/**
 * @file
 * The space, the fades, and the sounds the splash screens and the menus of the console share.
 */

#include <array>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string_view>
#include <vector>

#include <SDL3/SDL.h>
#include <nlohmann/json.hpp>

#include "common/audio.h"
#include "scenerenderer.h"

namespace coastline {

/** A quarter turn, a half turn, and a whole turn, in the single precision of the console. */
constexpr float HALF_PI = 1.570796371f;
constexpr float PI = 3.141592741f;
constexpr float TWO_PI = 6.283185482f;

/** The low-resolution screen of the console, in pixels. */
constexpr float SCREEN_WIDTH = 320.0f;
constexpr float SCREEN_HEIGHT = 240.0f;

/** The silence after each sound, for the mixer to read zeros past its end. */
constexpr std::size_t SOUND_PADDING = 0x100;

/** The 32.32 step that plays a sample at its own rate. */
constexpr std::uint64_t SFX_STEP_ONE = 1ull << 32;

/** The volume of the sound effects at full level. */
constexpr u8 MAX_SFX_VOLUME = 0xFF;

/**
 * @param right The level of the right side, from 0 to 32.
 * @param left The level of the left side, from 0 to 32.
 * @return The levels of a sound effect channel, the right in the upper half and the left in the
 * lower.
 */
constexpr std::uint32_t SfxVolume(std::uint32_t right, std::uint32_t left) {
    return (right << 16) + left;
}

/** The inputs of the colour combiner of the RDP, as gDPSetCombineLERP() takes them. */
constexpr std::uint32_t COMBINED = 0;
constexpr std::uint32_t TEXEL0 = 1;
constexpr std::uint32_t TEXEL1 = 2;
constexpr std::uint32_t PRIMITIVE = 3;
constexpr std::uint32_t SHADE = 4;
constexpr std::uint32_t ENVIRONMENT = 5;
constexpr std::uint32_t ZERO_AB = 15;
constexpr std::uint32_t ZERO_D = 7;
constexpr std::uint32_t ZERO_ALPHA = 7;

/**
 * The two words of gDPSetCombineLERP(), from the colour and alpha inputs of each cycle.
 *
 * @return The upper and the lower word of the combine command.
 */
constexpr std::array<std::uint32_t, 2> CombineLerp(std::uint32_t a0,
                                                   std::uint32_t b0,
                                                   std::uint32_t c0,
                                                   std::uint32_t d0,
                                                   std::uint32_t alphaA0,
                                                   std::uint32_t alphaB0,
                                                   std::uint32_t alphaC0,
                                                   std::uint32_t alphaD0,
                                                   std::uint32_t a1,
                                                   std::uint32_t b1,
                                                   std::uint32_t c1,
                                                   std::uint32_t d1,
                                                   std::uint32_t alphaA1,
                                                   std::uint32_t alphaB1,
                                                   std::uint32_t alphaC1,
                                                   std::uint32_t alphaD1) {
    return {(a0 & 0xf) << 20 | (c0 & 0x1f) << 15 | (alphaA0 & 7) << 12 | (alphaC0 & 7) << 9 |
                (a1 & 0xf) << 5 | (c1 & 0x1f),
            (b0 & 0xf) << 28 | (b1 & 0xf) << 24 | (alphaA1 & 7) << 21 | (alphaC1 & 7) << 18 |
                (d0 & 7) << 15 | (alphaB0 & 7) << 12 | (alphaD0 & 7) << 9 | (d1 & 7) << 6 |
                (alphaB1 & 7) << 3 | (alphaD1 & 7)};
}

/** @return A move by x, y, and z. */
Matrix4 Translation(float x, float y, float z);

/** @return A scale by s on every axis. */
Matrix4 Scale(float s);

/** @return A turn by an angle in radians about the x axis, as guRotateF() turns it. */
Matrix4 RotationX(float angle);

/** @return A turn by an angle in radians about the z axis, as guRotateF() turns it. */
Matrix4 RotationZ(float angle);

/**
 * The camera of the menus, which looks at the middle of the 320 by 240 screen from 290 in front
 * of it with a field of 45 degrees, over the whole frame, its height the height of the 4:3
 * screen, to a depth from 0 to 1.
 *
 * @param aspect The width of the frame over its height.
 * @param nearPlane The nearest distance drawn.
 * @param farPlane The farthest distance drawn.
 * @return The camera and its projection.
 */
Matrix4 MenuProjection(float aspect, float nearPlane, float farPlane);

/**
 * @param aspect The width of the frame over its height.
 * @return The flat draws of the 640 by 480 screen, centred in the frame at 4:3 with y down.
 */
Matrix4 FlatProjection(float aspect);

/**
 * Choose whether the background of the menus fills a frame wider than 4:3, scaled to its width and
 * cropped, rather than the 4:3 middle of the frame.
 *
 * @param wide Whether the background fills the frame.
 */
void SetWideMenus(bool wide);

/** @return Whether the background of the menus fills a frame wider than 4:3. */
bool WideMenus();

/**
 * DrawModelAt() of the console: a model turned about its own axis, scaled, stood up, and placed.
 * The glTF of a model of the console is stood up from z up to y up, and this turns it back.
 *
 * @param x The place across, in the space of the screen.
 * @param y The place up.
 * @param z The place forward.
 * @param angle The turn about the axis of the model, in radians.
 * @param scale The scale.
 * @return The place of the model.
 */
Matrix4 ScreenModelAt(float x, float y, float z, float angle, float scale);

/**
 * SetupCameraMatrix() of the console for the trackside camera of a stage of the menus. The camera
 * looks at the origin from an eye with z up, in a viewport of the 320 by 240 screen. The picture is
 * moved and scaled to the rectangle of the viewport in the frame, where the screen is centred at
 * 4:3.
 *
 * @param aspect The width of the frame over its height.
 * @param eye The place of the camera, in the space of the race.
 * @param x The left of the viewport.
 * @param y The top of the viewport.
 * @param width The width of the viewport.
 * @param height The height of the viewport.
 * @return The camera and its projection, to a depth from 0 to 1.
 */
Matrix4
StageCamera(float aspect, const std::array<float, 3> &eye, int x, int y, int width, int height);

/**
 * The frame of a car on a stage of the menus in the space of the console, facing an angle about z,
 * 0.254 above the stage.
 *
 * @param x The place across.
 * @param y The place along.
 * @param angle The direction the car faces, in radians.
 * @return The frame of the car.
 */
Matrix4 StageFrameAt(float x, float y, float angle);

/**
 * The place of a car on a stage of the menus, facing an angle about z, 0.254 above the stage. The
 * frame of the car is turned back from the z up of the console to the y up of the glTF.
 *
 * @param x The place across.
 * @param y The place along.
 * @param angle The direction the car faces, in radians.
 * @return The place of the car.
 */
Matrix4 StageCarAt(float x, float y, float angle);

/**
 * The parts of a car RenderVehicleModel() draws in the menus: the near detail, the textured parts,
 * the painted body, and the normal wheels. The underside shows only from below.
 */
inline constexpr std::array<std::string_view, 7> STAGE_CAR_PARTS{"lod0-nearDetail",
                                                                 "lod0-textured",
                                                                 "lod0-paint",
                                                                 "lod0-wheel0",
                                                                 "lod0-wheel1",
                                                                 "lod0-wheel2",
                                                                 "lod0-wheel3"};

/**
 * The lights RenderVehicleModel() lights a car with in the menus: g_DefaultLights, from
 * (84, 84, 84), in the colours of SetupWeatherLights() in clear weather.
 */
extern const Lighting STAGE_CAR_LIGHTING;

/**
 * The reflection of the sky on a car as RenderVehicleModel() sets it up for the trackside camera
 * of the menus looking at the origin from an eye: the axes of guLookAtReflectF() from the heading
 * of the camera, and the window of the tile turning with the heading.
 *
 * @param texture The image of the sky.
 * @param eye The place of the camera.
 * @return The reflection.
 */
Reflection StageReflection(const RenderTexture *texture, const std::array<float, 3> &eye);

/** A vertex of a shaded triangle of a stage: its place and its colour. */
struct StageVertex {
    std::array<float, 3> position;     /*!< The place, in the units of its display list. */
    std::array<std::uint8_t, 4> color; /*!< The colour, R, G, B, and A. */
};

/**
 * Add a shaded triangle to flat quads, as a quad whose last corner repeats its third.
 *
 * @param quads The corners of the quads.
 * @param a The first corner.
 * @param b The second corner.
 * @param c The third corner.
 */
void AddTriangle(std::vector<FlatVertex> &quads,
                 const StageVertex &a,
                 const StageVertex &b,
                 const StageVertex &c);

/**
 * @param frameWidth The width of the frame, in pixels.
 * @param frameHeight The height of the frame, in pixels.
 * @param x The left of a rectangle of the 320 by 240 screen.
 * @param y The top of the rectangle.
 * @param width The width of the rectangle.
 * @param height The height of the rectangle.
 * @return The rectangle in pixels of the frame, where the screen is centred at 4:3.
 */
SDL_Rect FrameRect(Uint32 frameWidth, Uint32 frameHeight, int x, int y, int width, int height);

/**
 * @param light The lights of a screen in lights.json.
 * @return The lights, in the space of the console.
 */
Lighting ReadLighting(const nlohmann::json &light);

/**
 * @param path A sound of the import, an unsigned 8-bit WAV file.
 * @return The signed 8-bit samples of the sound, then SOUND_PADDING of silence, or nothing with the
 * reason logged.
 */
std::optional<std::vector<s8>> ReadSound(const std::filesystem::path &path);

/**
 * Play a sound on a free sound effect channel.
 *
 * @param sample The samples, with the padding after them.
 * @param volume The levels of the channel, from SfxVolume().
 * @return The channel, or -1 when none is free.
 */
s16 PlaySoundEffect(std::vector<s8> &sample, std::uint32_t volume);

/**
 * The fade of the screen: a wipe whose level goes to its target at a rate, and the edges of the
 * picture it shows, in pixels of the 320 by 240 screen. The same level fades the sound effects.
 */
struct ScreenFade {
    float level = 0.0f;       /*!< From 0 for black to 1 for the whole picture. */
    float target = 0.0f;      /*!< The level the fade moves to. */
    float rate = 0.0f;        /*!< The change of the level in a second. */
    bool autoReverse = false; /*!< Whether a fade out waits for the fade in to finish. */
    bool skipFrame = false;   /*!< Whether the next frame leaves the level as it is. */
    int left = 0;             /*!< The left edge of the picture. */
    int right = 0;            /*!< The right edge of the picture. */

    /**
     * SetFadeTarget() of the console.
     *
     * @param to The level to fade to.
     * @param seconds The time of a fade across the whole range.
     */
    void SetTarget(float to, float seconds);

    /** @return Whether the fade moves towards the whole picture. */
    bool FadingIn() const;

    /** @return Whether the fade moves towards black. */
    bool FadingOut() const;

    /** @return Whether the level has arrived at its target. */
    bool Complete() const;

    /**
     * Advance the fade by a frame.
     *
     * @param seconds The time of the frame.
     */
    void Update(float seconds);

    /**
     * Draw the black of the wipe left and right of the picture, across the whole frame.
     *
     * @param renderer The renderer of the frame.
     */
    void Draw(SceneRenderer &renderer) const;
};

} // namespace coastline
