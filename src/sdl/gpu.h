#pragma once

/**
 * @file
 * The window of the game, its GPU device and frame texture, the display options, and the GPU
 * resource wrappers that count the bytes the game allocates on the device.
 */

#include <filesystem>
#include <functional>
#include <future>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

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

/** Display simulation applied after the internal frame is scaled to the window. */
enum class ScreenShader {
    None,           /*!< Display the scaled frame without a CRT simulation. */
    ConsumerCrt,    /*!< Simulate a consumer television with scanlines and a shadow mask. */
    Pvm,            /*!< Simulate a professional video monitor. */
    ApertureGrille, /*!< Simulate an aperture-grille television. */
    PcCrt           /*!< Simulate a progressive PC monitor. */
};

/** Whether HDR output follows the window, is requested, or is disabled. */
enum class HdrOutput {
    Auto, /*!< Use HDR when the window reports additional display headroom. */
    On,   /*!< Request HDR when the swapchain supports it. */
    Off   /*!< Use SDR output. */
};

/** The options of the window and the GPU device. */
struct DisplayOptions {
    bool n64Effects = false; /*!< Console particle textures, opacity, and skid-mark visibility. */
    bool n64WindowReflections = false; /*!< Generate car reflection coordinates as the N64 RSP. */
    bool wideMenus = false;            /*!< Fill wide frames with the menu background. */
    bool autoWideMenus = false;        /*!< Fill menu backgrounds only for a 16:9 internal frame. */
    float fieldOfViewScale = 1.0f;     /*!< Multiplier of the race camera's field of view. */
    float drawDistanceScale = 1.0f;    /*!< Multiplier of the race view and mirror distances. */
    std::string gpuPreference = "auto"; /*!< Automatic, high-performance, or low-power selection. */
    HdrOutput hdrOutput = HdrOutput::Auto; /*!< Selection of HDR swapchain output. */
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
    /** The width over the height the frame retains in a window and in the full screen, with bars
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
    /** The most samples of anisotropic filtering of the mipmapped textures of models, from 2 to
     * 16, or 0 for neither mipmaps nor anisotropic filtering. */
    int anisotropy = 0;
    TextureFilter textureFilter = TextureFilter::Modern; /*!< How textures are filtered. */
    /** Whether the alpha of the texels a model cuts covers the samples of a multisampled pixel. */
    bool alphaToCoverage = true;
    std::optional<std::string>
        driver; /*!< An SDL GPU driver, such as vulkan, metal, or direct3d12. */
};

