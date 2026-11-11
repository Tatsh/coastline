#include "gpu.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <span>

#include "appicon.h"
#include "config.h"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <dxgi.h>
#endif

#include "shaders/fxaa_frag_msl.h"
#include "shaders/fxaa_frag_spirv.h"
#include "shaders/hdr_frag_msl.h"
#include "shaders/hdr_frag_spirv.h"
#include "shaders/post_vert_msl.h"
#include "shaders/post_vert_spirv.h"
#include "shaders/presentation_frag_msl.h"
#include "shaders/presentation_frag_spirv.h"

namespace coastline {

bool GpuWindow::WideMenus() const {
    // Internal aspect expansion rounds dimensions to whole pixels.
    return m_options.wideMenus ||
           (m_options.autoWideMenus && m_frameHeight != 0 &&
            std::abs(static_cast<double>(m_frameWidth) - m_frameHeight * (16.0 / 9.0)) <= 1.0);
}

namespace {

constexpr SDL_GPUTextureFormat FRAME_FORMAT = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;

// The Vulkan driver of SDL submits a command buffer of its own for each texture it creates, and
// recycles the command buffers of a window only when a frame is presented. Each command buffer
// retains its allocations in the GPU driver until the device is destroyed, and a race of hundreds
// of textures loaded in one frame grew the heap by tens of megabytes. A wait for the GPU after
// every TEXTURES_BEFORE_WAIT textures between presents lets SDL recycle them.
constexpr unsigned TEXTURES_BEFORE_WAIT = 32;
constexpr const char *GPU_WINDOW_PROPERTY = "coastline.gpu-window";

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
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "GPU driver requires SPIR-V or MSL support.");
        return nullptr;
    }
    info.stage = stage;
    info.num_samplers = samplers;
    info.num_uniform_buffers = uniformBuffers;
    SDL_GPUShader *shader = SDL_CreateGPUShader(device, &info);
    if (shader == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "Cannot create shader: %s", SDL_GetError());
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
    result->m_options.drawDistanceScale = std::isfinite(options.drawDistanceScale) ?
                                              std::clamp(options.drawDistanceScale, 1.0f, 4.0f) :
                                              1.0f;
    result->m_exclusive = options.exclusive;
    result->m_refreshRate = options.refreshRate;
    // The window shows once it covers the display, at the mode of the options.
    SDL_WindowFlags flags =
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_HIDDEN;
    result->m_window.reset(SDL_CreateWindow(title, options.width, options.height, flags));
    if (!result->m_window) {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "Cannot create window: %s", SDL_GetError());
        return nullptr;
    }
#ifndef SDL_PLATFORM_MACOS
    auto *icon =
        SDL_LoadPNG_IO(SDL_IOFromConstMem(COASTLINE_ICON_PNG, sizeof(COASTLINE_ICON_PNG)), true);
    if (icon) {
        if (!SDL_SetWindowIcon(result->m_window.get(), icon)) {
            SDL_LogWarn(SDL_LOG_CATEGORY_VIDEO, "Cannot set window icon: %s", SDL_GetError());
        }
        SDL_DestroySurface(icon);
    } else {
        SDL_LogWarn(SDL_LOG_CATEGORY_VIDEO, "Cannot load window icon: %s", SDL_GetError());
    }
