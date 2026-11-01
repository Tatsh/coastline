#pragma once

#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

#include <SDL3/SDL.h>

namespace coastline {

/** How the frame is scaled to the window. */
enum class ScaleFilter {
    Linear,       /*!< Each pixel of the window blends the nearest four of the frame. */
    Nearest,      /*!< Each pixel of the window takes the nearest pixel of the frame. */
    EdgeAdaptive, /*!< A directional reconstruction filter follows edges of the frame. */
};

/** How the textures the game filters are filtered. */
enum class TextureFilter {
    N64,     /*!< Three texels, as the RDP of the Nintendo 64 filters. */
    Modern,  /*!< Four texels, bilinear, and trilinear and anisotropic with mipmaps. */
    Nearest, /*!< The nearest texel, unfiltered. */
};

/** The anti-aliasing of the finished frame, before it is scaled to the window. */
enum class PostAntialias {
    None, /*!< The frame as the scene drew it. */
    Fxaa, /*!< Fast approximate anti-aliasing blends along the edges it finds. */
};

enum class ScreenShader { None, ConsumerCrt, Pvm, ApertureGrille, PcCrt };

/** The options of the window and the GPU device. */
struct DisplayOptions {
    bool n64Effects = false; /*!< Console particle textures, opacity, and skid-mark visibility. */
    bool wideMenus = false;  /*!< Fill wide frames with the menu background. */
    float fieldOfViewScale = 1.0f;      /*!< Multiplier of the race camera's field of view. */
    float drawDistanceScale = 1.0f;     /*!< Multiplier of the race view and mirror distances. */
    std::string gpuPreference = "auto"; /*!< Automatic, high-performance, or low-power selection. */
    bool unlockedFramerate =
        false;           /*!< Disable presentation pacing and the software frame limit. */
    int width = 1280;    /*!< The width of the window and of the exclusive mode in pixels. */
    int height = 720;    /*!< The height of the window and of the exclusive mode in pixels. */
    int frameWidth = 0;  /*!< The minimum internal width, 0 to follow the window. */
    int frameHeight = 0; /*!< The minimum internal height, 0 to follow the window. */
    /** The pixels of the frame for each pixel of the window, when the frame follows the window. */
    float renderScale = 1.0f;
    ScaleFilter scaleFilter = ScaleFilter::Linear; /*!< How the frame is scaled to the window. */
    PostAntialias postAntialias = PostAntialias::None; /*!< The anti-aliasing of the frame. */
    ScreenShader screenShader = ScreenShader::None;    /*!< The display simulation after scaling. */
    /** Whether the frame stays at the width and the height, for captures that are the same on
     * every machine. */
    bool fixedFrame = false;
    /** Whether the frame expands from 320x240 to the selected aspect ratio. */
    bool lowestFrame = false;
    /** The width over the height the frame keeps in a window and in the full screen, with bars
     * around it, or 0 for the shape of the window. */
    float aspectRatio = 0.0f;
    /** Whether the GPU device checks its use, with the validation layers of Vulkan when they are
     * installed. */
    bool gpuDebug = false;
    /** Whether the window starts minimized, for runs that save their frames unattended. */
    bool minimized = false;
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
    TextureFilter textureFilter = TextureFilter::Modern; /*!< How textures are filtered. */
    /** Whether the alpha of the texels a model cuts covers the samples of a multisampled pixel. */
    bool alphaToCoverage = true;
    std::optional<std::string>
        driver; /*!< An SDL GPU driver, such as vulkan, metal, or direct3d12. */
};

/**
 * The window of the game and the GPU device that draws into it. Each frame is drawn into a
 * texture of its own, which is scaled to the window when presented and can be read back.
 */
class GpuWindow {
public:
    /** Draw directly over the output texture after scaling and screen filters. */
    using OutputOverlay =
        std::function<void(SDL_GPUCommandBuffer *, SDL_GPUTexture *, Uint32, Uint32)>;
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

    /** @return Whether menu backgrounds fill wide frames. */
    bool WideMenus() const {
        return m_options.wideMenus;
    }

    /** @return The configured multiplier of the race camera's field of view. */
    float FieldOfViewScale() const {
        return m_options.fieldOfViewScale;
    }

