#pragma once

/**
 * @file
 * The main menu of the Nintendo 64 release: the modes of the game, one at a time, each as a model
 * turning on a stage under a spotlight.
 */

#include <filesystem>
#include <memory>
#include <string>

namespace coastline {

class SceneRenderer;

/** The presses of the first controller that act on a menu, each true on the frame it starts. */
struct MenuButtons {
    bool left = false;   /*!< Left on the pad or the stick, which moves to the item before. */
    bool right = false;  /*!< Right on the pad or the stick, which moves to the item after. */
    bool choose = false; /*!< A, Start, or Z, which chooses the item. */
    bool back = false;   /*!< B, which leaves the menu. */
    bool held = false;   /*!< Whether any button is held, which keeps the menu from timing out. */
};

/** Where the main menu goes once it fades out. */
enum class MainMenuEnd {
    Running, /*!< The menu is still showing. */
    Chosen,  /*!< The player chose an item, which Selection() reports. */
    Left,    /*!< The player went back, or the menu timed out, for the splash screens. */
};

/** The main menu, drawn with the import of a release. */
class MainMenu {
public:
    /**
     * Read the models of the items, the textures, the fonts, the lights, and the sounds of the
     * main menu.
     *
     * @param renderer The renderer that draws it.
     * @param release The directory of the import of the release, such as n64/usa.
     * @return The menu, or nothing with the reason logged.
     */
    static std::unique_ptr<MainMenu> Load(SceneRenderer &renderer,
                                          const std::filesystem::path &release);
    ~MainMenu();
    MainMenu(const MainMenu &) = delete;
    MainMenu &operator=(const MainMenu &) = delete;

    /** Show the menu again, fading in, on the item chosen last or the first on the first time. */
    void Restart();

    /**
     * Advance the menu by a frame.
     *
     * @param seconds The time of the frame.
     * @param buttons The presses of the first controller.
     * @return Where the menu goes, once it has faded out.
     */
    MainMenuEnd Update(float seconds, const MenuButtons &buttons);

    /** @return The index of the item under the cursor, from 0 for the championship. */
    int Selection() const;

    /** @return The name of the item under the cursor. */
    const std::string &SelectionName() const;

    /**
     * Draw the frame.
     *
     * @param renderer The renderer of the frame, between Begin() and End().
     */
    void Draw(SceneRenderer &renderer) const;

private:
    struct State;
    explicit MainMenu(std::unique_ptr<State> state);
    std::unique_ptr<State> m_state;
};

} // namespace coastline
