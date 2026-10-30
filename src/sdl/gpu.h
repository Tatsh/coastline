#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <string>

#include <SDL3/SDL.h>

namespace coastline {

/** How the frame is scaled to the window. */
enum class ScaleFilter {
    Linear,  /*!< Each pixel of the window blends the nearest four of the frame. */
    Nearest, /*!< Each pixel of the window takes the nearest pixel of the frame. */
};

/** The options of the window and the GPU device. */
struct DisplayOptions {
    int width = 1280;    /*!< The width of the window and of the exclusive mode in pixels. */
    int height = 720;    /*!< The height of the window and of the exclusive mode in pixels. */
    int frameWidth = 0;  /*!< The width of the frame in pixels, 0 to follow the window. */
    int frameHeight = 0; /*!< The height of the frame in pixels, 0 to follow the window. */
    /** The pixels of the frame for each pixel of the window, when the frame follows the window. */
    float renderScale = 1.0f;
    ScaleFilter scaleFilter = ScaleFilter::Linear; /*!< How the frame is scaled to the window. */
    /** Whether the frame stays at the width and the height, for captures that are the same on
     * every machine. */
    bool fixedFrame = false;
    bool fullscreen = false; /*!< Whether the window covers the display. */
    /** Whether the window covers the display in a mode of the width, the height, and the refresh
     * rate, rather than at the desktop mode without borders. */
    bool exclusive = false;
    int refreshRate = 0; /*!< The refresh rate of the exclusive mode in hertz, 0 for the desktop. */
    /** How frames are presented, with the other mode that does not wait and then vertical sync
     * taking the place of a mode the window does not support. */
    SDL_GPUPresentMode presentMode = SDL_GPU_PRESENTMODE_VSYNC;
    /** Whether vertical sync of a Wayland window waits for the vertical blank as on other
     * systems, rather than presenting to the mailbox at the refresh rate of the display. */
    bool fifo = false;
    int antialias = 1; /*!< The samples of each pixel: 1 for none, or 2, 4, or 8. */
    /** The most samples of anisotropic filtering of the textures of models, which are then
     * mipmapped, from 2 to 16, or 0 for neither. */
    int anisotropy = 0;
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

    /** @return Whether the window covers the display. */
    bool Fullscreen() const {
        return (SDL_GetWindowFlags(m_window.get()) & SDL_WINDOW_FULLSCREEN) != 0;
    }

    /**
     * Cover the display, at the desktop mode or the exclusive mode of the options, or return to a
     * window. An exclusive mode the display does not have falls back to the desktop mode.
     *
     * @param fullscreen Whether the window covers the display.
     * @return Whether the window changed, with the reason logged when it did not.
     */
    bool SetFullscreen(bool fullscreen);

    /**
     * @return The nanoseconds between the frames the game paces itself to, one refresh of the
     * display of the window when vertical sync presents to the mailbox, or 0 for no pacing.
     */
    Uint64 RefreshPeriod() const;

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

    /**
     * Give the frame the size of the window in pixels times the render scale, or the size of the
     * options, recreating the frame texture when the size changes. A frame of a size the device
     * cannot create remains at the size before.
     */
    void UpdateFrameSize();

    /** @return The samples of each pixel the options requested, 1 for no anti-aliasing. */
    int Antialias() const {
        return m_antialias;
    }

    /** @return The most samples of anisotropic filtering of the options, 0 for no mipmaps. */
    int Anisotropy() const {
        return m_options.anisotropy;
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
    bool CreateFrame(Uint32 width, Uint32 height);

    std::unique_ptr<SDL_Window, WindowDeleter> m_window;
    std::unique_ptr<SDL_GPUDevice, DeviceDeleter> m_device;
    SDL_GPUTexture *m_frame = nullptr;
    Uint32 m_frameWidth = 0;
    Uint32 m_frameHeight = 0;
    DisplayOptions m_options;
    int m_antialias = 1;
    bool m_exclusive = false;
    int m_refreshRate = 0;
    bool m_pacedToRefresh = false;
};

} // namespace coastline
