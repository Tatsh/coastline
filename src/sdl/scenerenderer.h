#pragma once

/**
 * @file
 * The renderer of the converted models and of flat pictures and text, on the GPU of SDL.
 */

#include <array>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include <SDL3/SDL.h>

namespace coastline {

class GpuWindow;
class SceneRenderer;

/** A 4 by 4 matrix, column by column. */
using Matrix4 = std::array<float, 16>;

/** The matrix that changes nothing. */
inline constexpr Matrix4 IDENTITY_MATRIX{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

/**
 * @param a The matrix applied second.
 * @param b The matrix applied first.
 * @return The product a b.
 */
Matrix4 MultiplyMatrices(const Matrix4 &a, const Matrix4 &b);

/** The ambient colours of a ramped instance of a track on a clear day, from 0 to 1. */
inline constexpr std::array<std::array<float, 3>, 4> CLEAR_LIGHT_RAMP{
    {{63 / 255.0f, 63 / 255.0f, 51 / 255.0f},
     {127 / 255.0f, 127 / 255.0f, 102 / 255.0f},
     {190 / 255.0f, 190 / 255.0f, 153 / 255.0f},
     {1.0f, 1.0f, 204 / 255.0f}}};

/**
 * The lights of a model, an ambient colour and a directional colour from a direction in the space
 * the model is placed in. A ramped instance of a track takes its ambient colour from the ramp by
 * the low bits of its flags, and a directional colour only where it is lit at night.
 */
struct Lighting {
    std::array<float, 3> direction{0.0f, 0.0f, 1.0f}; /*!< The unit direction toward the light. */
    std::array<float, 3> ambient{1.0f, 1.0f, 1.0f};   /*!< The ambient colour, from 0 to 1. */
    std::array<float, 3> directional{};               /*!< The directional colour, from 0 to 1. */
    std::array<std::array<float, 3>, 4> ramp = CLEAR_LIGHT_RAMP; /*!< The ambient ramp. */
    std::array<float, 3> nightLit{}; /*!< The directional colour of a night-lit ramped instance. */
};

/** The pose of the vertices of an animated model: two keys and the share of the second. */
struct MorphPose {
    int first = 0;      /*!< The morph target of the first key. */
    int second = 0;     /*!< The morph target of the second key. */
    float blend = 0.0f; /*!< The share of the second key, from 0 to 1. */
};

/** A vertex animation of a model, as the game plays it. */
struct VertexAnimationInfo {
    std::vector<float> keyTimes; /*!< The time of each key, in seconds. */
    std::uint16_t flags = 0;     /*!< The MODEL_ANIMATION_ flags. */
    float startTime = 0.0f;      /*!< The time the animation starts at. */
    float endTime = 0.0f;        /*!< The time the animation ends at. */
    float time = 0.0f;           /*!< The time the animation is at when the model loads. */
    int firstTarget = 0;         /*!< The morph target of its first key. */
};

class RenderTexture;

/**
 * The reflection of the sky on a car: the image the parts whose texture coordinates come from
 * their normals sample, the axes those coordinates follow, and the column of the image the
 * window starts at, as the tile of the reflection of the race takes it.
 */
struct Reflection {
    const RenderTexture *texture = nullptr; /*!< The image of the sky. */
    std::array<float, 3> across{};          /*!< The axis of the coordinate across the image. */
    std::array<float, 3> up{};              /*!< The axis of the coordinate down the image. */
    float column = 0.0f;                    /*!< The first column of the window, in texels. */
    /** The environment colour the reflection blends towards by its alpha, the fog. */
    std::array<float, 4> environment{1.0f, 1.0f, 1.0f, 1.0f};
};

/**
 * The specular highlight of a car, as the PC release generates its coordinates for the tile of
 * gDPSetHilite1Tile(): a quarter of the dot of the normal with each axis across the texture, from
 * the offset of the tile, added to the colour drawn where the depth is equal.
 */
struct Highlight {
    const RenderTexture *texture =
        nullptr;                   /*!< The texture of the highlight, its alpha the shine. */
    std::array<float, 3> across{}; /*!< The axis of the coordinate across the texture. */
    std::array<float, 3> up{};     /*!< The axis of the coordinate down the texture. */
    float s = 0.0f;                /*!< The offset across the texture, from 0 to 1. */
    float t = 0.0f;                /*!< The offset down the texture, from 0 to 1. */
    std::array<std::uint32_t, 2> combine{}; /*!< The words of gDPSetCombineLERP(). */
    std::array<float, 4> primitive{};       /*!< The primitive colour, from 0 to 1. */
};

/**
 * The fog of the PC release: each vertex shows the share of its colour of the range by the
 * reciprocal of its w, a share below 8 of 255 none, and the rest is the colour of the fog.
 */
struct Fog {
    std::array<float, 3> color{}; /*!< The colour of the fog, from 0 to 1. */
    float range = 0.0f;           /*!< The range, 8192 or on a clear night 16384. */
};

/**
 * The shadow of a car on the track, as RenderTrackInstance() lays its texture on the ground under
 * the car: 1024 across the texture, centred under the car, along and across its heading.
 */
struct CarShadow {
    const RenderTexture *texture = nullptr; /*!< The texture of the shadow, its alpha the shade. */
    float x = 0.0f;                         /*!< The place of the car across the world. */
    float y = 0.0f;                         /*!< The place of the car along the world. */
    float along = 0.0f;  /*!< The units of the texture for each unit along the car. */
    float across = 0.0f; /*!< The units of the texture for each unit across the car. */
    std::array<float, 2> forward{1.0f, 0.0f}; /*!< The heading of the car on the ground. */
    std::array<float, 4> color{};             /*!< The colour the shade blends towards. */
    std::vector<int> instances; /*!< The instances of the track the shadow falls on. */
};

/**
 * The pose of a vertex animation at a time, as the console blends its keys.
 *
 * @param animation The animation.
 * @param time The time of the animation, in seconds.
 * @return The two keys around the time and the share of the second, in 4096ths, or nothing for an
 * animation of fewer than two keys.
 */
std::optional<MorphPose> PoseAtTime(const VertexAnimationInfo &animation, float time);

/** What of a glTF document a RenderModel takes, and how. */
struct ModelOptions {
    /** The name of a node to draw in place of the default scene, such as the sky of a track or a
     * wheel of a car, at its origin without its translation, or none for the scene. */
    std::optional<std::string> node;
    /** The red, green, and blue of the colour of a car, which paints the first two colours of the
     * images of its materials as SetMaterialPaletteColors() paints them, or none for the colours
     * of the images. */
    std::optional<std::array<std::uint8_t, 3>> paint;
    /** Whether a node of a mesh is drawn, by its name, or none for every node. */
    std::function<bool(const std::string &)> parts;
    /** Another glTF document of the same model whose images take the place of these, one for
     * one, such as the textures of the cartridge on a car of the PC release, or none. A document
     * with another number of images is not taken. */
    std::optional<std::filesystem::path> images;
    /** The number of palette variants of the images of materials, such as the lights of a car. */
    int variants = 0;
    /** Change the palette of an image of a material for a variant, and report whether the image
     * has variants; called for each variant of each image with a material. */
    std::function<bool(SDL_Palette *, int material, int variant)> paletteVariant;
    /** Whether the document may have nothing to draw, such as a part a level of detail of a car
     * does not have, which then loads no model without an error. */
    bool optional = false;
};

/** A converted model on the GPU: its meshes, its parts, and its animations. */
class RenderModel {
public:
    /**
     * Read a glTF document of the import and upload its images and vertices.
     *
     * @param renderer The renderer that draws the model.
     * @param path The glTF document.
     * @param options What of the document the model takes, and how.
     * @return The model, or nothing with the reason logged.
     */
    static std::unique_ptr<RenderModel> Load(SceneRenderer &renderer,
                                             const std::filesystem::path &path,
                                             const ModelOptions &options = {});
    ~RenderModel();
    RenderModel(const RenderModel &) = delete;
    RenderModel &operator=(const RenderModel &) = delete;

    /** @return The animations of the vertices of the model. */
    const std::vector<VertexAnimationInfo> &Animations() const;

    /** @return The centre of what the model draws, in the units of its document. */
    std::array<float, 3> Centre() const;

    /** @return The radius of what the model draws, in the units of its document. */
    float Radius() const;

    /**
     * Pick the palette variant the images with variants show until the next call.
     *
     * @param variant The variant, from 0 to the variants of the options of the model.
     */
    void SetVariant(int variant);

private:
    friend class SceneRenderer;
    struct State;
    explicit RenderModel(std::unique_ptr<State> state);
    std::unique_ptr<State> m_state;
};

/** A texture of the renderer for flat pictures and text. */
class RenderTexture {
public:
    /**
     * Read a PNG image.
     *
     * @param renderer The renderer that draws with the texture.
     * @param path The image.
     * @param linear Whether the texture filters linearly, or takes the nearest texel.
     * @param repeat Whether the texture repeats past its edges, or clamps to them.
     * @return The texture, or nothing with the reason logged.
     */
    static std::unique_ptr<RenderTexture> Load(SceneRenderer &renderer,
                                               const std::filesystem::path &path,
                                               bool linear,
                                               bool repeat = false);

    /**
     * Upload texels the game has in memory, such as a texture of the race built into the engine.
     *
     * @param renderer The renderer that draws with the texture.
     * @param width The width in texels.
     * @param height The height in texels.
     * @param rgba The texels as 8-bit RGBA, row by row from the top.
     * @param linear Whether the texture filters linearly, or takes the nearest texel.
     * @param repeat Whether the texture repeats past its edges, or clamps to them.
     * @return The texture, or nothing when the upload failed.
     */
    static std::unique_ptr<RenderTexture> FromTexels(SceneRenderer &renderer,
                                                     int width,
                                                     int height,
                                                     std::span<const std::uint8_t> rgba,
                                                     bool linear,
                                                     bool repeat);
    ~RenderTexture();
    RenderTexture(const RenderTexture &) = delete;
    RenderTexture &operator=(const RenderTexture &) = delete;

    /** @return The width in texels. */
    int Width() const;

    /** @return The height in texels. */
    int Height() const;

private:
    friend class SceneRenderer;
    RenderTexture(SDL_GPUDevice *device,
                  SDL_GPUTexture *texture,
                  SDL_GPUSampler *sampler,
                  int width,
                  int height);
    SDL_GPUDevice *m_device;
    SDL_GPUTexture *m_texture;
    SDL_GPUSampler *m_sampler;
    int m_width;
    int m_height;
};

/** A corner of a flat quad: its place in the space of the flat draws, its texels, and its colour.
 */
struct FlatVertex {
    float x = 0.0f;                                        /*!< The left to right place. */
    float y = 0.0f;                                        /*!< The top to bottom place. */
    float u = 0.0f;                                        /*!< The texel across, from 0 to 1. */
    float v = 0.0f;                                        /*!< The texel down, from 0 to 1. */
    std::array<std::uint8_t, 4> color{255, 255, 255, 255}; /*!< The colour, R, G, B, and A. */
    float z = 0.0f;                                        /*!< The back to front place. */
};

/** How a run of flat quads is coloured: the textures and the colour combiner of the RDP. */
struct FlatMaterial {
    const RenderTexture *texel0 = nullptr; /*!< The first texture, or none for white. */
    const RenderTexture *texel1 = nullptr; /*!< The second texture, or none for white. */
    std::array<std::uint32_t, 2>
        combine{};         /*!< The words of the combiner, or 0 for texel times colour. */
    bool twoCycle = false; /*!< Whether the combiner runs both of its cycles. */
    std::array<float, 4> prim{1.0f, 1.0f, 1.0f, 1.0f}; /*!< The primitive colour. */
    std::array<float, 4> env{1.0f, 1.0f, 1.0f, 1.0f};  /*!< The environment colour. */
    /** The scale of u and v, then their offset, from the texels of texel0 to those of texel1. */
    std::array<float, 4> texel1Transform{1.0f, 1.0f, 0.0f, 0.0f};
    float alphaCut = -1.0f; /*!< The alpha below which nothing is drawn, below 0 for none. */
};

/**
 * The renderer of a frame. Begin() starts it, the draws record what it shows, and End() uploads the
 * vertices of the flat draws, draws the frame, and presents it.
 */
class SceneRenderer {
public:
    /**
     * Create the pipelines of the renderer for a window.
     *
     * @param window The window to draw into.
     * @return The renderer, or nothing with the reason logged.
     */
    static std::unique_ptr<SceneRenderer> Create(GpuWindow &window);
    ~SceneRenderer();
    SceneRenderer(const SceneRenderer &) = delete;
    SceneRenderer &operator=(const SceneRenderer &) = delete;

    /** @return The window the renderer draws into. */
    GpuWindow &Window();

    /**
     * Start a frame, cleared to a colour.
     *
     * @param clear The red, green, blue, and alpha of the clear, from 0 to 1.
     */
    void Begin(std::array<float, 4> clear);

    /**
     * Set the time the animated textures of the frame show, from 0 at Begin().
     *
     * @param milliseconds The time, such as the elapsed time of a race.
     */
    void SetTextureTime(std::uint32_t milliseconds);

    /**
     * Set the instances of a track the frame does not draw, none from Begin().
     *
     * @param flags The TRACK_INSTANCE_ flags, any of which hides an instance.
     */
    void SetHiddenInstanceFlags(std::uint32_t flags);

    /**
     * Place the instances of a track at the matrices the race moves them to, the default places
     * from Begin().
     *
     * @param matrix The 16 values of the matrix of an instance by its number, or none for its
     * place in the model.
     */
    void SetInstanceMatrices(std::function<const float *(int)> matrix);

    /**
     * Set the fog the models of the frame are drawn in, none from Begin().
     *
     * @param fog The fog, or none.
     */
    void SetFog(std::optional<Fog> fog);

    /**
     * Set whether the models drawn after it are drawn through a projection that mirrors them
     * across, their faces then turning the other way, not from Begin().
     *
     * @param mirrored Whether the projection mirrors the frame.
     */
    void SetMirrored(bool mirrored);

    /**
     * Set the rectangle of the frame the draws after it fill, as a view of a split screen fills its
     * part of the screen, the whole frame from Begin(). The draws are cut to the rectangle, and the
     * fog and the hidden instances of each draw are those set when it is made.
     *
     * @param view The rectangle in pixels, or none for the whole frame.
     */
    void SetViewport(std::optional<SDL_Rect> view);

    /**
     * Set whether the models drawn after it are ghosts, not from Begin(). Every part of a ghost is
     * drawn after the opaque draws, its colour by itself added to the frame without writing the
     * depth, as BRD3D draws a ghost.
     *
     * @param ghost Whether the models are ghosts.
     */
    void SetGhost(bool ghost);

    /**
     * Set whether the models drawn after it draw their translucent parts straight after their
     * opaque parts, before the next draw, rather than after every opaque draw, not from Begin().
     *
     * @param inOrder Whether the models are drawn in order.
     */
    void SetDrawnInOrder(bool inOrder);

    /**
     * Draw a model, its opaque parts now and its translucent ones after every opaque draw.
     *
     * @param model The model.
     * @param world The place of the model.
     * @param viewProjection The camera and its projection, to a depth from 0 to 1.
     * @param lighting The lights of the model.
     * @param pose The pose of its vertices, or none for the pose of the document.
     * @param depth Whether the model tests and writes the depth.
     * @param scissor The rectangle of the frame in pixels the model is cut to, or none for the
     * whole frame.
     */
    void DrawModel(const RenderModel &model,
                   const Matrix4 &world,
                   const Matrix4 &viewProjection,
                   const Lighting &lighting,
                   std::optional<MorphPose> pose,
                   bool depth,
                   std::optional<SDL_Rect> scissor = std::nullopt);

    /**
     * Draw a model whose vertices take the sum of the poses of several vertex animations, each
     * moving its own vertices, as the parts of the start lights move.
     *
     * @param model The model.
     * @param world The place of the model.
     * @param viewProjection The camera and its projection, to a depth from 0 to 1.
     * @param lighting The lights of the model.
     * @param poses The pose of each animation, the first 64 of them.
     * @param depth Whether the model tests and writes the depth.
     * @param scissor The rectangle of the frame in pixels the model is cut to, or none for the
     * whole frame.
     */
    void DrawModel(const RenderModel &model,
                   const Matrix4 &world,
                   const Matrix4 &viewProjection,
                   const Lighting &lighting,
                   std::vector<MorphPose> poses,
                   bool depth,
                   std::optional<SDL_Rect> scissor = std::nullopt);

    /**
     * Draw a car whose parts with texture coordinates from their normals and no texture of their
     * own reflect the sky.
     *
     * @param model The model.
     * @param world The place of the model.
     * @param viewProjection The camera and its projection, to a depth from 0 to 1.
     * @param lighting The lights of the model.
     * @param reflection The reflection of the sky.
     * @param scissor The rectangle of the frame in pixels the model is cut to, or none for the
     * whole frame.
     */
    void DrawReflectingModel(const RenderModel &model,
                             const Matrix4 &world,
                             const Matrix4 &viewProjection,
                             const Lighting &lighting,
                             const Reflection &reflection,
                             std::optional<SDL_Rect> scissor = std::nullopt);

    /**
     * Draw the specular highlight of a car over the parts drawn before it.
     *
     * @param model The model of the highlighted parts.
     * @param world The place of the model.
     * @param viewProjection The camera and its projection, to a depth from 0 to 1.
     * @param highlight The highlight.
     */
    void DrawHighlight(const RenderModel &model,
                       const Matrix4 &world,
                       const Matrix4 &viewProjection,
                       const Highlight &highlight);

    /**
     * Draw instances of a track in an order, with the shadow of a car on the instances near the
     * car.
     *
     * @param model The model of the track.
     * @param world The place of the model.
     * @param viewProjection The camera and its projection, to a depth from 0 to 1.
     * @param lighting The lights of the model.
     * @param instances The instances drawn, in their order, with -1 for the nodes of no instance.
     * @param shadow The shadow of the car, or none.
     */
    void DrawShadowedModel(const RenderModel &model,
                           const Matrix4 &world,
                           const Matrix4 &viewProjection,
                           const Lighting &lighting,
                           std::vector<int> instances,
                           std::optional<CarShadow> shadow);

    /**
     * Draw flat quads over everything drawn before them, blended by their alpha.
     *
     * @param corners Four corners for each quad, clockwise from the top left.
     * @param material How the quads are coloured.
     * @param projection The matrix from the space of the corners to the screen.
     * @param depthTest Whether the quads are hidden behind what is nearer, as the puffs of a race
     * are, without writing the depth.
     */
    void DrawQuads(std::span<const FlatVertex> corners,
                   const FlatMaterial &material,
                   const Matrix4 &projection,
                   bool depthTest = false);

    /**
     * Draw the frame and present it.
     *
     * @param capture A PNG file to write the frame to, or null.
     */
    void End(const std::filesystem::path *capture);

private:
    friend class RenderModel;
    friend class RenderTexture;
    struct State;
    explicit SceneRenderer(std::unique_ptr<State> state);
    std::unique_ptr<State> m_state;
};

} // namespace coastline
