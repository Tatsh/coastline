#pragma once

/**
 * @file
 * The menu screens of the Nintendo 64 release and the order the console runs them in, each screen a
 * routine SetCurrentScreenCallback() of the console would switch to.
 */

#include <array>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "menuscreen.h"

namespace coastline {

class SceneRenderer;

/**
 * The parts of the profile of the first player of the console the menus read, as
 * InitPlayerProfileDefaults() of the console sets them.
 */
struct MenuProfile {
    /** The progress of the championship across its years, bit 0 set while the seasons race the
     * tracks mirrored. */
    s32 championshipFlags = 0;
    u8 season = 0;                     /*!< The season of the championship being raced. */
    u8 round = 0;                      /*!< The race of the season being raced. */
    std::array<u16, 6> seasonPoints{}; /*!< The points scored in each season. */
    std::array<f32, 10> lapRecords{};  /*!< The best lap on each track, or 0 for none. */
    std::array<f32, 10> raceRecords{}; /*!< The best race on each track, or 0 for none. */
    u16 unlocks = 0x102;               /*!< The unlocked cars, a bit for each. */
    u16 unlockedTracks = 4;            /*!< The unlocked tracks, a bit for each. */
    u16 unlockedWeathers = 1;          /*!< The weathers the championship has raced in. */
};

/** A season of the championship, as g_SeasonTable of the console records it. */
struct MenuSeason {
    std::string name;                          /*!< The name, the title of the track menu. */
    std::string subtitle;                      /*!< The heading of the records of the season. */
    int raceCount = 0;                         /*!< The number of races. */
    int pointsNeeded = 0;                      /*!< The points that advance to the next season. */
    std::array<int, 2> cars{};                 /*!< The two cars, shown in the records. */
    std::array<std::array<int, 2>, 4> races{}; /*!< The track and the weather of each race. */
};

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
     * @param secondController Whether a device is bound to the second port.
     * @return Where the menus go.
     */
    MenusEnd
    Frame(SceneRenderer &renderer, float seconds, const MenuButtons &first, bool secondController);

private:
    // The screens of the console, each the routine of the same name.
    enum class Screen {
        MainMenu,
        PlayerCountSelect,
        TrackMenu,
        WeatherSelect,
        CarSelect,
        DataSaveLoadMenu,
        OptionsMenu,
    };

    explicit Menus(std::unique_ptr<MenuScreen> screen);
    void SetCurrentScreen(Screen screen);
    void ProcessMainMenu(SceneRenderer &renderer, float seconds, const MenuButtons &first);
    void StartChampionshipRace();
    void StartTimeAttack();
    bool LoadPlayerCountItems(SceneRenderer &renderer, const std::filesystem::path &release);
    void TwoControllersPostDraw(SceneRenderer &renderer, int item) const;
    void ProcessPlayerCountSelect(SceneRenderer &renderer,
                                  float seconds,
                                  const MenuButtons &first,
                                  bool secondController);
    bool LoadTrackMenu(SceneRenderer &renderer, const std::filesystem::path &release);
    void SetTrackFromSeason();
    void LoadSeasonCars(SceneRenderer &renderer);
    bool IsTrackUnlocked(int track) const;
    void ValidateChampionshipProgress();
    void TrackMenuPostDraw(SceneRenderer &renderer, int item);
    void SeasonRecordsPreDraw(SceneRenderer &renderer);
    void ProcessTrackMenu(SceneRenderer &renderer, float seconds, const MenuButtons &first);
    bool LoadWeatherItems(SceneRenderer &renderer, const std::filesystem::path &release);
    bool IsWeatherSelectable(int weather) const;
    void ProcessWeatherSelect(SceneRenderer &renderer, float seconds, const MenuButtons &first);

    std::unique_ptr<MenuScreen> m_screen;
    std::filesystem::path m_release;
    Screen m_current = Screen::MainMenu;
    MenusEnd m_end = MenusEnd::Running;
    GameMode m_gameMode = GameMode::Championship;
    int m_numLocalPlayers = 1;
    bool m_paintMode = false;
    MenuProfile m_profile;
    float m_frameSeconds = 0.0f;
    // The custom colours of the text. The console never turns them off once set.
    std::optional<std::array<int, 6>> m_textCustomColors;

    std::vector<MenuItem> m_mainMenuItems;
    int m_mainMenuInitState = 0;
    int m_mainMenuSelection = 0;
    int m_mainMenuEntrySource = 0;

    std::vector<MenuItem> m_playerCountItems;
    int m_playerCountSelectState = 0;
    int m_playerCountSelection = 0;

    std::vector<MenuItem> m_trackMenuItems;
    std::vector<bool> m_trackExists;
    std::vector<std::string> m_weatherNames;
    std::vector<std::string> m_carDirectories;
    std::vector<MenuSeason> m_seasons;
    std::array<std::unique_ptr<RenderModel>, 2> m_seasonCars;
    int m_numTracks = 0;
    int m_trackMenuInitState = 0;
    int m_trackMenuSelection = 0;
    int m_selectedTrackId = 2;
    int m_currentWeather = 1;
    int m_totalLaps = 3;
    bool m_seasonFirstRace = false;
    bool m_continueFromResults = false;
    bool m_trackMenuChampionship = false;
    bool m_trackMenuPakScreen = false;
    float m_seasonRecordsBlinkTime = 0.0f;
    int m_ghostRecordingSize = 0;

    std::vector<MenuItem> m_weatherItems;
    int m_weatherSelectState = 0;
    int m_weatherMenuSelection = 0;
};

} // namespace coastline
