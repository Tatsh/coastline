#include "gpu.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <span>
#include <unordered_map>

#include "shaders/fxaa_frag_msl.h"
#include "shaders/fxaa_frag_spirv.h"
#include "shaders/post_vert_msl.h"
#include "shaders/post_vert_spirv.h"

namespace coastline {

namespace {

constexpr SDL_GPUTextureFormat FRAME_FORMAT = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;

// A shader of a pass after the scene, in SPIR-V or MSL as the driver takes.
SDL_GPUShader *CreatePostShader(SDL_GPUDevice *device,
                                SDL_GPUShaderStage stage,
                                Uint32 samplers,
                                Uint32 uniformBuffers,
                                std::span<const std::uint8_t> spirv,
                                std::span<const std::uint8_t> msl) {
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
    info.num_uniform_buffers = uniformBuffers;
    SDL_GPUShader *shader = SDL_CreateGPUShader(device, &info);
    if (shader == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "A shader was not created: %s", SDL_GetError());
    }
    return shader;
}

struct SurfaceDeleter {
    void operator()(SDL_Surface *surface) const {
        SDL_DestroySurface(surface);
    }
};

const char *PresentModeName(SDL_GPUPresentMode mode) {
    switch (mode) {
    case SDL_GPU_PRESENTMODE_IMMEDIATE:
        return "immediate";
    case SDL_GPU_PRESENTMODE_MAILBOX:
        return "mailbox";
    default:
        return "vsync";
    }
}

// Mailbox and immediate presentation stand in for each other, as neither waits for the vertical
// blank, and every window supports vertical sync.
SDL_GPUPresentMode
SupportedPresentMode(SDL_GPUDevice *device, SDL_Window *window, SDL_GPUPresentMode wanted) {
    if (wanted != SDL_GPU_PRESENTMODE_VSYNC) {
        SDL_GPUPresentMode other = wanted == SDL_GPU_PRESENTMODE_MAILBOX ?
                                       SDL_GPU_PRESENTMODE_IMMEDIATE :
                                       SDL_GPU_PRESENTMODE_MAILBOX;
        for (SDL_GPUPresentMode mode : {wanted, other}) {
            if (SDL_WindowSupportsGPUPresentMode(device, window, mode)) {
                return mode;
            }
        }
    }
    return SDL_GPU_PRESENTMODE_VSYNC;
}

} // namespace