#endif
    // The window is never smaller than the 320 by 240 of the screen of the console.
    SDL_SetWindowMinimumSize(result->m_window.get(), 320, 240);
    if (options.fullscreen) {
        result->SetFullscreen(true);
    }
    SDL_ShowWindow(result->m_window.get());
    if (options.minimized) {
        SDL_MinimizeWindow(result->m_window.get());
    }
    // The shaders ship as SPIR-V for Vulkan, MSL for Metal, and DXIL for Direct3D 12.
    const auto properties = SDL_CreateProperties();
    SDL_SetBooleanProperty(properties, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_SPIRV_BOOLEAN, true);
    SDL_SetBooleanProperty(properties, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_MSL_BOOLEAN, true);
    SDL_SetBooleanProperty(properties, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_DXIL_BOOLEAN, true);
    SDL_SetBooleanProperty(
        properties, SDL_PROP_GPU_DEVICE_CREATE_DEBUGMODE_BOOLEAN, options.gpuDebug);
    if (options.driver) {
        SDL_SetStringProperty(
            properties, SDL_PROP_GPU_DEVICE_CREATE_NAME_STRING, options.driver->c_str());
    }
    if (options.gpuPreference != "auto") {
        SDL_SetBooleanProperty(properties,
                               SDL_PROP_GPU_DEVICE_CREATE_PREFERLOWPOWER_BOOLEAN,
                               options.gpuPreference == "low-power");
    }
    result->m_device.reset(SDL_CreateGPUDeviceWithProperties(properties));
    SDL_DestroyProperties(properties);
    if (!result->m_device) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "Cannot create GPU device: %s", SDL_GetError());
        return nullptr;
    }
    SDL_SetPointerProperty(
        SDL_GetGPUDeviceProperties(result->m_device.get()), GPU_WINDOW_PROPERTY, result.get());
    if (!SDL_ClaimWindowForGPUDevice(result->m_device.get(), result->m_window.get())) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "Cannot bind window to GPU: %s", SDL_GetError());
        return nullptr;
    }
    result->SetPresentMode(options.presentMode);
    result->m_antialias = options.antialias;
    if (options.postAntialias != PostAntialias::None && !result->CreatePostPasses()) {
        return nullptr;
    }
    if ((options.screenShader != ScreenShader::None ||
         options.scaleFilter == ScaleFilter::EdgeAdaptive) &&
        !result->CreatePresentationPass()) {
        return nullptr;
    }
    result->UpdateFrameSize();
    if (result->m_frame == nullptr) {
        return nullptr;
    }
    const char *backend = SDL_GetGPUDeviceDriver(result->m_device.get());
    if (std::strcmp(backend, "vulkan") == 0) {
        backend = "Vulkan";
    } else if (std::strcmp(backend, "metal") == 0) {
        backend = "Metal";
    } else if (std::strcmp(backend, "direct3d12") == 0 || std::strcmp(backend, "d3d12") == 0) {
        backend = "Direct3D12";
    }
    SDL_Log("Using %s GPU backend.", backend);
    const auto identity = SDL_GetGPUDeviceProperties(result->m_device.get());
    SDL_Log("Using GPU adapter %s with preference %s.",
            result->DeviceName().c_str(),
            options.gpuPreference.c_str());
    SDL_Log(
        "Using GPU driver %s, version %s.",
        SDL_GetStringProperty(identity, SDL_PROP_GPU_DEVICE_DRIVER_NAME_STRING, "unknown"),
        SDL_GetStringProperty(
            identity,
            SDL_PROP_GPU_DEVICE_DRIVER_INFO_STRING,
            SDL_GetStringProperty(identity, SDL_PROP_GPU_DEVICE_DRIVER_VERSION_STRING, "unknown")));
    SDL_Log("NVML sampling %s for the active adapter.",
            result->NvmlAllowed() ? "allowed" : "disabled");
    return result;
}

std::string GpuWindow::DeviceName() const {
    return SDL_GetStringProperty(
        SDL_GetGPUDeviceProperties(m_device.get()), SDL_PROP_GPU_DEVICE_NAME_STRING, "unknown");
}

bool GpuWindow::NvmlAllowed() const {
    const auto properties = SDL_GetGPUDeviceProperties(m_device.get());
    if (SDL_strcmp(SDL_GetStringProperty(properties, SDL_PROP_GPU_DEVICE_DRIVER_NAME_STRING, ""),
                   "NVIDIA") == 0) {
        return true;
    }
#ifdef _WIN32
    if (SDL_strcmp(SDL_GetGPUDeviceDriver(m_device.get()), "direct3d12") != 0) {
        return false;
    }
    IDXGIFactory1 *factory = nullptr;
    if (FAILED(CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void **>(&factory)))) {
        return false;
    }
    unsigned matches = 0;
    bool nvidia = false;
    const auto active = DeviceName();
    for (unsigned index = 0;; ++index) {
        IDXGIAdapter1 *adapter = nullptr;
        if (factory->EnumAdapters1(index, &adapter) != S_OK) {
            break;
        }
        DXGI_ADAPTER_DESC1 description{};
        if (SUCCEEDED(adapter->GetDesc1(&description))) {
            char *name =
                SDL_iconv_string("UTF-8",
                                 "UTF-16LE",
                                 reinterpret_cast<const char *>(description.Description),
                                 (SDL_wcslen(description.Description) + 1) * sizeof(wchar_t));
            if (name && active == name) {
                ++matches;
                nvidia = description.VendorId == 0x10DE;
            }
            SDL_free(name);
        }
        adapter->Release();
    }
    factory->Release();
    return matches == 1 && nvidia;
#else
    return false;
#endif
}

void GpuWindow::SetFramerateUnlocked(bool unlocked) {
    m_options.unlockedFramerate = unlocked;
    SetPresentMode(m_options.presentMode);
    SDL_Log("Unlocked frame rate %s.", unlocked ? "enabled" : "disabled");
}

