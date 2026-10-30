#include "scenerenderer.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iterator>
#include <map>
#include <string>
#include <variant>

#include <nlohmann/json.hpp>

#include "gpu.h"
#include "shaders/model_frag_msl.h"
#include "shaders/model_frag_spirv.h"
#include "shaders/model_vert_msl.h"
#include "shaders/model_vert_spirv.h"

namespace coastline {

Matrix4 MultiplyMatrices(const Matrix4 &a, const Matrix4 &b) {
    Matrix4 out{};
    for (int column = 0; column < 4; ++column) {
        for (int row = 0; row < 4; ++row) {
            float sum = 0.0f;
            for (int k = 0; k < 4; ++k) {
                sum += a[k * 4 + row] * b[column * 4 + k];
            }
            out[column * 4 + row] = sum;
        }
    }
    return out;
}

namespace {

// The transform of a node of glTF, from its matrix, or its translation, rotation, and scale.
Matrix4 NodeMatrix(const nlohmann::json &node) {
    if (node.contains("matrix")) {
        Matrix4 matrix{};
        for (std::size_t i = 0; i < 16; ++i) {
            matrix[i] = node["matrix"][i].get<float>();
        }
        return matrix;
    }
    auto t = node.value("translation", std::vector<float>{0, 0, 0});
    auto r = node.value("rotation", std::vector<float>{0, 0, 0, 1});
    auto s = node.value("scale", std::vector<float>{1, 1, 1});
    float x = r[0], y = r[1], z = r[2], w = r[3];
    return {(1 - 2 * (y * y + z * z)) * s[0],
            2 * (x * y + z * w) * s[0],
            2 * (x * z - y * w) * s[0],
            0,
            2 * (x * y - z * w) * s[1],
            (1 - 2 * (x * x + z * z)) * s[1],
            2 * (y * z + x * w) * s[1],
            0,
            2 * (x * z + y * w) * s[2],
            2 * (y * z - x * w) * s[2],
            (1 - 2 * (x * x + y * y)) * s[2],
            0,
            t[0],
            t[1],
            t[2],
            1};
}

std::array<float, 3> Transform(const Matrix4 &m, const float *p) {
    return {m[0] * p[0] + m[4] * p[1] + m[8] * p[2] + m[12],
            m[1] * p[0] + m[5] * p[1] + m[9] * p[2] + m[13],
            m[2] * p[0] + m[6] * p[1] + m[10] * p[2] + m[14]};
}

// A vertex as the shaders take it.
struct Vertex {
    float position[3];
    float texCoord[2];
    float normal[3];
    std::uint8_t color[4];
};

// SetMaterialPaletteColors(): the first colour of the palette of a material takes the colour of
// the car in 5 bits a component, and the second a darker shade of it, both with the alpha of the
// entry of the palette at the number of the material.
void PaintPalette(SDL_Palette *palette, std::array<std::uint8_t, 3> paint, int material) {
    if (palette == nullptr || material >= palette->ncolors || palette->ncolors < 2) {
        return;
    }
    // The components of RGBA5551 widen to 8 bits as the import widens them.
    auto light = [&](std::size_t i) { return static_cast<Uint8>((paint[i] >> 3) * 255 / 31); };
    auto dark = [&](std::size_t i) {
        return static_cast<Uint8>(((paint[i] >> 3 & 0x1e) >> 1) * 255 / 31);
    };
    Uint8 alpha = palette->colors[material].a;
    std::array<SDL_Color, 2> colors{
        {{light(0), light(1), light(2), alpha}, {dark(0), dark(1), dark(2), alpha}}};
    SDL_SetPaletteColors(palette, colors.data(), 0, 2);
}

std::array<float, 4> Rgba(std::uint32_t value) {
    return {static_cast<float>(value >> 24 & 0xff) / 255.0f,
            static_cast<float>(value >> 16 & 0xff) / 255.0f,
            static_cast<float>(value >> 8 & 0xff) / 255.0f,
            static_cast<float>(value & 0xff) / 255.0f};
}

// StreamAnimatedTrackTextures(): the frame of an animated texture at a time of the race, the last
// whose time is not past the time within the span of the frames, or none for a still texture.
std::optional<std::size_t>
AnimationFrame(const std::vector<std::pair<std::uint32_t, std::size_t>> &frames,
               std::uint32_t time) {
    if (frames.size() < 2 || frames.back().first <= frames.front().first) {
        return std::nullopt;
    }
    std::uint32_t phase = time % (frames.back().first - frames.front().first);
    std::size_t frame = 1;
    while (frame < frames.size() && !(phase < frames[frame].first)) {
        ++frame;
    }
    return frames[frame - 1].second;
}

// The most poses of vertex animations a draw blends, as many as the vertex shader takes.
constexpr std::size_t MAX_MORPH_POSES = 64;

// The uniforms of the vertex shader, in the layout of std140.
struct VertexUniforms {
    Matrix4 viewProjection;
    Matrix4 model;
    std::array<std::int32_t, 4> morph;
    std::array<float, 4> blend;
    // The axes of the coordinates of a reflection, and in w the first column of its window.
    std::array<float, 4> reflectAcross;
    std::array<float, 4> reflectUp;
    // The first and the second target and the share of the second of each pose.
    std::array<std::array<float, 4>, MAX_MORPH_POSES> poses;
};

// The uniforms of the fragment shader, in the layout of std140.
struct FragmentUniforms {
    float flags[4];
    float lightDirection[4];
    std::uint32_t combine[4];
    float prim[4];
    float env[4];
    float ambient[4];
    float directional[4];
    float texel1Transform[4];
    float shadowCenter[4];
    float shadowForward[4];
    float shadowColor[4];
};

// The cycle type of the RDP in its high other mode word.
constexpr std::uint32_t CYCLE_TYPE_SHIFT = 20;
constexpr std::uint32_t TWO_CYCLE = 1;
// The bit of the geometry mode that generates the texture coordinates from the normals.
constexpr std::uint32_t G_TEXTURE_GEN = 0x40000;

SDL_GPUShader *CreateShader(SDL_GPUDevice *device,
                            SDL_GPUShaderStage stage,
                            std::uint32_t samplers,
                            std::uint32_t storageBuffers,
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
    info.num_storage_buffers = storageBuffers;
    info.num_uniform_buffers = 1;
    SDL_GPUShader *shader = SDL_CreateGPUShader(device, &info);
    if (shader == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "A shader was not created: %s", SDL_GetError());
    }
    return shader;
}

SDL_GPUSamplerAddressMode AddressMode(int wrap) {
    switch (wrap) {
    case 33071:
        return SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    case 33648:
        return SDL_GPU_SAMPLERADDRESSMODE_MIRRORED_REPEAT;
    default:
        return SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    }
}

// Uploads bytes to new buffers and textures through one copy pass.
class Uploader {
public:
    explicit Uploader(SDL_GPUDevice *device) : m_device(device) {
    }

    SDL_GPUBuffer *Buffer(SDL_GPUBufferUsageFlags usage, std::span<const std::uint8_t> bytes) {
        SDL_GPUBufferCreateInfo info{};
        info.usage = usage;
        info.size = static_cast<Uint32>(bytes.size());
        SDL_GPUBuffer *buffer = SDL_CreateGPUBuffer(m_device, &info);
        if (buffer != nullptr) {
            m_buffers.push_back({buffer, std::vector<std::uint8_t>(bytes.begin(), bytes.end())});
        }
        return buffer;
    }

