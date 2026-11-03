#pragma once

/**
 * @file
 * The splash screens of the Nintendo 64 release: the logos of Midway, Kemco, and Boss Game Studios
 * flying in, then the title and its copyright.
 */

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string_view>

#include <nlohmann/json.hpp>

namespace coastline {

class SceneRenderer;

/** The buttons of the first controller that act on the splash screens. */
struct IntroButtons {
    bool skip = false;       /*!< A, Start, or Z, which skip to the title and then leave it. */
    bool buildDate = false;  /*!< Every C button together, which shows the date of the build. */
    bool pauseVideo = false; /*!< Toggle movie playback pause. */
    int seekVideo = 0; /*!< Movie seek direction, negative for backward and positive for forward. */
    bool restartVideo = false; /*!< Seek the current movie to its beginning. */
};

/** Where the splash screens go once they fade out. */
enum class IntroEnd {
    Running,     /*!< The splash screens are still showing. */
    MainMenu,    /*!< The player skipped them, for the main menu. */
    AttractDemo, /*!< They ran to their end, for the demo of a race. */
};

/** The regional splash screens, drawn with converted models or the Japanese title image. */
class Intro {
public:
    /**
     * Read the models, the lights, the fonts, and the sounds of the splash screens.
     *
     * @param renderer The renderer that draws them.
     * @param release The selected cartridge directory, n64/base of the prepared assets.
     * @param region The chosen title region, or empty to read converted metadata.
     * @param assets The prepared asset root for pack-defined sequences, or empty.
     * @param settings The merged asset-pack settings, or an empty object.
     * @return The splash screens, or nothing with the reason logged.
     */
    static std::unique_ptr<Intro> Load(SceneRenderer &renderer,
                                       const std::filesystem::path &release,
                                       std::string_view region = {},
                                       const std::filesystem::path &assets = {},
                                       const nlohmann::json &settings = nlohmann::json::object());
    /** Release splash-screen resources before their renderer is destroyed. */
    ~Intro();
    /** Prevent copying renderer-bound splash-screen resources. */
    Intro(const Intro &) = delete;
    /** Prevent assignment of renderer-bound splash-screen resources. */
    Intro &operator=(const Intro &) = delete;

    /** Start the splash screens again from the first logo, fading in. */
    void Restart();

    /**
     * Advance the splash screens by a frame.
     *
     * @param seconds The nonnegative elapsed frame time, in seconds.
     * @param buttons The buttons held on the first controller.
     * @return Where the splash screens go, once they have faded out.
     */
    IntroEnd Update(float seconds, const IntroButtons &buttons);

    /**
     * Draw the frame.
     *
     * @param renderer The renderer of the frame, between Begin() and End().
     */
    void Draw(SceneRenderer &renderer) const;

    /** @return Whether a pack movie currently replaces menu music. */
    bool PlayingVideo() const;

private:
    struct State;
    explicit Intro(std::unique_ptr<State> state);
    std::unique_ptr<State> m_state;
};

} // namespace coastline
