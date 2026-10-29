#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <span>
#include <unordered_map>
#include <vector>

#include <SDL3/SDL.h>

#include "common/renderdevice.h"

namespace coastline {

class GpuWindow;

/** A vertex the game transformed and lit, laid out as D3DTLVERTEX. */
using TlVertex = ::DeviceVertex;
static_assert(sizeof(TlVertex) == 32, "TlVertex has the 32 bytes of D3DTLVERTEX.");

/** The comparison functions of Direct3D, with their D3DCMPFUNC values. */
enum class CompareFunction : std::uint8_t {
    Never = 1,        /*!< D3DCMP_NEVER. */
    Less = 2,         /*!< D3DCMP_LESS. */
    Equal = 3,        /*!< D3DCMP_EQUAL. */
    LessEqual = 4,    /*!< D3DCMP_LESSEQUAL. */
    Greater = 5,      /*!< D3DCMP_GREATER. */
    NotEqual = 6,     /*!< D3DCMP_NOTEQUAL. */
    GreaterEqual = 7, /*!< D3DCMP_GREATEREQUAL. */
    Always = 8,       /*!< D3DCMP_ALWAYS. */
};

/** The blend factors of Direct3D, with their D3DBLEND values. */
enum class BlendFactor : std::uint8_t {
    Zero = 1,                     /*!< D3DBLEND_ZERO. */
    One = 2,                      /*!< D3DBLEND_ONE. */
    SourceColor = 3,              /*!< D3DBLEND_SRCCOLOR. */
    InverseSourceColor = 4,       /*!< D3DBLEND_INVSRCCOLOR. */
    SourceAlpha = 5,              /*!< D3DBLEND_SRCALPHA. */
    InverseSourceAlpha = 6,       /*!< D3DBLEND_INVSRCALPHA. */
    DestinationAlpha = 7,         /*!< D3DBLEND_DESTALPHA. */
    InverseDestinationAlpha = 8,  /*!< D3DBLEND_INVDESTALPHA. */
    DestinationColor = 9,         /*!< D3DBLEND_DESTCOLOR. */
    InverseDestinationColor = 10, /*!< D3DBLEND_INVDESTCOLOR. */
};

/** The face culling of Direct3D, with its D3DCULL values. */
enum class CullMode : std::uint8_t {
    None = 1,             /*!< D3DCULL_NONE. */
    Clockwise = 2,        /*!< D3DCULL_CW: clockwise triangles are culled. */
    CounterClockwise = 3, /*!< D3DCULL_CCW: counter-clockwise triangles are culled. */
};

/** The render states of Direct3D 5 the game sets. */
struct RenderStates {
    bool depthTest = true;                                      /*!< D3DRENDERSTATE_ZENABLE. */
    bool depthWrite = true;                                     /*!< D3DRENDERSTATE_ZWRITEENABLE. */
    CompareFunction depthFunction = CompareFunction::LessEqual; /*!< D3DRENDERSTATE_ZFUNC. */
    bool blend = false;                               /*!< D3DRENDERSTATE_ALPHABLENDENABLE. */
    BlendFactor sourceBlend = BlendFactor::One;       /*!< D3DRENDERSTATE_SRCBLEND. */
    BlendFactor destinationBlend = BlendFactor::Zero; /*!< D3DRENDERSTATE_DESTBLEND. */
    CullMode cull = CullMode::CounterClockwise;       /*!< D3DRENDERSTATE_CULLMODE. */
    bool fog = false;                                 /*!< D3DRENDERSTATE_FOGENABLE. */
    std::uint32_t fogColor = 0;                       /*!< D3DRENDERSTATE_FOGCOLOR. */
    bool specular = false;                            /*!< D3DRENDERSTATE_SPECULARENABLE. */
    bool alphaTest = false;                           /*!< D3DRENDERSTATE_ALPHATESTENABLE. */
    CompareFunction alphaFunction = CompareFunction::Always; /*!< D3DRENDERSTATE_ALPHAFUNC. */
    std::uint8_t alphaReference = 0;                         /*!< D3DRENDERSTATE_ALPHAREF. */
    bool colorKey = false;                                   /*!< D3DRENDERSTATE_COLORKEYENABLE. */
    bool linearFilter = true; /*!< D3DRENDERSTATE_TEXTUREMIN and TEXTUREMAG are linear. */
    bool wrapU = true;        /*!< D3DRENDERSTATE_TEXTUREADDRESSU wraps rather than clamps. */
    bool wrapV = true;        /*!< D3DRENDERSTATE_TEXTUREADDRESSV wraps rather than clamps. */
};

/** A texture of the renderer, from 1; 0 draws untextured. */
using TextureHandle = std::uint32_t;

/**
 * The colour combiner of the RDP of the Nintendo 64, for the draws of its display lists. Each cycle
 * computes (a - b) * c + d for the colour and for the alpha, from the texels of two tiles, the
 * colour of the vertex as the shade, and the primitive and environment colours.
 */
struct CombinerState {
    bool enabled = false;          /*!< Whether the draws combine; otherwise they draw as D3D 5. */
    bool twoCycles = false;        /*!< Whether the second cycle combines the first. */
    std::int32_t color[2][4] = {}; /*!< The a, b, c, and d of the colour of each cycle. */
    std::int32_t alpha[2][4] = {}; /*!< The a, b, c, and d of the alpha of each cycle. */
    float primitive[4] = {};       /*!< The primitive colour, from 0 to 1. */
    float environment[4] = {};     /*!< The environment colour, from 0 to 1. */
    float lodFraction = 0;         /*!< The primitive level of detail fraction. */
    float alphaThreshold = -1;     /*!< The alpha a pixel needs, or -1 for no compare. */
    TextureHandle texture1 = 0;    /*!< The texture of the second tile, or 0 for the first. */
    /** The scale and offset that take the coordinates of the first tile to the second. */
    float texture1Transform[4] = {1, 1, 0, 0};
};

/**
 * The immediate mode of Direct3D 5 on the SDL GPU API: triangles of transformed vertices under the
 * render states the game sets. A scene records its draws, and EndScene() uploads their vertices
 * once and draws them into the frame texture of the window.
 */
class TlRenderer {
public:
    /**
     * Create the pipelines' shaders and the depth buffer for @p window.
     *
     * @param window The window to draw into.
     * @return The renderer, or nothing with the reason logged.
     */
    static std::unique_ptr<TlRenderer> Create(GpuWindow &window);