    SDL_GPUTexture *Texture(int width, int height, std::span<const std::uint8_t> rgba) {
        SDL_GPUTextureCreateInfo info{};
        info.type = SDL_GPU_TEXTURETYPE_2D;
        info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
        info.width = static_cast<Uint32>(width);
        info.height = static_cast<Uint32>(height);
        info.layer_count_or_depth = 1;
        info.num_levels = 1;
        SDL_GPUTexture *texture = SDL_CreateGPUTexture(m_device, &info);
        if (texture != nullptr) {
            m_textures.push_back(
                {texture, width, height, std::vector<std::uint8_t>(rgba.begin(), rgba.end())});
        }
        return texture;
    }

    bool Submit() {
        std::size_t total = 0;
        for (const auto &buffer : m_buffers) {
            total += buffer.bytes.size();
        }
        for (const auto &texture : m_textures) {
            total += texture.bytes.size();
        }
        if (total == 0) {
            return true;
        }
        SDL_GPUTransferBufferCreateInfo info{};
        info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        info.size = static_cast<Uint32>(total);
        SDL_GPUTransferBuffer *transfer = SDL_CreateGPUTransferBuffer(m_device, &info);
        if (transfer == nullptr) {
            return false;
        }
        auto *mapped =
            static_cast<std::uint8_t *>(SDL_MapGPUTransferBuffer(m_device, transfer, false));
        std::size_t offset = 0;
        for (const auto &buffer : m_buffers) {
            std::memcpy(mapped + offset, buffer.bytes.data(), buffer.bytes.size());
            offset += buffer.bytes.size();
        }
        for (const auto &texture : m_textures) {
            std::memcpy(mapped + offset, texture.bytes.data(), texture.bytes.size());
            offset += texture.bytes.size();
        }
        SDL_UnmapGPUTransferBuffer(m_device, transfer);
        SDL_GPUCommandBuffer *commands = SDL_AcquireGPUCommandBuffer(m_device);
        SDL_GPUCopyPass *pass = SDL_BeginGPUCopyPass(commands);
        offset = 0;
        for (const auto &buffer : m_buffers) {
            SDL_GPUTransferBufferLocation source{transfer, static_cast<Uint32>(offset)};
            SDL_GPUBufferRegion target{buffer.buffer, 0, static_cast<Uint32>(buffer.bytes.size())};
            SDL_UploadToGPUBuffer(pass, &source, &target, false);
            offset += buffer.bytes.size();
        }
        for (const auto &texture : m_textures) {
            SDL_GPUTextureTransferInfo source{};
            source.transfer_buffer = transfer;
            source.offset = static_cast<Uint32>(offset);
            SDL_GPUTextureRegion target{};
            target.texture = texture.texture;
            target.w = static_cast<Uint32>(texture.width);
            target.h = static_cast<Uint32>(texture.height);
            target.d = 1;
            SDL_UploadToGPUTexture(pass, &source, &target, false);
            offset += texture.bytes.size();
        }
        SDL_EndGPUCopyPass(pass);
        bool submitted = SDL_SubmitGPUCommandBuffer(commands);
        SDL_ReleaseGPUTransferBuffer(m_device, transfer);
        return submitted;
    }

private:
    struct PendingBuffer {
        SDL_GPUBuffer *buffer;
        std::vector<std::uint8_t> bytes;
    };
    struct PendingTexture {
        SDL_GPUTexture *texture;
        int width;
        int height;
        std::vector<std::uint8_t> bytes;
    };
    SDL_GPUDevice *m_device;
    std::vector<PendingBuffer> m_buffers;
    std::vector<PendingTexture> m_textures;
};

// The bytes of an accessor of the buffer, by element. A sparse accessor gives its indices and
// values, and reads as zeros elsewhere.
struct AccessorView {
    const std::uint8_t *data = nullptr;
    std::size_t count = 0;
    std::size_t stride = 0;
};

std::optional<AccessorView> ViewOf(const nlohmann::json &document,
                                   std::span<const std::uint8_t> buffer,
                                   std::size_t view,
                                   std::size_t offset,
                                   std::size_t count,
                                   std::size_t elementSize) {
    const auto &bufferView = document["bufferViews"].at(view);
    AccessorView result;
    result.count = count;
    result.stride = bufferView.value("byteStride", elementSize);
    std::size_t start = bufferView.value("byteOffset", std::size_t{0}) + offset;
    if (count == 0 || start > buffer.size() ||
        (count - 1) * result.stride + elementSize > buffer.size() - start) {
        return std::nullopt;
    }
    result.data = buffer.data() + start;
    return result;
}

std::optional<AccessorView> Accessor(const nlohmann::json &document,
                                     std::span<const std::uint8_t> buffer,
                                     std::size_t index,
                                     std::size_t elementSize) {
    const auto &accessor = document["accessors"].at(index);
    if (!accessor.contains("bufferView")) {
        return std::nullopt;
    }
    return ViewOf(document,
                  buffer,
                  accessor["bufferView"].get<std::size_t>(),
                  accessor.value("byteOffset", std::size_t{0}),
                  accessor["count"].get<std::size_t>(),
                  elementSize);
}

// The three floats of each element of a VEC3 accessor of floats, dense or sparse.
std::vector<float> Vec3Floats(const nlohmann::json &document,
                              std::span<const std::uint8_t> buffer,
                              std::size_t index) {
    const auto &accessor = document["accessors"].at(index);
    std::size_t count = accessor["count"].get<std::size_t>();
    std::vector<float> values(count * 3, 0.0f);
    if (auto dense = Accessor(document, buffer, index, 12)) {
        for (std::size_t i = 0; i < count; ++i) {
            std::memcpy(&values[i * 3], dense->data + i * dense->stride, 12);
        }
    }
    if (accessor.contains("sparse")) {
        const auto &sparse = accessor["sparse"];
        std::size_t moved = sparse["count"].get<std::size_t>();
        auto indices = ViewOf(document,
                              buffer,
                              sparse["indices"]["bufferView"].get<std::size_t>(),
                              sparse["indices"].value("byteOffset", std::size_t{0}),
                              moved,
                              4);
        auto changed = ViewOf(document,
                              buffer,
                              sparse["values"]["bufferView"].get<std::size_t>(),
                              sparse["values"].value("byteOffset", std::size_t{0}),
                              moved,
                              12);
        if (indices && changed) {
            for (std::size_t i = 0; i < moved; ++i) {
                std::uint32_t at;
                std::memcpy(&at, indices->data + i * indices->stride, 4);
                if (at < count) {
                    std::memcpy(&values[at * 3], changed->data + i * changed->stride, 12);
                }
            }
        }
    }
    return values;
}

// A primitive of a mesh on the GPU, and how its material draws it.
struct Primitive {
    SDL_GPUBuffer *vertices = nullptr;
    SDL_GPUBuffer *indices = nullptr;
    std::uint32_t indexCount = 0;
    std::optional<std::size_t> texture;
    float alphaCut = -1.0f;
    bool blend = false;
    bool twoSided = false;
    bool lit = false;
    // The combiner of the RDP, its colours, and the image of its second texel, where recorded.
    std::optional<std::array<std::uint32_t, 2>> combine;
    bool twoCycle = false;
    // Whether the texture coordinates come from the normals, as G_TEXTURE_GEN of the RSP.
    bool textureGen = false;
    std::array<float, 4> prim{1, 1, 1, 1};
    std::array<float, 4> env{1, 1, 1, 1};
    std::optional<std::size_t> texel1;
    // The frames of an animated texture, each its time in milliseconds and its image.
    std::vector<std::pair<std::uint32_t, std::size_t>> frames;
};

// A mesh: its primitives, and the displacements of its morph targets.
struct Mesh {
    std::vector<Primitive> primitives;
    SDL_GPUBuffer *targets = nullptr;
    int vertexCount = 0;
};

// A mesh placed by a node of the scene, and the lights a placed model of a track has of its own.
struct Placed {
    std::size_t mesh;
    Matrix4 matrix;
    std::optional<Lighting> lighting;
    int instance;        // The instance of the track the node draws, or -1.
    std::uint32_t flags; // The TRACK_INSTANCE_ flags of the instance, or 0.
};

// The lights of a clear day as the race sets them up: a directional light of 255, 255, 204 from
// 15, 10, 20 and an ambient light of 102, 102, 119. A placed model with a ramped light has no
// directional light, and an ambient light of a quarter, a half, three quarters, or all of the
// directional colour by the low bits of its flags.
constexpr std::array<int, 3> RACE_DIRECTIONAL{255, 255, 204};
constexpr std::array<int, 3> RACE_AMBIENT{102, 102, 119};
// The direction toward the light, 15, 10, 20 of the console, which is z up, normalised in the y
// up space of glTF.
constexpr std::array<float, 3> RACE_LIGHT_DIRECTION{0.5571f, 0.7428f, -0.3714f};
constexpr std::uint32_t TRACK_INSTANCE_LIGHT_RAMP = 0x0003;
constexpr std::uint32_t TRACK_INSTANCE_RAMPED = 0x0400;

Lighting TrackInstanceLighting(std::uint32_t flags) {
    Lighting lighting;
    lighting.direction = RACE_LIGHT_DIRECTION;
    for (std::size_t i = 0; i < 3; ++i) {
        int full = RACE_DIRECTIONAL[i];
        if ((flags & TRACK_INSTANCE_RAMPED) != 0) {
            std::array<int, 4> ramp{full >> 2, full >> 1, (full >> 1) + (full >> 2), full};
            lighting.ambient[i] =
                static_cast<float>(ramp[flags & TRACK_INSTANCE_LIGHT_RAMP]) / 255.0f;
            lighting.directional[i] = 0.0f;
        } else {
            lighting.ambient[i] = static_cast<float>(RACE_AMBIENT[i]) / 255.0f;
            lighting.directional[i] = static_cast<float>(full) / 255.0f;
        }
    }
    return lighting;
}

// A key of a pipeline: blending, both sides, and the depth test and write.
enum class DepthMode { None, Test, TestAndWrite };

struct PipelineKey {
    bool blend;
    bool twoSided;
    DepthMode depth;
    auto operator<=>(const PipelineKey &) const = default;
};

struct ModelDraw {
    const RenderModel *model;
    Matrix4 world;
    Matrix4 viewProjection;
    Lighting lighting;
    std::vector<MorphPose> poses;
    bool depth;
    std::optional<SDL_Rect> scissor;
    std::optional<Reflection> reflection;
    std::optional<CarShadow> shadow;
};

struct QuadDraw {
    std::size_t firstVertex;
    std::size_t quadCount;
    FlatMaterial material;
    Matrix4 projection;
    bool depthTest;
};

} // namespace