    /** @return The race draw-distance multiplier, from 1 to 4. */
    float DrawDistanceScale() const {
        return m_options.drawDistanceScale;
    }

    bool N64Effects() const {
        return m_options.n64Effects;
    }

    /** Apply camera, particle, frame-size, and post-processing settings between frames. */
    bool ApplyLiveOptions(const DisplayOptions &options);
    /** Update sampling and texture filtering before rebuilding scene resources. */
    bool ApplyRendererOptions(const DisplayOptions &options);

    /** @return The window. */
    SDL_Window *Window() const {
        return m_window.get();
    }

    /** @return The GPU device. */
    SDL_GPUDevice *Device() const {
        return m_device.get();
    }

    /**
     * @return Bytes of the GPU device's textures, buffers, and transfer buffers created by the game
     * and not released, excluding allocations internal to the driver.
     */
    Uint64 GpuMemoryBytes() const {
        return m_gpuMemoryBytes;
    }

    /** @return The actual adapter reported by SDL. */
    std::string DeviceName() const;

    /** @return The requested GPU-selection preference used when the device was created. */
    const std::string &GpuPreference() const {
        return m_options.gpuPreference;
    }

    /** @return Whether the active adapter is identified as NVIDIA. */
    bool NvmlAllowed() const;

    /** Set uncapped presentation while retaining the configured present mode. */
    void SetFramerateUnlocked(bool unlocked);

    /** @return Whether software and presentation pacing are disabled. */
    bool FramerateUnlocked() const {
        return m_options.unlockedFramerate;
    }