    ~TlRenderer();
    TlRenderer(const TlRenderer &) = delete;
    TlRenderer &operator=(const TlRenderer &) = delete;

    /**
     * Create a texture from 32-bit pixels.
     *
     * @param width The width in texels.
     * @param height The height in texels.
     * @param rgba The texels, four bytes each in the order R, G, B, A.
     * @param hasAlpha Whether the texture provides the alpha of a pixel.
     * @return The texture, or 0 when it was not created.
     */
    TextureHandle CreateTexture(std::uint32_t width,
                                std::uint32_t height,
                                std::span<const std::uint8_t> rgba,
                                bool hasAlpha);

    /**
     * Replace the pixels of a texture, at the size it was created with or a new size.
     *
     * @param texture The texture.
     * @param width The width in texels.
     * @param height The height in texels.
     * @param rgba The texels, four bytes each in the order R, G, B, A.
     */
    void UpdateTexture(TextureHandle texture,
                       std::uint32_t width,
                       std::uint32_t height,
                       std::span<const std::uint8_t> rgba);

    /**
     * Start a scene. The frame texture and the depth buffer are cleared.
     *
     * @param clearColor The colour, 0xAARRGGBB.
     */
    void BeginScene(std::uint32_t clearColor);

    /** @return The render states for the next draws to change. */
    RenderStates &States() {
        return m_states;
    }