struct RenderModel::State {
    SDL_GPUDevice *device = nullptr;
    std::vector<SDL_GPUTexture *> images;
    std::vector<SDL_GPUSampler *> samplers;
    std::vector<std::pair<std::size_t, std::size_t>> textures; // Image and sampler.
    std::vector<Mesh> meshes;
    std::vector<Placed> placed;
    std::vector<VertexAnimationInfo> animations;
    std::array<float, 3> centre{};
    float radius = 1.0f;

    ~State() {
        for (auto &mesh : meshes) {
            for (auto &primitive : mesh.primitives) {
                SDL_ReleaseGPUBuffer(device, primitive.vertices);
                SDL_ReleaseGPUBuffer(device, primitive.indices);
            }
            if (mesh.targets != nullptr) {
                SDL_ReleaseGPUBuffer(device, mesh.targets);
            }
        }
        for (auto *image : images) {
            SDL_ReleaseGPUTexture(device, image);
        }
        for (auto *sampler : samplers) {
            SDL_ReleaseGPUSampler(device, sampler);
        }
    }
};

struct SceneRenderer::State {
    // The time of the frame the animated textures show, in milliseconds.
    std::uint32_t textureTime = 0;
    // The TRACK_INSTANCE_ flags of the instances the frame does not draw.
    std::uint32_t hiddenFlags = 0;
    GpuWindow *window = nullptr;
    SDL_GPUDevice *device = nullptr;
    SDL_GPUShader *vertexShader = nullptr;
    SDL_GPUShader *fragmentShader = nullptr;
    std::map<PipelineKey, SDL_GPUGraphicsPipeline *> pipelines;
    SDL_GPUTexture *depth = nullptr;
    SDL_GPUTextureFormat depthFormat = SDL_GPU_TEXTUREFORMAT_D16_UNORM;
    SDL_GPUTexture *white = nullptr;
    SDL_GPUSampler *plainSampler = nullptr;
    SDL_GPUBuffer *noTargets = nullptr;
    // The vertices of the flat draws, and the indices of their quads, which grow as they need.
    SDL_GPUBuffer *flatVertices = nullptr;
    std::size_t flatVertexCapacity = 0;
    SDL_GPUBuffer *quadIndices = nullptr;
    std::size_t quadCapacity = 0;
    // The draws of the frame, in order, and the vertices of its flat draws.
    std::array<float, 4> clear{};
    std::vector<std::variant<ModelDraw, QuadDraw>> draws;
    std::vector<Vertex> flat;

    ~State() {
        if (device == nullptr) {
            return;
        }
        for (auto &[key, pipeline] : pipelines) {
            SDL_ReleaseGPUGraphicsPipeline(device, pipeline);
        }
        for (auto *shader : {vertexShader, fragmentShader}) {
            if (shader != nullptr) {
                SDL_ReleaseGPUShader(device, shader);
            }
        }
        for (auto *texture : {depth, white}) {
            if (texture != nullptr) {
                SDL_ReleaseGPUTexture(device, texture);
            }
        }
        if (plainSampler != nullptr) {
            SDL_ReleaseGPUSampler(device, plainSampler);
        }
        for (auto *buffer : {noTargets, flatVertices, quadIndices}) {
            if (buffer != nullptr) {
                SDL_ReleaseGPUBuffer(device, buffer);
            }
        }
    }

    SDL_GPUGraphicsPipeline *Pipeline(const PipelineKey &key) {
        if (auto found = pipelines.find(key); found != pipelines.end()) {
            return found->second;
        }
        SDL_GPUVertexBufferDescription binding{};
        binding.slot = 0;
        binding.pitch = sizeof(Vertex);
        binding.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
        std::array<SDL_GPUVertexAttribute, 4> attributes{{
            {0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, offsetof(Vertex, position)},
            {1, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, offsetof(Vertex, texCoord)},
            {2, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, offsetof(Vertex, normal)},
            {3, 0, SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM, offsetof(Vertex, color)},
        }};
        SDL_GPUColorTargetDescription target{};
        target.format = GpuWindow::FrameFormat();
        if (key.blend) {
            target.blend_state.enable_blend = true;
            target.blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
            target.blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
            target.blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
            target.blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
            target.blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
            target.blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
        }
        SDL_GPUGraphicsPipelineCreateInfo info{};
        info.vertex_shader = vertexShader;
        info.fragment_shader = fragmentShader;
        info.vertex_input_state.vertex_buffer_descriptions = &binding;
        info.vertex_input_state.num_vertex_buffers = 1;
        info.vertex_input_state.vertex_attributes = attributes.data();
        info.vertex_input_state.num_vertex_attributes = attributes.size();
        info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
        info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
        info.rasterizer_state.cull_mode =
            key.twoSided ? SDL_GPU_CULLMODE_NONE : SDL_GPU_CULLMODE_BACK;
        info.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
        info.depth_stencil_state.enable_depth_test = key.depth != DepthMode::None;
        info.depth_stencil_state.enable_depth_write = key.depth == DepthMode::TestAndWrite;
        info.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL;
        info.target_info.color_target_descriptions = &target;
        info.target_info.num_color_targets = 1;
        info.target_info.depth_stencil_format = depthFormat;
        info.target_info.has_depth_stencil_target = true;
        SDL_GPUGraphicsPipeline *pipeline = SDL_CreateGPUGraphicsPipeline(device, &info);
        if (pipeline == nullptr) {
            SDL_LogError(SDL_LOG_CATEGORY_GPU, "A pipeline was not created: %s", SDL_GetError());
        }
        pipelines[key] = pipeline;
        return pipeline;
    }

