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
    // A caption displayed during the credits race.
    struct Caption {
        std::string title;              // The caption heading.
        float duration;                 // The display duration in seconds.
        std::vector<std::string> lines; // The lines below the heading.
    };

    // A line in the scrolling end credits.
    struct ScrollLine {
        int y; // The current vertical position.
        int size;
        int gap; // The spacing after the line.
        std::string text;
    };

    // Report whether the race is the caption race of the credits with a caption still to show.
    bool CaptionsShow() const;

    std::vector<Caption> m_captions;       // The captions read from the release, in order.
    std::size_t m_caption = 0;             // The index of the caption on display.
    float m_captionTimer = 0.0f;           // The seconds the current caption has been displayed.
    std::vector<ScrollLine> m_scrollLines; // The lines of the scrolling end credits.
};

} // namespace coastline