/**
 * The window of the game and the GPU device that draws into it. Each frame is drawn into a
 * separate texture. The texture is scaled to the window when presented and can be read back.
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

    /** Release presentation resources and the window after all device-dependent users. */
    ~GpuWindow();
    /** Prevent copying the window and GPU device. */
    GpuWindow(const GpuWindow &) = delete;
    /**
     * Prevent assignment of the window and GPU device.
     *
     * @return This window; the operator is deleted and never returns.
     */
    GpuWindow &operator=(const GpuWindow &) = delete;

    /**
     * Report whether menus draw their background across a wide frame.
     *
     * @return Whether wide menu background rendering is enabled. Automatic mode requires a 16:9
     * internal frame, allowing at most one pixel of width rounding error.
     */
    bool WideMenus() const;

    /**
     * Report the field-of-view setting of the options.
     *
     * @return The configured multiplier of the race camera's field of view.
     */
    float FieldOfViewScale() const {
        return m_options.fieldOfViewScale;
    }

    /**
     * Report the draw-distance setting, clamped by ApplyLiveOptions().
     *
     * @return The race draw-distance multiplier, from 1 to 4.
     */
    float DrawDistanceScale() const {
        return m_options.drawDistanceScale;
    }

    /**
     * Report whether the races draw the particle effects of the console.
     *
     * @return Whether console particle textures, opacity, and skid-mark rules are selected.
     */
    bool N64Effects() const {
        return m_options.n64Effects;
    }

    /**
     * Report which reflection mapping the car windows use.
     *
     * @return Whether car windows use N64 RSP reflection coordinates instead of PC mapping.
     */
    bool N64WindowReflections() const {
        return m_options.n64WindowReflections;
    }

    /**
     * Apply camera, particle, frame-size, post-processing, and pacing settings between frames.
     *
     * @param options The replacement live settings; window and adapter selection are unchanged.
     * @return False if preparing newly enabled filters fails; allocated resources are not rolled
     * back. Later presentation-mode and frame-resize failures are only logged.
     */
    bool ApplyLiveOptions(const DisplayOptions &options);
    /**
     * Update sampling and texture filtering, then apply live settings.
     *
     * @param options The replacement renderer settings; rebuild scene resources after this call.
     * @return The result of ApplyLiveOptions(); sampling settings change even on failure.
     */
    bool ApplyRendererOptions(const DisplayOptions &options);

    /**
     * Give the SDL window for event and property queries.
     *
     * @return The window, managed by this object.
     */
    SDL_Window *Window() const {
        return m_window.get();
    }

    /**
     * Give the GPU device for creating resources and command buffers.
     *
     * @return The GPU device, managed by this object.
     */
    SDL_GPUDevice *Device() const {
        return m_device.get();
    }

    /**
     * Report the GPU memory the game has allocated through CreateGpuTexture(), CreateGpuBuffer(),
     * and CreateGpuTransferBuffer().
     *
     * @return Bytes of the GPU device's textures, buffers, and transfer buffers created by the game
     * and not released, excluding allocations internal to the driver.
     */
    Uint64 GpuMemoryBytes() const {
        return m_gpuMemoryBytes;
    }

    /**
     * Read the adapter name from the properties of the GPU device.
     *
     * @return The actual adapter reported by SDL, or `unknown` when SDL does not report one.
     */
    std::string DeviceName() const;

    /**
     * Report the adapter preference of the options.
     *
     * @return The requested GPU-selection preference used when the device was created.
     */
    const std::string &GpuPreference() const {
        return m_options.gpuPreference;
    }

    /**
     * Report whether NVML may be loaded to sample the utilisation of the adapter.
     *
     * @return Whether the active adapter is identified as NVIDIA.
     */
    bool NvmlAllowed() const;

    /**
     * Report the aspect ratio the frame is fitted to in the window.
     *
     * @return The requested output aspect ratio, or zero to fill the window.
     */
    float AspectRatio() const {
        return m_options.aspectRatio;
    }

    /**
     * Set uncapped presentation while retaining the configured present mode.
     *
     * @param unlocked Whether to disable software pacing and request an uncapped present mode.
     */
    void SetFramerateUnlocked(bool unlocked);

    /**
     * Report whether the frame rate is unlocked.
     *
     * @return Whether software and presentation pacing are disabled.
     */
    bool FramerateUnlocked() const {
        return m_options.unlockedFramerate;
    }

    /**
     * Report the time Present() has waited on the swapchain since the window was created.
     *
     * @return Cumulative nanoseconds spent acquiring and submitting presentation buffers.
     */
    Uint64 PresentationWaitNanoseconds() const {
        return m_presentationWait;
    }

    /**
     * Read the full-screen flag of the window.
     *
     * @return Whether the window covers the display.
     */
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
     * Report the requested present mode, before any fallback the window requires.
     *
     * @return The present mode of the options or of the last SetPresentMode().
     */
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
     * Compute the software pacing period from the current display mode of the window. A display
     * without a known refresh rate is paced at 60 hertz.
     *
     * @return The nanoseconds between the frames the game paces itself to, one refresh of the
     * display of the window when vertical sync presents to the mailbox, or 0 for no pacing.
     */
    Uint64 RefreshPeriod() const;

    /**
     * Clear the frame texture to @p color, present it, and save it when @p capture is given.
     *
     * @param color The clear colour.
     * @param capture A PNG file a thread writes the frame to, logging the result.
     * @return Whether the frame was submitted, and copied for saving when requested.
     */
    bool PresentClear(SDL_FColor color, const std::filesystem::path *capture = nullptr);

    /**
     * Scale the frame texture to the window and submit @p commands, the draws of the frame.
     *
     * @param commands The command buffer of the frame, submitted here.
     * @param capture A PNG file a thread writes the frame to, logging the result.
     * @param overlay Optional draws over the full output after presentation filters.
     * @return Whether the frame was submitted, and copied for saving when requested.
     */
    bool Present(SDL_GPUCommandBuffer *commands,
                 const std::filesystem::path *capture = nullptr,
                 const OutputOverlay &overlay = {});

    /**
     * Give the render target of the scene. UpdateFrameSize() replaces the texture when the frame
     * size changes.
     *
     * @return The texture each frame is drawn into, managed by this object.
     */
    SDL_GPUTexture *FrameTexture() const {
        return m_frame;
    }

    /**
     * Give the format pipelines that draw into the frame texture must target.
     *
     * @return The format of the frame texture, 8-bit RGBA.
     */
    static constexpr SDL_GPUTextureFormat FrameFormat() {
        return SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    }

    /**
     * Report the width of the internal frame.
     *
     * @return The width of the frame texture in pixels.
     */
    Uint32 FrameWidth() const {
        return m_frameWidth;
    }

    /**
     * Report the height of the internal frame.
     *
     * @return The height of the frame texture in pixels.
     */
    Uint32 FrameHeight() const {
        return m_frameHeight;
    }

    /**
     * Give the frame the size of the window in pixels times the render scale, or the size of the
     * options, recreating the frame texture when the size changes. A frame of a size the device
     * cannot create remains at the size before.
     */
    void UpdateFrameSize();

    /**
     * Report the multisample count of the scene.
     *
     * @return The samples of each pixel the options requested, 1 for no anti-aliasing.
     */
    int Antialias() const {
        return m_antialias;
    }

    /**
     * Report the texture filter of the options.
     *
     * @return How the textures the game filters are filtered.
     */
    TextureFilter Filter() const {
        return m_options.textureFilter;
    }

    /**
     * Report whether cut-out textures use alpha to coverage.
     *
     * @return Whether the alpha of cut texels covers the samples of a multisampled pixel.
     */
    bool AlphaToCoverage() const {
        return m_options.alphaToCoverage;
    }

    /**
     * Report the anisotropic filtering of the options.
     *
     * @return The most samples of anisotropic filtering of the options, 0 for no mipmaps.
     */
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

    // Find the window that created a GPU device, through a pointer property Create() records on
    // the device. A device without one gives null.
    static GpuWindow *ForDevice(SDL_GPUDevice *device);
    // Record the size of a created GPU object in GpuMemoryBytes(). A null object (failed
    // creation) is not recorded.
    void AddGpuMemory(const void *object, Uint64 bytes);
    // Subtract the recorded size of a released GPU object from GpuMemoryBytes(). An object not
    // recorded by AddGpuMemory() is ignored.
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

    // An empty window for Create() to fill.
    GpuWindow() = default;
    // Copy a texture (in the format of FrameFormat()) to a transfer buffer, submit commands, wait
    // for the GPU, and start a thread that writes the pixels to a PNG file without alpha, logging
    // the result. At most four saves are pending, and a further save first waits for the oldest.
    // The commands are submitted on every path. Returns whether the pixels were read back and the
    // save started.
    bool SaveFrame(SDL_GPUCommandBuffer *commands,
                   SDL_GPUTexture *source,
                   Uint32 width,
                   Uint32 height,
                   const std::filesystem::path &path);
    // Create the frame texture, and the anti-aliased texture when the anti-aliasing pass exists,
    // releasing the textures before. On failure the textures before remain and the reason is
    // logged.
    bool CreateFrame(Uint32 width, Uint32 height);
    // Create the FXAA pipeline and the linear clamping sampler of the anti-aliasing pass, logging
    // the reason on failure.
    bool CreatePostPasses();
    // Record the FXAA pass from the frame texture into the anti-aliased texture when the options
    // select it and its resources exist. Returns the frame texture when the pass does not run.
    SDL_GPUTexture *AntialiasFrame(SDL_GPUCommandBuffer *commands);
    // Create the pipeline and sampler of the pass that scales the frame with the edge-adaptive
    // filter and the screen shaders. Both are released and the reason logged when either fails.
    bool CreatePresentationPass();
    // Create the pipeline and sampler that write the frame to a 16-bit floating-point HDR
    // swapchain. Both are released and a warning logged when either fails.
    bool CreateHdrPass();
    // Record the presentation pass from the frame or anti-aliased texture into the filtered
    // texture, recreating it when the size changes. Returns null with the reason logged when the
    // filtered texture cannot be created.
    SDL_GPUTexture *FilterFrame(SDL_GPUCommandBuffer *commands,
                                SDL_GPUTexture *source,
                                Uint32 width,
                                Uint32 height);

    // Bytes of each live GPU object the game created.
    std::unordered_map<const void *, Uint64> m_gpuMemoryObjects;
    Uint64 m_gpuMemoryBytes = 0; // Sum of the bytes in m_gpuMemoryObjects.
    // Every 32nd texture created since the last present waits for the GPU to go idle.
    unsigned m_texturesSincePresent = 0;
    std::unique_ptr<SDL_Window, WindowDeleter> m_window;
    std::unique_ptr<SDL_GPUDevice, DeviceDeleter> m_device;
    SDL_GPUTexture *m_frame = nullptr; // The scene draws into this texture each frame.
    // Null without the anti-aliasing pass.
    SDL_GPUTexture *m_antialiased = nullptr;
    SDL_GPUGraphicsPipeline *m_antialiasPipeline = nullptr; // FXAA, or null.
    SDL_GPUSampler *m_postSampler = nullptr;                // Linear sampler of the FXAA pass.
    // Output of the presentation pass, or null before it first runs.
    SDL_GPUTexture *m_filtered = nullptr;
    // Edge-adaptive filter and screen shaders, or null.
    SDL_GPUGraphicsPipeline *m_presentationPipeline = nullptr;
    SDL_GPUSampler *m_presentationSampler = nullptr;  // Linear.
    SDL_GPUGraphicsPipeline *m_hdrPipeline = nullptr; // Null without an HDR swapchain.
    SDL_GPUSampler *m_hdrSampler = nullptr;           // Linear.
    Uint32 m_filteredWidth = 0;
    Uint32 m_filteredHeight = 0;
    // Window-sized texture an output overlay draws into, released when unused.
    SDL_GPUTexture *m_composite = nullptr;
    Uint32 m_compositeWidth = 0;
    Uint32 m_compositeHeight = 0;
    Uint32 m_frameWidth = 0;
    Uint32 m_frameHeight = 0;
    DisplayOptions m_options;      // Updated by the setters.
    Uint64 m_presentationWait = 0; // See PresentationWaitNanoseconds().
    int m_antialias = 1;           // Multisample count of the options.
    // Fixed when the window is created.
    bool m_exclusive = false;
    // Hertz of the exclusive mode, fixed when the window is created.
    int m_refreshRate = 0;
    // Whether Wayland vertical sync was replaced by the mailbox paced by RefreshPeriod().
    bool m_pacedToRefresh = false;
    bool m_hdrActive = false; // Whether the swapchain presents HDR through the HDR pass.
    // Pending PNG saves of captured frames, waited for before the window is destroyed.
    std::vector<std::future<void>> m_frameSaves;
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
