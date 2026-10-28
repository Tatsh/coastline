#include "tlrenderer.h"

#include <cstring>

#include "gpu.h"
#include "shaders/tl_frag_msl.h"
#include "shaders/tl_frag_spirv.h"
#include "shaders/tl_vert_msl.h"
#include "shaders/tl_vert_spirv.h"

namespace coastline {

namespace {

struct PixelUniforms {
    float fogColor[4];
    std::int32_t flags[4];
    std::int32_t modes[4];
    float alphaReference[4];
};

SDL_FColor ColorFromArgb(std::uint32_t argb) {
    return SDL_FColor{static_cast<float>((argb >> 16) & 0xFF) / 255.0f,
                      static_cast<float>((argb >> 8) & 0xFF) / 255.0f,
                      static_cast<float>(argb & 0xFF) / 255.0f,
                      static_cast<float>(argb >> 24) / 255.0f};
}

SDL_GPUCompareOp CompareOp(CompareFunction function) {
    switch (function) {
    case CompareFunction::Never:
        return SDL_GPU_COMPAREOP_NEVER;
    case CompareFunction::Less:
        return SDL_GPU_COMPAREOP_LESS;
    case CompareFunction::Equal:
        return SDL_GPU_COMPAREOP_EQUAL;
    case CompareFunction::LessEqual:
        return SDL_GPU_COMPAREOP_LESS_OR_EQUAL;
    case CompareFunction::Greater:
        return SDL_GPU_COMPAREOP_GREATER;
    case CompareFunction::NotEqual:
        return SDL_GPU_COMPAREOP_NOT_EQUAL;
    case CompareFunction::GreaterEqual:
        return SDL_GPU_COMPAREOP_GREATER_OR_EQUAL;
    case CompareFunction::Always:
    default:
        return SDL_GPU_COMPAREOP_ALWAYS;
    }
}

SDL_GPUBlendFactor Factor(BlendFactor factor) {
    switch (factor) {
    case BlendFactor::Zero:
        return SDL_GPU_BLENDFACTOR_ZERO;
    case BlendFactor::SourceColor:
        return SDL_GPU_BLENDFACTOR_SRC_COLOR;
    case BlendFactor::InverseSourceColor:
        return SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_COLOR;
    case BlendFactor::SourceAlpha:
        return SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    case BlendFactor::InverseSourceAlpha:
        return SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    case BlendFactor::DestinationAlpha:
        return SDL_GPU_BLENDFACTOR_DST_ALPHA;
    case BlendFactor::InverseDestinationAlpha:
        return SDL_GPU_BLENDFACTOR_ONE_MINUS_DST_ALPHA;
    case BlendFactor::DestinationColor:
        return SDL_GPU_BLENDFACTOR_DST_COLOR;
    case BlendFactor::InverseDestinationColor:
        return SDL_GPU_BLENDFACTOR_ONE_MINUS_DST_COLOR;
    case BlendFactor::One:
    default:
        return SDL_GPU_BLENDFACTOR_ONE;
    }
}

SDL_GPUShader *CreateShader(SDL_GPUDevice *device,
                            SDL_GPUShaderStage stage,
                            std::uint32_t samplers,
                            std::span<const unsigned char> spirv,
                            std::span<const unsigned char> msl) {
    SDL_GPUShaderCreateInfo info{};
    SDL_GPUShaderFormat formats = SDL_GetGPUShaderFormats(device);
    if (formats & SDL_GPU_SHADERFORMAT_SPIRV) {
        info.format = SDL_GPU_SHADERFORMAT_SPIRV;
        info.code = spirv.data();
        info.code_size = spirv.size();
        info.entrypoint = "main";
    } else if (formats & SDL_GPU_SHADERFORMAT_MSL) {
        info.format = SDL_GPU_SHADERFORMAT_MSL;
        info.code = msl.data();
        info.code_size = msl.size();
        // SPIRV-Cross renames main, which Metal reserves.
        info.entrypoint = "main0";
    } else {
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "The GPU driver takes neither SPIR-V nor MSL shaders.");
        return nullptr;
    }
    info.stage = stage;
    info.num_samplers = samplers;
    info.num_uniform_buffers = 1;
    SDL_GPUShader *shader = SDL_CreateGPUShader(device, &info);
    if (shader == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "A shader was not created: %s", SDL_GetError());
    }
    return shader;
}