std::unique_ptr<GpuWindow> GpuWindow::Create(const char *title, const DisplayOptions &options) {
    std::unique_ptr<GpuWindow> result(new GpuWindow());
    result->m_options = options;
    result->m_exclusive = options.exclusive;
    result->m_refreshRate = options.refreshRate;
    // The window shows once it covers the display, at the mode of the options.
    SDL_WindowFlags flags =
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_HIDDEN;
    result->m_window.reset(SDL_CreateWindow(title, options.width, options.height, flags));
    if (!result->m_window) {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "The window was not created: %s", SDL_GetError());
        return nullptr;
    }
    // The window is never smaller than the 320 by 240 of the screen of the console.
    SDL_SetWindowMinimumSize(result->m_window.get(), 320, 240);
    if (options.fullscreen) {
        result->SetFullscreen(true);
    }
    SDL_ShowWindow(result->m_window.get());
    // The shaders ship as SPIR-V for Vulkan, MSL for Metal, and DXIL for Direct3D 12.
    result->m_device.reset(SDL_CreateGPUDevice(
        SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_MSL | SDL_GPU_SHADERFORMAT_DXIL,
        options.gpuDebug,
        options.driver ? options.driver->c_str() : nullptr));
    if (!result->m_device) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "The GPU device was not created: %s", SDL_GetError());
        return nullptr;
    }
    if (!SDL_ClaimWindowForGPUDevice(result->m_device.get(), result->m_window.get())) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "The window was not claimed: %s", SDL_GetError());
        return nullptr;
    }
    // Vertical sync of a Wayland window waits more than a refresh for some frames and drops the
    // rate below the refresh of the display. A Wayland compositor does not tear, and the mailbox
    // paced at the refresh rate shows every frame in its place.
    SDL_GPUPresentMode wanted = options.presentMode;
    if (wanted == SDL_GPU_PRESENTMODE_VSYNC && !options.fifo &&
        SDL_strcmp(SDL_GetCurrentVideoDriver(), "wayland") == 0 &&
        SDL_WindowSupportsGPUPresentMode(
            result->m_device.get(), result->m_window.get(), SDL_GPU_PRESENTMODE_MAILBOX)) {
        wanted = SDL_GPU_PRESENTMODE_MAILBOX;
        result->m_pacedToRefresh = true;
    }
    SDL_GPUPresentMode presentMode =
        SupportedPresentMode(result->m_device.get(), result->m_window.get(), wanted);
    if (presentMode != SDL_GPU_PRESENTMODE_VSYNC &&
        !SDL_SetGPUSwapchainParameters(result->m_device.get(),
                                       result->m_window.get(),
                                       SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
                                       presentMode)) {
        SDL_LogWarn(SDL_LOG_CATEGORY_GPU,
                    "The frames wait for vertical sync, as %s presentation did not start: %s",
                    PresentModeName(presentMode),
                    SDL_GetError());
        presentMode = SDL_GPU_PRESENTMODE_VSYNC;
    }
    if (presentMode != wanted) {
        SDL_LogWarn(SDL_LOG_CATEGORY_GPU,
                    "The window does not support %s presentation.",
                    PresentModeName(wanted));
    }
    result->m_pacedToRefresh =
        result->m_pacedToRefresh && presentMode == SDL_GPU_PRESENTMODE_MAILBOX;
    SDL_Log("Presenting frames with %s%s.",
            PresentModeName(presentMode),
            result->m_pacedToRefresh ? " at the refresh rate of the display" : "");
    result->m_antialias = options.antialias;
    if (options.postAntialias != PostAntialias::None && !result->CreatePostPasses()) {
        return nullptr;
    }
    result->UpdateFrameSize();
    if (result->m_frame == nullptr) {
        return nullptr;
    }
    SDL_Log("Drawing with the %s GPU driver.", SDL_GetGPUDeviceDriver(result->m_device.get()));
    return result;
}

bool GpuWindow::CreateFrame(Uint32 width, Uint32 height) {
    SDL_GPUTextureCreateInfo info{};
    info.type = SDL_GPU_TEXTURETYPE_2D;
    info.format = FRAME_FORMAT;
    info.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;
    info.width = width;
    info.height = height;
    info.layer_count_or_depth = 1;
    info.num_levels = 1;
    SDL_GPUTexture *frame = CreateGpuTexture(m_device.get(), &info);
    SDL_GPUTexture *antialiased =
        m_antialiasPipeline != nullptr ? CreateGpuTexture(m_device.get(), &info) : nullptr;
    if (frame == nullptr || (m_antialiasPipeline != nullptr && antialiased == nullptr)) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU,
                     "The frame texture of %u by %u was not created: %s",
                     width,
                     height,
                     SDL_GetError());
        for (auto *texture : {frame, antialiased}) {
            if (texture != nullptr) {
                ReleaseGpuTexture(m_device.get(), texture);
            }
        }
        return false;
    }
    // The device releases the textures before once the frames that draw into them are done.
    for (auto *texture : {m_frame, m_antialiased}) {
        if (texture != nullptr) {
            ReleaseGpuTexture(m_device.get(), texture);
        }
    }
    m_frame = frame;
    m_antialiased = antialiased;
    m_frameWidth = width;
    m_frameHeight = height;
    SDL_Log("Drawing frames of %u by %u.", width, height);
    return true;
}

