#pragma once

/**
 * @file
 * The renderer of the converted models and of flat pictures and text, on the GPU of SDL.
 */

#include <array>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <span>
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

/** The lights of a model: an ambient colour and a directional colour from a direction. */
struct Lighting {
    std::array<float, 3> direction{0.0f, 0.0f, 1.0f}; /*!< The unit direction toward the light. */
    std::array<float, 3> ambient{1.0f, 1.0f, 1.0f};   /*!< The ambient colour, from 0 to 1. */
    std::array<float, 3> directional{};               /*!< The directional colour, from 0 to 1. */
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

/** A converted model on the GPU: its meshes, its parts, and its animations. */
class RenderModel {
public:
    /**
     * Read a glTF document of the import and upload its images and vertices.
     *
     * @param renderer The renderer that draws the model.
     * @param path The glTF document.
     * @return The model, or nothing with the reason logged.
     */
    static std::unique_ptr<RenderModel> Load(SceneRenderer &renderer,
                                             const std::filesystem::path &path);
    ~RenderModel();
    RenderModel(const RenderModel &) = delete;
    RenderModel &operator=(const RenderModel &) = delete;

    /** @return The animations of the vertices of the model. */
    const std::vector<VertexAnimationInfo> &Animations() const;

    /** @return The centre of what the model draws, in the units of its document. */
    std::array<float, 3> Centre() const;

    /** @return The radius of what the model draws, in the units of its document. */
    float Radius() const;

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
     * @return The texture, or nothing with the reason logged.
     */
    static std::unique_ptr<RenderTexture>
    Load(SceneRenderer &renderer, const std::filesystem::path &path, bool linear);
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
     * Draw a model, its opaque parts now and its translucent ones after every opaque draw.
     *
     * @param model The model.
     * @param world The place of the model.
     * @param viewProjection The camera and its projection, to a depth from 0 to 1.
     * @param lighting The lights of the model.
     * @param pose The pose of its vertices, or none for the pose of the document.
     * @param depth Whether the model tests and writes the depth.
     */
    void DrawModel(const RenderModel &model,
                   const Matrix4 &world,
                   const Matrix4 &viewProjection,
                   const Lighting &lighting,
                   std::optional<MorphPose> pose,
                   bool depth);

    /**
     * Draw flat quads over everything drawn before them, blended by their alpha.
     *
     * @param corners Four corners for each quad, clockwise from the top left.
     * @param material How the quads are coloured.
     * @param projection The matrix from the space of the corners to the screen.
     */
    void DrawQuads(std::span<const FlatVertex> corners,
                   const FlatMaterial &material,
                   const Matrix4 &projection);

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
