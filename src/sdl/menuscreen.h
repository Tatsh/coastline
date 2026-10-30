#pragma once

/**
 * @file
 * The menu screen of the Nintendo 64 release that every menu shares, ProcessScreenState() of the
 * console: a title, the items one at a time as models turning on a stage under a spotlight, the
 * buttons at the bottom, and the fades and the sounds of moving and choosing.
 */

#include <array>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "common/types.h"
#include "scenerenderer.h"

namespace coastline {

class MenuText;

/** The flag of an item whose up and down change it in place, as MenuResult::Up and Down. */
constexpr u32 MENU_ITEM_ADJUSTABLE = 0x01;

/** The flag of an item chosen at once, without fading the screen out. */
constexpr u32 MENU_ITEM_IMMEDIATE = 0x02;

/** The flag of an item drawn as its model spinning round as the items slide. */
constexpr u32 MENU_ITEM_SPINNING = 0x04;

/** The flag of an item that cannot be chosen, whose screen explains why. */
constexpr u32 MENU_ITEM_UNAVAILABLE = 0x08;

/** The flag of an item whose model is drawn mirrored. */
constexpr u32 MENU_ITEM_MIRRORED_MODEL = 0x10;

/** The flag of an item only the first player may choose. */
constexpr u32 MENU_ITEM_FIRST_PLAYER = 0x20;

/** The flag of an item that cannot be chosen, without a reason shown. */
constexpr u32 MENU_ITEM_LOCKED = 0x40;

/** The presses of a controller that act on a menu, each true on the frame it starts. */
struct MenuButtons {
    bool left = false;   /*!< Left on the pad or the stick, to move to the item before. */
    bool right = false;  /*!< Right on the pad or the stick, to move to the item after. */
    bool up = false;     /*!< Up on the pad or the stick, to raise an adjustable item. */
    bool down = false;   /*!< Down on the pad or the stick, to lower an adjustable item. */
    bool choose = false; /*!< A, Start, or Z, to choose the item. */
    bool start = false;  /*!< Start, read by itself on the car select screen too. */
    bool back = false;   /*!< B, to exit the screen. */
    bool held = false;   /*!< Whether any button is down, restarting the wait of the timeout. */
    bool cDown = false;  /*!< C down, to switch the look of a car on the car select screen. */
    /** The ControllerButton bits held, without the directions the stick pushes, which the car
     * select screen paints the car with. */
    std::uint16_t heldButtons = 0;
    std::int8_t stickY = 0; /*!< The stick, up positive, about -80 to 80. */
};

/** What a frame of a menu screen ends with, the results of ProcessScreenState(). */
enum class MenuResult {
    None,      /*!< The screen is still showing. */
    Chosen,    /*!< The player chose the item under the cursor. */
    Cancelled, /*!< The player exited the screen, or it timed out. */
    Up,        /*!< The player pushed up on an adjustable item. */
    Down,      /*!< The player pushed down on an adjustable item. */
};

/** An item of a menu screen. */
struct MenuItem {
    std::string name;                   /*!< The text of the item. */
    u32 flags = 0;                      /*!< The MENU_ITEM_ flags. */
    const RenderModel *model = nullptr; /*!< The model of the item, or none to show its name. */
};

/** What a frame of a screen shows, the arguments of ProcessScreenState(). */
struct MenuSetup {
    std::string_view title;                /*!< The heading, drawn in red and yellow. */
    std::span<const MenuItem> items;       /*!< The items. */
    int *selection = nullptr;              /*!< The item chosen, read when the screen starts. */
    std::function<bool(int)> isSelectable; /*!< Whether an item may be moved to, or none for all. */
    std::array<int, 3> top{};              /*!< The red, green, and blue of the dark background. */
    std::array<int, 3> bottom{};           /*!< The colour the texels of the background lerp to. */
};

/** A picture over the title of a screen, and its rectangle on the 320 by 240 screen. */
struct MenuTitleImage {
    const RenderTexture *texture = nullptr; /*!< The picture. */
    int x = 0;                              /*!< The left. */
    int y = 0;                              /*!< The top. */
    int width = 0;                          /*!< The width. */
    int height = 0;                         /*!< The height. */
};

/** The state and the assets of the menu screens, read once from the import of a release. */
class MenuScreen {
public:
    /**
     * Read the background, the arrows, the title pictures, the fonts, the lights, the choice
     * marker, and the sounds the screens share.
     *
     * @param renderer The renderer that draws the screens.
     * @param release The directory of the import of the release, such as n64/usa.
     * @return The screens, or nothing with the reason logged.
     */
    static std::unique_ptr<MenuScreen> Load(SceneRenderer &renderer,
                                            const std::filesystem::path &release);
    ~MenuScreen();
    MenuScreen(const MenuScreen &) = delete;
    MenuScreen &operator=(const MenuScreen &) = delete;

    /**
     * Read the items of a directory of the import, as LoadMenuItemModels() of the console. Items
     * whose model is the same file share it.
     *
     * @param renderer The renderer that draws the models.
     * @param directory The directory, with items.json and a glTF file for each item.
     * @param stems The names of the items in items.json, in the order of the screen.
     * @return The items, or nothing with the reason logged.
     */
    std::optional<std::vector<MenuItem>> LoadItems(SceneRenderer &renderer,
                                                   const std::filesystem::path &directory,
                                                   std::span<const char *const> stems);

    /**
     * Read a model for an item, shared with every other item of the same file.
     *
     * @param renderer The renderer that draws the model.
     * @param path The glTF file.
     * @return The model, or nothing with the reason logged.
     */
    const RenderModel *LoadModel(SceneRenderer &renderer, const std::filesystem::path &path);

