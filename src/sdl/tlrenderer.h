#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <span>
#include <unordered_map>
#include <vector>

#include <SDL3/SDL.h>

namespace coastline {

class GpuWindow;

/** A vertex the game transformed and lit, laid out as D3DTLVERTEX. */
struct TlVertex {
    float sx;               /*!< The screen x in pixels. */
    float sy;               /*!< The screen y in pixels. */
    float sz;               /*!< The depth, from 0 to 1. */
    float rhw;              /*!< The reciprocal of the homogeneous w. */
    std::uint32_t color;    /*!< The diffuse colour, 0xAARRGGBB. */
    std::uint32_t specular; /*!< The specular colour, with the fog share in its alpha. */
    float tu;               /*!< The texture u. */
    float tv;               /*!< The texture v. */
};
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

private:
    struct PipelineKey;
    struct Draw {
        RenderStates states;
        TextureHandle texture;
        std::uint32_t firstIndex;
        std::uint32_t indexCount;
        std::int32_t vertexOffset;
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
    SDL_GPUShader *m_vertexShader = nullptr;
    SDL_GPUShader *m_fragmentShader = nullptr;
    SDL_GPUTexture *m_depth = nullptr;
    SDL_GPUTextureFormat m_depthFormat = SDL_GPU_TEXTUREFORMAT_INVALID;
    SDL_GPUTexture *m_white = nullptr;
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
    std::uint32_t m_clearColor = 0;
    std::vector<TlVertex> m_vertices;
    std::vector<std::uint16_t> m_indices;
    std::vector<Draw> m_draws;
};

} // namespace coastline
