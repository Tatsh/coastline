#include "gpu.h"

#include <algorithm>
#include <cstring>

namespace coastline {

namespace {

constexpr SDL_GPUTextureFormat FRAME_FORMAT = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;

struct SurfaceDeleter {
    void operator()(SDL_Surface *surface) const {
        SDL_DestroySurface(surface);
    }
};

} // namespace

std::unique_ptr<GpuWindow> GpuWindow::Create(const char *title, const DisplayOptions &options) {
    std::unique_ptr<GpuWindow> result(new GpuWindow());
    SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    if (options.fullscreen) {
        flags |= SDL_WINDOW_FULLSCREEN;
    }
    result->m_window.reset(SDL_CreateWindow(title, options.width, options.height, flags));
    if (!result->m_window) {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "The window was not created: %s", SDL_GetError());
        return nullptr;
    }
    // The shaders ship as SPIR-V for Vulkan, MSL for Metal, and DXIL for Direct3D 12.
    result->m_device.reset(SDL_CreateGPUDevice(
        SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_MSL | SDL_GPU_SHADERFORMAT_DXIL,
        false,
        options.driver ? options.driver->c_str() : nullptr));
    if (!result->m_device) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "The GPU device was not created: %s", SDL_GetError());
        return nullptr;
    }
    if (!SDL_ClaimWindowForGPUDevice(result->m_device.get(), result->m_window.get())) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "The window was not claimed: %s", SDL_GetError());
        return nullptr;
    }
    if (!options.vsync) {
        SDL_SetGPUSwapchainParameters(result->m_device.get(),
                                      result->m_window.get(),
                                      SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
                                      SDL_GPU_PRESENTMODE_IMMEDIATE);
    }
    result->m_frameWidth = static_cast<Uint32>(options.width);
    result->m_frameHeight = static_cast<Uint32>(options.height);
    result->m_antialias = options.antialias;
    SDL_GPUTextureCreateInfo frame{};
    frame.type = SDL_GPU_TEXTURETYPE_2D;
    frame.format = FRAME_FORMAT;
    frame.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;
    frame.width = result->m_frameWidth;
    frame.height = result->m_frameHeight;
    frame.layer_count_or_depth = 1;
    frame.num_levels = 1;
    result->m_frame = SDL_CreateGPUTexture(result->m_device.get(), &frame);
    if (result->m_frame == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "The frame texture was not created: %s", SDL_GetError());
        return nullptr;
    }
    SDL_Log("Drawing with the %s GPU driver.", SDL_GetGPUDeviceDriver(result->m_device.get()));
    return result;
}

GpuWindow::~GpuWindow() {
    if (m_device) {
        if (m_frame != nullptr) {
            SDL_ReleaseGPUTexture(m_device.get(), m_frame);
        }
        if (m_window) {
            SDL_ReleaseWindowFromGPUDevice(m_device.get(), m_window.get());
        }
    }
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
        blit.source.texture = m_frame;
        blit.source.w = m_frameWidth;
        blit.source.h = m_frameHeight;
        blit.destination.texture = swapchain;
        blit.destination.x = (width - fittedWidth) / 2;
        blit.destination.y = (height - fittedHeight) / 2;
        blit.destination.w = fittedWidth;
        blit.destination.h = fittedHeight;
        blit.load_op = SDL_GPU_LOADOP_CLEAR;
        blit.clear_color = SDL_FColor{0.0f, 0.0f, 0.0f, 1.0f};
        blit.filter = SDL_GPU_FILTER_LINEAR;
        SDL_BlitGPUTexture(commands, &blit);
    }
    if (capture != nullptr) {
        return SaveFrame(commands, *capture);
    }
    return SDL_SubmitGPUCommandBuffer(commands);
}

bool GpuWindow::SaveFrame(SDL_GPUCommandBuffer *commands, const std::filesystem::path &path) {
    Uint32 size = m_frameWidth * m_frameHeight * 4;
    SDL_GPUTransferBufferCreateInfo info{};
    info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD;
    info.size = size;
    SDL_GPUTransferBuffer *buffer = SDL_CreateGPUTransferBuffer(m_device.get(), &info);
    if (buffer == nullptr) {
        SDL_SubmitGPUCommandBuffer(commands);
        return false;
    }
    SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(commands);
    SDL_GPUTextureRegion source{};
    source.texture = m_frame;
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
    SDL_ReleaseGPUTransferBuffer(m_device.get(), buffer);
    if (saved) {
        SDL_Log("Saved the frame to %s.", path.string().c_str());
    } else {
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "The frame was not saved: %s", SDL_GetError());
    }
    return saved;
}

} // namespace coastline