void GpuWindow::UpdateFrameSize() {
    // The largest texture every driver of SDL creates.
    constexpr int LARGEST = 16384;
    int width = m_options.width;
    int height = m_options.height;
    float aspect = m_options.aspectRatio;
    if (m_options.lowestFrame) {
        constexpr int LOWEST_WIDTH = 320;
        constexpr int LOWEST_HEIGHT = 240;
        // The shape is the aspect ratio of the settings, or else the shape of the display in the
        // full screen and 4:3 in a window. From 320 by 240, a wider shape widens the frame and a
        // narrower one makes it taller.
        if (aspect <= 0.0f) {
            aspect = static_cast<float>(LOWEST_WIDTH) / LOWEST_HEIGHT;
            int windowWidth = 0;
            int windowHeight = 0;
            SDL_GetWindowSizeInPixels(m_window.get(), &windowWidth, &windowHeight);
            if (Fullscreen() && windowWidth > 0 && windowHeight > 0) {
                aspect = static_cast<float>(windowWidth) / static_cast<float>(windowHeight);
            }
        }
        width = LOWEST_WIDTH;
        height = LOWEST_HEIGHT;
        if (aspect * LOWEST_HEIGHT >= LOWEST_WIDTH) {
            width = static_cast<int>(LOWEST_HEIGHT * aspect + 0.5f);
        } else {
            height = static_cast<int>(LOWEST_WIDTH / aspect + 0.5f);
        }
    } else if (m_options.frameWidth > 0 && m_options.frameHeight > 0) {
        width = m_options.frameWidth;
        height = m_options.frameHeight;
    } else {
        if (!m_options.fixedFrame) {
            SDL_GetWindowSizeInPixels(m_window.get(), &width, &height);
        }
        width = static_cast<int>(static_cast<float>(width) * m_options.renderScale + 0.5f);
        height = static_cast<int>(static_cast<float>(height) * m_options.renderScale + 0.5f);
        // An aspect ratio of the settings keeps the largest frame of its shape in the window.
        if (aspect > 0.0f && width > 0 && height > 0) {
            if (static_cast<float>(width) > static_cast<float>(height) * aspect) {
                width = static_cast<int>(static_cast<float>(height) * aspect + 0.5f);
            } else {
                height = static_cast<int>(static_cast<float>(width) / aspect + 0.5f);
            }
        }
    }
    auto wantedWidth = static_cast<Uint32>(std::clamp(width, 1, LARGEST));
    auto wantedHeight = static_cast<Uint32>(std::clamp(height, 1, LARGEST));
    if (m_frame == nullptr || wantedWidth != m_frameWidth || wantedHeight != m_frameHeight) {
        CreateFrame(wantedWidth, wantedHeight);
    }
}

bool GpuWindow::CreatePostPasses() {
    SDL_GPUDevice *device = m_device.get();
    SDL_GPUShader *vertex =
        CreatePostShader(device, SDL_GPU_SHADERSTAGE_VERTEX, 0, 0, POST_VERT_SPIRV, POST_VERT_MSL);
    SDL_GPUShader *fragment = CreatePostShader(
        device, SDL_GPU_SHADERSTAGE_FRAGMENT, 1, 1, FXAA_FRAG_SPIRV, FXAA_FRAG_MSL);
    if (vertex != nullptr && fragment != nullptr) {
        SDL_GPUColorTargetDescription target{};
        target.format = FRAME_FORMAT;
        SDL_GPUGraphicsPipelineCreateInfo info{};
        info.vertex_shader = vertex;
        info.fragment_shader = fragment;
        info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
        info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
        info.target_info.color_target_descriptions = &target;
        info.target_info.num_color_targets = 1;
        m_antialiasPipeline = SDL_CreateGPUGraphicsPipeline(device, &info);
    }
    for (auto *shader : {vertex, fragment}) {
        if (shader != nullptr) {
            SDL_ReleaseGPUShader(device, shader);
        }
    }
    SDL_GPUSamplerCreateInfo sampler{};
    sampler.min_filter = SDL_GPU_FILTER_LINEAR;
    sampler.mag_filter = SDL_GPU_FILTER_LINEAR;
    sampler.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sampler.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sampler.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    m_postSampler = SDL_CreateGPUSampler(device, &sampler);
    if (m_antialiasPipeline == nullptr || m_postSampler == nullptr) {
        SDL_LogError(
            SDL_LOG_CATEGORY_GPU, "The anti-aliasing pass was not created: %s", SDL_GetError());
        return false;
    }
    return true;
}

