#pragma once

#include <filesystem>
#include <memory>

namespace coastline {

class SceneRenderer;
class RenderTexture;

/** A software-decoded video and its audio, with bounded frame buffering. */
class VideoPlayer {
public:
    /**
     * Open a local movie without starting its clock.
     * @param renderer The renderer used for video textures.
     * @param path The movie file.
     * @return The player, or null if decoding is unavailable or opening fails.
     */
    static std::unique_ptr<VideoPlayer> Open(SceneRenderer &renderer,
                                             const std::filesystem::path &path);
    /** Release decoder, texture, and audio resources. */
    ~VideoPlayer();
    /** Prevent copying decoder resources. */
    VideoPlayer(const VideoPlayer &) = delete;
    /** Prevent assigning decoder resources. */
    VideoPlayer &operator=(const VideoPlayer &) = delete;
    /**
     * Decode through elapsed movie time and update audio gain.
     * @param seconds Nonnegative elapsed seconds since opening.
     * @param volume Audio gain from zero through one. Zero suppresses audio decoding.
     * @return Whether playback should continue; false on completion or a decoding error.
     */
    bool Update(double seconds, float volume);
    /** @return The borrowed video texture, initially blank and valid until player destruction. */
    const RenderTexture *Texture() const;

private:
    struct State;
    explicit VideoPlayer(std::unique_ptr<State> state);
    std::unique_ptr<State> m_state;
};

} // namespace coastline