std::uint32_t PipelineBits(const RenderStates &states) {
    return static_cast<std::uint32_t>(states.depthTest) |
           (static_cast<std::uint32_t>(states.depthWrite) << 1) |
           (static_cast<std::uint32_t>(states.depthFunction) << 2) |
           (static_cast<std::uint32_t>(states.blend) << 6) |
           (static_cast<std::uint32_t>(states.sourceBlend) << 7) |
           (static_cast<std::uint32_t>(states.destinationBlend) << 11) |
           (static_cast<std::uint32_t>(states.cull) << 15);
}

} // namespace

std::unique_ptr<TlRenderer> TlRenderer::Create(GpuWindow &window) {
    std::unique_ptr<TlRenderer> renderer(new TlRenderer(window));
    SDL_GPUDevice *device = window.Device();
    renderer->m_vertexShader =
        CreateShader(device, SDL_GPU_SHADERSTAGE_VERTEX, 0, TL_VERT_SPIRV, TL_VERT_MSL);
    renderer->m_fragmentShader =
        CreateShader(device, SDL_GPU_SHADERSTAGE_FRAGMENT, 1, TL_FRAG_SPIRV, TL_FRAG_MSL);
    if (renderer->m_vertexShader == nullptr || renderer->m_fragmentShader == nullptr) {
        return nullptr;
    }
    for (auto format : {SDL_GPU_TEXTUREFORMAT_D32_FLOAT,
                        SDL_GPU_TEXTUREFORMAT_D24_UNORM,
                        SDL_GPU_TEXTUREFORMAT_D16_UNORM}) {
        if (SDL_GPUTextureSupportsFormat(device,
                                         format,
                                         SDL_GPU_TEXTURETYPE_2D,
                                         SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET)) {
            renderer->m_depthFormat = format;
            break;
        }
    }
    SDL_GPUTextureCreateInfo depth{};
    depth.type = SDL_GPU_TEXTURETYPE_2D;
    depth.format = renderer->m_depthFormat;
    depth.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
    depth.width = window.FrameWidth();
    depth.height = window.FrameHeight();
    depth.layer_count_or_depth = 1;
    depth.num_levels = 1;
    renderer->m_depth = SDL_CreateGPUTexture(device, &depth);
    if (renderer->m_depth == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "The depth buffer was not created: %s", SDL_GetError());
        return nullptr;
    }
    // Untextured draws sample a white texel; the shader does not read it.
    static constexpr std::uint8_t white[4] = {0xFF, 0xFF, 0xFF, 0xFF};
    TextureHandle handle = renderer->CreateTexture(1, 1, white, false);
    renderer->m_white = handle != 0 ? renderer->m_textures[handle - 1].texture : nullptr;
    return renderer;
}

TlRenderer::~TlRenderer() {
    SDL_GPUDevice *device = m_window.Device();
    for (auto &[key, pipeline] : m_pipelines) {
        SDL_ReleaseGPUGraphicsPipeline(device, pipeline);
    }
    for (auto &[key, sampler] : m_samplers) {
        SDL_ReleaseGPUSampler(device, sampler);
    }
    for (auto &texture : m_textures) {
        SDL_ReleaseGPUTexture(device, texture.texture);
    }
    for (auto *buffer : {m_vertexBuffer, m_indexBuffer}) {
        if (buffer != nullptr) {
            SDL_ReleaseGPUBuffer(device, buffer);
        }
    }
    if (m_transfer != nullptr) {
        SDL_ReleaseGPUTransferBuffer(device, m_transfer);
    }
    if (m_depth != nullptr) {
        SDL_ReleaseGPUTexture(device, m_depth);
    }
    for (auto *shader : {m_vertexShader, m_fragmentShader}) {
        if (shader != nullptr) {
            SDL_ReleaseGPUShader(device, shader);
        }
    }
}

