#pragma once

#include <filesystem>
#include <memory>
#include <optional>

namespace coastline {

class SceneRenderer;
class RenderTexture;

/** A decoded video and its audio, with bounded frame buffering. */
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
     * @param volume Audio gain from zero through one.
     * @return Whether playback should continue; false on completion or a decoding error.
     */
    bool Update(double seconds, float volume);
    /**
     * Seek to a movie position and discard buffered video and audio.
     * @param seconds Requested movie time, clamped to [0, 86400]. A known duration caps the
     * result at max(0, duration - 0.05) seconds.
     * @return The selected time in seconds, or an empty optional on failure or non-finite input.
     */
    std::optional<double> Seek(double seconds);
    /**
     * Pause or resume audio and change the current frame to dim greyscale while paused.
     * @param paused Whether playback is paused. The caller must also stop its playback clock.
     */
    void SetPaused(bool paused);
    /** @return Movie duration in seconds, or zero when unavailable. */
    double Duration() const;
    /** @return The borrowed video texture, initially blank and valid until player destruction. */
    const RenderTexture *Texture() const;

private:
    struct State;
    explicit VideoPlayer(std::unique_ptr<State> state);
    std::unique_ptr<State> m_state;
};

} // namespace coastline