SDL_GPUTexture *GpuWindow::AntialiasFrame(SDL_GPUCommandBuffer *commands) {
    if (m_antialiasPipeline == nullptr || m_antialiased == nullptr) {
        return m_frame;
    }
    SDL_GPUColorTargetInfo target{};
    target.texture = m_antialiased;
    target.load_op = SDL_GPU_LOADOP_DONT_CARE;
    target.store_op = SDL_GPU_STOREOP_STORE;
    SDL_GPURenderPass *pass = SDL_BeginGPURenderPass(commands, &target, 1, nullptr);
    SDL_BindGPUGraphicsPipeline(pass, m_antialiasPipeline);
    SDL_GPUTextureSamplerBinding binding{m_frame, m_postSampler};
    SDL_BindGPUFragmentSamplers(pass, 0, &binding, 1);
    std::array<float, 4> pixel{1.0f / static_cast<float>(m_frameWidth),
                               1.0f / static_cast<float>(m_frameHeight),
                               0.0f,
                               0.0f};
    SDL_PushGPUFragmentUniformData(commands, 0, pixel.data(), sizeof(pixel));
    SDL_DrawGPUPrimitives(pass, 3, 1, 0, 0);
    SDL_EndGPURenderPass(pass);
    return m_antialiased;
}

GpuWindow::~GpuWindow() {
    if (m_device) {
        for (auto *texture : {m_frame, m_antialiased}) {
            if (texture != nullptr) {
                ReleaseGpuTexture(m_device.get(), texture);
            }
        }
        if (m_antialiasPipeline != nullptr) {
            SDL_ReleaseGPUGraphicsPipeline(m_device.get(), m_antialiasPipeline);
        }
        if (m_postSampler != nullptr) {
            SDL_ReleaseGPUSampler(m_device.get(), m_postSampler);
        }
        if (m_window) {
            SDL_ReleaseWindowFromGPUDevice(m_device.get(), m_window.get());
        }
    }
}

Uint64 GpuWindow::RefreshPeriod() const {
    if (!m_pacedToRefresh) {
        return 0;
    }
    const SDL_DisplayMode *mode =
        SDL_GetCurrentDisplayMode(SDL_GetDisplayForWindow(m_window.get()));
    if (mode == nullptr || mode->refresh_rate_numerator <= 0) {
        // A display of no known rate is paced at 60 hertz.
        return SDL_NS_PER_SECOND / 60;
    }
    return SDL_NS_PER_SECOND * static_cast<Uint64>(mode->refresh_rate_denominator) /
           static_cast<Uint64>(mode->refresh_rate_numerator);
}

bool GpuWindow::SetFullscreen(bool fullscreen) {
    if (fullscreen) {
        SDL_DisplayMode closest{};
        const SDL_DisplayMode *mode = nullptr;
        if (m_exclusive) {
            if (SDL_GetClosestFullscreenDisplayMode(SDL_GetDisplayForWindow(m_window.get()),
                                                    m_options.width,
                                                    m_options.height,
                                                    static_cast<float>(m_refreshRate),
                                                    false,
                                                    &closest)) {
                mode = &closest;
            } else {
                SDL_LogWarn(SDL_LOG_CATEGORY_VIDEO,
                            "The display has no mode of %d by %d, and the full screen is at the "
                            "desktop mode: %s",
                            m_options.width,
                            m_options.height,
                            SDL_GetError());
            }
        }
        // A null mode is the desktop mode, without borders.
        if (!SDL_SetWindowFullscreenMode(m_window.get(), mode)) {
            SDL_LogWarn(
                SDL_LOG_CATEGORY_VIDEO, "The full-screen mode was not set: %s", SDL_GetError());
        }
        if (mode != nullptr) {
            SDL_Log("Covering the display at %d by %d and %.2f Hz.",
                    mode->w,
                    mode->h,
                    static_cast<double>(mode->refresh_rate));
        }
    }
    if (!SDL_SetWindowFullscreen(m_window.get(), fullscreen)) {
        SDL_LogWarn(SDL_LOG_CATEGORY_VIDEO,
                    "The window did not %s: %s",
                    fullscreen ? "cover the display" : "leave the full screen",
                    SDL_GetError());
        return false;
    }
    return true;
}