void GpuWindow::SetPresentMode(SDL_GPUPresentMode mode) {
    m_options.presentMode = mode;
    m_pacedToRefresh = false;
    // Vertical sync of a Wayland window waits more than a refresh for some frames and drops the
    // rate below the refresh of the display. A Wayland compositor does not tear, and the mailbox
    // paced at the refresh rate shows every frame in its place.
    SDL_GPUPresentMode wanted = m_options.unlockedFramerate ? SDL_GPU_PRESENTMODE_IMMEDIATE : mode;
    if (wanted == SDL_GPU_PRESENTMODE_VSYNC && !m_options.fifo &&
        SDL_strcmp(SDL_GetCurrentVideoDriver(), "wayland") == 0 &&
        SDL_WindowSupportsGPUPresentMode(
            m_device.get(), m_window.get(), SDL_GPU_PRESENTMODE_MAILBOX)) {
        wanted = SDL_GPU_PRESENTMODE_MAILBOX;
        m_pacedToRefresh = true;
    }
    SDL_GPUPresentMode presentMode = SupportedPresentMode(m_device.get(), m_window.get(), wanted);
    const auto properties = SDL_GetWindowProperties(m_window.get());
    const bool hdrSupported = SDL_WindowSupportsGPUSwapchainComposition(
        m_device.get(), m_window.get(), SDL_GPU_SWAPCHAINCOMPOSITION_HDR_EXTENDED_LINEAR);
    SDL_LogDebug(
        SDL_LOG_CATEGORY_GPU, "HDR swapchain %s.", hdrSupported ? "supported" : "unsupported");
    const bool hdrRequested =
        m_options.hdrOutput == HdrOutput::On ||
        (m_options.hdrOutput == HdrOutput::Auto &&
         SDL_GetBooleanProperty(properties, SDL_PROP_WINDOW_HDR_ENABLED_BOOLEAN, false));
    const bool hdrAvailable =
        hdrRequested && hdrSupported && (m_hdrPipeline != nullptr || CreateHdrPass());
    m_hdrActive = hdrAvailable &&
                  SDL_SetGPUSwapchainParameters(m_device.get(),
                                                m_window.get(),
                                                SDL_GPU_SWAPCHAINCOMPOSITION_HDR_EXTENDED_LINEAR,
                                                presentMode);
    if (m_hdrActive && SDL_GetGPUSwapchainTextureFormat(m_device.get(), m_window.get()) !=
                           SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT) {
        SDL_LogWarn(SDL_LOG_CATEGORY_GPU, "HDR swapchain has an unexpected texture format.");
        m_hdrActive = false;
    }
    if (!m_hdrActive &&
        !SDL_SetGPUSwapchainParameters(
            m_device.get(), m_window.get(), SDL_GPU_SWAPCHAINCOMPOSITION_SDR, presentMode) &&
        presentMode != SDL_GPU_PRESENTMODE_VSYNC) {
        SDL_LogWarn(SDL_LOG_CATEGORY_GPU,
                    "Cannot use %s presentation; using VSync: %s",
                    PresentModeName(presentMode),
                    SDL_GetError());
        presentMode = SDL_GPU_PRESENTMODE_VSYNC;
        SDL_SetGPUSwapchainParameters(
            m_device.get(), m_window.get(), SDL_GPU_SWAPCHAINCOMPOSITION_SDR, presentMode);
    }
    if (presentMode != wanted) {
        SDL_LogWarn(SDL_LOG_CATEGORY_GPU,
                    "Window does not support %s presentation.",
                    PresentModeName(wanted));
    }
    m_pacedToRefresh = m_pacedToRefresh && presentMode == SDL_GPU_PRESENTMODE_MAILBOX;
    SDL_Log("Presenting frames with %s%s.",
            PresentModeName(presentMode),
            m_pacedToRefresh ? " at display refresh rate" : "");
    SDL_Log("HDR output %s.", m_hdrActive ? "enabled" : "disabled");
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
                     "Cannot create %ux%u frame texture: %s",
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
    SDL_Log("Rendering at resolution %ux%u.", width, height);
    return true;
}