    // A buffer of at least a size, made again when it is too small.
    bool Reserve(SDL_GPUBuffer *&buffer,
                 std::size_t &capacity,
                 std::size_t bytes,
                 SDL_GPUBufferUsageFlags usage) {
        if (buffer != nullptr && capacity >= bytes) {
            return true;
        }
        if (buffer != nullptr) {
            SDL_ReleaseGPUBuffer(device, buffer);
        }
        capacity = std::max(bytes, capacity * 2);
        SDL_GPUBufferCreateInfo info{};
        info.usage = usage;
        info.size = static_cast<Uint32>(capacity);
        buffer = SDL_CreateGPUBuffer(device, &info);
        return buffer != nullptr;
    }
};

RenderModel::RenderModel(std::unique_ptr<State> state) : m_state(std::move(state)) {
}

RenderModel::~RenderModel() = default;

const std::vector<VertexAnimationInfo> &RenderModel::Animations() const {
    return m_state->animations;
}

std::array<float, 3> RenderModel::Centre() const {
    return m_state->centre;
}

float RenderModel::Radius() const {
    return m_state->radius;
}

std::optional<MorphPose> PoseAtTime(const VertexAnimationInfo &animation, float time) {
    if (animation.keyTimes.size() < 2) {
        return std::nullopt;
    }
    float t = std::clamp(time, animation.keyTimes.front(), animation.keyTimes.back());
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

std::unique_ptr<RenderModel> RenderModel::Load(SceneRenderer &renderer,
                                               const std::filesystem::path &path,
                                               const ModelOptions &options) {
    const auto &node = options.node;
    const auto &paint = options.paint;
    const auto &parts = options.parts;
    auto state = std::make_unique<State>();
    state->device = renderer.m_state->device;
    SDL_GPUDevice *device = state->device;
    nlohmann::json document;
    std::vector<std::uint8_t> buffer;
    try {
        std::ifstream file(path);
        document = nlohmann::json::parse(file);
        std::ifstream data(path.parent_path() / document["buffers"][0]["uri"].get<std::string>(),
                           std::ios::binary);
        buffer.assign(std::istreambuf_iterator<char>(data), {});
    } catch (const nlohmann::json::exception &error) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "%s is not a glTF document: %s",
                     path.string().c_str(),
                     error.what());
        return nullptr;
    }

    // The images of another document of the model, one for one, when it has as many.
    nlohmann::json images = document.value("images", nlohmann::json::array());
    std::filesystem::path imageDirectory = path.parent_path();
    if (options.images) {
        std::ifstream other(*options.images);
        auto replacement = nlohmann::json::parse(other, nullptr, false);
        if (!replacement.is_discarded() &&
            replacement.value("images", nlohmann::json::array()).size() == images.size()) {
            images = replacement["images"];
            imageDirectory = options.images->parent_path();
        } else {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                        "%s does not have the images of %s.",
                        options.images->string().c_str(),
                        path.string().c_str());
        }
    }

    Uploader upload(device);
    for (const auto &image : images) {
        auto file = imageDirectory / image["uri"].get<std::string>();
        SDL_Surface *loaded = SDL_LoadPNG(file.string().c_str());
        int material = image.value("extras", nlohmann::json::object())
                           .value("coastlinePaint", nlohmann::json::object())
                           .value("material", -1);
        if (loaded != nullptr && paint && material >= 0) {
            PaintPalette(SDL_GetSurfacePalette(loaded), *paint, material);
        }
        SDL_Surface *rgba = loaded ? SDL_ConvertSurface(loaded, SDL_PIXELFORMAT_RGBA32) : nullptr;
        SDL_DestroySurface(loaded);
        if (rgba == nullptr) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s cannot be read.", file.string().c_str());
            return nullptr;
        }
        std::vector<std::uint8_t> texels;
        for (int y = 0; y < rgba->h; ++y) {
            auto *row = static_cast<std::uint8_t *>(rgba->pixels) + y * rgba->pitch;
            texels.insert(texels.end(), row, row + rgba->w * 4);
        }
        state->images.push_back(upload.Texture(rgba->w, rgba->h, texels));
        SDL_DestroySurface(rgba);
    }
    for (const auto &sampler : document.value("samplers", nlohmann::json::array())) {
        SDL_GPUSamplerCreateInfo info{};
        info.min_filter = SDL_GPU_FILTER_LINEAR;
        info.mag_filter = SDL_GPU_FILTER_LINEAR;
        info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
        info.address_mode_u = AddressMode(sampler.value("wrapS", 10497));
        info.address_mode_v = AddressMode(sampler.value("wrapT", 10497));
        info.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
        state->samplers.push_back(SDL_CreateGPUSampler(device, &info));
    }
    for (const auto &texture : document.value("textures", nlohmann::json::array())) {
        state->textures.push_back(
            {texture["source"].get<std::size_t>(), texture["sampler"].get<std::size_t>()});
    }

    // The meshes, with the vertices of each primitive in the layout of the shaders, and the
    // displacements of the morph targets of the first primitive, which every primitive shares.
    std::vector<std::vector<std::array<float, 3>>> meshPositions;
    for (const auto &mesh : document["meshes"]) {
        Mesh &drawn = state->meshes.emplace_back();
        auto &positions = meshPositions.emplace_back();
        for (const auto &primitive : mesh["primitives"]) {
            const auto &attributes = primitive["attributes"];
            auto position = Accessor(document, buffer, attributes["POSITION"], 12);
            auto texCoord = attributes.contains("TEXCOORD_0") ?
                                Accessor(document, buffer, attributes["TEXCOORD_0"], 8) :
                                std::nullopt;
            auto normal = attributes.contains("NORMAL") ?
                              Accessor(document, buffer, attributes["NORMAL"], 12) :
                              std::nullopt;
            auto color = attributes.contains("COLOR_0") ?
                             Accessor(document, buffer, attributes["COLOR_0"], 4) :
                             std::nullopt;
            auto indices = Accessor(document, buffer, primitive["indices"], 4);
            if (!position || !indices) {
                continue;
            }
            std::vector<Vertex> vertices(position->count);
            for (std::size_t i = 0; i < vertices.size(); ++i) {
                Vertex &v = vertices[i];
                std::memcpy(v.position, position->data + i * position->stride, 12);
                positions.push_back({v.position[0], v.position[1], v.position[2]});
                if (texCoord && i < texCoord->count) {
                    std::memcpy(v.texCoord, texCoord->data + i * texCoord->stride, 8);
                }
                if (normal && i < normal->count) {
                    std::memcpy(v.normal, normal->data + i * normal->stride, 12);
                }
                std::array<std::uint8_t, 4> rgba{255, 255, 255, 255};
                if (color && i < color->count) {
                    std::memcpy(rgba.data(), color->data + i * color->stride, 4);
                }
                std::memcpy(v.color, rgba.data(), 4);
            }
            std::vector<std::uint32_t> list(indices->count);
            for (std::size_t i = 0; i < list.size(); ++i) {
                std::memcpy(&list[i], indices->data + i * indices->stride, 4);
            }
            if (drawn.targets == nullptr && primitive.contains("targets")) {
                // For each target, the position and then the normal of each vertex, as vec4.
                std::vector<float> displacements;
                for (const auto &target : primitive["targets"]) {
                    auto moved =
                        Vec3Floats(document, buffer, target["POSITION"].get<std::size_t>());
                    std::vector<float> turned(moved.size(), 0.0f);
                    if (target.contains("NORMAL")) {
                        turned = Vec3Floats(document, buffer, target["NORMAL"].get<std::size_t>());
                    }
                    for (std::size_t i = 0; i < vertices.size(); ++i) {
                        for (const auto *values : {&moved, &turned}) {
                            for (std::size_t axis = 0; axis < 3; ++axis) {
                                displacements.push_back(
                                    i * 3 + axis < values->size() ? (*values)[i * 3 + axis] : 0.0f);
                            }
                            displacements.push_back(0.0f);
                        }
                    }
                }
                drawn.vertexCount = static_cast<int>(vertices.size());
                drawn.targets =
                    upload.Buffer(SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ,
                                  {reinterpret_cast<const std::uint8_t *>(displacements.data()),
                                   displacements.size() * sizeof(float)});
            }
            Primitive part;
            part.vertices = upload.Buffer(SDL_GPU_BUFFERUSAGE_VERTEX,
                                          {reinterpret_cast<const std::uint8_t *>(vertices.data()),
                                           vertices.size() * sizeof(Vertex)});
            part.indices = upload.Buffer(
                SDL_GPU_BUFFERUSAGE_INDEX,
                {reinterpret_cast<const std::uint8_t *>(list.data()), list.size() * 4});
            part.indexCount = static_cast<std::uint32_t>(list.size());
            part.lit = normal.has_value();
            if (primitive.contains("material")) {
                const auto &material =
                    document["materials"][primitive["material"].get<std::size_t>()];
                const auto &pbr = material.value("pbrMetallicRoughness", nlohmann::json::object());
                if (pbr.contains("baseColorTexture")) {
                    part.texture = pbr["baseColorTexture"]["index"].get<std::size_t>();
                }
                auto mode = material.value("alphaMode", std::string("OPAQUE"));
                part.blend = mode == "BLEND";
                part.alphaCut = mode == "MASK" ? 0.5f : -1.0f;
                part.twoSided = material.value("doubleSided", false);
                if (material.contains("extras") && material["extras"].contains("coastlineRdp")) {
                    const auto &rdp = material["extras"]["coastlineRdp"];
                    std::array<std::uint32_t, 2> words{rdp["combine"][0].get<std::uint32_t>(),
                                                       rdp["combine"][1].get<std::uint32_t>()};
                    if (words[0] != 0 || words[1] != 0) {
                        part.combine = words;
                    }
                    part.twoCycle =
                        (rdp.value("otherModeH", 0u) >> CYCLE_TYPE_SHIFT & 3) == TWO_CYCLE;
                    part.textureGen = (rdp.value("geometryMode", 0u) & G_TEXTURE_GEN) != 0;
                    part.prim = Rgba(rdp["primColor"].get<std::uint32_t>());
                    part.env = Rgba(rdp["envColor"].get<std::uint32_t>());
                    if (rdp.contains("texel1Image")) {
                        part.texel1 = rdp["texel1Image"].get<std::size_t>();
                    }
                }
                // The frames of an animated texture of a track. The PC release does not swap the
                // textures of the snow, which have no time.
                if (material.contains("extras") &&
                    material["extras"].contains("coastlineAnimation")) {
                    const auto &animation = material["extras"]["coastlineAnimation"];
                    if (!animation.value("snow", false)) {
                        for (const auto &frame : animation["frames"]) {
                            if (frame["time"].is_number()) {
                                part.frames.emplace_back(frame["time"].get<std::uint32_t>(),
                                                         frame["image"].get<std::size_t>());
                            }
                        }
                    }
                }
            }
            drawn.primitives.push_back(part);
        }
    }
    if (!upload.Submit()) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "The model was not uploaded: %s", SDL_GetError());
        return nullptr;
    }

    for (const auto &animation : document.value("animations", nlohmann::json::array())) {
        if (!animation.contains("extras") ||
            !animation["extras"].contains("coastlineVertexAnimation")) {
            continue;
        }
        const auto &game = animation["extras"]["coastlineVertexAnimation"];
        VertexAnimationInfo info;
        info.flags = game.value("flags", std::uint16_t{0});
        info.startTime = game.value("startTime", 0.0f);
        info.endTime = game.value("endTime", 0.0f);
        info.time = game.value("time", 0.0f);
        info.firstTarget = game.value("firstTarget", 0);
        std::size_t input = animation["samplers"][0]["input"].get<std::size_t>();
        if (auto times = Accessor(document, buffer, input, 4)) {
            for (std::size_t k = 0; k < times->count; ++k) {
                float time;
                std::memcpy(&time, times->data + k * times->stride, 4);
                info.keyTimes.push_back(time);
            }
        }
        state->animations.push_back(std::move(info));
    }

    // The nodes of the first scene and their places, and the bounds of what they draw.
    std::array<float, 3> low{INFINITY, INFINITY, INFINITY};
    std::array<float, 3> high{-INFINITY, -INFINITY, -INFINITY};
    auto visit = [&](auto &self, std::size_t index, const Matrix4 &parent) -> void {
        const auto &node = document["nodes"].at(index);
        Matrix4 matrix = MultiplyMatrices(parent, NodeMatrix(node));
        if (node.contains("mesh") && (!parts || parts(node.value("name", "")))) {
            std::size_t mesh = node["mesh"].get<std::size_t>();
            std::optional<Lighting> lighting;
            int instance = -1;
            std::uint32_t flags = 0;
            if (node.contains("extras") && node["extras"].contains("coastlineInstance")) {
                const auto &extras = node["extras"]["coastlineInstance"];
                flags = extras["flags"].get<std::uint32_t>();
                lighting = TrackInstanceLighting(flags);
                instance = extras.value("index", -1);
            }
            state->placed.push_back({mesh, matrix, lighting, instance, flags});
            for (const auto &p : meshPositions[mesh]) {
                auto world = Transform(matrix, p.data());
                for (std::size_t axis = 0; axis < 3; ++axis) {
                    low[axis] = std::min(low[axis], world[axis]);
                    high[axis] = std::max(high[axis], world[axis]);
                }
            }
        }
        for (const auto &child : node.value("children", nlohmann::json::array())) {
            self(self, child.get<std::size_t>(), matrix);
        }
    };
    if (node) {
        const auto &nodes = document["nodes"];
        for (std::size_t index = 0; index < nodes.size(); ++index) {
            if (nodes[index].value("name", "") == *node) {
                // The node draws at its origin, for the caller to place it: its translation moves
                // every placement under it, and leaves them.
                std::size_t before = state->placed.size();
                visit(visit, index, IDENTITY_MATRIX);
                Matrix4 own = NodeMatrix(nodes[index]);
                for (std::size_t p = before; p < state->placed.size(); ++p) {
                    for (std::size_t axis = 0; axis < 3; ++axis) {
                        state->placed[p].matrix[12 + axis] -= own[12 + axis];
                    }
                }
            }
        }
    } else {
        const auto &scene = document["scenes"][document.value("scene", 0)];
        for (const auto &root : scene["nodes"]) {
            visit(visit, root.get<std::size_t>(), IDENTITY_MATRIX);
        }
    }
    if (state->placed.empty()) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s draws nothing.", path.string().c_str());
        return nullptr;
    }
    for (std::size_t axis = 0; axis < 3; ++axis) {
        state->centre[axis] = (low[axis] + high[axis]) / 2;
        state->radius = std::max(state->radius, (high[axis] - low[axis]) / 2);
    }
    return std::unique_ptr<RenderModel>(new RenderModel(std::move(state)));
}