TextureHandle TlRenderer::CreateTexture(std::uint32_t width,
                                        std::uint32_t height,
                                        std::span<const std::uint8_t> rgba,
                                        bool hasAlpha) {
    SDL_GPUDevice *device = m_window.Device();
    std::uint32_t bytes = width * height * 4;
    if (rgba.size() < bytes) {
        return 0;
    }
    SDL_GPUTextureCreateInfo info{};
    info.type = SDL_GPU_TEXTURETYPE_2D;
    info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
    info.width = width;
    info.height = height;
    info.layer_count_or_depth = 1;
    info.num_levels = 1;
    SDL_GPUTexture *texture = SDL_CreateGPUTexture(device, &info);
    if (texture == nullptr) {
        return 0;
    }
    SDL_GPUTransferBufferCreateInfo transferInfo{SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD, bytes, 0};
    SDL_GPUTransferBuffer *transfer = SDL_CreateGPUTransferBuffer(device, &transferInfo);
    void *mapped =
        transfer != nullptr ? SDL_MapGPUTransferBuffer(device, transfer, false) : nullptr;
    if (mapped == nullptr) {
        SDL_ReleaseGPUTexture(device, texture);
        if (transfer != nullptr) {
            SDL_ReleaseGPUTransferBuffer(device, transfer);
        }
        return 0;
    }
    std::memcpy(mapped, rgba.data(), bytes);
    SDL_UnmapGPUTransferBuffer(device, transfer);
    SDL_GPUCommandBuffer *commands = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(commands);
    SDL_GPUTextureTransferInfo source{transfer, 0, width, height};
    SDL_GPUTextureRegion destination{};
    destination.texture = texture;
    destination.w = width;
    destination.h = height;
    destination.d = 1;
    SDL_UploadToGPUTexture(copy, &source, &destination, false);
    SDL_EndGPUCopyPass(copy);
    SDL_SubmitGPUCommandBuffer(commands);
    SDL_ReleaseGPUTransferBuffer(device, transfer);
    m_textures.push_back({texture, hasAlpha});
    return static_cast<TextureHandle>(m_textures.size());
}

void TlRenderer::UpdateTexture(TextureHandle texture,
                               std::uint32_t width,
                               std::uint32_t height,
                               std::span<const std::uint8_t> rgba) {
    if (texture == 0 || texture > m_textures.size()) {
        return;
    }
    bool hasAlpha = m_textures[texture - 1].hasAlpha;
    TextureHandle replacement = CreateTexture(width, height, rgba, hasAlpha);
    if (replacement == 0) {
        return;
    }
    // The new texture takes the place of the old one, and its handle is retired.
    SDL_ReleaseGPUTexture(m_window.Device(), m_textures[texture - 1].texture);
    m_textures[texture - 1] = m_textures[replacement - 1];
    m_textures.pop_back();
}

void TlRenderer::BeginScene(std::uint32_t clearColor) {
    m_clearColor = clearColor;
    m_vertices.clear();
    m_indices.clear();
    m_draws.clear();
}

void TlRenderer::DrawIndexed(std::span<const TlVertex> vertices,
                             std::span<const std::uint16_t> indices) {
    if (vertices.empty() || indices.size() < 3) {
        return;
    }
    m_draws.push_back({m_states,
                       m_texture,
                       static_cast<std::uint32_t>(m_indices.size()),
                       static_cast<std::uint32_t>(indices.size() / 3 * 3),
                       static_cast<std::int32_t>(m_vertices.size())});
    m_vertices.insert(m_vertices.end(), vertices.begin(), vertices.end());
    m_indices.insert(m_indices.end(),
                     indices.begin(),
                     indices.begin() + static_cast<std::ptrdiff_t>(indices.size() / 3 * 3));
}

bool TlRenderer::EnsureBuffers(std::size_t vertexBytes, std::size_t indexBytes) {
    SDL_GPUDevice *device = m_window.Device();
    if (vertexBytes > m_vertexCapacity || indexBytes > m_indexCapacity) {
        // The buffers grow to the next power of two of what a scene needs.
        auto grow = [](std::size_t needed) {
            std::size_t size = 1 << 16;
            while (size < needed) {
                size <<= 1;
            }
            return size;
        };
        for (auto *buffer : {m_vertexBuffer, m_indexBuffer}) {
            if (buffer != nullptr) {
                SDL_ReleaseGPUBuffer(device, buffer);
            }
        }
        if (m_transfer != nullptr) {
            SDL_ReleaseGPUTransferBuffer(device, m_transfer);
        }
        m_vertexCapacity = grow(vertexBytes);
        m_indexCapacity = grow(indexBytes);
        SDL_GPUBufferCreateInfo vertexInfo{
            SDL_GPU_BUFFERUSAGE_VERTEX, static_cast<Uint32>(m_vertexCapacity), 0};
        SDL_GPUBufferCreateInfo indexInfo{
            SDL_GPU_BUFFERUSAGE_INDEX, static_cast<Uint32>(m_indexCapacity), 0};
        SDL_GPUTransferBufferCreateInfo transferInfo{
            SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            static_cast<Uint32>(m_vertexCapacity + m_indexCapacity),
            0};
        m_vertexBuffer = SDL_CreateGPUBuffer(device, &vertexInfo);
        m_indexBuffer = SDL_CreateGPUBuffer(device, &indexInfo);
        m_transfer = SDL_CreateGPUTransferBuffer(device, &transferInfo);
    }
    return m_vertexBuffer != nullptr && m_indexBuffer != nullptr && m_transfer != nullptr;
}

