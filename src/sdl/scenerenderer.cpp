#include "scenerenderer.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iterator>
#include <map>
#include <numeric>
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

// ApplyPaintDamageOverlay(): the paint of a set darkened by a mask of 64 by 64 intensities
// stretched over it, the paint placed as BlitPaintImageToPixels() places it in the image at twice
// its size, its bottom on the bottom of the rectangle of the set measured from the bottom of the
// image. A texel outside the paint is not darkened.
std::vector<std::uint8_t> DamageTexels(std::vector<std::uint8_t> texels,
                                       int width,
                                       int height,
                                       const std::vector<std::uint8_t> &mask,
                                       const ImageDamage &damage) {
    constexpr int MASK_SIDE = 64;
    auto [left, right, top, bottom] = damage.rectangle;
    if (left == 0 && right == 0 && top == 0 && bottom == 0) {
        right = width * 2;
        bottom = height * 2;
    }
    int paintWidth = right - left;
    int paintHeight = bottom - top;
    if (paintWidth <= 0 || paintHeight <= 0 || mask.size() < MASK_SIDE * MASK_SIDE) {
        return texels;
    }
    for (int y = 0; y < height; ++y) {
        int row = y * 2 - (height * 2 - bottom);
        for (int x = 0; x < width; ++x) {
            int column = x * 2 - left;
            if (row < 0 || row >= paintHeight || column < 0 || column >= paintWidth) {
                continue;
            }
            unsigned intensity = mask[static_cast<std::size_t>(
                row * MASK_SIDE / paintHeight * MASK_SIDE + column * MASK_SIDE / paintWidth)];
            std::uint8_t *texel = &texels[static_cast<std::size_t>((y * width + x) * 4)];
            for (int c = 0; c < 3; ++c) {
                texel[c] = static_cast<std::uint8_t>(texel[c] * intensity / 255);
            }
        }
    }
    return texels;
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
    // The axes of the coordinates of a reflection, in the w of the first the first column of its
    // window, and in the w of the second 1 where the coordinates are generated linearly.
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
    float shadowCenter[MAX_CAR_SHADOWS][4];
    float shadowForward[MAX_CAR_SHADOWS][4];
    float shadowColor[MAX_CAR_SHADOWS][4];
    float fogColor[4] = {};
};