void GpuWindow::UpdateFrameSize() {
    // The largest texture every driver of SDL creates.
    constexpr int LARGEST = 16384;
    int width = m_options.width;
    int height = m_options.height;
    float aspect = m_options.aspectRatio;
    if (m_options.lowestFrame || (m_options.frameWidth > 0 && m_options.frameHeight > 0)) {
        if (aspect <= 0.0f) {
            int windowWidth = 0;
            int windowHeight = 0;
            SDL_GetWindowSizeInPixels(m_window.get(), &windowWidth, &windowHeight);
            aspect = windowWidth > 0 && windowHeight > 0 ?
                         static_cast<float>(windowWidth) / static_cast<float>(windowHeight) :
                         4.0f / 3.0f;
        }
        const auto size =
            ExpandInternalResolution(m_options.lowestFrame ? 320 : m_options.frameWidth,
                                     m_options.lowestFrame ? 240 : m_options.frameHeight,
                                     aspect);
        width = size[0];
        height = size[1];
    } else {
        if (!m_options.fixedFrame) {
            SDL_GetWindowSizeInPixels(m_window.get(), &width, &height);
        }
        const auto size = ScaleFrameResolution(width, height, m_options.renderScale, aspect);
        width = size[0];
        height = size[1];
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
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "Cannot create anti-aliasing pass: %s", SDL_GetError());
        return false;
    }
    return true;
}

SDL_GPUTexture *GpuWindow::AntialiasFrame(SDL_GPUCommandBuffer *commands) {
    if (m_options.postAntialias == PostAntialias::None || m_antialiasPipeline == nullptr ||
        m_antialiased == nullptr) {
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

bool GpuWindow::CreatePresentationPass() {
    auto *device = m_device.get();
    auto *vertex =
        CreatePostShader(device, SDL_GPU_SHADERSTAGE_VERTEX, 0, 0, POST_VERT_SPIRV, POST_VERT_MSL);
    auto *fragment = CreatePostShader(
        device, SDL_GPU_SHADERSTAGE_FRAGMENT, 1, 1, PRESENTATION_FRAG_SPIRV, PRESENTATION_FRAG_MSL);
    SDL_GPUGraphicsPipeline *pipeline = nullptr;
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
        pipeline = SDL_CreateGPUGraphicsPipeline(device, &info);
    }
    for (auto *shader : {vertex, fragment}) {
        if (shader != nullptr) {
            SDL_ReleaseGPUShader(device, shader);
        }
    }
    SDL_GPUSamplerCreateInfo info{};
    info.min_filter = SDL_GPU_FILTER_LINEAR;
    info.mag_filter = SDL_GPU_FILTER_LINEAR;
    info.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    info.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    info.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    auto *sampler = SDL_CreateGPUSampler(device, &info);
    if (pipeline == nullptr || sampler == nullptr) {
        if (pipeline != nullptr) {
            SDL_ReleaseGPUGraphicsPipeline(device, pipeline);
        }
        if (sampler != nullptr) {
            SDL_ReleaseGPUSampler(device, sampler);
        }
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "Cannot create presentation filter: %s", SDL_GetError());
        return false;
    }
    m_presentationPipeline = pipeline;
    m_presentationSampler = sampler;
    return true;
}

bool GpuWindow::CreateHdrPass() {
    auto *device = m_device.get();
    auto *vertex =
        CreatePostShader(device, SDL_GPU_SHADERSTAGE_VERTEX, 0, 0, POST_VERT_SPIRV, POST_VERT_MSL);
    auto *fragment =
        CreatePostShader(device, SDL_GPU_SHADERSTAGE_FRAGMENT, 1, 1, HDR_FRAG_SPIRV, HDR_FRAG_MSL);
    SDL_GPUGraphicsPipeline *pipeline = nullptr;
    if (vertex != nullptr && fragment != nullptr) {
        SDL_GPUColorTargetDescription target{};
        target.format = SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT;
        SDL_GPUGraphicsPipelineCreateInfo info{};
        info.vertex_shader = vertex;
        info.fragment_shader = fragment;
        info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
        info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
        info.target_info.color_target_descriptions = &target;
        info.target_info.num_color_targets = 1;
        pipeline = SDL_CreateGPUGraphicsPipeline(device, &info);
    }
    for (auto *shader : {vertex, fragment}) {
        if (shader != nullptr) {
            SDL_ReleaseGPUShader(device, shader);
        }
    }
    SDL_GPUSamplerCreateInfo info{};
    info.min_filter = SDL_GPU_FILTER_LINEAR;
    info.mag_filter = SDL_GPU_FILTER_LINEAR;
    info.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    info.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    info.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    auto *sampler = SDL_CreateGPUSampler(device, &info);
    if (pipeline == nullptr || sampler == nullptr) {
        if (pipeline != nullptr) {
            SDL_ReleaseGPUGraphicsPipeline(device, pipeline);
        }
        if (sampler != nullptr) {
            SDL_ReleaseGPUSampler(device, sampler);
        }
        SDL_LogWarn(SDL_LOG_CATEGORY_GPU, "Cannot create HDR output: %s", SDL_GetError());
        return false;
    }
    m_hdrPipeline = pipeline;
    m_hdrSampler = sampler;
    return true;
}