RenderTexture::RenderTexture(
    SDL_GPUDevice *device, SDL_GPUTexture *texture, SDL_GPUSampler *sampler, int width, int height)
    : m_device(device), m_texture(texture), m_sampler(sampler), m_width(width), m_height(height) {
}

RenderTexture::~RenderTexture() {
    SDL_ReleaseGPUTexture(m_device, m_texture);
    SDL_ReleaseGPUSampler(m_device, m_sampler);
}

int RenderTexture::Width() const {
    return m_width;
}

int RenderTexture::Height() const {
    return m_height;
}

std::unique_ptr<RenderTexture> RenderTexture::Load(SceneRenderer &renderer,
                                                   const std::filesystem::path &path,
                                                   bool linear,
                                                   bool repeat) {
    SDL_Surface *loaded = SDL_LoadPNG(path.string().c_str());
    SDL_Surface *rgba = loaded ? SDL_ConvertSurface(loaded, SDL_PIXELFORMAT_RGBA32) : nullptr;
    SDL_DestroySurface(loaded);
    if (rgba == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s cannot be read.", path.string().c_str());
        return nullptr;
    }
    std::vector<std::uint8_t> texels;
    for (int y = 0; y < rgba->h; ++y) {
        auto *row = static_cast<std::uint8_t *>(rgba->pixels) + y * rgba->pitch;
        texels.insert(texels.end(), row, row + rgba->w * 4);
    }
    int width = rgba->w;
    int height = rgba->h;
    SDL_DestroySurface(rgba);
    auto texture = FromTexels(renderer, width, height, texels, linear, repeat);
    if (!texture) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "%s was not uploaded.", path.string().c_str());
    }
    return texture;
}

