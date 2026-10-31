#pragma once

/**
 * @file
 * The credits of the Nintendo 64 release over a race of a recording of the credits of the PC
 * release: the captions of the race of the options menu and the credits that scroll up after the
 * last season, read from the main segment of a release.
 */

#include <filesystem>
#include <string>
#include <vector>

namespace coastline {

class MenuText;
class SceneRenderer;

/** Captions and scrolling credits for one attract-demo controller. */
class ConsoleCredits {
public:
    /**
     * Read the captions and the scrolling credits of a release, and start them as the race of the
     * credits starts: the first caption, and the scrolling credits below the screen, as
     * CalculateScrollPositions() of the console places them.
     *
     * @param release The selected cartridge directory, n64/base of the prepared assets.
     * @return Whether the release has the credits, logged when it does not.
     */
    bool Start(const std::filesystem::path &release);

    /**
     * Advance the credits of the race of the credits that runs by a step of the race: the captions
     * as DrawCreditCaption() of the console advances them, or the scrolling credits up a pixel as
     * ScrollTextUp() of the console moves them once a frame.
     *
     * @param seconds The time of the step.
     */
    void Step(float seconds);

    /**
     * Draw the credits of the race of the credits that runs, as DrawCreditCaption() and
     * DrawScrollingText() of the console draw them.
     *
     * @param renderer The renderer of the frame.
     * @param text The fonts of the release.
     */
    void Draw(SceneRenderer &renderer, const MenuText &text) const;

private:
    struct Caption {
        std::string title;
        float duration;
        std::vector<std::string> lines;
    };

    struct ScrollLine {
        int y;
        int size;
        int gap;
        std::string text;
    };

    bool CaptionsShow() const;

    std::vector<Caption> m_captions;
    std::size_t m_caption = 0;
    float m_captionTimer = 0.0f;
    std::vector<ScrollLine> m_scrollLines;
};

} // namespace coastline