    /**
     * ResetAndInitDisplay() of the console: clear the idle timeout and the hooks of the screen
     * before another starts. The title picture is not cleared.
     */
    void Reset();

    /**
     * Run a frame of a screen and draw it, as ProcessScreenState() of the console.
     *
     * @param renderer The renderer of the frame, between Begin() and End().
     * @param seconds The time of the frame.
     * @param players The presses of each local player, from the first.
     * @param setup What the screen shows.
     * @return What the frame ends with.
     */
    MenuResult Process(SceneRenderer &renderer,
                       float seconds,
                       std::span<const MenuButtons> players,
                       const MenuSetup &setup);

    /**
     * SetScreenPendingSelection() of the console: move the cursor to an item on the next frame.
     *
     * @param item The item.
     */
    void SetPendingSelection(int item);

    /** @return The item under the cursor on the last frame, g_nScreenLastActiveIndex. */
    int LastActiveIndex() const;

    /**
     * Set the item under the cursor on the last frame, as a screen does before it starts.
     *
     * @param item The item.
     */
    void SetLastActiveIndex(int item);

    /** @return The player whose press chose the last item. */
    int ChoosingPlayer() const;

    /** @return The title picture of the main menu of the release. */
    const RenderTexture &MainMenuTitle() const;

    /** @return The plain title bar the other screens show. */
    const RenderTexture &TitleBar() const;

    /** @return The fonts of the screens. */
    const MenuText &Text() const;

    /** @return The lights of the models of the screens. */
    const Lighting &MenuLighting() const;

    /**
     * Draw a texture stretched over a rectangle of the 320 by 240 screen, as DrawTexturedRect()
     * of the console, its texels as they are.
     *
     * @param renderer The renderer of the frame.
     * @param texture The texture.
     * @param x The left.
     * @param y The top.
     * @param width The width.
     * @param height The height.
     */
    static void DrawTexture(
        SceneRenderer &renderer, const RenderTexture &texture, int x, int y, int width, int height);

    /**
     * DrawScreenFadeOverlay() of the console under a scissor: darken a rectangle of the 320 by
     * 240 screen.
     *
     * @param renderer The renderer of the frame.
     * @param level The level of the picture shown, from 0 for black to 1 for the picture as it is.
     * @param x The left.
     * @param y The top.
     * @param width The width.
     * @param height The height.
     */
    static void
    DrawFadeOverlay(SceneRenderer &renderer, float level, int x, int y, int width, int height);

    /**
     * @param renderer The renderer of the frame.
     * @return The flat draws of the 640 by 480 screen in the frame.
     */
    static Matrix4 FlatFor(SceneRenderer &renderer);

    /**
     * Draw the background of a screen, its texels lerping from one colour to another.
     *
     * @param renderer The renderer of the frame.
     * @param top The red, green, and blue of the dark background.
     * @param bottom The colour the texels of the background lerp to.
     */
    void DrawBackground(SceneRenderer &renderer,
                        const std::array<int, 3> &top,
                        const std::array<int, 3> &bottom) const;

    /**
     * DrawMenuFooterButtons() of the console: the buttons at the bottom and their names.
     *
     * @param renderer The renderer of the frame.
     */
    void DrawFooter(SceneRenderer &renderer) const;

    /**
     * DrawPauseMenuButtons() of the console: the buttons at the bottom to go on and to exit.
     *
     * @param renderer The renderer of the frame.
     */
    void DrawPauseFooter(SceneRenderer &renderer) const;

    /** Play the sound of moving the cursor. */
    void PlayScrollSound();

    /** Play the sound of choosing. */
    void PlaySelectSound();

    /**
     * Start the fade of a screen that draws itself in, as SetFadeTarget() and FadeSfxTo() of the
     * console fade it in from black.
     */
    void FadeIn();

    /** Fade the screen and the sound effects out. */
    void FadeOut();

    /**
     * Fade the sound effects and the music alone, as FadeSfxTo() and FadeMusicTo() of the console.
     *
     * @param level The level to fade to, from 0 for silence to 1.
     */
    void FadeSounds(float level);

    /** @return Whether the fade of the sound effects has arrived, as HasSfxFadeFinished(). */
    bool SoundsFaded() const;

    /**
     * Advance the fades of the screen and the sound effects by a frame.
     *
     * @param seconds The time of the frame.
     */
    void UpdateFade(float seconds);

    /** @return Whether the screen fades out, as IsFadingOut() of the console. */
    bool FadingOut() const;

    /** @return Whether the fade has arrived at its target, as IsFadeComplete() of the console. */
    bool FadeComplete() const;

    /**
     * Draw the black of the fade, as ApplyScreenFade() of the console.
     *
     * @param renderer The renderer of the frame.
     */
    void DrawFade(SceneRenderer &renderer) const;

    /** Silence every channel of the sound effects, as a screen does once it has faded out. */
    static void SilenceSounds();

    std::optional<MenuTitleImage> titleImage; /*!< The picture over the title, g_pMenuTitleImage. */
    float idleTimeout = 0.0f; /*!< The time without input after which the screen exits, or 0. */
    /** The draw after the item under the cursor, with that item, g_pfnScreenPostDrawCallback. */
    std::function<void(SceneRenderer &, int)> postDraw;
    /** The draw in place of the stage and the items, g_pfnScreenPreDrawCallback. */
    std::function<void(SceneRenderer &)> preDraw;

private:
    struct State;
    explicit MenuScreen(std::unique_ptr<State> state);
    MenuResult Update(float seconds, std::span<const MenuButtons> players, const MenuSetup &setup);
    void Draw(SceneRenderer &renderer, const MenuSetup &setup) const;
    std::unique_ptr<State> m_state;
};

} // namespace coastline