    /**
     * Bind a texture to the next draws.
     *
     * @param texture The texture, or 0 for none.
     */
    void SetTexture(TextureHandle texture) {
        m_texture = texture;
    }

    /**
     * Combine the next draws as the RDP does, or draw them as Direct3D 5 does.
     *
     * @param combiner The combiner, disabled for Direct3D 5.
     */
    void SetCombiner(const CombinerState &combiner) {
        m_combiner = combiner;
    }

    /**
     * Draw a triangle list of indexed vertices.
     *
     * @param vertices The vertices.
     * @param indices Three indices for each triangle.
     */
    void DrawIndexed(std::span<const TlVertex> vertices, std::span<const std::uint16_t> indices);

    /**
     * Draw the scene into the frame texture of the window, which presents it.
     *
     * @param capture A PNG file to save the frame to.
     * @return Whether the frame was presented.
     */
    bool EndScene(const std::filesystem::path *capture = nullptr);

    /**
     * Set the size of the screen the vertices are placed on. The screen is drawn into the largest
     * rectangle of its aspect ratio at the centre of the frame. The size of the frame is the
     * default.
     *
     * @param width The width in pixels, or 0 for the width of the frame.
     * @param height The height in pixels, or 0 for the height of the frame.
     */
    void SetScreenSize(std::uint32_t width, std::uint32_t height) {
        m_screenWidth = width;
        m_screenHeight = height;
    }

    /** @return The width of the frame of the window in pixels. */
    std::uint32_t FrameWidth() const;

    /** @return The height of the frame of the window in pixels. */
    std::uint32_t FrameHeight() const;

private:
    struct PipelineKey;
    struct Draw {
        RenderStates states;
        TextureHandle texture;
        std::uint32_t firstIndex;
        std::uint32_t indexCount;
        std::int32_t vertexOffset;
        CombinerState combiner;
    };
    struct Texture {
        SDL_GPUTexture *texture;
        bool hasAlpha;
    };

    explicit TlRenderer(GpuWindow &window) : m_window(window) {
    }
    SDL_GPUGraphicsPipeline *Pipeline(const RenderStates &states);
    SDL_GPUSampler *Sampler(const RenderStates &states);
    bool EnsureBuffers(std::size_t vertexBytes, std::size_t indexBytes);

    GpuWindow &m_window;
    std::uint32_t m_screenWidth = 0;
    std::uint32_t m_screenHeight = 0;
    SDL_GPUShader *m_vertexShader = nullptr;
    SDL_GPUShader *m_fragmentShader = nullptr;
    SDL_GPUTexture *m_depth = nullptr;
    SDL_GPUTextureFormat m_depthFormat = SDL_GPU_TEXTUREFORMAT_INVALID;
    SDL_GPUTexture *m_white = nullptr;
    // The target of the samples of each pixel when anti-aliasing, resolved into the frame texture.
    SDL_GPUTexture *m_multisampled = nullptr;
    SDL_GPUSampleCount m_sampleCount = SDL_GPU_SAMPLECOUNT_1;
    SDL_GPUBuffer *m_vertexBuffer = nullptr;
    SDL_GPUBuffer *m_indexBuffer = nullptr;
    SDL_GPUTransferBuffer *m_transfer = nullptr;
    std::size_t m_vertexCapacity = 0;
    std::size_t m_indexCapacity = 0;
    std::unordered_map<std::uint32_t, SDL_GPUGraphicsPipeline *> m_pipelines;
    std::unordered_map<std::uint32_t, SDL_GPUSampler *> m_samplers;
    std::vector<Texture> m_textures;
    RenderStates m_states;
    TextureHandle m_texture = 0;
    CombinerState m_combiner;
    std::uint32_t m_clearColor = 0;
    std::vector<TlVertex> m_vertices;
    std::vector<std::uint16_t> m_indices;
    std::vector<Draw> m_draws;
};

} // namespace coastline
