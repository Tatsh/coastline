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
#include <span>

class FrameSnapshot;

namespace coastline {

class SceneRenderer;

/** The car of a vehicle of a race. */
struct RaceCar {
    int car = 0;                         /*!< The car, an index of g_apszCarModelPrefixes. */
    std::array<std::uint8_t, 3> paint{}; /*!< The red, green, and blue the car is painted. */
};

/** The models of the track and the cars of a race, and their drawing from the state of the race. */
class RaceView {
public:
    /**
     * Read the models of a track and the cars of the import.
     *
     * @param renderer The renderer that draws them.
     * @param defaults The directory of the default assets.
     * @param track The track, an index of g_apszTrackFilenames.
     * @param cars The car of each vehicle of the race, from the first. A vehicle that takes
     * another car or colour during the race loads it as it is drawn.
     * @return The view, or nothing with the reason logged.
     */
    static std::unique_ptr<RaceView> Load(SceneRenderer &renderer,
                                          const std::filesystem::path &defaults,
                                          int track,
                                          std::span<const RaceCar> cars);
    ~RaceView();
    RaceView(const RaceView &) = delete;
    RaceView &operator=(const RaceView &) = delete;

    /**
     * Draw each view of the race from the active camera of its vehicle, the views laid out as the
     * race lays them out, blended between the last two frames of the race as the PC release blends
     * them, and the HUD of each view over it.
     *
     * @param renderer The renderer of the frame, between Begin() and End().
     * @param share How far the frame is from the previous frame of the race to the current one,
     * from 0 to 1.
     */
    void Draw(SceneRenderer &renderer, float share) const;

    /**
     * @return The colour the frame clears to: the fog of the race in a single view, and black
     * around the views of a split screen.
     */
    std::array<float, 4> ClearColor() const;

private:
    struct State;
    explicit RaceView(std::unique_ptr<State> state);
    struct CarInView;
    void DrawView(SceneRenderer &renderer, const FrameSnapshot *snapshot, int index) const;
    void DrawCar(SceneRenderer &renderer, const CarInView &view) const;
    std::unique_ptr<State> m_state;
};

} // namespace coastline