SDL_GPUGraphicsPipeline *TlRenderer::Pipeline(const RenderStates &states) {
    std::uint32_t key = PipelineBits(states);
    if (auto found = m_pipelines.find(key); found != m_pipelines.end()) {
        return found->second;
    }
    SDL_GPUVertexBufferDescription buffer{0, sizeof(TlVertex), SDL_GPU_VERTEXINPUTRATE_VERTEX, 0};
    SDL_GPUVertexAttribute attributes[] = {
        {0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, offsetof(TlVertex, sx)},
        {1, 0, SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM, offsetof(TlVertex, color)},
        {2, 0, SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM, offsetof(TlVertex, specular)},
        {3, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, offsetof(TlVertex, tu)},
    };
    SDL_GPUColorTargetDescription color{};
    color.format = GpuWindow::FrameFormat();
    color.blend_state.enable_blend = states.blend;
    color.blend_state.src_color_blendfactor = Factor(states.sourceBlend);
    color.blend_state.dst_color_blendfactor = Factor(states.destinationBlend);
    color.blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
    color.blend_state.src_alpha_blendfactor = Factor(states.sourceBlend);
    color.blend_state.dst_alpha_blendfactor = Factor(states.destinationBlend);
    color.blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;

    SDL_GPUGraphicsPipelineCreateInfo info{};
    info.vertex_shader = m_vertexShader;
    info.fragment_shader = m_fragmentShader;
    info.vertex_input_state.vertex_buffer_descriptions = &buffer;
    info.vertex_input_state.num_vertex_buffers = 1;
    info.vertex_input_state.vertex_attributes = attributes;
    info.vertex_input_state.num_vertex_attributes = 4;
    info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
    // The screen of Direct3D has y down and the device has y up, so a clockwise triangle on the
    // screen is counter-clockwise for the device.
    info.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
    info.rasterizer_state.cull_mode = states.cull == CullMode::None      ? SDL_GPU_CULLMODE_NONE :
                                      states.cull == CullMode::Clockwise ? SDL_GPU_CULLMODE_FRONT :
                                                                           SDL_GPU_CULLMODE_BACK;
    // The game draws vertices it transformed itself and does not expect the device to clip them
    // against the near and far planes.
    info.rasterizer_state.enable_depth_clip = false;
    info.depth_stencil_state.enable_depth_test = states.depthTest;
    info.depth_stencil_state.enable_depth_write = states.depthTest && states.depthWrite;
    info.depth_stencil_state.compare_op = CompareOp(states.depthFunction);
    info.target_info.color_target_descriptions = &color;
    info.target_info.num_color_targets = 1;
    info.target_info.depth_stencil_format = m_depthFormat;
    info.target_info.has_depth_stencil_target = true;
    SDL_GPUGraphicsPipeline *pipeline = SDL_CreateGPUGraphicsPipeline(m_window.Device(), &info);
    if (pipeline == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "A pipeline was not created: %s", SDL_GetError());
    }
    m_pipelines.emplace(key, pipeline);
    return pipeline;
}

SDL_GPUSampler *TlRenderer::Sampler(const RenderStates &states) {
    std::uint32_t key = static_cast<std::uint32_t>(states.linearFilter) |
                        (static_cast<std::uint32_t>(states.wrapU) << 1) |
                        (static_cast<std::uint32_t>(states.wrapV) << 2);
    if (auto found = m_samplers.find(key); found != m_samplers.end()) {
        return found->second;
    }
    SDL_GPUSamplerCreateInfo info{};
    info.min_filter = states.linearFilter ? SDL_GPU_FILTER_LINEAR : SDL_GPU_FILTER_NEAREST;
    info.mag_filter = info.min_filter;
    info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
    info.address_mode_u =
        states.wrapU ? SDL_GPU_SAMPLERADDRESSMODE_REPEAT : SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    info.address_mode_v =
        states.wrapV ? SDL_GPU_SAMPLERADDRESSMODE_REPEAT : SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    info.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    SDL_GPUSampler *sampler = SDL_CreateGPUSampler(m_window.Device(), &info);
    m_samplers.emplace(key, sampler);
    return sampler;
}

