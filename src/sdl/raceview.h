#pragma once

/**
 * @file
 * The view of a race of the PC release: the converted track and car drawn where the race engine
 * places them, from its active camera.
 */

#include <array>
#include <cstdint>
#include <filesystem>
#include <memory>

namespace coastline {

class SceneRenderer;

/** The models of the track and the car of a race, and their drawing from the state of the race. */
class RaceView {
public:
    /**
     * Read the models of a track and a car of the import.
     *
     * @param renderer The renderer that draws them.
     * @param defaults The directory of the default assets.
     * @param track The track, an index of g_apszTrackFilenames.
     * @param car The car, an index of g_apszCarModelPrefixes.
     * @param color The red, green, and blue the car is painted.
     * @return The view, or nothing with the reason logged.
     */
    static std::unique_ptr<RaceView> Load(SceneRenderer &renderer,
                                          const std::filesystem::path &defaults,
                                          int track,
                                          int car,
                                          std::array<std::uint8_t, 3> color);
    ~RaceView();
    RaceView(const RaceView &) = delete;
    RaceView &operator=(const RaceView &) = delete;

    /**
     * Draw the race from the active camera of the first vehicle, blended between its last two
     * frames as the PC release blends them.
     *
     * @param renderer The renderer of the frame, between Begin() and End().
     * @param share How far the frame is from the previous frame of the race to the current one,
     * from 0 to 1.
     */
    void Draw(SceneRenderer &renderer, float share) const;

    /** @return The colour the frame clears to, the fog of the race. */
    std::array<float, 4> ClearColor() const;

private:
    struct State;
    explicit RaceView(std::unique_ptr<State> state);
    std::unique_ptr<State> m_state;
};

} // namespace coastline
