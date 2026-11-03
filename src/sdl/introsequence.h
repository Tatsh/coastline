#pragma once

#include <filesystem>
#include <memory>

#include <nlohmann/json_fwd.hpp>

namespace coastline {

class SceneRenderer;

/** An ordered asset-pack sequence of images, movies, and static models. */
class IntroSequence {
public:
    /**
     * Validate a pack sequence and resolve its files relative to prepared assets.
     * @param renderer The renderer used for sequence resources.
     * @param assets The prepared, layered asset directory.
     * @param entries Up to 64 entry objects with required type and file settings and optional
     * duration, skippable, scale, and depth settings.
     * @param settings Asset-pack settings, including optional videoControls preferences.
     * @return The sequence, or null for an invalid definition.
     */
    static std::unique_ptr<IntroSequence> Load(SceneRenderer &renderer,
                                               const std::filesystem::path &assets,
                                               const nlohmann::json &entries,
                                               const nlohmann::json &settings);
    /** Release current sequence resources before renderer destruction. */
    ~IntroSequence();
    /** Prevent copying renderer-bound resources. */
    IntroSequence(const IntroSequence &) = delete;
    /** Prevent assigning renderer-bound resources. */
    IntroSequence &operator=(const IntroSequence &) = delete;
    /** Start again at the first available entry. */
    void Restart();
    /**
     * Advance playback and optionally skip the current entry.
     * @param seconds Elapsed frame time in seconds, clamped to [0, 0.25]; non-finite values
     * are ignored.
     * @param skip Whether a skip button was pressed during the frame.
     * @return Whether an entry remains active.
     */
    bool Update(float seconds, bool skip);
    /**
     * Control the current movie without affecting image or model entries.
     * @param pause Whether to toggle pause.
     * @param seek Seek direction, negative for backward and positive for forward.
     * @param restart Whether to restart the movie, taking precedence over seeking.
     */
    void ControlVideo(bool pause, int seek, bool restart);
    /** Draw the current entry, preserving image and movie aspect ratios. */
    void Draw() const;
    /**
     * Draw movie or timed-image progress across the bottom of an output overlay.
     * @param renderer The overlay renderer, between Begin() and RenderInto().
     * @param width Output width in pixels.
     * @param height Output height in pixels; the bar is two pixels high.
     */
    void DrawProgress(SceneRenderer &renderer, float width, float height) const;
    /** @return Whether the active movie or timed image has a progress bar enabled. */
    bool ShowsProgress() const;
    /** @return Whether current movie audio replaces menu music. */
    bool PlayingVideo() const;

private:
    struct State;
    explicit IntroSequence(std::unique_ptr<State> state);
    std::unique_ptr<State> m_state;
};

} // namespace coastline
