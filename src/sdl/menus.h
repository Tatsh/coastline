#pragma once

/**
 * @file
 * The menu screens of the Nintendo 64 release and the order the console runs them in, each screen a
 * routine SetCurrentScreenCallback() of the console would switch to.
 */

#include <filesystem>
#include <memory>
#include <vector>

#include "menuscreen.h"

namespace coastline {

class SceneRenderer;

/** Where the menus go next. */
enum class MenusEnd {
    Running, /*!< A menu screen is still showing. */
    Splash,  /*!< The player went back from the main menu, or it timed out. */
};

/** The modes of the game, as g_dwGameMode of the console. */
enum class GameMode {
    Championship, /*!< The races of a season of the championship. */
    Arcade,       /*!< A race against the clock and the gates. */
    TimeAttack,   /*!< A time attack against the ghost car. */
    Practice,     /*!< A race alone. */
};

/** The menus, drawn with the import of a release. */
class Menus {
public:
    /**
     * Read the assets of the menu screens and the items of each screen.
     *
     * @param renderer The renderer that draws them.
     * @param release The directory of the import of the release, such as n64/usa.
     * @return The menus, or nothing with the reason logged.
     */
    static std::unique_ptr<Menus> Load(SceneRenderer &renderer,
                                       const std::filesystem::path &release);
    ~Menus();
    Menus(const Menus &) = delete;
    Menus &operator=(const Menus &) = delete;

    /** Show the main menu again, as the splash screens and the attract demo switch to it. */
    void Restart();

    /**
     * Run a frame of the screen that shows and draw it.
     *
     * @param renderer The renderer of the frame, between Begin() and End().
     * @param seconds The time of the frame.
     * @param first The presses of the first controller.
     * @return Where the menus go.
     */
    MenusEnd Frame(SceneRenderer &renderer, float seconds, const MenuButtons &first);

private:
    // The screens of the console, each the routine of the same name.
    enum class Screen { MainMenu, NotBuilt };

    explicit Menus(std::unique_ptr<MenuScreen> screen);
    void ProcessMainMenu(SceneRenderer &renderer, float seconds, const MenuButtons &first);

    std::unique_ptr<MenuScreen> m_screen;
    Screen m_current = Screen::MainMenu;
    MenusEnd m_end = MenusEnd::Running;
    GameMode m_gameMode = GameMode::Championship;
    int m_numLocalPlayers = 1;
    bool m_paintMode = false;

    std::vector<MenuItem> m_mainMenuItems;
    int m_mainMenuInitState = 0;
    int m_mainMenuSelection = 0;
};

} // namespace coastline
