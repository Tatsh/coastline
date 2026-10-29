#pragma once

#include <filesystem>
#include <memory>

namespace coastline {

class GpuWindow;

/**
 * A view of a model or a track the import converted to glTF, drawn by the GPU with its textures
 * and materials around a camera the arrow keys turn.
 */
class ModelView {
public:
    /**
     * Read a glTF document of the import and upload its images and vertices.
     *
     * @param window The window to draw into.
     * @param path The glTF document.
     * @return The view, or nothing with the reason logged.
     */
    static std::unique_ptr<ModelView> Create(GpuWindow &window, const std::filesystem::path &path);
    ~ModelView();
    ModelView(const ModelView &) = delete;
    ModelView &operator=(const ModelView &) = delete;

    /**
     * Turn the camera by the keys held, and draw and present a frame.
     *
     * @param capture A PNG file to write the frame to, or null.
     */
    void Draw(const std::filesystem::path *capture);

private:
    struct State;
    explicit ModelView(std::unique_ptr<State> state);
    std::unique_ptr<State> m_state;
};

} // namespace coastline
