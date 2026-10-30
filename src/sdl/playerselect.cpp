#include <array>

#include "menus.h"
#include "menutext.h"

namespace coastline {

namespace {

// The items of the screen in their order, by the names of their models in the import.
constexpr std::array<const char *, 2> PLAYER_COUNT_STEMS{"one-player", "two-players"};

// The item of the screen that the controllers can make unavailable.
constexpr int ITEM_TWO_PLAYERS = 1;

} // namespace

bool Menus::LoadPlayerCountItems(SceneRenderer &renderer, const std::filesystem::path &release) {
    auto items =
        m_screen->LoadItems(renderer, release / "menu-items" / "playercount", PLAYER_COUNT_STEMS);
    if (!items) {
        return false;
    }
    m_playerCountItems = std::move(*items);
    return true;
}

void Menus::TwoControllersPostDraw(SceneRenderer &renderer, int item) const {
    if (!(m_playerCountItems[static_cast<std::size_t>(item)].flags & MENU_ITEM_UNAVAILABLE)) {
        return;
    }

    // Darken a panel below the menu and write the note on it.
    constexpr int WIDTH = 320;
    constexpr int HEIGHT = 240;
    MenuScreen::DrawFadeOverlay(
        renderer, 0.4f, WIDTH * 9 / 32, HEIGHT * 27 / 64, WIDTH * 14 / 32, HEIGHT * 4 / 16);
    TextStyle style;
    style.size = 15;
    style.align = TextStyle::Align::Center;
    Matrix4 flat = MenuScreen::FlatFor(renderer);
    const MenuText &text = m_screen->Text();
    text.Draw(renderer, "%wwYou Need To Have", WIDTH / 2, HEIGHT * 10 / 16 - 30, style, flat);
    text.Draw(renderer, "%wwTwo Controllers", WIDTH / 2, HEIGHT * 10 / 16 - 15, style, flat);
    text.Draw(renderer, "%wwPlugged in", WIDTH / 2, HEIGHT * 10 / 16, style, flat);
}

void Menus::ProcessPlayerCountSelect(SceneRenderer &renderer,
                                     float seconds,
                                     const MenuButtons &first,
                                     bool secondController) {
    if (m_playerCountSelectState <= 0) {
        m_screen->Reset();
        m_screen->postDraw = [this](SceneRenderer &frame, int item) {
            TwoControllersPostDraw(frame, item);
        };
        if (m_playerCountSelectState == 0) {
            m_playerCountSelection = 0;
        }
        m_playerCountSelectState = 1;
    }

    // Two players need a device for the second player, the keyboard or a gamepad.
    MenuItem &twoPlayers = m_playerCountItems[ITEM_TWO_PLAYERS];
    if (!secondController) {
        twoPlayers.flags |= MENU_ITEM_UNAVAILABLE;
    } else {
        twoPlayers.flags &= ~MENU_ITEM_UNAVAILABLE;
    }

    MenuResult result = m_screen->Process(renderer,
                                          seconds,
                                          {&first, 1},
                                          {.title = "1P/2P SELECT",
                                           .items = m_playerCountItems,
                                           .selection = &m_playerCountSelection,
                                           .isSelectable = {},
                                           .top = {0x00, 0x00, 0x00},
                                           .bottom = {0x3A, 0x86, 0x00}});
    if (result == MenuResult::Chosen) {
        m_numLocalPlayers = m_playerCountSelection + 1;
        SetCurrentScreen(Screen::TrackMenu);
        m_playerCountSelectState = -1;
    } else if (result == MenuResult::Cancelled) {
        SetCurrentScreen(Screen::MainMenu);
        m_playerCountSelectState = -1;
    }
}

} // namespace coastline
