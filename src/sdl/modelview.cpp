#include "modelview.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iterator>
#include <optional>
#include <span>
#include <vector>

#include <SDL3/SDL.h>
#include <nlohmann/json.hpp>

#include "gpu.h"
#include "shaders/model_frag_msl.h"
#include "shaders/model_frag_spirv.h"
#include "shaders/model_vert_msl.h"
#include "shaders/model_vert_spirv.h"

namespace coastline {

namespace {

// A 4 by 4 matrix, column by column.
using Matrix = std::array<float, 16>;

constexpr Matrix IDENTITY{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

Matrix Multiply(const Matrix &a, const Matrix &b) {
    Matrix out{};
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

// The transform of a node of glTF, from its matrix, or its translation, rotation, and scale.
Matrix NodeMatrix(const nlohmann::json &node) {
    if (node.contains("matrix")) {
        Matrix matrix{};
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

std::array<float, 3> Transform(const Matrix &m, const float *p) {
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
    std::array<float, 4> prim{1, 1, 1, 1};
    std::array<float, 4> env{1, 1, 1, 1};
    std::optional<std::size_t> texel1;
};

std::array<float, 4> Rgba(std::uint32_t value) {
    return {static_cast<float>(value >> 24 & 0xff) / 255.0f,
            static_cast<float>(value >> 16 & 0xff) / 255.0f,
            static_cast<float>(value >> 8 & 0xff) / 255.0f,
            static_cast<float>(value & 0xff) / 255.0f};
}

// A placed mesh: its primitives and the transform of its node.
struct Placed {
    std::size_t mesh;
    Matrix matrix;
    std::array<float, 4> ambient;
    std::array<float, 4> directional;
};

// The lights of a clear day as the race sets them up: a directional light of 255, 255, 204 from
// 15, 10, 20 and an ambient light of 102, 102, 119. A placed model with a ramped light has no
// directional light, and an ambient light of a quarter, a half, three quarters, or all of the
// directional colour by the low bits of its flags.
constexpr std::array<int, 3> DIRECTIONAL{255, 255, 204};
constexpr std::array<int, 3> AMBIENT{102, 102, 119};
// The direction toward the light, 15, 10, 20 of the console, which is z up, normalised in the y
// up space of glTF.
constexpr std::array<float, 3> LIGHT_DIRECTION{0.5571f, 0.7428f, -0.3714f};
constexpr std::uint32_t TRACK_INSTANCE_LIGHT_RAMP = 0x0003;
constexpr std::uint32_t TRACK_INSTANCE_RAMPED = 0x0400;

std::pair<std::array<float, 4>, std::array<float, 4>> Lights(std::uint32_t flags) {
    std::array<float, 4> ambient{};
    std::array<float, 4> directional{};
    for (std::size_t i = 0; i < 3; ++i) {
        int full = DIRECTIONAL[i];
        if ((flags & TRACK_INSTANCE_RAMPED) != 0) {
            std::array<int, 4> ramp{full >> 2, full >> 1, (full >> 1) + (full >> 2), full};
            ambient[i] = static_cast<float>(ramp[flags & TRACK_INSTANCE_LIGHT_RAMP]) / 255.0f;
        } else {
            ambient[i] = static_cast<float>(AMBIENT[i]) / 255.0f;
            directional[i] = static_cast<float>(full) / 255.0f;
        }
    }
    return {ambient, directional};
}

struct VertexUniforms {
    Matrix viewProjection;
    Matrix model;
};

struct FragmentUniforms {
    float flags[4];
    float lightDirection[4];
    std::uint32_t combine[4];
    float prim[4];
    float env[4];
    float ambient[4];
    float directional[4];
};

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

} // namespace

struct ModelView::State {
    GpuWindow *window = nullptr;
    SDL_GPUDevice *device = nullptr;
    std::array<SDL_GPUGraphicsPipeline *, 4> pipelines{}; // By blend, then by two sides.
    SDL_GPUTexture *depth = nullptr;
    SDL_GPUTextureFormat depthFormat = SDL_GPU_TEXTUREFORMAT_D16_UNORM;
    std::vector<SDL_GPUTexture *> images;
    std::vector<SDL_GPUSampler *> samplers;
    std::vector<std::pair<std::size_t, std::size_t>> textures; // Image and sampler.
    SDL_GPUTexture *white = nullptr;
    SDL_GPUSampler *whiteSampler = nullptr;
    std::vector<std::vector<Primitive>> meshes;
    std::vector<Placed> placed;
    std::array<float, 3> centre{};
    float radius = 1.0f;
    float yaw = 0.8f;
    float pitch = 0.5f;
    float distance = 2.0f;

    ~State() {
        if (device == nullptr) {
            return;
        }
        for (auto *pipeline : pipelines) {
            if (pipeline != nullptr) {
                SDL_ReleaseGPUGraphicsPipeline(device, pipeline);
            }
        }
        for (auto &mesh : meshes) {
            for (auto &primitive : mesh) {
                SDL_ReleaseGPUBuffer(device, primitive.vertices);
                SDL_ReleaseGPUBuffer(device, primitive.indices);
            }
        }
        for (auto *image : images) {
            SDL_ReleaseGPUTexture(device, image);
        }
        for (auto *sampler : samplers) {
            SDL_ReleaseGPUSampler(device, sampler);
        }
        if (white != nullptr) {
            SDL_ReleaseGPUTexture(device, white);
        }
        if (whiteSampler != nullptr) {
            SDL_ReleaseGPUSampler(device, whiteSampler);
        }
        if (depth != nullptr) {
            SDL_ReleaseGPUTexture(device, depth);
        }
    }
};

ModelView::ModelView(std::unique_ptr<State> state) : m_state(std::move(state)) {
}

ModelView::~ModelView() = default;

namespace {

// Uploads bytes to a new buffer or texture through one copy pass.
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

// The bytes of an accessor of the buffer, by element.
struct AccessorView {
    const std::uint8_t *data = nullptr;
    std::size_t count = 0;
    std::size_t stride = 0;
};

std::optional<AccessorView> Accessor(const nlohmann::json &document,
                                     std::span<const std::uint8_t> buffer,
                                     std::size_t index,
                                     std::size_t elementSize) {
    const auto &accessor = document["accessors"].at(index);
    const auto &view = document["bufferViews"].at(accessor["bufferView"].get<std::size_t>());
    AccessorView result;
    result.count = accessor["count"].get<std::size_t>();
    result.stride = view.value("byteStride", elementSize);
    std::size_t offset =
        view["byteOffset"].get<std::size_t>() + accessor.value("byteOffset", std::size_t{0});
    if (result.count == 0 || offset > buffer.size() ||
        (result.count - 1) * result.stride + elementSize > buffer.size() - offset) {
        return std::nullopt;
    }
    result.data = buffer.data() + offset;
    return result;
}

} // namespace

std::unique_ptr<ModelView> ModelView::Create(GpuWindow &window, const std::filesystem::path &path) {
    auto state = std::make_unique<State>();
    state->window = &window;
    state->device = window.Device();
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

    SDL_GPUShader *vertex =
        CreateShader(device, SDL_GPU_SHADERSTAGE_VERTEX, 0, MODEL_VERT_SPIRV, MODEL_VERT_MSL);
    SDL_GPUShader *fragment =
        CreateShader(device, SDL_GPU_SHADERSTAGE_FRAGMENT, 2, MODEL_FRAG_SPIRV, MODEL_FRAG_MSL);
    if (vertex == nullptr || fragment == nullptr) {
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
    for (int blend = 0; blend < 2; ++blend) {
        for (int twoSided = 0; twoSided < 2; ++twoSided) {
            SDL_GPUColorTargetDescription target{};
            target.format = GpuWindow::FrameFormat();
            if (blend != 0) {
                target.blend_state.enable_blend = true;
                target.blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
                target.blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
                target.blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
                target.blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
                target.blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
                target.blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
            }
            SDL_GPUGraphicsPipelineCreateInfo info{};
            info.vertex_shader = vertex;
            info.fragment_shader = fragment;
            info.vertex_input_state.vertex_buffer_descriptions = &binding;
            info.vertex_input_state.num_vertex_buffers = 1;
            info.vertex_input_state.vertex_attributes = attributes.data();
            info.vertex_input_state.num_vertex_attributes = attributes.size();
            info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
            info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
            info.rasterizer_state.cull_mode =
                twoSided != 0 ? SDL_GPU_CULLMODE_NONE : SDL_GPU_CULLMODE_BACK;
            info.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
            info.depth_stencil_state.enable_depth_test = true;
            info.depth_stencil_state.enable_depth_write = blend == 0;
            info.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL;
            info.target_info.color_target_descriptions = &target;
            info.target_info.num_color_targets = 1;
            info.target_info.depth_stencil_format = state->depthFormat;
            info.target_info.has_depth_stencil_target = true;
            state->pipelines[static_cast<std::size_t>(blend * 2 + twoSided)] =
                SDL_CreateGPUGraphicsPipeline(device, &info);
        }
    }
    SDL_ReleaseGPUShader(device, vertex);
    SDL_ReleaseGPUShader(device, fragment);
    if (state->depth == nullptr ||
        std::ranges::any_of(state->pipelines, [](auto *pipeline) { return pipeline == nullptr; })) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "The model view was not created: %s", SDL_GetError());
        return nullptr;
    }

    Uploader upload(device);
    for (const auto &image : document.value("images", nlohmann::json::array())) {
        auto file = path.parent_path() / image["uri"].get<std::string>();
        SDL_Surface *loaded = SDL_LoadPNG(file.string().c_str());
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
    constexpr std::array<std::uint8_t, 4> WHITE{255, 255, 255, 255};
    state->white = upload.Texture(1, 1, WHITE);
    SDL_GPUSamplerCreateInfo plain{};
    state->whiteSampler = SDL_CreateGPUSampler(device, &plain);

    // The meshes, with the vertices of each primitive in the layout of the shaders.
    std::vector<std::vector<std::array<float, 3>>> meshPositions;
    for (const auto &mesh : document["meshes"]) {
        auto &primitives = state->meshes.emplace_back();
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
            Primitive drawn;
            drawn.vertices = upload.Buffer(SDL_GPU_BUFFERUSAGE_VERTEX,
                                           {reinterpret_cast<const std::uint8_t *>(vertices.data()),
                                            vertices.size() * sizeof(Vertex)});
            drawn.indices = upload.Buffer(
                SDL_GPU_BUFFERUSAGE_INDEX,
                {reinterpret_cast<const std::uint8_t *>(list.data()), list.size() * 4});
            drawn.indexCount = static_cast<std::uint32_t>(list.size());
            drawn.lit = normal.has_value();
            if (primitive.contains("material")) {
                const auto &material =
                    document["materials"][primitive["material"].get<std::size_t>()];
                const auto &pbr = material.value("pbrMetallicRoughness", nlohmann::json::object());
                if (pbr.contains("baseColorTexture")) {
                    drawn.texture = pbr["baseColorTexture"]["index"].get<std::size_t>();
                }
                auto mode = material.value("alphaMode", std::string("OPAQUE"));
                drawn.blend = mode == "BLEND";
                drawn.alphaCut = mode == "MASK" ? 0.5f : -1.0f;
                drawn.twoSided = material.value("doubleSided", false);
                if (material.contains("extras") && material["extras"].contains("coastlineRdp")) {
                    const auto &rdp = material["extras"]["coastlineRdp"];
                    std::array<std::uint32_t, 2> words{rdp["combine"][0].get<std::uint32_t>(),
                                                       rdp["combine"][1].get<std::uint32_t>()};
                    if (words[0] != 0 || words[1] != 0) {
                        drawn.combine = words;
                    }
                    drawn.prim = Rgba(rdp["primColor"].get<std::uint32_t>());
                    drawn.env = Rgba(rdp["envColor"].get<std::uint32_t>());
                    if (rdp.contains("texel1Image")) {
                        drawn.texel1 = rdp["texel1Image"].get<std::size_t>();
                    }
                }
            }
            primitives.push_back(drawn);
        }
    }
    if (!upload.Submit()) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "The model was not uploaded: %s", SDL_GetError());
        return nullptr;
    }

    // The nodes of the first scene and their places, and the bounds of what they draw.
    std::array<float, 3> low{INFINITY, INFINITY, INFINITY};
    std::array<float, 3> high{-INFINITY, -INFINITY, -INFINITY};
    auto visit = [&](auto &self, std::size_t index, const Matrix &parent) -> void {
        const auto &node = document["nodes"].at(index);
        Matrix matrix = Multiply(parent, NodeMatrix(node));
        if (node.contains("mesh")) {
            std::size_t mesh = node["mesh"].get<std::size_t>();
            std::uint32_t flags = 0;
            if (node.contains("extras") && node["extras"].contains("coastlineInstance")) {
                flags = node["extras"]["coastlineInstance"]["flags"].get<std::uint32_t>();
            }
            auto [ambient, directional] = Lights(flags);
            state->placed.push_back({mesh, matrix, ambient, directional});
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
    const auto &scene = document["scenes"][document.value("scene", 0)];
    for (const auto &root : scene["nodes"]) {
        visit(visit, root.get<std::size_t>(), IDENTITY);
    }
    if (state->placed.empty()) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s draws nothing.", path.string().c_str());
        return nullptr;
    }
    for (std::size_t axis = 0; axis < 3; ++axis) {
        state->centre[axis] = (low[axis] + high[axis]) / 2;
        state->radius = std::max(state->radius, (high[axis] - low[axis]) / 2);
    }
    state->distance = state->radius * 2.2f;
    return std::unique_ptr<ModelView>(new ModelView(std::move(state)));
}

void ModelView::Draw(const std::filesystem::path *capture) {
    State &s = *m_state;
    const bool *keys = SDL_GetKeyboardState(nullptr);
    s.yaw += (keys[SDL_SCANCODE_RIGHT] ? 0.03f : 0.0f) - (keys[SDL_SCANCODE_LEFT] ? 0.03f : 0.0f);
    s.pitch += (keys[SDL_SCANCODE_UP] ? 0.02f : 0.0f) - (keys[SDL_SCANCODE_DOWN] ? 0.02f : 0.0f);
    s.pitch = std::clamp(s.pitch, -1.5f, 1.5f);
    float zoom = keys[SDL_SCANCODE_W] ? 0.98f : keys[SDL_SCANCODE_S] ? 1.02f : 1.0f;
    s.distance = std::clamp(s.distance * zoom, s.radius * 0.02f, s.radius * 10.0f);

    // A camera about the centre, y up, looking at it.
    std::array<float, 3> eye{s.centre[0] + s.distance * std::cos(s.pitch) * std::sin(s.yaw),
                             s.centre[1] + s.distance * std::sin(s.pitch),
                             s.centre[2] + s.distance * std::cos(s.pitch) * std::cos(s.yaw)};
    std::array<float, 3> forward{s.centre[0] - eye[0], s.centre[1] - eye[1], s.centre[2] - eye[2]};
    float length =
        std::sqrt(forward[0] * forward[0] + forward[1] * forward[1] + forward[2] * forward[2]);
    for (float &f : forward) {
        f /= length;
    }
    std::array<float, 3> right{forward[2], 0.0f, -forward[0]};
    float rightLength = std::sqrt(right[0] * right[0] + right[2] * right[2]);
    right = {-right[0] / rightLength, 0.0f, -right[2] / rightLength};
    std::array<float, 3> up{right[1] * forward[2] - right[2] * forward[1],
                            right[2] * forward[0] - right[0] * forward[2],
                            right[0] * forward[1] - right[1] * forward[0]};
    auto dot = [](const std::array<float, 3> &a, const std::array<float, 3> &b) {
        return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
    };
    Matrix view{right[0],
                up[0],
                -forward[0],
                0,
                right[1],
                up[1],
                -forward[1],
                0,
                right[2],
                up[2],
                -forward[2],
                0,
                -dot(right, eye),
                -dot(up, eye),
                dot(forward, eye),
                1};
    float aspect = static_cast<float>(s.window->FrameWidth()) /
                   static_cast<float>(std::max<Uint32>(s.window->FrameHeight(), 1));
    float nearPlane = s.radius * 0.005f;
    float farPlane = s.distance + s.radius * 3.0f;
    float focal = 1.0f / std::tan(0.5f * 1.0f);
    // Depth from 0 to 1, as Vulkan, Metal, and Direct3D 12 take it.
    Matrix projection{focal / aspect,
                      0,
                      0,
                      0,
                      0,
                      focal,
                      0,
                      0,
                      0,
                      0,
                      farPlane / (nearPlane - farPlane),
                      -1,
                      0,
                      0,
                      nearPlane * farPlane / (nearPlane - farPlane),
                      0};
    VertexUniforms vertexUniforms{Multiply(projection, view), IDENTITY};

    SDL_GPUCommandBuffer *commands = SDL_AcquireGPUCommandBuffer(s.device);
    SDL_GPUColorTargetInfo color{};
    color.texture = s.window->FrameTexture();
    color.clear_color = {0.18f, 0.22f, 0.3f, 1.0f};
    color.load_op = SDL_GPU_LOADOP_CLEAR;
    color.store_op = SDL_GPU_STOREOP_STORE;
    SDL_GPUDepthStencilTargetInfo depth{};
    depth.texture = s.depth;
    depth.clear_depth = 1.0f;
    depth.load_op = SDL_GPU_LOADOP_CLEAR;
    depth.store_op = SDL_GPU_STOREOP_DONT_CARE;
    SDL_GPURenderPass *pass = SDL_BeginGPURenderPass(commands, &color, 1, &depth);
    // The opaque and cut out primitives, then the blended ones over them.
    for (bool blending : {false, true}) {
        for (const Placed &placed : s.placed) {
            vertexUniforms.model = placed.matrix;
            for (const Primitive &primitive : s.meshes[placed.mesh]) {
                if (primitive.blend != blending) {
                    continue;
                }
                SDL_BindGPUGraphicsPipeline(
                    pass,
                    s.pipelines[static_cast<std::size_t>((blending ? 2 : 0) +
                                                         (primitive.twoSided ? 1 : 0))]);
                SDL_PushGPUVertexUniformData(commands, 0, &vertexUniforms, sizeof(vertexUniforms));
                FragmentUniforms fragmentUniforms{
                    {primitive.alphaCut,
                     primitive.lit ? 1.0f : 0.0f,
                     primitive.texture ? 1.0f : 0.0f,
                     primitive.combine ? 1.0f : 0.0f},
                    {LIGHT_DIRECTION[0], LIGHT_DIRECTION[1], LIGHT_DIRECTION[2], 0.0f},
                    {primitive.combine ? (*primitive.combine)[0] : 0u,
                     primitive.combine ? (*primitive.combine)[1] : 0u,
                     0u,
                     0u},
                    {primitive.prim[0], primitive.prim[1], primitive.prim[2], primitive.prim[3]},
                    {primitive.env[0], primitive.env[1], primitive.env[2], primitive.env[3]},
                    {placed.ambient[0], placed.ambient[1], placed.ambient[2], 1.0f},
                    {placed.directional[0], placed.directional[1], placed.directional[2], 1.0f}};
                SDL_PushGPUFragmentUniformData(
                    commands, 0, &fragmentUniforms, sizeof(fragmentUniforms));
                std::array<SDL_GPUTextureSamplerBinding, 2> bindings{
                    {{s.white, s.whiteSampler}, {s.white, s.whiteSampler}}};
                if (primitive.texture && *primitive.texture < s.textures.size()) {
                    auto [image, sampler] = s.textures[*primitive.texture];
                    bindings[0] = {s.images.at(image), s.samplers.at(sampler)};
                    if (primitive.texel1 && *primitive.texel1 < s.images.size()) {
                        bindings[1] = {s.images[*primitive.texel1], s.samplers.at(sampler)};
                    }
                }
                SDL_BindGPUFragmentSamplers(pass, 0, bindings.data(), bindings.size());
                SDL_GPUBufferBinding vertices{primitive.vertices, 0};
                SDL_BindGPUVertexBuffers(pass, 0, &vertices, 1);
                SDL_GPUBufferBinding indices{primitive.indices, 0};
                SDL_BindGPUIndexBuffer(pass, &indices, SDL_GPU_INDEXELEMENTSIZE_32BIT);
                SDL_DrawGPUIndexedPrimitives(pass, primitive.indexCount, 1, 0, 0, 0);
            }
        }
    }
    SDL_EndGPURenderPass(pass);
    s.window->Present(commands, capture);
}

} // namespace coastline