std::unique_ptr<RenderTexture> RenderTexture::FromTexels(SceneRenderer &renderer,
                                                         int width,
                                                         int height,
                                                         std::span<const std::uint8_t> rgba,
                                                         bool linear,
                                                         bool repeat) {
    SDL_GPUDevice *device = renderer.m_state->device;
    Uploader upload(device);
    SDL_GPUTexture *texture = upload.Texture(width, height, rgba);
    if (texture == nullptr || !upload.Submit()) {
        return nullptr;
    }
    SDL_GPUSamplerCreateInfo info{};
    info.min_filter = linear ? SDL_GPU_FILTER_LINEAR : SDL_GPU_FILTER_NEAREST;
    info.mag_filter = info.min_filter;
    info.address_mode_u =
        repeat ? SDL_GPU_SAMPLERADDRESSMODE_REPEAT : SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    info.address_mode_v = info.address_mode_u;
    info.address_mode_w = info.address_mode_u;
    return std::unique_ptr<RenderTexture>(
        new RenderTexture(device, texture, SDL_CreateGPUSampler(device, &info), width, height));
}

SceneRenderer::SceneRenderer(std::unique_ptr<State> state) : m_state(std::move(state)) {
}

SceneRenderer::~SceneRenderer() = default;

GpuWindow &SceneRenderer::Window() {
    return *m_state->window;
}

std::unique_ptr<SceneRenderer> SceneRenderer::Create(GpuWindow &window) {
    auto state = std::make_unique<State>();
    state->window = &window;
    state->device = window.Device();
    SDL_GPUDevice *device = state->device;
    state->vertexShader =
        CreateShader(device, SDL_GPU_SHADERSTAGE_VERTEX, 0, 1, MODEL_VERT_SPIRV, MODEL_VERT_MSL);
    state->fragmentShader =
        CreateShader(device, SDL_GPU_SHADERSTAGE_FRAGMENT, 3, 0, MODEL_FRAG_SPIRV, MODEL_FRAG_MSL);
    if (state->vertexShader == nullptr || state->fragmentShader == nullptr) {
        return nullptr;
    }
    for (auto format : {SDL_GPU_TEXTUREFORMAT_D32_FLOAT, SDL_GPU_TEXTUREFORMAT_D24_UNORM}) {
        if (SDL_GPUTextureSupportsFormat(device,
                                         format,
                                         SDL_GPU_TEXTURETYPE_2D,
                                         SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET)) {
            state->depthFormat = format;
            break;
        }
    }
    SDL_GPUTextureCreateInfo depth{};
    depth.type = SDL_GPU_TEXTURETYPE_2D;
    depth.format = state->depthFormat;
    depth.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
    depth.width = window.FrameWidth();
    depth.height = window.FrameHeight();
    depth.layer_count_or_depth = 1;
    depth.num_levels = 1;
    state->depth = SDL_CreateGPUTexture(device, &depth);
    Uploader upload(device);
    constexpr std::array<std::uint8_t, 4> WHITE{255, 255, 255, 255};
    state->white = upload.Texture(1, 1, WHITE);
    constexpr std::array<float, 8> NONE{};
    state->noTargets =
        upload.Buffer(SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ,
                      {reinterpret_cast<const std::uint8_t *>(NONE.data()), sizeof(NONE)});
    SDL_GPUSamplerCreateInfo plain{};
    state->plainSampler = SDL_CreateGPUSampler(device, &plain);
    if (state->depth == nullptr || !upload.Submit()) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "The renderer was not created: %s", SDL_GetError());
        return nullptr;
    }
    return std::unique_ptr<SceneRenderer>(new SceneRenderer(std::move(state)));
}

void SceneRenderer::Begin(std::array<float, 4> clear) {
    m_state->clear = clear;
    m_state->textureTime = 0;
    m_state->hiddenFlags = 0;
    m_state->draws.clear();
    m_state->flat.clear();
}

void SceneRenderer::SetTextureTime(std::uint32_t milliseconds) {
    m_state->textureTime = milliseconds;
}

void SceneRenderer::SetHiddenInstanceFlags(std::uint32_t flags) {
    m_state->hiddenFlags = flags;
}

void SceneRenderer::DrawModel(const RenderModel &model,
                              const Matrix4 &world,
                              const Matrix4 &viewProjection,
                              const Lighting &lighting,
                              std::optional<MorphPose> pose,
                              bool depth,
                              std::optional<SDL_Rect> scissor) {
    std::vector<MorphPose> poses;
    if (pose) {
        poses.push_back(*pose);
    }
    DrawModel(model, world, viewProjection, lighting, std::move(poses), depth, scissor);
}

void SceneRenderer::DrawModel(const RenderModel &model,
                              const Matrix4 &world,
                              const Matrix4 &viewProjection,
                              const Lighting &lighting,
                              std::vector<MorphPose> poses,
                              bool depth,
                              std::optional<SDL_Rect> scissor) {
    if (poses.size() > MAX_MORPH_POSES) {
        poses.resize(MAX_MORPH_POSES);
    }
    m_state->draws.emplace_back(ModelDraw{&model,
                                          world,
                                          viewProjection,
                                          lighting,
                                          std::move(poses),
                                          depth,
                                          scissor,
                                          std::nullopt,
                                          std::nullopt});
}

void SceneRenderer::DrawReflectingModel(const RenderModel &model,
                                        const Matrix4 &world,
                                        const Matrix4 &viewProjection,
                                        const Lighting &lighting,
                                        const Reflection &reflection) {
    m_state->draws.emplace_back(ModelDraw{
        &model, world, viewProjection, lighting, {}, true, std::nullopt, reflection, std::nullopt});
}

void SceneRenderer::DrawShadowedModel(const RenderModel &model,
                                      const Matrix4 &world,
                                      const Matrix4 &viewProjection,
                                      const Lighting &lighting,
                                      CarShadow shadow) {
    m_state->draws.emplace_back(ModelDraw{&model,
                                          world,
                                          viewProjection,
                                          lighting,
                                          {},
                                          true,
                                          std::nullopt,
                                          std::nullopt,
                                          std::move(shadow)});
}