bool TlRenderer::EndScene(const std::filesystem::path *capture) {
    SDL_GPUDevice *device = m_window.Device();
    SDL_GPUCommandBuffer *commands = SDL_AcquireGPUCommandBuffer(device);
    if (commands == nullptr) {
        return false;
    }
    std::size_t vertexBytes = m_vertices.size() * sizeof(TlVertex);
    std::size_t indexBytes = m_indices.size() * sizeof(std::uint16_t);
    bool upload = !m_draws.empty() && EnsureBuffers(vertexBytes, indexBytes);
    if (upload) {
        auto *mapped =
            static_cast<std::uint8_t *>(SDL_MapGPUTransferBuffer(device, m_transfer, true));
        std::memcpy(mapped, m_vertices.data(), vertexBytes);
        std::memcpy(mapped + vertexBytes, m_indices.data(), indexBytes);
        SDL_UnmapGPUTransferBuffer(device, m_transfer);
        SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(commands);
        SDL_GPUTransferBufferLocation from{m_transfer, 0};
        SDL_GPUBufferRegion to{m_vertexBuffer, 0, static_cast<Uint32>(vertexBytes)};
        SDL_UploadToGPUBuffer(copy, &from, &to, true);
        from.offset = static_cast<Uint32>(vertexBytes);
        to = {m_indexBuffer, 0, static_cast<Uint32>(indexBytes)};
        SDL_UploadToGPUBuffer(copy, &from, &to, true);
        SDL_EndGPUCopyPass(copy);
    }

    SDL_GPUColorTargetInfo target{};
    target.texture = m_window.FrameTexture();
    target.clear_color = ColorFromArgb(m_clearColor);
    target.load_op = SDL_GPU_LOADOP_CLEAR;
    target.store_op = SDL_GPU_STOREOP_STORE;
    SDL_GPUDepthStencilTargetInfo depth{};
    depth.texture = m_depth;
    depth.clear_depth = 1.0f;
    depth.load_op = SDL_GPU_LOADOP_CLEAR;
    depth.store_op = SDL_GPU_STOREOP_DONT_CARE;
    depth.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE;
    depth.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;
    SDL_GPURenderPass *pass = SDL_BeginGPURenderPass(commands, &target, 1, &depth);
    if (upload) {
        float viewport[4] = {2.0f / static_cast<float>(m_window.FrameWidth()),
                             2.0f / static_cast<float>(m_window.FrameHeight()),
                             0.5f,
                             0.5f};
        SDL_PushGPUVertexUniformData(commands, 0, viewport, sizeof(viewport));
        SDL_GPUBufferBinding vertices{m_vertexBuffer, 0};
        SDL_GPUBufferBinding indices{m_indexBuffer, 0};
        SDL_BindGPUVertexBuffers(pass, 0, &vertices, 1);
        SDL_BindGPUIndexBuffer(pass, &indices, SDL_GPU_INDEXELEMENTSIZE_16BIT);
        for (const auto &draw : m_draws) {
            SDL_GPUGraphicsPipeline *pipeline = Pipeline(draw.states);
            if (pipeline == nullptr) {
                continue;
            }
            SDL_BindGPUGraphicsPipeline(pass, pipeline);
            bool textured = draw.texture != 0 && draw.texture <= m_textures.size();
            SDL_GPUTextureSamplerBinding binding{
                textured ? m_textures[draw.texture - 1].texture : m_white, Sampler(draw.states)};
            SDL_BindGPUFragmentSamplers(pass, 0, &binding, 1);
            SDL_FColor fog = ColorFromArgb(draw.states.fogColor);
            PixelUniforms uniforms{
                {fog.r, fog.g, fog.b, 1.0f},
                {textured,
                 textured && m_textures[draw.texture - 1].hasAlpha,
                 draw.states.colorKey,
                 draw.states.specular},
                {draw.states.fog,
                 draw.states.alphaTest ? static_cast<std::int32_t>(draw.states.alphaFunction) : 0,
                 0,
                 0},
                {static_cast<float>(draw.states.alphaReference) / 255.0f, 0.0f, 0.0f, 0.0f}};
            SDL_PushGPUFragmentUniformData(commands, 0, &uniforms, sizeof(uniforms));
            SDL_DrawGPUIndexedPrimitives(
                pass, draw.indexCount, 1, draw.firstIndex, draw.vertexOffset, 0);
        }
    }
    SDL_EndGPURenderPass(pass);
    return m_window.Present(commands, capture);
}

} // namespace coastline