    /** @return Cumulative nanoseconds spent acquiring and submitting presentation buffers. */
    Uint64 PresentationWaitNanoseconds() const {
        return m_presentationWait;
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

    /** @return The present mode of the options or of the last SetPresentMode(). */
    SDL_GPUPresentMode PresentMode() const {
        return m_options.presentMode;
    }

    /**
     * Present the frames with @p mode, with the other mode that does not wait and then vertical
     * sync in the place of a mode the window does not support. Vertical sync of a Wayland window
     * presents to the mailbox at the refresh rate of the display, unless the options choose fifo.
     *
     * @param mode How frames are presented.
     */
    void SetPresentMode(SDL_GPUPresentMode mode);

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
     * @param overlay Optional draws over the full output after presentation filters.
     * @return Whether the frame was submitted, and saved when requested.
     */
    bool Present(SDL_GPUCommandBuffer *commands,
                 const std::filesystem::path *capture = nullptr,
                 const OutputOverlay &overlay = {});

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

    /** @return How the textures the game filters are filtered. */
    TextureFilter Filter() const {
        return m_options.textureFilter;
    }

    /** @return Whether the alpha of cut texels covers the samples of a multisampled pixel. */
    bool AlphaToCoverage() const {
        return m_options.alphaToCoverage;
    }

    /** @return The most samples of anisotropic filtering of the options, 0 for no mipmaps. */
    int Anisotropy() const {
        return m_options.anisotropy;
    }

private:
    friend SDL_GPUTexture *CreateGpuTexture(SDL_GPUDevice *device,
                                            const SDL_GPUTextureCreateInfo *info);
    friend SDL_GPUBuffer *CreateGpuBuffer(SDL_GPUDevice *device,
                                          const SDL_GPUBufferCreateInfo *info);
    friend SDL_GPUTransferBuffer *
    CreateGpuTransferBuffer(SDL_GPUDevice *device, const SDL_GPUTransferBufferCreateInfo *info);
    friend void ReleaseGpuTexture(SDL_GPUDevice *device, SDL_GPUTexture *texture);
    friend void ReleaseGpuBuffer(SDL_GPUDevice *device, SDL_GPUBuffer *buffer);
    friend void ReleaseGpuTransferBuffer(SDL_GPUDevice *device, SDL_GPUTransferBuffer *buffer);

    static GpuWindow *ForDevice(SDL_GPUDevice *device);
    void AddGpuMemory(const void *object, Uint64 bytes);
    void RemoveGpuMemory(const void *object);

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
    bool SaveFrame(SDL_GPUCommandBuffer *commands,
                   SDL_GPUTexture *source,
                   Uint32 width,
                   Uint32 height,
                   const std::filesystem::path &path);
    bool CreateFrame(Uint32 width, Uint32 height);
    bool CreatePostPasses();
    SDL_GPUTexture *AntialiasFrame(SDL_GPUCommandBuffer *commands);
    bool CreatePresentationPass();
    SDL_GPUTexture *FilterFrame(SDL_GPUCommandBuffer *commands,
                                SDL_GPUTexture *source,
                                Uint32 width,
                                Uint32 height);

    std::unordered_map<const void *, Uint64> m_gpuMemoryObjects;
    Uint64 m_gpuMemoryBytes = 0;
    unsigned m_texturesSincePresent = 0;
    std::unique_ptr<SDL_Window, WindowDeleter> m_window;
    std::unique_ptr<SDL_GPUDevice, DeviceDeleter> m_device;
    SDL_GPUTexture *m_frame = nullptr;
    // The frame after the anti-aliasing pass, its pipeline, and the sampler it reads the frame by.
    SDL_GPUTexture *m_antialiased = nullptr;
    SDL_GPUGraphicsPipeline *m_antialiasPipeline = nullptr;
    SDL_GPUSampler *m_postSampler = nullptr;
    SDL_GPUTexture *m_filtered = nullptr;
    SDL_GPUGraphicsPipeline *m_presentationPipeline = nullptr;
    SDL_GPUSampler *m_presentationSampler = nullptr;
    Uint32 m_filteredWidth = 0;
    Uint32 m_filteredHeight = 0;
    SDL_GPUTexture *m_composite = nullptr;
    Uint32 m_compositeWidth = 0;
    Uint32 m_compositeHeight = 0;
    Uint32 m_frameWidth = 0;
    Uint32 m_frameHeight = 0;
    DisplayOptions m_options;
    Uint64 m_presentationWait = 0;
    int m_antialias = 1;
    bool m_exclusive = false;
    int m_refreshRate = 0;
    bool m_pacedToRefresh = false;
};

/**
 * SDL_CreateGPUTexture(), recording bytes of every level and sample in GpuWindow::GpuMemoryBytes().
 *
 * @param device The GPU device.
 * @param info The texture to create.
 * @return The texture, or null when the device cannot create it.
 */
SDL_GPUTexture *CreateGpuTexture(SDL_GPUDevice *device, const SDL_GPUTextureCreateInfo *info);

/**
 * SDL_CreateGPUBuffer(), recording its size in GpuWindow::GpuMemoryBytes().
 *
 * @param device The GPU device.
 * @param info The buffer to create.
 * @return The buffer, or null when the device cannot create it.
 */
SDL_GPUBuffer *CreateGpuBuffer(SDL_GPUDevice *device, const SDL_GPUBufferCreateInfo *info);

/**
 * SDL_CreateGPUTransferBuffer(), recording its size in GpuWindow::GpuMemoryBytes().
 *
 * @param device The GPU device.
 * @param info The transfer buffer to create.
 * @return The transfer buffer, or null when the device cannot create it.
 */
SDL_GPUTransferBuffer *CreateGpuTransferBuffer(SDL_GPUDevice *device,
                                               const SDL_GPUTransferBufferCreateInfo *info);

/**
 * SDL_ReleaseGPUTexture(), subtracting its bytes from GpuWindow::GpuMemoryBytes().
 *
 * @param device The GPU device.
 * @param texture A texture of CreateGpuTexture(), or null.
 */
void ReleaseGpuTexture(SDL_GPUDevice *device, SDL_GPUTexture *texture);

/**
 * SDL_ReleaseGPUBuffer(), subtracting its bytes from GpuWindow::GpuMemoryBytes().
 *
 * @param device The GPU device.
 * @param buffer A buffer of CreateGpuBuffer(), or null.
 */
void ReleaseGpuBuffer(SDL_GPUDevice *device, SDL_GPUBuffer *buffer);

/**
 * SDL_ReleaseGPUTransferBuffer(), subtracting its bytes from GpuWindow::GpuMemoryBytes().
 *
 * @param device The GPU device.
 * @param buffer A transfer buffer of CreateGpuTransferBuffer(), or null.
 */
void ReleaseGpuTransferBuffer(SDL_GPUDevice *device, SDL_GPUTransferBuffer *buffer);

} // namespace coastline