void SceneRenderer::DrawQuads(std::span<const FlatVertex> corners,
                              const FlatMaterial &material,
                              const Matrix4 &projection,
                              bool depthTest) {
    std::size_t first = m_state->flat.size();
    for (const FlatVertex &corner : corners) {
        Vertex v{};
        v.position[0] = corner.x;
        v.position[1] = corner.y;
        v.position[2] = corner.z;
        v.texCoord[0] = corner.u;
        v.texCoord[1] = corner.v;
        std::memcpy(v.color, corner.color.data(), 4);
        m_state->flat.push_back(v);
    }
    m_state->draws.emplace_back(
        QuadDraw{first, corners.size() / 4, material, projection, depthTest});
}

void SceneRenderer::End(const std::filesystem::path *capture) {
    State &s = *m_state;
    SDL_GPUCommandBuffer *commands = SDL_AcquireGPUCommandBuffer(s.device);

    // The vertices of the flat draws, and the indices of as many quads, uploaded before the frame.
    std::size_t quads = s.flat.size() / 4;
    if (quads > 0) {
        std::size_t vertexBytes = s.flat.size() * sizeof(Vertex);
        bool growIndices = s.quadIndices == nullptr || s.quadCapacity < quads * 6 * 4;
        if (!s.Reserve(
                s.flatVertices, s.flatVertexCapacity, vertexBytes, SDL_GPU_BUFFERUSAGE_VERTEX) ||
            !s.Reserve(s.quadIndices, s.quadCapacity, quads * 6 * 4, SDL_GPU_BUFFERUSAGE_INDEX)) {
            SDL_LogError(SDL_LOG_CATEGORY_GPU, "The flat draws have no buffer: %s", SDL_GetError());
            s.flat.clear();
        } else {
            std::vector<std::uint32_t> indices;
            if (growIndices) {
                for (std::uint32_t quad = 0; quad < s.quadCapacity / 24; ++quad) {
                    for (std::uint32_t corner : {0u, 1u, 2u, 0u, 2u, 3u}) {
                        indices.push_back(quad * 4 + corner);
                    }
                }
            }
            std::size_t indexBytes = indices.size() * 4;
            SDL_GPUTransferBufferCreateInfo info{};
            info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
            info.size = static_cast<Uint32>(vertexBytes + indexBytes);
            SDL_GPUTransferBuffer *transfer = SDL_CreateGPUTransferBuffer(s.device, &info);
            auto *mapped =
                static_cast<std::uint8_t *>(SDL_MapGPUTransferBuffer(s.device, transfer, false));
            std::memcpy(mapped, s.flat.data(), vertexBytes);
            std::memcpy(mapped + vertexBytes, indices.data(), indexBytes);
            SDL_UnmapGPUTransferBuffer(s.device, transfer);
            SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(commands);
            SDL_GPUTransferBufferLocation from{transfer, 0};
            SDL_GPUBufferRegion to{s.flatVertices, 0, static_cast<Uint32>(vertexBytes)};
            SDL_UploadToGPUBuffer(copy, &from, &to, true);
            if (indexBytes > 0) {
                from.offset = static_cast<Uint32>(vertexBytes);
                to = {s.quadIndices, 0, static_cast<Uint32>(indexBytes)};
                SDL_UploadToGPUBuffer(copy, &from, &to, false);
            }
            SDL_EndGPUCopyPass(copy);
            SDL_ReleaseGPUTransferBuffer(s.device, transfer);
        }
    }

    SDL_GPUColorTargetInfo color{};
    color.texture = s.window->FrameTexture();
    color.clear_color = {s.clear[0], s.clear[1], s.clear[2], s.clear[3]};
    color.load_op = SDL_GPU_LOADOP_CLEAR;
    color.store_op = SDL_GPU_STOREOP_STORE;
    SDL_GPUDepthStencilTargetInfo depth{};
    depth.texture = s.depth;
    depth.clear_depth = 1.0f;
    depth.load_op = SDL_GPU_LOADOP_CLEAR;
    depth.store_op = SDL_GPU_STOREOP_DONT_CARE;
    SDL_GPURenderPass *pass = SDL_BeginGPURenderPass(commands, &color, 1, &depth);

    auto drawPrimitive = [&](const Primitive &primitive,
                             const RenderModel::State &model,
                             const VertexUniforms &vertexUniforms,
                             const Lighting &lighting,
                             SDL_GPUBuffer *targets,
                             bool depthTest,
                             const Reflection *reflection,
                             const CarShadow *shadow) {
        SDL_GPUGraphicsPipeline *pipeline = s.Pipeline({primitive.blend,
                                                        primitive.twoSided,
                                                        !depthTest      ? DepthMode::None :
                                                        primitive.blend ? DepthMode::Test :
                                                                          DepthMode::TestAndWrite});
        if (pipeline == nullptr) {
            return;
        }
        SDL_BindGPUGraphicsPipeline(pass, pipeline);
        SDL_PushGPUVertexUniformData(commands, 0, &vertexUniforms, sizeof(vertexUniforms));
        FragmentUniforms fragmentUniforms{
            {primitive.alphaCut,
             primitive.lit ? 1.0f : 0.0f,
             primitive.texture || reflection != nullptr ? 1.0f : 0.0f,
             primitive.combine ? 1.0f : 0.0f},
            {lighting.direction[0], lighting.direction[1], lighting.direction[2], 0.0f},
            {primitive.combine ? (*primitive.combine)[0] : 0u,
             primitive.combine ? (*primitive.combine)[1] : 0u,
             primitive.twoCycle ? 1u : 0u,
             0u},
            {primitive.prim[0], primitive.prim[1], primitive.prim[2], primitive.prim[3]},
            {primitive.env[0], primitive.env[1], primitive.env[2], primitive.env[3]},
            {lighting.ambient[0], lighting.ambient[1], lighting.ambient[2], 1.0f},
            {lighting.directional[0], lighting.directional[1], lighting.directional[2], 1.0f},
            {1.0f, 1.0f, 0.0f, 0.0f},
            {},
            {},
            {}};
        if (reflection != nullptr) {
            std::ranges::copy(reflection->environment, fragmentUniforms.env);
        }
        if (shadow != nullptr) {
            float length = std::hypot(shadow->forward[0], shadow->forward[1]);
            fragmentUniforms.shadowCenter[0] = shadow->x;
            fragmentUniforms.shadowCenter[1] = shadow->y;
            fragmentUniforms.shadowCenter[2] = shadow->along;
            fragmentUniforms.shadowCenter[3] = shadow->across;
            fragmentUniforms.shadowForward[0] = length > 0.0f ? shadow->forward[0] / length : 1.0f;
            fragmentUniforms.shadowForward[1] = length > 0.0f ? shadow->forward[1] / length : 0.0f;
            fragmentUniforms.shadowForward[2] = 1.0f;
            std::ranges::copy(shadow->color, fragmentUniforms.shadowColor);
        }
        SDL_PushGPUFragmentUniformData(commands, 0, &fragmentUniforms, sizeof(fragmentUniforms));
        std::array<SDL_GPUTextureSamplerBinding, 3> bindings{
            {{s.white, s.plainSampler}, {s.white, s.plainSampler}, {s.white, s.plainSampler}}};
        if (shadow != nullptr) {
            bindings[2] = {shadow->texture->m_texture, shadow->texture->m_sampler};
        }
        if (primitive.texture && *primitive.texture < model.textures.size()) {
            auto [image, sampler] = model.textures[*primitive.texture];
            image = AnimationFrame(primitive.frames, s.textureTime).value_or(image);
            bindings[0] = {model.images.at(image), model.samplers.at(sampler)};
            if (primitive.texel1 && *primitive.texel1 < model.images.size()) {
                bindings[1] = {model.images[*primitive.texel1], model.samplers.at(sampler)};
            }
        }
        if (reflection != nullptr) {
            bindings[0] = {reflection->texture->m_texture, reflection->texture->m_sampler};
        }
        SDL_BindGPUFragmentSamplers(pass, 0, bindings.data(), bindings.size());
        SDL_BindGPUVertexStorageBuffers(pass, 0, &targets, 1);
        SDL_GPUBufferBinding vertices{primitive.vertices, 0};
        SDL_BindGPUVertexBuffers(pass, 0, &vertices, 1);
        SDL_GPUBufferBinding indices{primitive.indices, 0};
        SDL_BindGPUIndexBuffer(pass, &indices, SDL_GPU_INDEXELEMENTSIZE_32BIT);
        SDL_DrawGPUIndexedPrimitives(pass, primitive.indexCount, 1, 0, 0, 0);
    };

    // Each model draws its opaque primitives where it comes, and the blended ones of every model
    // follow every opaque one. A flat draw comes after the models drawn before it.
    SDL_Rect whole{
        0, 0, static_cast<int>(s.window->FrameWidth()), static_cast<int>(s.window->FrameHeight())};
    auto drawModel = [&](const ModelDraw &draw, bool blending) {
        const RenderModel::State &model = *draw.model->m_state;
        SDL_SetGPUScissor(pass, draw.scissor ? &*draw.scissor : &whole);
        for (const Placed &placed : model.placed) {
            if (placed.flags & s.hiddenFlags) {
                continue;
            }
            const Mesh &mesh = model.meshes[placed.mesh];
            VertexUniforms vertexUniforms{draw.viewProjection,
                                          MultiplyMatrices(draw.world, placed.matrix),
                                          {0, 0, mesh.vertexCount, 0},
                                          {0.0f, 0.0f, 0.0f, 0.0f},
                                          {},
                                          {},
                                          {}};
            SDL_GPUBuffer *targets = s.noTargets;
            if (!draw.poses.empty() && mesh.targets != nullptr) {
                vertexUniforms.morph = {
                    0, 0, mesh.vertexCount, static_cast<std::int32_t>(draw.poses.size())};
                for (std::size_t i = 0; i < draw.poses.size(); ++i) {
                    vertexUniforms.poses[i] = {static_cast<float>(draw.poses[i].first),
                                               static_cast<float>(draw.poses[i].second),
                                               draw.poses[i].blend,
                                               0.0f};
                }
                targets = mesh.targets;
            }
            const Lighting &lighting = placed.lighting ? *placed.lighting : draw.lighting;
            // The shadow of the car falls on the instances near it.
            bool shaded = draw.shadow && draw.shadow->texture != nullptr && placed.instance >= 0 &&
                          std::ranges::find(draw.shadow->instances, placed.instance) !=
                              draw.shadow->instances.end();
            for (const Primitive &primitive : mesh.primitives) {
                if (primitive.blend == blending) {
                    VertexUniforms uniforms = vertexUniforms;
                    uniforms.blend[1] = primitive.textureGen ? 1.0f : 0.0f;
                    // A part with generated coordinates and no texture reflects the sky.
                    const Reflection *reflection = draw.reflection && primitive.textureGen &&
                                                           !primitive.texture &&
                                                           draw.reflection->texture != nullptr ?
                                                       &*draw.reflection :
                                                       nullptr;
                    if (reflection != nullptr) {
                        uniforms.blend[2] = 1.0f;
                        uniforms.reflectAcross = {reflection->across[0],
                                                  reflection->across[1],
                                                  reflection->across[2],
                                                  reflection->column};
                        uniforms.reflectUp = {
                            reflection->up[0], reflection->up[1], reflection->up[2], 0.0f};
                    }
                    drawPrimitive(primitive,
                                  model,
                                  uniforms,
                                  lighting,
                                  targets,
                                  draw.depth,
                                  reflection,
                                  shaded ? &*draw.shadow : nullptr);
                }
            }
        }
    };
    std::vector<const ModelDraw *> pending;
    auto flushBlended = [&]() {
        for (const ModelDraw *draw : pending) {
            drawModel(*draw, true);
        }
        pending.clear();
    };
    for (const auto &entry : s.draws) {
        if (const auto *draw = std::get_if<ModelDraw>(&entry)) {
            drawModel(*draw, false);
            pending.push_back(draw);
            continue;
        }
        flushBlended();
        const QuadDraw &quad = std::get<QuadDraw>(entry);
        if (s.flatVertices == nullptr || s.flat.empty()) {
            continue;
        }
        SDL_GPUGraphicsPipeline *pipeline =
            s.Pipeline({true, true, quad.depthTest ? DepthMode::Test : DepthMode::None});
        if (pipeline == nullptr) {
            continue;
        }
        SDL_BindGPUGraphicsPipeline(pass, pipeline);
        SDL_SetGPUScissor(pass, &whole);
        VertexUniforms vertexUniforms{
            quad.projection, IDENTITY_MATRIX, {0, 0, 0, 0}, {0.0f, 0.0f, 0.0f, 0.0f}, {}, {}, {}};
        SDL_PushGPUVertexUniformData(commands, 0, &vertexUniforms, sizeof(vertexUniforms));
        const FlatMaterial &material = quad.material;
        bool combined = material.combine[0] != 0 || material.combine[1] != 0;
        FragmentUniforms fragmentUniforms{
            {-1.0f, 0.0f, material.texel0 != nullptr ? 1.0f : 0.0f, combined ? 1.0f : 0.0f},
            {0.0f, 0.0f, 1.0f, 0.0f},
            {material.combine[0], material.combine[1], material.twoCycle ? 1u : 0u, 0u},
            {material.prim[0], material.prim[1], material.prim[2], material.prim[3]},
            {material.env[0], material.env[1], material.env[2], material.env[3]},
            {1.0f, 1.0f, 1.0f, 1.0f},
            {0.0f, 0.0f, 0.0f, 1.0f},
            {material.texel1Transform[0],
             material.texel1Transform[1],
             material.texel1Transform[2],
             material.texel1Transform[3]},
            {},
            {},
            {}};
        SDL_PushGPUFragmentUniformData(commands, 0, &fragmentUniforms, sizeof(fragmentUniforms));
        std::array<SDL_GPUTextureSamplerBinding, 3> bindings{
            {{s.white, s.plainSampler}, {s.white, s.plainSampler}, {s.white, s.plainSampler}}};
        if (material.texel0 != nullptr) {
            bindings[0] = {material.texel0->m_texture, material.texel0->m_sampler};
        }
        if (material.texel1 != nullptr) {
            bindings[1] = {material.texel1->m_texture, material.texel1->m_sampler};
        }
        SDL_BindGPUFragmentSamplers(pass, 0, bindings.data(), bindings.size());
        SDL_BindGPUVertexStorageBuffers(pass, 0, &s.noTargets, 1);
        SDL_GPUBufferBinding vertices{s.flatVertices, 0};
        SDL_BindGPUVertexBuffers(pass, 0, &vertices, 1);
        SDL_GPUBufferBinding indices{s.quadIndices, 0};
        SDL_BindGPUIndexBuffer(pass, &indices, SDL_GPU_INDEXELEMENTSIZE_32BIT);
        SDL_DrawGPUIndexedPrimitives(pass,
                                     static_cast<Uint32>(quad.quadCount * 6),
                                     1,
                                     static_cast<Uint32>(quad.firstVertex / 4 * 6),
                                     0,
                                     0);
    }
    flushBlended();
    SDL_EndGPURenderPass(pass);
    s.window->Present(commands, capture);
}

} // namespace coastline
