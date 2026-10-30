#pragma once

/**
 * @file
 * The splash screens of the Nintendo 64 release: the logos of Midway, Kemco, and Boss Game Studios
 * flying in, then the title and its copyright.
 */

#include <cstdint>
#include <filesystem>
#include <memory>

namespace coastline {

class SceneRenderer;

/** The buttons of the first controller that act on the splash screens. */
struct IntroButtons {
    bool skip = false;      /*!< A, Start, or Z, which skip to the title and then leave it. */
    bool buildDate = false; /*!< Every C button together, which shows the date of the build. */
};

/** Where the splash screens go once they fade out. */
enum class IntroEnd {
    Running,     /*!< The splash screens are still showing. */
    MainMenu,    /*!< The player skipped them, for the main menu. */
    AttractDemo, /*!< They ran to their end, for the demo of a race. */
};

/** The splash screens of the release of the United States, drawn with the import. */
class Intro {
public:
    /**
     * Read the models, the lights, the fonts, and the sounds of the splash screens.
     *
     * @param renderer The renderer that draws them.
     * @param release The directory of the import of the release, such as n64/usa.
     * @return The splash screens, or nothing with the reason logged.
     */
    static std::unique_ptr<Intro> Load(SceneRenderer &renderer,
                                       const std::filesystem::path &release);
    ~Intro();
    Intro(const Intro &) = delete;
    Intro &operator=(const Intro &) = delete;

    /** Start the splash screens again from the first logo, fading in. */
    void Restart();

    /**
     * Advance the splash screens by a frame.
     *
     * @param seconds The time of the frame.
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

private:
    struct State;
    explicit Intro(std::unique_ptr<State> state);
    std::unique_ptr<State> m_state;
};

} // namespace coastline
