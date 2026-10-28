#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <string>

#include <SDL3/SDL.h>

namespace coastline {

/** The options of the window and the GPU device. */
struct DisplayOptions {
    int width = 1280;        /*!< The width of the window in pixels. */
    int height = 720;        /*!< The height of the window in pixels. */
    bool fullscreen = false; /*!< Whether the window covers the display. */
    bool vsync = true;       /*!< Whether presentation waits for the vertical blank. */
    std::optional<std::string>
        driver; /*!< An SDL GPU driver, such as vulkan, metal, or direct3d12. */
};

/**
 * The window of the game and the GPU device that draws into it. Each frame is drawn into a
 * texture of its own, which is scaled to the window when presented and can be read back.
 */
class GpuWindow {
public:
    /**
     * Create the window, a GPU device for it, and the frame texture.
     *
     * @param title The title of the window.
     * @param options The size and the driver.
     * @return The window, or nothing with the reason logged.
     */
    static std::unique_ptr<GpuWindow> Create(const char *title, const DisplayOptions &options);

    ~GpuWindow();
    GpuWindow(const GpuWindow &) = delete;
    GpuWindow &operator=(const GpuWindow &) = delete;

    /** @return The window. */
    SDL_Window *Window() const {
        return m_window.get();
    }

    /** @return The GPU device. */
    SDL_GPUDevice *Device() const {
        return m_device.get();
    }

    /**
     * Clear the frame texture to @p color, present it, and save it when @p capture is given.
     *
     * @param color The clear colour.
     * @param capture A PNG file to write the frame to.
     * @return Whether the frame was submitted, and saved when requested.
     */
    bool PresentClear(SDL_FColor color, const std::filesystem::path *capture = nullptr);

    /**
     * Scale the frame texture to the window and submit @p commands, which drew the frame.
     *
     * @param commands The command buffer of the frame, submitted here.
     * @param capture A PNG file to write the frame to.
     * @return Whether the frame was submitted, and saved when requested.
     */
    bool Present(SDL_GPUCommandBuffer *commands, const std::filesystem::path *capture = nullptr);

    /** @return The texture each frame is drawn into. */
    SDL_GPUTexture *FrameTexture() const {
        return m_frame;
    }

    /** @return The format of the frame texture. */
    static constexpr SDL_GPUTextureFormat FrameFormat() {
        return SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    }

    /** @return The width of the frame texture in pixels. */
    Uint32 FrameWidth() const {
        return m_frameWidth;
    }

    /** @return The height of the frame texture in pixels. */
    Uint32 FrameHeight() const {
        return m_frameHeight;
    }

private:
    struct WindowDeleter {
        void operator()(SDL_Window *window) const {
            SDL_DestroyWindow(window);
        }
    };
    struct DeviceDeleter {
        void operator()(SDL_GPUDevice *device) const {
            SDL_DestroyGPUDevice(device);
        }
    };

    GpuWindow() = default;
    bool SaveFrame(SDL_GPUCommandBuffer *commands, const std::filesystem::path &path);

    std::unique_ptr<SDL_Window, WindowDeleter> m_window;
    std::unique_ptr<SDL_GPUDevice, DeviceDeleter> m_device;
    SDL_GPUTexture *m_frame = nullptr;
    Uint32 m_frameWidth = 0;
    Uint32 m_frameHeight = 0;
};

} // namespace coastline