// The cycle type of the RDP in its high other mode word.
constexpr std::uint32_t CYCLE_TYPE_SHIFT = 20;
constexpr std::uint32_t TWO_CYCLE = 1;
// The bit of the geometry mode that generates the texture coordinates from the normals.
constexpr std::uint32_t G_TEXTURE_GEN = 0x40000;
// The combiner of the reflection pass of RenderVehicleModel(), (TEXEL0 - ENVIRONMENT) * ENV_ALPHA
// + ENVIRONMENT with the alpha of TEXEL0 in both cycles.
constexpr std::array<std::uint32_t, 2> REFLECTION_COMBINE{0x167E2C, 0x55FEF379};

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
        SDL_GPUBuffer *buffer = CreateGpuBuffer(m_device, &info);
        if (buffer != nullptr) {
            m_buffers.push_back({buffer, std::vector<std::uint8_t>(bytes.begin(), bytes.end())});
        }
        return buffer;
    }

    // A mipmapped texture has every level down to a pixel, generated from the first after the
    // upload.
    SDL_GPUTexture *
    Texture(int width, int height, std::span<const std::uint8_t> rgba, bool mipmapped = false) {
        SDL_GPUTextureCreateInfo info{};
        info.type = SDL_GPU_TEXTURETYPE_2D;
        info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
        info.width = static_cast<Uint32>(width);
        info.height = static_cast<Uint32>(height);
        info.layer_count_or_depth = 1;
        info.num_levels = 1;
        mipmapped = mipmapped && std::max(width, height) > 1;
        if (mipmapped) {
            info.usage |= SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;
            info.num_levels =
                static_cast<Uint32>(std::bit_width(static_cast<unsigned>(std::max(width, height))));
        }
        SDL_GPUTexture *texture = CreateGpuTexture(m_device, &info);
        if (texture != nullptr) {
            m_textures.push_back({texture,
                                  width,
                                  height,
                                  std::vector<std::uint8_t>(rgba.begin(), rgba.end()),
                                  mipmapped});
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
        SDL_GPUTransferBuffer *transfer = CreateGpuTransferBuffer(m_device, &info);
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
        for (const auto &texture : m_textures) {
            if (texture.mipmapped) {
                SDL_GenerateMipmapsForGPUTexture(commands, texture.texture);
            }
        }
        bool submitted = SDL_SubmitGPUCommandBuffer(commands);
        ReleaseGpuTransferBuffer(m_device, transfer);
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
        bool mipmapped;
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
    // Whether the state of the RDP is recorded, and whether the display list sets two cycles
    // itself, which a model whose caller sets two draws in alike.
    bool recorded = false;
    bool twoCycle = false;
    // Whether the texture coordinates come from the normals, as G_TEXTURE_GEN of the RSP.
    bool textureGen = false;
    std::array<float, 4> prim{1, 1, 1, 1};
    std::array<float, 4> env{1, 1, 1, 1};
    std::optional<std::size_t> texel1;
    // The frames of an animated texture, each its time in milliseconds and its image.
    std::vector<std::pair<std::uint32_t, std::size_t>> frames;
    // The image of a texture of the snow that a race in the snow draws, or none.
    std::optional<std::size_t> snowImage;
    // The primitive and environment colours of a race in the snow off the coastline, where they
    // differ.
    std::optional<std::array<float, 4>> snowPrim;
    std::optional<std::array<float, 4>> snowEnv;
    // Whether the animated texture stays on its first frame in a race in the snow off the
    // coastline.
    bool snowStill = false;
};

// A mesh: its primitives, and the displacements of its morph targets.
struct Mesh {
    std::vector<Primitive> primitives;
    SDL_GPUBuffer *targets = nullptr;
    int vertexCount = 0;
    // The vertices of the first primitive, and the most of any primitive.
    std::size_t firstVertices = 0;
    int mostVertices = 0;
};

// A mesh placed by a node of the scene.
struct Placed {
    std::size_t mesh;
    Matrix4 matrix;
    int instance;        // The instance of the track the node draws, or -1.
    std::uint32_t flags; // The TRACK_INSTANCE_ flags of the instance, or 0.
    Matrix4 parent;      // The place of the parent of the node, under the matrix of the instance.
};

constexpr std::uint32_t TRACK_INSTANCE_LIGHT_RAMP = 0x0003;
constexpr std::uint32_t TRACK_INSTANCE_NIGHT_LIT = 0x0100;
constexpr std::uint32_t TRACK_INSTANCE_RAMPED = 0x0400;

// RenderTrackGeometry() lights an instance of a track with the lights of the track, and a ramped
// instance with the colour of its ramp as the ambient light and a directional light only where it
// is lit at night.
Lighting TrackInstanceLighting(const Lighting &track, std::uint32_t flags) {
    Lighting lighting = track;
    if ((flags & TRACK_INSTANCE_RAMPED) != 0) {
        lighting.ambient = track.ramp[flags & TRACK_INSTANCE_LIGHT_RAMP];
        lighting.directional =
            (flags & TRACK_INSTANCE_NIGHT_LIT) != 0 ? track.nightLit : std::array<float, 3>{};
    }
    return lighting;
}

// A key of a pipeline: blending, both sides, and the depth test and write.
enum class DepthMode { None, Test, TestAndWrite, Equal };

struct PipelineKey {
    bool blend;
    bool twoSided;
    DepthMode depth;
    bool additive = false;
    bool mirrored = false;
    bool ghost = false;
    // Whether the alpha of a fragment covers that share of the samples of its pixel.
    bool coverage = false;
    auto operator<=>(const PipelineKey &) const = default;
};

// The state of the frame a draw is made in: its view, its fog, the instances it hides, and whether
// its projection mirrors it across.
struct DrawState {
    std::optional<SDL_Rect> view;
    std::optional<Fog> fog;
    std::uint32_t hiddenFlags = 0;
    bool ghost = false;
    bool inOrder = false;
    bool mirrored = false;
    CarDamage damage{};
    std::shared_ptr<const VertexDisplacement> displacement;
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
    std::vector<CarShadow> shadows;
    std::optional<Highlight> highlight;
    DrawState state;
    // The instances of a track the draw draws in their order, or none for every node in its own.
    std::optional<std::vector<int>> instances;
    // The palette variant of the model when it was drawn.
    int variant = 0;
};

struct QuadDraw {
    std::size_t firstVertex;
    std::size_t quadCount;
    FlatMaterial material;
    Matrix4 projection;
    bool depthTest;
    DrawState state;
};

// The textures of images of a document by the image: each image, its palette variants, and its
// damaged images, the side of the car and the image at each level of damage after the first.
struct ImageSet {
    std::map<std::size_t, SDL_GPUTexture *> images;
    std::map<std::size_t, std::vector<SDL_GPUTexture *>> variants;
    std::map<std::size_t, std::pair<int, std::vector<SDL_GPUTexture *>>> damaged;

    void Release(SDL_GPUDevice *device) {
        for (auto &[image, texture] : images) {
            ReleaseGpuTexture(device, texture);
        }
        for (auto &[image, textures] : variants) {
            for (auto *texture : textures) {
                ReleaseGpuTexture(device, texture);
            }
        }
        for (auto &[image, levels] : damaged) {
            for (auto *texture : levels.second) {
                ReleaseGpuTexture(device, texture);
            }
        }
        *this = {};
    }
};

// The textures of an image a model draws: the image, its palette variants, and its damaged images,
// each none where the image has none.
struct ModelImage {
    SDL_GPUTexture *texture = nullptr;
    const std::vector<SDL_GPUTexture *> *variants = nullptr;
    const std::pair<int, std::vector<SDL_GPUTexture *>> *damaged = nullptr;
};

// How the images of a document become textures: the palette variants of the images of materials,
// the masks of the damage, and whether the textures are mipmapped.
struct ImageOptions {
    int variants = 0;
    std::function<bool(SDL_Palette *, int material, int variant)> paletteVariant;
    std::array<std::vector<std::uint8_t>, CAR_DAMAGE_LEVELS> damageMasks;
    bool mipmapped = false;
};

// A decoded image: its texels, the material whose palette the paint colours or -1, and its damage.
struct DecodedImage {
    SDL_Surface *surface = nullptr;
    int material = -1;
    std::optional<ImageDamage> damage;
};

// A copy of an image with a palette of its own, which changes without changing the image.
SDL_Surface *CopyImage(SDL_Surface *image) {
    SDL_Surface *copy = SDL_DuplicateSurface(image);
    SDL_Palette *palette = SDL_GetSurfacePalette(image);
    if (copy == nullptr || palette == nullptr) {
        return copy;
    }
    SDL_Palette *own = SDL_CreatePalette(palette->ncolors);
    if (own == nullptr || !SDL_SetPaletteColors(own, palette->colors, 0, palette->ncolors) ||
        !SDL_SetSurfacePalette(copy, own)) {
        SDL_DestroyPalette(own);
        SDL_DestroySurface(copy);
        return nullptr;
    }
    // The copy retains its reference to the palette.
    SDL_DestroyPalette(own);
    return copy;
}

// The texels of an image as 8-bit RGBA, row by row from the top, or none.
std::optional<std::vector<std::uint8_t>> ImageTexels(SDL_Surface *image, int &width, int &height) {
    SDL_Surface *rgba = SDL_ConvertSurface(image, SDL_PIXELFORMAT_RGBA32);
    if (rgba == nullptr) {
        return std::nullopt;
    }
    std::vector<std::uint8_t> texels;
    for (int y = 0; y < rgba->h; ++y) {
        auto *row = static_cast<std::uint8_t *>(rgba->pixels) + y * rgba->pitch;
        texels.insert(texels.end(), row, row + rgba->w * 4);
    }
    width = rgba->w;
    height = rgba->h;
    SDL_DestroySurface(rgba);
    return texels;
}

// Add the textures of an image to a set: the image, its palette variants where it is the image of
// a material whose palette a variant changes, and its damaged images. The image is not changed.
bool AddImage(Uploader &upload,
              ImageSet &set,
              std::size_t index,
              SDL_Surface *image,
              const DecodedImage &decoded,
              const ImageOptions &options) {
    int width = 0;
    int height = 0;
    if (options.paletteVariant && decoded.material >= 0 &&
        SDL_GetSurfacePalette(image) != nullptr) {
        std::vector<SDL_GPUTexture *> textures;
        for (int variant = 0; variant < options.variants; ++variant) {
            SDL_Surface *copy = CopyImage(image);
            if (copy == nullptr ||
                !options.paletteVariant(SDL_GetSurfacePalette(copy), decoded.material, variant)) {
                SDL_DestroySurface(copy);
                break;
            }
            auto texels = ImageTexels(copy, width, height);
            SDL_DestroySurface(copy);
            if (!texels) {
                break;
            }
            textures.push_back(upload.Texture(width, height, *texels, options.mipmapped));
        }
        if (!textures.empty()) {
            set.variants[index] = std::move(textures);
        }
    }
    auto texels = ImageTexels(image, width, height);
    if (!texels) {
        return false;
    }
    if (decoded.damage) {
        std::vector<SDL_GPUTexture *> levels;
        for (const auto &mask : options.damageMasks) {
            levels.push_back(
                upload.Texture(width,
                               height,
                               DamageTexels(*texels, width, height, mask, *decoded.damage),
                               options.mipmapped));
        }
        set.damaged[index] = {decoded.damage->side, std::move(levels)};
    }
    set.images[index] = upload.Texture(width, height, *texels, options.mipmapped);
    return true;
}

// A glTF document on the GPU, which every model of its parts shares.
struct DocumentData {
    SDL_GPUDevice *device = nullptr;
    std::filesystem::path path;
    nlohmann::json document;
    std::vector<SDL_GPUSampler *> samplers;
    // Whether the shader filters the texels of the images as the RDP does.
    bool threePoint = false;
    std::vector<std::pair<std::size_t, std::size_t>> textures; // Image and sampler.
    std::vector<Mesh> meshes;
    // The positions of the vertices of each mesh, for the bounds of a model.
    std::vector<std::vector<std::array<float, 3>>> meshPositions;
    std::vector<VertexAnimationInfo> animations;
    ImageOptions imageOptions;
    // The textures of the images the paint does not colour.
    ImageSet plain;
    // The decoded images of the materials, which the paint colours.
    std::map<std::size_t, DecodedImage> materials;
    // The textures of the images of the materials in each colour, or unpainted under none.
    std::map<std::optional<std::array<std::uint8_t, 3>>, ImageSet> painted;

    DocumentData() = default;
    DocumentData(const DocumentData &) = delete;
    DocumentData &operator=(const DocumentData &) = delete;

    // The images of the materials in a colour, painted and uploaded the first time, or none when
    // they were not uploaded.
    const ImageSet *Painted(const std::optional<std::array<std::uint8_t, 3>> &paint) {
        if (auto found = painted.find(paint); found != painted.end()) {
            return &found->second;
        }
        Uploader upload(device);
        ImageSet set;
        for (const auto &[index, decoded] : materials) {
            SDL_Surface *copy = CopyImage(decoded.surface);
            if (copy != nullptr && paint) {
                PaintPalette(SDL_GetSurfacePalette(copy), *paint, decoded.material);
            }
            bool added =
                copy != nullptr && AddImage(upload, set, index, copy, decoded, imageOptions);
            SDL_DestroySurface(copy);
            if (!added) {
                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                             "Image %zu of %s cannot be painted.",
                             index,
                             path.string().c_str());
                set.Release(device);
                return nullptr;
            }
        }
        if (!upload.Submit()) {
            SDL_LogError(SDL_LOG_CATEGORY_GPU, "The paint was not uploaded: %s", SDL_GetError());
            set.Release(device);
            return nullptr;
        }
        return &painted.emplace(paint, std::move(set)).first->second;
    }

    ~DocumentData() {
        plain.Release(device);
        for (auto &[paint, set] : painted) {
            set.Release(device);
        }
        for (auto &[index, decoded] : materials) {
            SDL_DestroySurface(decoded.surface);
        }
        for (auto &mesh : meshes) {
            for (auto &primitive : mesh.primitives) {
                ReleaseGpuBuffer(device, primitive.vertices);
                ReleaseGpuBuffer(device, primitive.indices);
            }
            if (mesh.targets != nullptr) {
                ReleaseGpuBuffer(device, mesh.targets);
            }
        }
        for (auto *sampler : samplers) {
            SDL_ReleaseGPUSampler(device, sampler);
        }
    }
};

} // namespace