bool GpuWindow::PresentClear(SDL_FColor color, const std::filesystem::path *capture) {
    SDL_GPUCommandBuffer *commands = SDL_AcquireGPUCommandBuffer(m_device.get());
    if (commands == nullptr) {
        return false;
    }
    SDL_GPUColorTargetInfo target{};
    target.texture = m_frame;
    target.clear_color = color;
    target.load_op = SDL_GPU_LOADOP_CLEAR;
    target.store_op = SDL_GPU_STOREOP_STORE;
    SDL_EndGPURenderPass(SDL_BeginGPURenderPass(commands, &target, 1, nullptr));
    return Present(commands, capture);
}

bool GpuWindow::Present(SDL_GPUCommandBuffer *commands, const std::filesystem::path *capture) {
    // The passes after the scene, each from the texture of the pass before: the anti-aliasing of
    // PostAntialias, then the scale to the window by ScaleFilter.
    SDL_GPUTexture *source = AntialiasFrame(commands);
    SDL_GPUTexture *swapchain = nullptr;
    Uint32 width = 0;
    Uint32 height = 0;
    if (SDL_WaitAndAcquireGPUSwapchainTexture(
            commands, m_window.get(), &swapchain, &width, &height) &&
        swapchain != nullptr) {
        // The frame keeps its shape in a window of another shape, with black bars at the sides or
        // at the top and bottom.
        float scale = std::min(static_cast<float>(width) / static_cast<float>(m_frameWidth),
                               static_cast<float>(height) / static_cast<float>(m_frameHeight));
        auto fittedWidth = static_cast<Uint32>(static_cast<float>(m_frameWidth) * scale);
        auto fittedHeight = static_cast<Uint32>(static_cast<float>(m_frameHeight) * scale);
        SDL_GPUBlitInfo blit{};
        blit.source.texture = source;
        blit.source.w = m_frameWidth;
        blit.source.h = m_frameHeight;
        blit.destination.texture = swapchain;
        blit.destination.x = (width - fittedWidth) / 2;
        blit.destination.y = (height - fittedHeight) / 2;
        blit.destination.w = fittedWidth;
        blit.destination.h = fittedHeight;
        blit.load_op = SDL_GPU_LOADOP_CLEAR;
        blit.clear_color = SDL_FColor{0.0f, 0.0f, 0.0f, 1.0f};
        blit.filter = m_options.scaleFilter == ScaleFilter::Nearest ? SDL_GPU_FILTER_NEAREST :
                                                                      SDL_GPU_FILTER_LINEAR;
        SDL_BlitGPUTexture(commands, &blit);
    }
    if (capture != nullptr) {
        return SaveFrame(commands, source, *capture);
    }
    return SDL_SubmitGPUCommandBuffer(commands);
}

bool GpuWindow::SaveFrame(SDL_GPUCommandBuffer *commands,
                          SDL_GPUTexture *frame,
                          const std::filesystem::path &path) {
    Uint32 size = m_frameWidth * m_frameHeight * 4;
    SDL_GPUTransferBufferCreateInfo info{};
    info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD;
    info.size = size;
    SDL_GPUTransferBuffer *buffer = CreateGpuTransferBuffer(m_device.get(), &info);
    if (buffer == nullptr) {
        SDL_SubmitGPUCommandBuffer(commands);
        return false;
    }
    SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(commands);
    SDL_GPUTextureRegion source{};
    source.texture = frame;
    source.w = m_frameWidth;
    source.h = m_frameHeight;
    source.d = 1;
    SDL_GPUTextureTransferInfo destination{};
    destination.transfer_buffer = buffer;
    SDL_DownloadFromGPUTexture(copy, &source, &destination);
    SDL_EndGPUCopyPass(copy);
    SDL_GPUFence *fence = SDL_SubmitGPUCommandBufferAndAcquireFence(commands);
    bool saved = false;
    if (fence != nullptr && SDL_WaitForGPUFences(m_device.get(), true, &fence, 1)) {
        const void *pixels = SDL_MapGPUTransferBuffer(m_device.get(), buffer, false);
        if (pixels != nullptr) {
            // R8G8B8A8 bytes in memory order are XBGR8888 in the packed order of SDL. The display
            // does not show the alpha of the frame, and the image leaves it out too.
            std::unique_ptr<SDL_Surface, SurfaceDeleter> surface(
                SDL_CreateSurface(static_cast<int>(m_frameWidth),
                                  static_cast<int>(m_frameHeight),
                                  SDL_PIXELFORMAT_XBGR8888));
            if (surface) {
                for (Uint32 row = 0; row < m_frameHeight; ++row) {
                    std::memcpy(static_cast<Uint8 *>(surface->pixels) + row * surface->pitch,
                                static_cast<const Uint8 *>(pixels) + row * m_frameWidth * 4,
                                m_frameWidth * 4);
                }
                saved = SDL_SavePNG(surface.get(), path.string().c_str());
            }
            SDL_UnmapGPUTransferBuffer(m_device.get(), buffer);
        }
    }
    if (fence != nullptr) {
        SDL_ReleaseGPUFence(m_device.get(), fence);
    }
    ReleaseGpuTransferBuffer(m_device.get(), buffer);
    if (saved) {
        SDL_Log("Saved the frame to %s.", path.string().c_str());
    } else {
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "The frame was not saved: %s", SDL_GetError());
    }
    return saved;
}