SDL_GPUTexture *GpuWindow::FilterFrame(SDL_GPUCommandBuffer *commands,
                                       SDL_GPUTexture *source,
                                       Uint32 width,
                                       Uint32 height) {
    if (m_filtered == nullptr || width != m_filteredWidth || height != m_filteredHeight) {
        SDL_GPUTextureCreateInfo info{};
        info.type = SDL_GPU_TEXTURETYPE_2D;
        info.format = FRAME_FORMAT;
        info.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;
        info.width = width;
        info.height = height;
        info.layer_count_or_depth = 1;
        info.num_levels = 1;
        auto *texture = CreateGpuTexture(m_device.get(), &info);
        if (texture == nullptr) {
            SDL_LogError(
                SDL_LOG_CATEGORY_GPU, "Cannot create presentation texture: %s", SDL_GetError());
            return nullptr;
        }
        if (m_filtered != nullptr) {
            ReleaseGpuTexture(m_device.get(), m_filtered);
        }
        m_filtered = texture;
        m_filteredWidth = width;
        m_filteredHeight = height;
    }
    SDL_GPUColorTargetInfo target{};
    target.texture = m_filtered;
    target.load_op = SDL_GPU_LOADOP_DONT_CARE;
    target.store_op = SDL_GPU_STOREOP_STORE;
    auto *pass = SDL_BeginGPURenderPass(commands, &target, 1, nullptr);
    SDL_BindGPUGraphicsPipeline(pass, m_presentationPipeline);
    SDL_GPUTextureSamplerBinding binding{source, m_presentationSampler};
    SDL_BindGPUFragmentSamplers(pass, 0, &binding, 1);
    const bool upscale = width > m_frameWidth || height > m_frameHeight;
    const float filter = m_options.scaleFilter == ScaleFilter::Nearest                 ? 1.0f :
                         m_options.scaleFilter == ScaleFilter::EdgeAdaptive && upscale ? 2.0f :
                                                                                         0.0f;
    const std::array<float, 8> uniforms{static_cast<float>(m_frameWidth),
                                        static_cast<float>(m_frameHeight),
                                        static_cast<float>(width),
                                        static_cast<float>(height),
                                        filter,
                                        static_cast<float>(m_options.screenShader),
                                        0.0f,
                                        0.0f};
    SDL_PushGPUFragmentUniformData(commands, 0, uniforms.data(), sizeof(uniforms));
    SDL_DrawGPUPrimitives(pass, 3, 1, 0, 0);
    SDL_EndGPURenderPass(pass);
    return m_filtered;
}

bool GpuWindow::ApplyLiveOptions(const DisplayOptions &options) {
    if ((options.screenShader != ScreenShader::None ||
         options.scaleFilter == ScaleFilter::EdgeAdaptive) &&
        m_presentationPipeline == nullptr && !CreatePresentationPass()) {
        return false;
    }
    if (options.postAntialias != PostAntialias::None && m_antialiasPipeline == nullptr) {
        if (!CreatePostPasses() || !CreateFrame(m_frameWidth, m_frameHeight)) {
            return false;
        }
    }
    const bool presentChanged = m_options.presentMode != options.presentMode ||
                                m_options.fifo != options.fifo ||
                                m_options.unlockedFramerate != options.unlockedFramerate ||
                                m_options.hdrOutput != options.hdrOutput;
    m_options.fifo = options.fifo;
    m_options.unlockedFramerate = options.unlockedFramerate;
    m_options.hdrOutput = options.hdrOutput;
    if (presentChanged) {
        SetPresentMode(options.presentMode);
    }
    m_options.wideMenus = options.wideMenus;
    m_options.autoWideMenus = options.autoWideMenus;
    m_options.fieldOfViewScale = options.fieldOfViewScale;
    m_options.drawDistanceScale = std::isfinite(options.drawDistanceScale) ?
                                      std::clamp(options.drawDistanceScale, 1.0f, 4.0f) :
                                      1.0f;
    m_options.n64Effects = options.n64Effects;
    m_options.n64WindowReflections = options.n64WindowReflections;
    m_options.frameWidth = options.frameWidth;
    m_options.frameHeight = options.frameHeight;
    m_options.renderScale = options.renderScale;
    m_options.fixedFrame = options.fixedFrame;
    m_options.lowestFrame = options.lowestFrame;
    m_options.aspectRatio = options.aspectRatio;
    m_options.postAntialias = options.postAntialias;
    m_options.scaleFilter = options.scaleFilter;
    m_options.screenShader = options.screenShader;
    UpdateFrameSize();
    return true;
}

