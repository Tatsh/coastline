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
     * @return The sequence, or null for an invalid definition.
     */
    static std::unique_ptr<IntroSequence> Load(SceneRenderer &renderer,
                                               const std::filesystem::path &assets,
                                               const nlohmann::json &entries);
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
    /** Draw the current entry, preserving image and movie aspect ratios. */
    void Draw() const;
    /** @return Whether current movie audio replaces menu music. */
    bool PlayingVideo() const;

private:
    struct State;
    explicit IntroSequence(std::unique_ptr<State> state);
    std::unique_ptr<State> m_state;
};

} // namespace coastline