namespace {

// The bytes of each object of the GPU the game created, by the object.
std::unordered_map<const void *, Uint64> g_gpuMemoryObjects;
Uint64 g_gpuMemoryBytes = 0;

void AddGpuMemory(const void *object, Uint64 bytes) {
    if (object != nullptr) {
        g_gpuMemoryObjects[object] = bytes;
        g_gpuMemoryBytes += bytes;
    }
}

void RemoveGpuMemory(const void *object) {
    auto found = g_gpuMemoryObjects.find(object);
    if (found != g_gpuMemoryObjects.end()) {
        g_gpuMemoryBytes -= found->second;
        g_gpuMemoryObjects.erase(found);
    }
}

} // namespace

SDL_GPUTexture *CreateGpuTexture(SDL_GPUDevice *device, const SDL_GPUTextureCreateInfo *info) {
    SDL_GPUTexture *texture = SDL_CreateGPUTexture(device, info);
    Uint64 bytes = 0;
    for (Uint32 level = 0; level < info->num_levels; ++level) {
        bytes += SDL_CalculateGPUTextureFormatSize(info->format,
                                                   std::max(info->width >> level, 1u),
                                                   std::max(info->height >> level, 1u),
                                                   info->layer_count_or_depth);
    }
    // SDL_GPU_SAMPLECOUNT_2 and the counts after it are 1, 2, and 3.
    AddGpuMemory(texture, bytes << info->sample_count);
    return texture;
}

SDL_GPUBuffer *CreateGpuBuffer(SDL_GPUDevice *device, const SDL_GPUBufferCreateInfo *info) {
    SDL_GPUBuffer *buffer = SDL_CreateGPUBuffer(device, info);
    AddGpuMemory(buffer, info->size);
    return buffer;
}

SDL_GPUTransferBuffer *CreateGpuTransferBuffer(SDL_GPUDevice *device,
                                               const SDL_GPUTransferBufferCreateInfo *info) {
    SDL_GPUTransferBuffer *buffer = SDL_CreateGPUTransferBuffer(device, info);
    AddGpuMemory(buffer, info->size);
    return buffer;
}

void ReleaseGpuTexture(SDL_GPUDevice *device, SDL_GPUTexture *texture) {
    RemoveGpuMemory(texture);
    SDL_ReleaseGPUTexture(device, texture);
}

void ReleaseGpuBuffer(SDL_GPUDevice *device, SDL_GPUBuffer *buffer) {
    RemoveGpuMemory(buffer);
    SDL_ReleaseGPUBuffer(device, buffer);
}

void ReleaseGpuTransferBuffer(SDL_GPUDevice *device, SDL_GPUTransferBuffer *buffer) {
    RemoveGpuMemory(buffer);
    SDL_ReleaseGPUTransferBuffer(device, buffer);
}

Uint64 GpuMemoryBytes() {
    return g_gpuMemoryBytes;
}

} // namespace coastline