struct RenderDocument::State {
    DocumentData data;
};

RenderDocument::RenderDocument(std::unique_ptr<State> state) : m_state(std::move(state)) {
}

RenderDocument::~RenderDocument() = default;

struct RenderModel::State {
    std::shared_ptr<RenderDocument> owner;
    DocumentData *document = nullptr;
    // The images of the materials in the colour of the model.
    const ImageSet *painted = nullptr;
    std::vector<Placed> placed;
    // The nodes of each instance of a track by the instance plus 1, the first the nodes of no
    // instance.
    std::vector<std::vector<std::size_t>> placedByInstance;
    // The textures of each image of the document, resolved once from the images of the colour of
    // the model and those the paint does not colour.
    std::vector<ModelImage> images;
    std::array<float, 3> centre{};
    float radius = 1.0f;
    // Whether the caller of the display lists sets one cycle.
    bool oneCycle = false;
    // Whether every part reflects the sky with the combiner of the reflection.
    bool reflects = false;
    // Whether the model draws the palette variants of the images, and the variant it draws.
    bool varied = false;
    int variant = 0;

    // Resolve the textures of every image, an image of the colour of the model before one the
    // paint does not colour.
    void ResolveImages() {
        std::size_t count = 0;
        auto grow = [&count](const auto &map) {
            if (!map.empty()) {
                count = std::max(count, map.rbegin()->first + 1);
            }
        };
        const std::array<const ImageSet *, 2> sets{&document->plain, painted};
        for (const ImageSet *set : sets) {
            if (set != nullptr) {
                grow(set->images);
                grow(set->variants);
                grow(set->damaged);
            }
        }
        images.assign(count, {});
        for (const ImageSet *set : sets) {
            if (set == nullptr) {
                continue;
            }
            for (const auto &[image, texture] : set->images) {
                images[image].texture = texture;
            }
            for (const auto &[image, textures] : set->variants) {
                images[image].variants = varied ? &textures : nullptr;
            }
            for (const auto &[image, levels] : set->damaged) {
                images[image].damaged = &levels;
            }
        }
    }

    // The textures of an image, or none.
    const ModelImage *Image(std::size_t image) const {
        return image < images.size() ? &images[image] : nullptr;
    }
};

struct SceneRenderer::State {
    // The time of the frame the animated textures show, in milliseconds.
    std::uint32_t textureTime = 0;
    // Whether the textures of the snow show their first frame.
    bool snow = false;
    // Whether the colours and the still textures of the snow off the coastline are drawn.
    bool snowTint = false;
    // The TRACK_INSTANCE_ flags of the instances the frame does not draw.
    std::uint32_t hiddenFlags = 0;
    // The matrices of the instances of a track as the race moves them, or none.
    std::function<const float *(int)> instanceMatrix;
    // The fog the models of the frame are drawn in, or none.
    std::optional<Fog> fog;
    bool mirrored = false;
    std::optional<SDL_Rect> view;
    bool ghost = false;
    bool inOrder = false;
    CarDamage damage{};
    std::shared_ptr<const VertexDisplacement> displacement;
    GpuWindow *window = nullptr;
    SDL_GPUDevice *device = nullptr;
    SDL_GPUShader *vertexShader = nullptr;
    SDL_GPUShader *fragmentShader = nullptr;
    std::map<PipelineKey, SDL_GPUGraphicsPipeline *> pipelines;
    SDL_GPUTexture *depth = nullptr;
    // The samples of each pixel, and the target of that many the frame resolves from when there
    // are more than one.
    SDL_GPUSampleCount sampleCount = SDL_GPU_SAMPLECOUNT_1;
    SDL_GPUTexture *multisampled = nullptr;
    bool alphaToCoverage = false;
    Uint32 depthWidth = 0;
    Uint32 depthHeight = 0;
    SDL_GPUTextureFormat depthFormat = SDL_GPU_TEXTUREFORMAT_D16_UNORM;
    SDL_GPUTexture *white = nullptr;
    SDL_GPUSampler *plainSampler = nullptr;
    SDL_GPUBuffer *noTargets = nullptr;
    // The vertices of the flat draws, and the indices of their quads, which grow as they need.
    SDL_GPUBuffer *flatVertices = nullptr;
    std::size_t flatVertexCapacity = 0;
    SDL_GPUBuffer *quadIndices = nullptr;
    std::size_t quadCapacity = 0;
    // The upload of the flat draws, kept from frame to frame. Mapping it cycles past an upload the
    // GPU still reads.
    SDL_GPUTransferBuffer *flatTransfer = nullptr;
    std::size_t flatTransferCapacity = 0;
    // The draws of the frame, in order, and the vertices of its flat draws.
    std::array<float, 4> clear{};
    std::vector<std::variant<ModelDraw, QuadDraw>> draws;
    std::vector<Vertex> flat;