bool GpuWindow::ApplyRendererOptions(const DisplayOptions &options) {
    m_antialias = options.antialias;
    m_options.anisotropy = options.anisotropy;
    m_options.textureFilter = options.textureFilter;
    m_options.alphaToCoverage = options.alphaToCoverage;
    return ApplyLiveOptions(options);
}

GpuWindow::~GpuWindow() {
    if (m_device) {
        for (auto *texture : {m_frame, m_antialiased, m_filtered, m_composite}) {
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
        if (m_presentationPipeline != nullptr) {
            SDL_ReleaseGPUGraphicsPipeline(m_device.get(), m_presentationPipeline);
        }
        if (m_presentationSampler != nullptr) {
            SDL_ReleaseGPUSampler(m_device.get(), m_presentationSampler);
        }
        if (m_hdrPipeline != nullptr) {
            SDL_ReleaseGPUGraphicsPipeline(m_device.get(), m_hdrPipeline);
        }
        if (m_hdrSampler != nullptr) {
            SDL_ReleaseGPUSampler(m_device.get(), m_hdrSampler);
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
                            "Display mode %dx%d unavailable; using desktop mode: %s",
                            m_options.width,
                            m_options.height,
                            SDL_GetError());
            }
        }
        // A null mode is the desktop mode, without borders.
        if (!SDL_SetWindowFullscreenMode(m_window.get(), mode)) {
            SDL_LogWarn(SDL_LOG_CATEGORY_VIDEO, "Cannot set full-screen mode: %s", SDL_GetError());
        }
        if (mode != nullptr) {
            SDL_Log("Using full screen at %dx%d, %.2f Hz.",
                    mode->w,
                    mode->h,
                    static_cast<double>(mode->refresh_rate));
        }
    }
    if (!SDL_SetWindowFullscreen(m_window.get(), fullscreen)) {
        SDL_LogWarn(SDL_LOG_CATEGORY_VIDEO,
                    "Cannot %s: %s",
                    fullscreen ? "enter full screen" : "exit full screen",
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

bool GpuWindow::Present(SDL_GPUCommandBuffer *commands,
                        const std::filesystem::path *capture,
                        const OutputOverlay &overlay) {
    // The passes after the scene, each from the texture of the pass before: the anti-aliasing of
    // PostAntialias, then the scale to the window by ScaleFilter.
    SDL_GPUTexture *source = AntialiasFrame(commands);
    SDL_GPUTexture *swapchain = nullptr;
    m_texturesSincePresent = 0;
    Uint32 width = 0;
    Uint32 height = 0;
    const auto acquireStart = SDL_GetTicksNS();
    const bool acquired = SDL_WaitAndAcquireGPUSwapchainTexture(
        commands, m_window.get(), &swapchain, &width, &height);
    m_presentationWait += SDL_GetTicksNS() - acquireStart;
    Uint32 sourceWidth = m_frameWidth;
    Uint32 sourceHeight = m_frameHeight;
    if (width == 0 || height == 0) {
        int windowWidth = 0;
        int windowHeight = 0;
        SDL_GetWindowSizeInPixels(m_window.get(), &windowWidth, &windowHeight);
        width = static_cast<Uint32>(std::max(windowWidth, 1));
        height = static_cast<Uint32>(std::max(windowHeight, 1));
    }
    // Fit explicit ratios with bars. Fill screen also absorbs internal pixel-rounding differences.
    auto fittedWidth = width;
    auto fittedHeight = height;
    if (m_options.aspectRatio > 0.0f) {
        const double aspect = m_options.aspectRatio;
        if (width > height * aspect) {
            fittedWidth = std::clamp<Uint32>(std::lround(height * aspect), 1, width);
        } else {
            fittedHeight = std::clamp<Uint32>(std::lround(width / aspect), 1, height);
        }
    }
    if (m_options.screenShader != ScreenShader::None ||
        m_options.scaleFilter == ScaleFilter::EdgeAdaptive) {
        auto *filtered =
            FilterFrame(commands, source, std::max(fittedWidth, 1u), std::max(fittedHeight, 1u));
        if (filtered == nullptr) {
            SDL_CancelGPUCommandBuffer(commands);
            return false;
        }
        source = filtered;
        sourceWidth = m_filteredWidth;
        sourceHeight = m_filteredHeight;
    }
    if (overlay) {
        if (m_composite == nullptr || m_compositeWidth != width || m_compositeHeight != height) {
            SDL_GPUTextureCreateInfo info{};
            info.type = SDL_GPU_TEXTURETYPE_2D;
            info.format = FRAME_FORMAT;
            info.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;
            info.width = width;
            info.height = height;
            info.layer_count_or_depth = 1;
            info.num_levels = 1;
            auto *texture = CreateGpuTexture(m_device.get(), &info);
            if (texture == nullptr) {
                SDL_LogError(
                    SDL_LOG_CATEGORY_GPU, "Cannot create output overlay: %s", SDL_GetError());
                SDL_CancelGPUCommandBuffer(commands);
                return false;
            }
            if (m_composite != nullptr) {
                ReleaseGpuTexture(m_device.get(), m_composite);
            }
            m_composite = texture;
            m_compositeWidth = width;
            m_compositeHeight = height;
        }
        SDL_GPUBlitInfo blit{};
        blit.source.texture = source;
        blit.source.w = sourceWidth;
        blit.source.h = sourceHeight;
        blit.destination.texture = m_composite;
        blit.destination.x = (width - fittedWidth) / 2;
        blit.destination.y = (height - fittedHeight) / 2;
        blit.destination.w = fittedWidth;
        blit.destination.h = fittedHeight;
        blit.load_op = SDL_GPU_LOADOP_CLEAR;
        blit.clear_color = {0, 0, 0, 1};
        blit.filter = m_options.scaleFilter == ScaleFilter::Nearest ? SDL_GPU_FILTER_NEAREST :
                                                                      SDL_GPU_FILTER_LINEAR;
        blit.cycle = true;
        SDL_BlitGPUTexture(commands, &blit);
        overlay(commands, m_composite, width, height);
        source = m_composite;
        sourceWidth = width;
        sourceHeight = height;
        fittedWidth = width;
        fittedHeight = height;
    } else if (m_composite != nullptr) {
        ReleaseGpuTexture(m_device.get(), m_composite);
        m_composite = nullptr;
        m_compositeWidth = m_compositeHeight = 0;
    }
    if (acquired && swapchain != nullptr) {
        if (m_hdrActive) {
            SDL_GPUColorTargetInfo target{};
            target.texture = swapchain;
            target.clear_color = SDL_FColor{0.0f, 0.0f, 0.0f, 1.0f};
            target.load_op = SDL_GPU_LOADOP_CLEAR;
            target.store_op = SDL_GPU_STOREOP_STORE;
            auto *pass = SDL_BeginGPURenderPass(commands, &target, 1, nullptr);
            SDL_GPUViewport viewport{static_cast<float>((width - fittedWidth) / 2),
                                     static_cast<float>((height - fittedHeight) / 2),
                                     static_cast<float>(fittedWidth),
                                     static_cast<float>(fittedHeight),
                                     0.0f,
                                     1.0f};
            SDL_SetGPUViewport(pass, &viewport);
            SDL_BindGPUGraphicsPipeline(pass, m_hdrPipeline);
            SDL_GPUTextureSamplerBinding binding{source, m_hdrSampler};
            SDL_BindGPUFragmentSamplers(pass, 0, &binding, 1);
            const auto properties = SDL_GetWindowProperties(m_window.get());
            const std::array<float, 4> uniforms{
                SDL_GetFloatProperty(properties, SDL_PROP_WINDOW_SDR_WHITE_LEVEL_FLOAT, 1.0f),
                SDL_GetFloatProperty(properties, SDL_PROP_WINDOW_HDR_HEADROOM_FLOAT, 1.0f),
                m_options.screenShader != ScreenShader::None ? 1.0f : 0.0f,
                m_options.scaleFilter == ScaleFilter::Nearest ? 1.0f : 0.0f};
            SDL_PushGPUFragmentUniformData(commands, 0, uniforms.data(), sizeof(uniforms));
            SDL_DrawGPUPrimitives(pass, 3, 1, 0, 0);
            SDL_EndGPURenderPass(pass);
        } else {
            SDL_GPUBlitInfo blit{};
            blit.source.texture = source;
            blit.source.w = sourceWidth;
            blit.source.h = sourceHeight;
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
    }
    if (capture != nullptr) {
        return SaveFrame(commands, source, sourceWidth, sourceHeight, *capture);
    }
    const auto submitStart = SDL_GetTicksNS();
    const bool submitted = SDL_SubmitGPUCommandBuffer(commands);
    m_presentationWait += SDL_GetTicksNS() - submitStart;
    return submitted;
}

bool GpuWindow::SaveFrame(SDL_GPUCommandBuffer *commands,
                          SDL_GPUTexture *frame,
                          Uint32 width,
                          Uint32 height,
                          const std::filesystem::path &path) {
    Uint32 size = width * height * 4;
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
    source.w = width;
    source.h = height;
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
            std::unique_ptr<SDL_Surface, SurfaceDeleter> surface(SDL_CreateSurface(
                static_cast<int>(width), static_cast<int>(height), SDL_PIXELFORMAT_XBGR8888));
            if (surface) {
                for (Uint32 row = 0; row < height; ++row) {
                    std::memcpy(static_cast<Uint8 *>(surface->pixels) + row * surface->pitch,
                                static_cast<const Uint8 *>(pixels) + row * width * 4,
                                width * 4);
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
        SDL_Log("Saved frame %s.", path.string().c_str());
    } else {
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "Cannot save frame: %s", SDL_GetError());
    }
    return saved;
}

GpuWindow *GpuWindow::ForDevice(SDL_GPUDevice *device) {
    return static_cast<GpuWindow *>(
        SDL_GetPointerProperty(SDL_GetGPUDeviceProperties(device), GPU_WINDOW_PROPERTY, nullptr));
}

void GpuWindow::AddGpuMemory(const void *object, Uint64 bytes) {
    if (object != nullptr) {
        m_gpuMemoryObjects[object] = bytes;
        m_gpuMemoryBytes += bytes;
    }
}

void GpuWindow::RemoveGpuMemory(const void *object) {
    auto found = m_gpuMemoryObjects.find(object);
    if (found != m_gpuMemoryObjects.end()) {
        m_gpuMemoryBytes -= found->second;
        m_gpuMemoryObjects.erase(found);
    }
}

SDL_GPUTexture *CreateGpuTexture(SDL_GPUDevice *device, const SDL_GPUTextureCreateInfo *info) {
    auto *window = GpuWindow::ForDevice(device);
    if (window && ++window->m_texturesSincePresent % TEXTURES_BEFORE_WAIT == 0) {
        SDL_WaitForGPUIdle(device);
    }
    SDL_GPUTexture *texture = SDL_CreateGPUTexture(device, info);
    Uint64 bytes = 0;
    for (Uint32 level = 0; level < info->num_levels; ++level) {
        bytes += SDL_CalculateGPUTextureFormatSize(info->format,
                                                   std::max(info->width >> level, 1u),
                                                   std::max(info->height >> level, 1u),
                                                   info->layer_count_or_depth);
    }
    // SDL_GPU_SAMPLECOUNT_2 and the counts after it are 1, 2, and 3.
    if (window) {
        window->AddGpuMemory(texture, bytes << info->sample_count);
    }
    return texture;
}

SDL_GPUBuffer *CreateGpuBuffer(SDL_GPUDevice *device, const SDL_GPUBufferCreateInfo *info) {
    SDL_GPUBuffer *buffer = SDL_CreateGPUBuffer(device, info);
    if (auto *window = GpuWindow::ForDevice(device)) {
        window->AddGpuMemory(buffer, info->size);
    }
    return buffer;
}

SDL_GPUTransferBuffer *CreateGpuTransferBuffer(SDL_GPUDevice *device,
                                               const SDL_GPUTransferBufferCreateInfo *info) {
    SDL_GPUTransferBuffer *buffer = SDL_CreateGPUTransferBuffer(device, info);
    if (auto *window = GpuWindow::ForDevice(device)) {
        window->AddGpuMemory(buffer, info->size);
    }
    return buffer;
}

void ReleaseGpuTexture(SDL_GPUDevice *device, SDL_GPUTexture *texture) {
    if (auto *window = GpuWindow::ForDevice(device)) {
        window->RemoveGpuMemory(texture);
    }
    SDL_ReleaseGPUTexture(device, texture);
}

void ReleaseGpuBuffer(SDL_GPUDevice *device, SDL_GPUBuffer *buffer) {
    if (auto *window = GpuWindow::ForDevice(device)) {
        window->RemoveGpuMemory(buffer);
    }
    SDL_ReleaseGPUBuffer(device, buffer);
}

void ReleaseGpuTransferBuffer(SDL_GPUDevice *device, SDL_GPUTransferBuffer *buffer) {
    if (auto *window = GpuWindow::ForDevice(device)) {
        window->RemoveGpuMemory(buffer);
    }
    SDL_ReleaseGPUTransferBuffer(device, buffer);
}

} // namespace coastline
