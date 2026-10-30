#include "racecredits.h"

#include <algorithm>
#include <array>
#include <string>
#include <string_view>
#include <vector>

#include <SDL3/SDL.h>

#include "common/game.h"
#include "consolesegment.h"
#include "menuscreen.h"
#include "menutext.h"

namespace coastline {

namespace {

// The addresses of g_aCreditCaptions and g_aCreditScrollLines of each release.
struct CreditAddresses {
    std::string_view release;
    u32 captions;
    u32 scrollLines;
};

constexpr std::array<CreditAddresses, 3> RELEASE_ADDRESSES{{
    {"usa", 0x8026FFB4, 0x80270194},
    {"eur", 0x80271F84, 0x80272164},
    {"jpn", 0x80271A34, 0x80271C14},
}};

// A RaceCaption of the console: its title, the time it shows, and its lines from the third word,
// to the first that is NULL.
constexpr u32 CAPTION_SIZE = 0x20;
constexpr std::size_t CAPTION_LINES = 5;

struct Caption {
    std::string title;
    float duration;
    std::vector<std::string> lines;
};

// The time a caption shows before its text.
constexpr float CAPTION_DELAY = 0.5f;

// A CreditLine of the console: its baseline, the size of its text, the space below it, and its
// text, to the first whose text is NULL.
constexpr u32 SCROLL_LINE_SIZE = 0x10;

struct ScrollLine {
    int y;
    int size;
    int gap;
    std::string text;
};

// The lines DrawScrollingText() draws, from a little above the screen to a little below it.
constexpr int SCROLL_TOP = -0x4F;
constexpr int SCROLL_BOTTOM = 280;

std::vector<Caption> g_captions;
std::size_t g_caption = 0;
float g_captionTimer = 0.0f;
std::vector<ScrollLine> g_scrollLines;

bool IsCreditsRace(s32 session) {
    return g_dwGameMode == GAME_MODE_DEMO && g_dwRaceSessionType == session;
}

bool CaptionsShow() {
    return IsCreditsRace(RACE_SESSION_CREDIT_CAPTIONS) && g_caption < g_captions.size();
}

} // namespace

bool StartConsoleCredits(const std::filesystem::path &release) {
    g_captions.clear();
    g_caption = 0;
    g_captionTimer = 0.0f;
    g_scrollLines.clear();
    const auto at = std::ranges::find(
        RELEASE_ADDRESSES, release.filename().string(), &CreditAddresses::release);
    auto segment = ConsoleSegment::Load(release);
    if (!segment || at == RELEASE_ADDRESSES.end()) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "%s has no main segment of a release with the credits.",
                     release.string().c_str());
        return false;
    }
    for (u32 entry = at->captions; segment->Has(entry, CAPTION_SIZE) && segment->Word(entry) != 0;
         entry += CAPTION_SIZE) {
        Caption caption{segment->Text(segment->Word(entry)), segment->Float(entry + 4), {}};
        for (u32 line = 0; line < CAPTION_LINES && segment->Word(entry + 8 + line * 4) != 0;
             ++line) {
            caption.lines.push_back(segment->Text(segment->Word(entry + 8 + line * 4)));
        }
        g_captions.push_back(std::move(caption));
    }
    // CalculateScrollPositions(): each line below the last, from the bottom of the screen.
    int y = 240;
    for (u32 entry = at->scrollLines;
         segment->Has(entry, SCROLL_LINE_SIZE) && segment->Word(entry + 12) != 0;
         entry += SCROLL_LINE_SIZE) {
        ScrollLine line{0,
                        segment->Signed(entry + 4),
                        segment->Signed(entry + 8),
                        segment->Text(segment->Word(entry + 12))};
        int above = line.size * 3 / 4;
        y += above;
        line.y = y;
        y += line.size - above + line.gap;
        g_scrollLines.push_back(std::move(line));
    }
    return true;
}

void StepConsoleCredits(float seconds) {
    // ScrollTextUp() moves the lines up a pixel a frame.
    if (IsCreditsRace(RACE_SESSION_SCROLLING_CREDITS)) {
        for (ScrollLine &line : g_scrollLines) {
            --line.y;
        }
        return;
    }
    if (!CaptionsShow()) {
        return;
    }
    g_captionTimer += seconds;
    if (g_captions[g_caption].duration < g_captionTimer) {
        g_captionTimer = 0.0f;
        ++g_caption;
    }
}

void DrawConsoleCredits(SceneRenderer &renderer, const MenuText &text) {
    const Matrix4 flat = MenuScreen::FlatFor(renderer);
    TextStyle style;
    style.align = TextStyle::Align::Center;
    style.colors = std::array<int, 6>{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    if (IsCreditsRace(RACE_SESSION_SCROLLING_CREDITS)) {
        for (const ScrollLine &line : g_scrollLines) {
            if (line.y >= SCROLL_TOP && line.y < SCROLL_BOTTOM) {
                style.size = line.size;
                text.Draw(renderer, line.text, 160, line.y, style, flat);
            }
        }
        return;
    }
    if (!CaptionsShow() || g_captionTimer <= CAPTION_DELAY) {
        return;
    }
    const Caption &caption = g_captions[g_caption];
    int count = static_cast<int>(caption.lines.size());
    int top = 132 - count * 40 / 4;
    if (caption.title.empty()) {
        top -= 5;
    }
    style.size = 20;
    for (int line = 0; line < count; ++line) {
        text.Draw(renderer,
                  caption.lines[static_cast<std::size_t>(line)],
                  160,
                  line * 40 / 2 + top,
                  style,
                  flat);
    }
    style.size = 15;
    text.Draw(renderer, caption.title, 160, top - 20, style, flat);
}

} // namespace coastline