    // Give the depth, and the multisampled target, the size of the frame, which follows the window.
    bool MatchFrame() {
        if (depth != nullptr && depthWidth == window->FrameWidth() &&
            depthHeight == window->FrameHeight()) {
            return true;
        }
        SDL_GPUTextureCreateInfo info{};
        info.type = SDL_GPU_TEXTURETYPE_2D;
        info.format = depthFormat;
        info.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
        info.width = window->FrameWidth();
        info.height = window->FrameHeight();
        info.layer_count_or_depth = 1;
        info.num_levels = 1;
        info.sample_count = sampleCount;
        SDL_GPUTexture *createdDepth = CreateGpuTexture(device, &info);
        SDL_GPUTexture *createdColor = nullptr;
        if (sampleCount != SDL_GPU_SAMPLECOUNT_1) {
            info.format = GpuWindow::FrameFormat();
            info.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;
            createdColor = CreateGpuTexture(device, &info);
        }
        if (createdDepth == nullptr ||
            (sampleCount != SDL_GPU_SAMPLECOUNT_1 && createdColor == nullptr)) {
            SDL_LogError(SDL_LOG_CATEGORY_GPU, "The targets were not created: %s", SDL_GetError());
            for (auto *texture : {createdDepth, createdColor}) {
                if (texture != nullptr) {
                    ReleaseGpuTexture(device, texture);
                }
            }
            return false;
        }
        for (auto *texture : {depth, multisampled}) {
            if (texture != nullptr) {
                ReleaseGpuTexture(device, texture);
            }
        }
        depth = createdDepth;
        multisampled = createdColor;
        depthWidth = info.width;
        depthHeight = info.height;
        return true;
    }

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
        for (auto *texture : {depth, multisampled, white}) {
            if (texture != nullptr) {
                ReleaseGpuTexture(device, texture);
            }
        }
        if (plainSampler != nullptr) {
            SDL_ReleaseGPUSampler(device, plainSampler);
        }
        if (flatTransfer != nullptr) {
            ReleaseGpuTransferBuffer(device, flatTransfer);
        }
        for (auto *buffer : {noTargets, flatVertices, quadIndices}) {
            if (buffer != nullptr) {
                ReleaseGpuBuffer(device, buffer);
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
            target.blend_state.dst_color_blendfactor =
                key.additive ? SDL_GPU_BLENDFACTOR_ONE : SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
            target.blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
            target.blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
            target.blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
            target.blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
            // The render mode of a ghost in BRD3D adds the colour by itself to the frame.
            if (key.ghost) {
                target.blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_COLOR;
                target.blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
            }
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
        info.rasterizer_state.front_face =
            key.mirrored ? SDL_GPU_FRONTFACE_CLOCKWISE : SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
        info.depth_stencil_state.enable_depth_test = key.depth != DepthMode::None;
        info.depth_stencil_state.enable_depth_write = key.depth == DepthMode::TestAndWrite;
        info.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL;
        // BRD3D draws the highlight with the vertices of the parts under it, and their depths are
        // equal. The highlighted parts here are their own node, transformed without the turns of
        // the root, and their depths differ by a rounding that flickers across a near surface such
        // as the bonnet from the bumper. They are drawn a little nearer instead, where the depth is
        // less or equal.
        if (key.depth == DepthMode::Equal) {
            info.rasterizer_state.enable_depth_bias = true;
            info.rasterizer_state.depth_bias_constant_factor = -2.0f;
            info.rasterizer_state.depth_bias_slope_factor = -1.0f;
        }
        info.multisample_state.sample_count = sampleCount;
        info.multisample_state.enable_alpha_to_coverage = key.coverage;
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

    DrawState Current() const {
        return {view, fog, hiddenFlags, ghost, inOrder, mirrored, damage, displacement};
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
            ReleaseGpuBuffer(device, buffer);
        }
        capacity = std::max(bytes, capacity * 2);
        SDL_GPUBufferCreateInfo info{};
        info.usage = usage;
        info.size = static_cast<Uint32>(capacity);
        buffer = CreateGpuBuffer(device, &info);
        return buffer != nullptr;
    }

    // The upload of the flat draws, of at least a size, made again when it is too small.
    bool ReserveFlatTransfer(std::size_t bytes) {
        if (flatTransfer != nullptr && flatTransferCapacity >= bytes) {
            return true;
        }
        if (flatTransfer != nullptr) {
            ReleaseGpuTransferBuffer(device, flatTransfer);
        }
        flatTransferCapacity = std::max(bytes, flatTransferCapacity * 2);
        SDL_GPUTransferBufferCreateInfo info{};
        info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        info.size = static_cast<Uint32>(flatTransferCapacity);
        flatTransfer = CreateGpuTransferBuffer(device, &info);
        return flatTransfer != nullptr;
    }
};

RenderModel::RenderModel(std::unique_ptr<State> state) : m_state(std::move(state)) {
}

RenderModel::~RenderModel() = default;

const std::vector<VertexAnimationInfo> &RenderModel::Animations() const {
    return m_state->document->animations;
}

std::array<float, 3> RenderModel::Centre() const {
    return m_state->centre;
}

float RenderModel::Radius() const {
    return m_state->radius;
}

std::span<const std::array<float, 3>> RenderModel::Positions() const {
    if (m_state->placed.empty()) {
        return {};
    }
    std::size_t mesh = m_state->placed.front().mesh;
    return std::span(m_state->document->meshPositions[mesh])
        .first(m_state->document->meshes[mesh].firstVertices);
}

void RenderModel::SetVariant(int variant) {
    m_state->variant = variant;
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

std::shared_ptr<RenderDocument> RenderDocument::Load(SceneRenderer &renderer,
                                                     const std::filesystem::path &path,
                                                     const ModelOptions &options) {
    auto owned = std::make_unique<State>();
    DocumentData *state = &owned->data;
    state->device = renderer.m_state->device;
    state->path = path;
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
    // The paint sets are those of this document, whichever document the images come from.
    const nlohmann::json ownImages = images;
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

    // The textures of a model are mipmapped for anisotropic filtering of the modern filter, and
    // other textures are not.
    const TextureFilter filter = renderer.m_state->window->Filter();
    const int anisotropy = renderer.m_state->window->Anisotropy();
    const bool mipmapped = anisotropy > 0 && filter == TextureFilter::Modern;
    state->threePoint = filter == TextureFilter::N64;
    state->imageOptions.variants = options.variants;
    state->imageOptions.paletteVariant = options.paletteVariant;
    if (options.damageMasks != nullptr) {
        state->imageOptions.damageMasks = *options.damageMasks;
    }
    state->imageOptions.mipmapped = mipmapped;
    Uploader upload(device);
    for (std::size_t index = 0; index < images.size(); ++index) {
        const auto &image = images[index];
        auto file = imageDirectory / image["uri"].get<std::string>();
        SDL_Surface *loaded = SDL_LoadPNG(file.string().c_str());
        if (loaded == nullptr) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s cannot be read.", file.string().c_str());
            return nullptr;
        }
        DecodedImage decoded;
        decoded.material = image.value("extras", nlohmann::json::object())
                               .value("coastlinePaint", nlohmann::json::object())
                               .value("material", -1);
        int paintSet = ownImages[index]
                           .value("extras", nlohmann::json::object())
                           .value("coastlinePaint", nlohmann::json::object())
                           .value("set", -1);
        if (options.damage && options.damageMasks != nullptr && paintSet >= 0) {
            decoded.damage = options.damage(paintSet);
        }
        // The paint colours an image of a material for each colour as a model takes it.
        if (decoded.material >= 0) {
            decoded.surface = loaded;
            state->materials.emplace(index, decoded);
            continue;
        }
        bool added = AddImage(upload, state->plain, index, loaded, decoded, state->imageOptions);
        SDL_DestroySurface(loaded);
        if (!added) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s cannot be read.", file.string().c_str());
            return nullptr;
        }
    }
    for (const auto &sampler : document.value("samplers", nlohmann::json::array())) {
        SDL_GPUSamplerCreateInfo info{};
        info.min_filter =
            filter == TextureFilter::Modern ? SDL_GPU_FILTER_LINEAR : SDL_GPU_FILTER_NEAREST;
        info.mag_filter = info.min_filter;
        info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
        if (mipmapped) {
            info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
            info.max_lod = 1000.0f;
            info.enable_anisotropy = anisotropy > 1;
            info.max_anisotropy = static_cast<float>(anisotropy);
        }
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
    for (const auto &mesh : document["meshes"]) {
        Mesh &drawn = state->meshes.emplace_back();
        auto &positions = state->meshPositions.emplace_back();
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
            if (drawn.primitives.empty()) {
                drawn.firstVertices = vertices.size();
            }
            drawn.mostVertices = std::max(drawn.mostVertices, static_cast<int>(vertices.size()));
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
                    part.recorded = true;
                    part.twoCycle =
                        (rdp.value("otherModeH", 0u) >> CYCLE_TYPE_SHIFT & 3) == TWO_CYCLE;
                    part.textureGen = (rdp.value("geometryMode", 0u) & G_TEXTURE_GEN) != 0;
                    part.prim = Rgba(rdp["primColor"].get<std::uint32_t>());
                    part.env = Rgba(rdp["envColor"].get<std::uint32_t>());
                    if (rdp.contains("snowPrimColor")) {
                        part.snowPrim = Rgba(rdp["snowPrimColor"].get<std::uint32_t>());
                    }
                    if (rdp.contains("snowEnvColor")) {
                        part.snowEnv = Rgba(rdp["snowEnvColor"].get<std::uint32_t>());
                    }
                    if (rdp.contains("texel1Image")) {
                        part.texel1 = rdp["texel1Image"].get<std::size_t>();
                    }
                }
                // The frames of an animated texture of a track. A texture of the snow does not
                // animate. CopyTextureAndPaletteFromSegment() copies its first frame, which has no
                // time, over the texture of a race in the snow.
                if (material.contains("extras") &&
                    material["extras"].contains("coastlineAnimation")) {
                    const auto &animation = material["extras"]["coastlineAnimation"];
                    if (animation.value("snow", false)) {
                        part.snowImage = animation["frames"][0]["image"].get<std::size_t>();
                    } else {
                        part.snowStill = animation.value("snowStill", false);
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
    state->document = std::move(document);
    return std::shared_ptr<RenderDocument>(new RenderDocument(std::move(owned)));
}

std::unique_ptr<RenderModel> RenderModel::Load(SceneRenderer &renderer,
                                               const std::filesystem::path &path,
                                               const ModelOptions &options) {
    auto document = RenderDocument::Load(renderer, path, options);
    if (!document) {
        return nullptr;
    }
    return Load(std::move(document), options);
}

std::unique_ptr<RenderModel> RenderModel::Load(std::shared_ptr<RenderDocument> owner,
                                               const ModelOptions &options) {
    const auto &node = options.node;
    const auto &parts = options.parts;
    auto state = std::make_unique<State>();
    state->document = &owner->m_state->data;
    state->owner = std::move(owner);
    state->oneCycle = options.oneCycle;
    state->reflects = options.reflects;
    state->varied = options.variants > 0;
    const DocumentData &data = *state->document;
    const nlohmann::json &document = data.document;
    state->painted = state->document->Painted(options.paint);
    if (state->painted == nullptr) {
        return nullptr;
    }
    state->ResolveImages();

    // The nodes of the first scene and their places, and the bounds of what they draw.
    std::array<float, 3> low{INFINITY, INFINITY, INFINITY};
    std::array<float, 3> high{-INFINITY, -INFINITY, -INFINITY};
    auto visit = [&](auto &self, std::size_t index, const Matrix4 &parent) -> void {
        const auto &node = document["nodes"].at(index);
        Matrix4 matrix = MultiplyMatrices(parent, NodeMatrix(node));
        if (node.contains("mesh") && (!parts || parts(node.value("name", "")))) {
            std::size_t mesh = node["mesh"].get<std::size_t>();
            int instance = -1;
            std::uint32_t flags = 0;
            if (node.contains("extras") && node["extras"].contains("coastlineInstance")) {
                const auto &extras = node["extras"]["coastlineInstance"];
                flags = extras["flags"].get<std::uint32_t>();
                instance = extras.value("index", -1);
            }
            state->placed.push_back({mesh, matrix, instance, flags, parent});
            for (const auto &p : data.meshPositions[mesh]) {
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
        if (!options.optional) {
            SDL_LogError(
                SDL_LOG_CATEGORY_APPLICATION, "%s draws nothing.", data.path.string().c_str());
        }
        return nullptr;
    }
    for (std::size_t p = 0; p < state->placed.size(); ++p) {
        if (state->placed[p].instance < -1) {
            continue;
        }
        auto slot = static_cast<std::size_t>(state->placed[p].instance + 1);
        if (slot >= state->placedByInstance.size()) {
            state->placedByInstance.resize(slot + 1);
        }
        state->placedByInstance[slot].push_back(p);
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
    ReleaseGpuTexture(m_device, m_texture);
    SDL_ReleaseGPUSampler(m_device, m_sampler);
    if (m_transfer != nullptr) {
        ReleaseGpuTransferBuffer(m_device, m_transfer);
    }
}

bool RenderTexture::Update(std::span<const std::uint8_t> rgba) {
    auto bytes = static_cast<std::size_t>(m_width) * static_cast<std::size_t>(m_height) * 4;
    if (rgba.size() != bytes) {
        return false;
    }
    if (m_transfer == nullptr) {
        SDL_GPUTransferBufferCreateInfo info{};
        info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        info.size = static_cast<Uint32>(bytes);
        m_transfer = CreateGpuTransferBuffer(m_device, &info);
        if (m_transfer == nullptr) {
            return false;
        }
    }
    auto *mapped =
        static_cast<std::uint8_t *>(SDL_MapGPUTransferBuffer(m_device, m_transfer, true));
    if (mapped == nullptr) {
        return false;
    }
    std::memcpy(mapped, rgba.data(), bytes);
    SDL_UnmapGPUTransferBuffer(m_device, m_transfer);
    SDL_GPUCommandBuffer *commands = SDL_AcquireGPUCommandBuffer(m_device);
    if (commands == nullptr) {
        return false;
    }
    SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(commands);
    SDL_GPUTextureTransferInfo source{};
    source.transfer_buffer = m_transfer;
    source.pixels_per_row = static_cast<Uint32>(m_width);
    source.rows_per_layer = static_cast<Uint32>(m_height);
    SDL_GPUTextureRegion region{};
    region.texture = m_texture;
    region.w = static_cast<Uint32>(m_width);
    region.h = static_cast<Uint32>(m_height);
    region.d = 1;
    SDL_UploadToGPUTexture(copy, &source, &region, true);
    SDL_EndGPUCopyPass(copy);
    return SDL_SubmitGPUCommandBuffer(commands);
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
    // A filtered texture is filtered as TextureFilter chooses: by the sampler, by the shader as the
    // RDP filters, or not at all.
    TextureFilter filter = renderer.m_state->window->Filter();
    SDL_GPUSamplerCreateInfo info{};
    info.min_filter =
        linear && filter == TextureFilter::Modern ? SDL_GPU_FILTER_LINEAR : SDL_GPU_FILTER_NEAREST;
    info.mag_filter = info.min_filter;
    info.address_mode_u =
        repeat ? SDL_GPU_SAMPLERADDRESSMODE_REPEAT : SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    info.address_mode_v = info.address_mode_u;
    info.address_mode_w = info.address_mode_u;
    std::unique_ptr<RenderTexture> result(
        new RenderTexture(device, texture, SDL_CreateGPUSampler(device, &info), width, height));
    result->m_threePoint = linear && filter == TextureFilter::N64;
    return result;
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
    // The most samples Antialias allows that the device supports for both targets.
    for (auto [samples, count] : {std::pair{8, SDL_GPU_SAMPLECOUNT_8},
                                  {4, SDL_GPU_SAMPLECOUNT_4},
                                  {2, SDL_GPU_SAMPLECOUNT_2}}) {
        if (window.Antialias() >= samples &&
            SDL_GPUTextureSupportsSampleCount(device, GpuWindow::FrameFormat(), count) &&
            SDL_GPUTextureSupportsSampleCount(device, state->depthFormat, count)) {
            state->sampleCount = count;
            SDL_Log("Drawing %d samples of each pixel.", samples);
            break;
        }
    }
    state->alphaToCoverage =
        state->sampleCount != SDL_GPU_SAMPLECOUNT_1 && window.AlphaToCoverage();
    state->MatchFrame();
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
    m_state->snow = false;
    m_state->snowTint = false;
    m_state->hiddenFlags = 0;
    m_state->instanceMatrix = nullptr;
    m_state->fog.reset();
    m_state->mirrored = false;
    m_state->view.reset();
    m_state->ghost = false;
    m_state->inOrder = false;
    m_state->draws.clear();
    m_state->flat.clear();
}

void SceneRenderer::SetTextureTime(std::uint32_t milliseconds) {
    m_state->textureTime = milliseconds;
}

void SceneRenderer::SetSnow(bool snow, bool tint) {
    m_state->snow = snow;
    m_state->snowTint = snow && tint;
}

void SceneRenderer::SetHiddenInstanceFlags(std::uint32_t flags) {
    m_state->hiddenFlags = flags;
}

void SceneRenderer::SetInstanceMatrices(std::function<const float *(int)> matrix) {
    m_state->instanceMatrix = std::move(matrix);
}

void SceneRenderer::SetFog(std::optional<Fog> fog) {
    m_state->fog = fog;
}

void SceneRenderer::SetMirrored(bool mirrored) {
    m_state->mirrored = mirrored;
}

void SceneRenderer::SetViewport(std::optional<SDL_Rect> view) {
    m_state->view = view;
}

void SceneRenderer::SetGhost(bool ghost) {
    m_state->ghost = ghost;
}

void SceneRenderer::SetDamage(const CarDamage &damage) {
    m_state->damage = damage;
}

VertexDisplacement::VertexDisplacement(SDL_GPUDevice *device,
                                       SDL_GPUBuffer *buffer,
                                       int vertexCount)
    : m_device(device), m_buffer(buffer), m_vertexCount(vertexCount) {
}

VertexDisplacement::~VertexDisplacement() {
    ReleaseGpuBuffer(m_device, m_buffer);
}

std::shared_ptr<const VertexDisplacement>
SceneRenderer::CreateDisplacement(std::span<const std::array<float, 3>> offsets) {
    // The position and then the normal of each vertex, as vec4, as the morph targets have them.
    std::vector<float> displacements(offsets.size() * 8, 0.0f);
    for (std::size_t i = 0; i < offsets.size(); ++i) {
        std::copy(offsets[i].begin(), offsets[i].end(), displacements.begin() + i * 8);
    }
    Uploader upload(m_state->device);
    SDL_GPUBuffer *buffer =
        upload.Buffer(SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ,
                      {reinterpret_cast<const std::uint8_t *>(displacements.data()),
                       displacements.size() * sizeof(float)});
    if (buffer == nullptr || !upload.Submit()) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "A displacement was not uploaded: %s", SDL_GetError());
        ReleaseGpuBuffer(m_state->device, buffer);
        return nullptr;
    }
    return std::shared_ptr<const VertexDisplacement>(
        new VertexDisplacement(m_state->device, buffer, static_cast<int>(offsets.size())));
}

void SceneRenderer::SetDisplacement(std::shared_ptr<const VertexDisplacement> displacement) {
    m_state->displacement = std::move(displacement);
}

void SceneRenderer::SetDrawnInOrder(bool inOrder) {
    m_state->inOrder = inOrder;
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
                                          {},
                                          std::nullopt,
                                          m_state->Current(),
                                          std::nullopt,
                                          model.m_state->variant});
}

void SceneRenderer::DrawReflectingModel(const RenderModel &model,
                                        const Matrix4 &world,
                                        const Matrix4 &viewProjection,
                                        const Lighting &lighting,
                                        const Reflection &reflection,
                                        std::optional<SDL_Rect> scissor) {
    m_state->draws.emplace_back(ModelDraw{&model,
                                          world,
                                          viewProjection,
                                          lighting,
                                          {},
                                          true,
                                          scissor,
                                          reflection,
                                          {},
                                          std::nullopt,
                                          m_state->Current(),
                                          std::nullopt,
                                          model.m_state->variant});
}

void SceneRenderer::DrawShadowedModel(const RenderModel &model,
                                      const Matrix4 &world,
                                      const Matrix4 &viewProjection,
                                      const Lighting &lighting,
                                      std::vector<int> instances,
                                      std::vector<CarShadow> shadows) {
    m_state->draws.emplace_back(ModelDraw{&model,
                                          world,
                                          viewProjection,
                                          lighting,
                                          {},
                                          true,
                                          std::nullopt,
                                          std::nullopt,
                                          std::move(shadows),
                                          std::nullopt,
                                          m_state->Current(),
                                          std::move(instances),
                                          model.m_state->variant});
}

void SceneRenderer::DrawHighlight(const RenderModel &model,
                                  const Matrix4 &world,
                                  const Matrix4 &viewProjection,
                                  const Highlight &highlight) {
    m_state->draws.emplace_back(ModelDraw{&model,
                                          world,
                                          viewProjection,
                                          {},
                                          {},
                                          true,
                                          std::nullopt,
                                          std::nullopt,
                                          {},
                                          highlight,
                                          m_state->Current(),
                                          std::nullopt,
                                          model.m_state->variant});
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
        QuadDraw{first, corners.size() / 4, material, projection, depthTest, m_state->Current()});
}

void SceneRenderer::End(const std::filesystem::path *capture) {
    State &s = *m_state;
    s.MatchFrame();
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
            if (!s.ReserveFlatTransfer(vertexBytes + indexBytes)) {
                SDL_LogError(
                    SDL_LOG_CATEGORY_GPU, "The flat draws have no upload: %s", SDL_GetError());
                s.flat.clear();
            } else {
                auto *mapped = static_cast<std::uint8_t *>(
                    SDL_MapGPUTransferBuffer(s.device, s.flatTransfer, true));
                std::memcpy(mapped, s.flat.data(), vertexBytes);
                std::memcpy(mapped + vertexBytes, indices.data(), indexBytes);
                SDL_UnmapGPUTransferBuffer(s.device, s.flatTransfer);
                SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(commands);
                SDL_GPUTransferBufferLocation from{s.flatTransfer, 0};
                SDL_GPUBufferRegion to{s.flatVertices, 0, static_cast<Uint32>(vertexBytes)};
                SDL_UploadToGPUBuffer(copy, &from, &to, true);
                if (indexBytes > 0) {
                    from.offset = static_cast<Uint32>(vertexBytes);
                    to = {s.quadIndices, 0, static_cast<Uint32>(indexBytes)};
                    SDL_UploadToGPUBuffer(copy, &from, &to, false);
                }
                SDL_EndGPUCopyPass(copy);
            }
        }
    }

    SDL_GPUColorTargetInfo color{};
    color.texture = s.window->FrameTexture();
    color.clear_color = {s.clear[0], s.clear[1], s.clear[2], s.clear[3]};
    color.load_op = SDL_GPU_LOADOP_CLEAR;
    color.store_op = SDL_GPU_STOREOP_STORE;
    // The samples resolve into the frame at the end of the pass.
    if (s.multisampled != nullptr) {
        color.resolve_texture = color.texture;
        color.texture = s.multisampled;
        color.store_op = SDL_GPU_STOREOP_RESOLVE;
    }
    SDL_GPUDepthStencilTargetInfo depth{};
    depth.texture = s.depth;
    depth.clear_depth = 1.0f;
    depth.load_op = SDL_GPU_LOADOP_CLEAR;
    depth.store_op = SDL_GPU_STOREOP_DONT_CARE;
    SDL_GPURenderPass *pass = SDL_BeginGPURenderPass(commands, &color, 1, &depth);

    // The state last bound in the pass, which a draw binds again only where it differs. The
    // uniforms pushed to a slot of the command buffer remain through a change of the pipeline.
    struct Bound {
        SDL_GPUGraphicsPipeline *pipeline = nullptr;
        std::array<SDL_GPUTextureSamplerBinding, 3> samplers{};
        SDL_GPUBuffer *targets = nullptr;
        SDL_GPUBuffer *vertices = nullptr;
        SDL_GPUBuffer *indices = nullptr;
        std::optional<VertexUniforms> vertexUniforms;
        std::optional<FragmentUniforms> fragmentUniforms;
    } bound;
    auto bindPipeline = [&](SDL_GPUGraphicsPipeline *pipeline) {
        if (pipeline != bound.pipeline) {
            SDL_BindGPUGraphicsPipeline(pass, pipeline);
            bound.pipeline = pipeline;
        }
    };
    auto pushUniforms = [&](const VertexUniforms &vertexUniforms,
                            const FragmentUniforms &fragmentUniforms) {
        if (!bound.vertexUniforms ||
            std::memcmp(&*bound.vertexUniforms, &vertexUniforms, sizeof(vertexUniforms)) != 0) {
            SDL_PushGPUVertexUniformData(commands, 0, &vertexUniforms, sizeof(vertexUniforms));
            bound.vertexUniforms = vertexUniforms;
        }
        if (!bound.fragmentUniforms ||
            std::memcmp(&*bound.fragmentUniforms, &fragmentUniforms, sizeof(fragmentUniforms)) !=
                0) {
            SDL_PushGPUFragmentUniformData(
                commands, 0, &fragmentUniforms, sizeof(fragmentUniforms));
            bound.fragmentUniforms = fragmentUniforms;
        }
    };
    // Bind the samplers from the first slot that differs to the last, and the buffers that differ.
    auto bindResources = [&](const std::array<SDL_GPUTextureSamplerBinding, 3> &samplers,
                             SDL_GPUBuffer *targets,
                             SDL_GPUBuffer *vertices,
                             SDL_GPUBuffer *indices) {
        auto same = [](const SDL_GPUTextureSamplerBinding &a,
                       const SDL_GPUTextureSamplerBinding &b) {
            return a.texture == b.texture && a.sampler == b.sampler;
        };
        std::size_t first = 0;
        while (first < samplers.size() && same(samplers[first], bound.samplers[first])) {
            ++first;
        }
        std::size_t last = samplers.size();
        while (last > first && same(samplers[last - 1], bound.samplers[last - 1])) {
            --last;
        }
        if (first < last) {
            SDL_BindGPUFragmentSamplers(pass,
                                        static_cast<Uint32>(first),
                                        samplers.data() + first,
                                        static_cast<Uint32>(last - first));
            std::copy(
                samplers.begin() + first, samplers.begin() + last, bound.samplers.begin() + first);
        }
        if (targets != bound.targets) {
            SDL_BindGPUVertexStorageBuffers(pass, 0, &targets, 1);
            bound.targets = targets;
        }
        if (vertices != bound.vertices) {
            SDL_GPUBufferBinding binding{vertices, 0};
            SDL_BindGPUVertexBuffers(pass, 0, &binding, 1);
            bound.vertices = vertices;
        }
        if (indices != bound.indices) {
            SDL_GPUBufferBinding binding{indices, 0};
            SDL_BindGPUIndexBuffer(pass, &binding, SDL_GPU_INDEXELEMENTSIZE_32BIT);
            bound.indices = indices;
        }
    };

    // The view, the fog, and the hidden instances of the draw being made.
    SDL_Rect whole{
        0, 0, static_cast<int>(s.window->FrameWidth()), static_cast<int>(s.window->FrameHeight())};
    const DrawState *drawState = nullptr;
    // Fill the view of a draw and cut it to its scissor within the view, or report that nothing of
    // it shows.
    auto useView = [&](const DrawState &state, const std::optional<SDL_Rect> &scissor) {
        drawState = &state;
        SDL_Rect view = state.view.value_or(whole);
        SDL_GPUViewport viewport{static_cast<float>(view.x),
                                 static_cast<float>(view.y),
                                 static_cast<float>(view.w),
                                 static_cast<float>(view.h),
                                 0.0f,
                                 1.0f};
        SDL_SetGPUViewport(pass, &viewport);
        SDL_Rect cut = view;
        if (scissor && !SDL_GetRectIntersection(&view, &*scissor, &cut)) {
            return false;
        }
        SDL_SetGPUScissor(pass, &cut);
        return true;
    };

    auto drawPrimitive = [&](const Primitive &primitive,
                             const RenderModel::State &model,
                             int variant,
                             const VertexUniforms &vertexUniforms,
                             const Lighting &lighting,
                             SDL_GPUBuffer *targets,
                             bool depthTest,
                             const Reflection *reflection,
                             std::span<const CarShadow *const> shadows) {
        bool ghost = drawState->ghost;
        // An opaque primitive whose texels are cut by their alpha covers the samples of a pixel by
        // its alpha.
        bool coverage = s.alphaToCoverage && !primitive.blend && !ghost && primitive.alphaCut >= 0;
        SDL_GPUGraphicsPipeline *pipeline =
            s.Pipeline({primitive.blend || ghost,
                        primitive.twoSided,
                        !depthTest               ? DepthMode::None :
                        primitive.blend || ghost ? DepthMode::Test :
                                                   DepthMode::TestAndWrite,
                        false,
                        drawState->mirrored,
                        ghost,
                        coverage});
        if (pipeline == nullptr) {
            return;
        }
        bindPipeline(pipeline);
        const std::array<float, 4> &prim =
            s.snowTint && primitive.snowPrim ? *primitive.snowPrim : primitive.prim;
        const std::array<float, 4> &env =
            s.snowTint && primitive.snowEnv ? *primitive.snowEnv : primitive.env;
        FragmentUniforms fragmentUniforms{
            {primitive.alphaCut,
             primitive.lit ? 1.0f : 0.0f,
             primitive.texture || reflection != nullptr ? 1.0f : 0.0f,
             primitive.combine ? 1.0f : 0.0f},
            {lighting.direction[0],
             lighting.direction[1],
             lighting.direction[2],
             coverage ? 1.0f : 0.0f},
            // The reflection combines its one texture alike in both cycles, which the second cycle
            // reads as the second texel.
            {primitive.combine ? (*primitive.combine)[0] : 0u,
             primitive.combine ? (*primitive.combine)[1] : 0u,
             primitive.recorded && (!model.oneCycle || primitive.twoCycle) &&
                     reflection == nullptr ?
                 1u :
                 0u,
             0u},
            {prim[0], prim[1], prim[2], prim[3]},
            {env[0], env[1], env[2], env[3]},
            {lighting.ambient[0], lighting.ambient[1], lighting.ambient[2], 1.0f},
            {lighting.directional[0], lighting.directional[1], lighting.directional[2], 1.0f},
            {1.0f, 1.0f, 0.0f, 0.0f},
            {},
            {},
            {}};
        if (reflection != nullptr) {
            std::ranges::copy(reflection->environment, fragmentUniforms.env);
        }
        if (drawState->fog) {
            std::ranges::copy(drawState->fog->color, fragmentUniforms.fogColor);
            fragmentUniforms.fogColor[3] = drawState->fog->range;
        }
        for (std::size_t i = 0; i < shadows.size() && i < MAX_CAR_SHADOWS; ++i) {
            const CarShadow &shadow = *shadows[i];
            float length = std::hypot(shadow.forward[0], shadow.forward[1]);
            fragmentUniforms.shadowCenter[i][0] = shadow.x;
            fragmentUniforms.shadowCenter[i][1] = shadow.y;
            fragmentUniforms.shadowCenter[i][2] = shadow.along;
            fragmentUniforms.shadowCenter[i][3] = shadow.across;
            fragmentUniforms.shadowForward[i][0] =
                length > 0.0f ? shadow.forward[0] / length : 1.0f;
            fragmentUniforms.shadowForward[i][1] =
                length > 0.0f ? shadow.forward[1] / length : 0.0f;
            fragmentUniforms.shadowForward[i][2] = 1.0f;
            std::ranges::copy(shadow.color, fragmentUniforms.shadowColor[i]);
        }
        std::array<SDL_GPUTextureSamplerBinding, 3> bindings{
            {{s.white, s.plainSampler}, {s.white, s.plainSampler}, {s.white, s.plainSampler}}};
        // The textures the shader filters as the RDP does, by their bits. The shadows of every car
        // take the one texture.
        std::uint32_t threePoint = 0;
        if (!shadows.empty()) {
            bindings[2] = {shadows[0]->texture->m_texture, shadows[0]->texture->m_sampler};
            threePoint |= shadows[0]->texture->m_threePoint ? 4u : 0u;
        }
        const DocumentData &document = *model.document;
        if (primitive.texture && *primitive.texture < document.textures.size()) {
            threePoint |= document.threePoint ? 1u : 0u;
            threePoint |= document.threePoint && primitive.texel1 ? 2u : 0u;
            auto [image, sampler] = document.textures[*primitive.texture];
            if (s.snowTint && primitive.snowStill && !primitive.frames.empty()) {
                image = primitive.frames.front().second;
            } else {
                image = AnimationFrame(primitive.frames, s.textureTime).value_or(image);
            }
            if (s.snow && primitive.snowImage) {
                image = *primitive.snowImage;
            }
            bindings[0].sampler = document.samplers.at(sampler);
            const ModelImage *resolved = model.Image(image);
            if (resolved != nullptr && resolved->texture != nullptr) {
                bindings[0].texture = resolved->texture;
            }
            const auto *varied = resolved != nullptr ? resolved->variants : nullptr;
            if (varied != nullptr && variant >= 0 &&
                static_cast<std::size_t>(variant) < varied->size()) {
                bindings[0].texture = (*varied)[static_cast<std::size_t>(variant)];
            }
            const auto *damaged = resolved != nullptr ? resolved->damaged : nullptr;
            if (damaged != nullptr) {
                int level = drawState->damage[static_cast<std::size_t>(damaged->first)];
                if (level > 0 && static_cast<std::size_t>(level) <= damaged->second.size()) {
                    bindings[0].texture = damaged->second[static_cast<std::size_t>(level - 1)];
                }
            }
            const ModelImage *texel1 = primitive.texel1 ? model.Image(*primitive.texel1) : nullptr;
            if (texel1 != nullptr && texel1->texture != nullptr) {
                bindings[1] = {texel1->texture, document.samplers.at(sampler)};
            }
        }
        if (reflection != nullptr) {
            bindings[0] = {reflection->texture->m_texture, reflection->texture->m_sampler};
            threePoint = (threePoint & ~1u) | (reflection->texture->m_threePoint ? 1u : 0u);
        }
        fragmentUniforms.combine[3] = threePoint;
        pushUniforms(vertexUniforms, fragmentUniforms);
        bindResources(bindings, targets, primitive.vertices, primitive.indices);
        SDL_DrawGPUIndexedPrimitives(pass, primitive.indexCount, 1, 0, 0, 0);
    };

    // The render mode 4 of the PC release: the colour by its alpha added to the frame where the
    // depth is equal, a texel of alpha below 8 drawing nothing.
    auto drawHighlight = [&](const Primitive &primitive,
                             const VertexUniforms &vertexUniforms,
                             SDL_GPUBuffer *targets,
                             const Highlight &highlight) {
        SDL_GPUGraphicsPipeline *pipeline =
            s.Pipeline({true, primitive.twoSided, DepthMode::Equal, true, drawState->mirrored});
        if (pipeline == nullptr || highlight.texture == nullptr) {
            return;
        }
        bindPipeline(pipeline);
        FragmentUniforms fragmentUniforms{{8.0f / 255.0f, 0.0f, 1.0f, 1.0f},
                                          {0.0f, 0.0f, 1.0f, 0.0f},
                                          {highlight.combine[0],
                                           highlight.combine[1],
                                           0u,
                                           highlight.texture->m_threePoint ? 1u : 0u},
                                          {highlight.primitive[0],
                                           highlight.primitive[1],
                                           highlight.primitive[2],
                                           highlight.primitive[3]},
                                          {1.0f, 1.0f, 1.0f, 1.0f},
                                          {1.0f, 1.0f, 1.0f, 1.0f},
                                          {0.0f, 0.0f, 0.0f, 1.0f},
                                          {1.0f, 1.0f, 0.0f, 0.0f},
                                          {},
                                          {},
                                          {}};
        if (drawState->fog) {
            std::ranges::copy(drawState->fog->color, fragmentUniforms.fogColor);
            fragmentUniforms.fogColor[3] = drawState->fog->range;
        }
        pushUniforms(vertexUniforms, fragmentUniforms);
        std::array<SDL_GPUTextureSamplerBinding, 3> bindings{
            {{highlight.texture->m_texture, highlight.texture->m_sampler},
             {s.white, s.plainSampler},
             {s.white, s.plainSampler}}};
        bindResources(bindings, targets, primitive.vertices, primitive.indices);
        SDL_DrawGPUIndexedPrimitives(pass, primitive.indexCount, 1, 0, 0, 0);
    };

    // Each model draws its opaque primitives where it comes, and the blended ones of every model
    // follow every opaque one. A model drawn in order draws its blended primitives straight after
    // its opaque ones and before the next draw, as DrawModelList() draws the translucent parts of a
    // model after its opaque ones. A flat draw comes after the models drawn before it.
    enum class Pass { Opaque, Blended };
    auto drawModel = [&](const ModelDraw &draw, Pass drawn) {
        const RenderModel::State &model = *draw.model->m_state;
        if (!useView(draw.state, draw.scissor)) {
            return;
        }
        // The nodes of the model in their order, or those of the instances of a draw of
        // instances in the order of the instances.
        std::vector<std::size_t> order;
        if (draw.instances) {
            for (int instance : *draw.instances) {
                auto slot = static_cast<std::size_t>(instance + 1);
                if (instance >= -1 && slot < model.placedByInstance.size()) {
                    const auto &nodes = model.placedByInstance[slot];
                    order.insert(order.end(), nodes.begin(), nodes.end());
                }
            }
        } else {
            order.resize(model.placed.size());
            std::iota(order.begin(), order.end(), std::size_t{0});
        }
        for (std::size_t node : order) {
            const Placed &placed = model.placed[node];
            if (placed.flags & draw.state.hiddenFlags) {
                continue;
            }
            const Mesh &mesh = model.document->meshes[placed.mesh];
            Matrix4 local = placed.matrix;
            const float *moved = placed.instance >= 0 && s.instanceMatrix ?
                                     s.instanceMatrix(placed.instance) :
                                     nullptr;
            if (moved != nullptr) {
                Matrix4 instance{};
                std::copy_n(moved, instance.size(), instance.begin());
                local = MultiplyMatrices(placed.parent, instance);
            }
            VertexUniforms vertexUniforms{draw.viewProjection,
                                          MultiplyMatrices(draw.world, local),
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
            } else if (draw.state.displacement &&
                       mesh.mostVertices <= draw.state.displacement->m_vertexCount) {
                // The displacement is a single target that the one pose takes in full.
                vertexUniforms.morph = {0, 0, draw.state.displacement->m_vertexCount, 1};
                targets = draw.state.displacement->m_buffer;
            }
            Lighting lighting = TrackInstanceLighting(draw.lighting, placed.flags);
            // The shadow of each car falls on the instances near it.
            std::vector<const CarShadow *> shaded;
            for (const CarShadow &shadow : draw.shadows) {
                if (shadow.texture != nullptr && placed.instance >= 0 &&
                    std::ranges::contains(shadow.instances, placed.instance) &&
                    shaded.size() < MAX_CAR_SHADOWS) {
                    shaded.push_back(&shadow);
                }
            }
            for (const Primitive &part : mesh.primitives) {
                Primitive reflecting;
                if (model.reflects) {
                    reflecting = part;
                    reflecting.textureGen = true;
                    reflecting.combine = REFLECTION_COMBINE;
                }
                const Primitive &primitive = model.reflects ? reflecting : part;
                if (draw.highlight) {
                    // The highlight adds to the opaque parts drawn before it, in their place.
                    if (drawn != Pass::Blended) {
                        VertexUniforms uniforms = vertexUniforms;
                        uniforms.blend[2] = 2.0f;
                        uniforms.reflectAcross = {draw.highlight->across[0],
                                                  draw.highlight->across[1],
                                                  draw.highlight->across[2],
                                                  draw.highlight->s};
                        uniforms.reflectUp = {draw.highlight->up[0],
                                              draw.highlight->up[1],
                                              draw.highlight->up[2],
                                              draw.highlight->t};
                        drawHighlight(primitive, uniforms, targets, *draw.highlight);
                    }
                    continue;
                }
                // Every part of a ghost waits for the blended draws.
                if ((primitive.blend || draw.state.ghost) == (drawn == Pass::Blended)) {
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
                        uniforms.reflectUp = {reflection->up[0],
                                              reflection->up[1],
                                              reflection->up[2],
                                              reflection->linear ? 1.0f : 0.0f};
                    }
                    drawPrimitive(primitive,
                                  model,
                                  draw.variant,
                                  uniforms,
                                  lighting,
                                  targets,
                                  draw.depth,
                                  reflection,
                                  shaded);
                }
            }
        }
    };
    std::vector<const ModelDraw *> pending;
    auto flushBlended = [&]() {
        for (const ModelDraw *draw : pending) {
            drawModel(*draw, Pass::Blended);
        }
        pending.clear();
    };
    for (const auto &entry : s.draws) {
        const auto *draw = std::get_if<ModelDraw>(&entry);
        if (draw != nullptr && draw->state.inOrder) {
            flushBlended();
            drawModel(*draw, Pass::Opaque);
            drawModel(*draw, Pass::Blended);
            continue;
        }
        if (draw != nullptr) {
            drawModel(*draw, Pass::Opaque);
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
        bindPipeline(pipeline);
        useView(quad.state, std::nullopt);
        VertexUniforms vertexUniforms{
            quad.projection, IDENTITY_MATRIX, {0, 0, 0, 0}, {0.0f, 0.0f, 0.0f, 0.0f}, {}, {}, {}};
        const FlatMaterial &material = quad.material;
        bool combined = material.combine[0] != 0 || material.combine[1] != 0;
        FragmentUniforms fragmentUniforms{
            {material.alphaCut,
             0.0f,
             material.texel0 != nullptr ? 1.0f : 0.0f,
             combined ? 1.0f : 0.0f},
            {0.0f, 0.0f, 1.0f, 0.0f},
            {material.combine[0],
             material.combine[1],
             material.twoCycle ? 1u : 0u,
             (material.texel0 != nullptr && material.texel0->m_threePoint ? 1u : 0u) |
                 (material.texel1 != nullptr && material.texel1->m_threePoint ? 2u : 0u)},
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
        pushUniforms(vertexUniforms, fragmentUniforms);
        std::array<SDL_GPUTextureSamplerBinding, 3> bindings{
            {{s.white, s.plainSampler}, {s.white, s.plainSampler}, {s.white, s.plainSampler}}};
        if (material.texel0 != nullptr) {
            bindings[0] = {material.texel0->m_texture, material.texel0->m_sampler};
        }
        if (material.texel1 != nullptr) {
            bindings[1] = {material.texel1->m_texture, material.texel1->m_sampler};
        }
        bindResources(bindings, s.noTargets, s.flatVertices, s.quadIndices);
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
