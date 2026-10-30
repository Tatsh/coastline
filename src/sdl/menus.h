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

#include "frontend.h"
#include "menuscreen.h"
#include "paintshop.h"
#include "racesession.h"

namespace coastline {

class SceneRenderer;

/**
 * The parts of the profile of a player of the console the menus read, as
 * InitPlayerProfileDefaults() of the console sets them.
 */
struct MenuProfile {
    int handling = 0;     /*!< The handling the player chose last. */
    int transmission = 1; /*!< The transmission the player chose last, 1 for the automatic. */
    int tires = 2;        /*!< The tyres the player chose last. */
    int suspension = 1;   /*!< The suspension the player chose last. */
    /** The progress of the championship across its years, bit 0 set while the seasons race the
     * tracks mirrored. */
    s32 championshipFlags = 0;
    u8 season = 0;                     /*!< The season of the championship being raced. */
    u8 round = 0;                      /*!< The race of the season being raced. */
    std::array<u16, 6> seasonPoints{}; /*!< The points scored in each season. */
    /** The place of each race of each season, from 0 for the winner. */
    std::array<std::array<u8, 4>, 6> seasonPlaces{};
    /** The time of each race of each season, in seconds. */
    std::array<std::array<f32, 4>, 6> seasonTimes{};
    std::array<f32, 10> lapRecords{};  /*!< The best lap on each track, or 0 for none. */
    std::array<f32, 10> raceRecords{}; /*!< The best race on each track, or 0 for none. */
    u16 unlocks = 0x102;               /*!< The unlocked cars, a bit for each. */
    u16 unlockedTracks = 4;            /*!< The unlocked tracks, a bit for each. */
    u16 unlockedWeathers = 1;          /*!< The weathers the championship has raced in. */
    int controllerLayout = 0;          /*!< The button layout of the controller screen. */
};

/** The settings of the options menu, as OptionSettings of the console. */
struct MenuOptions {
    int musicVolume = 10;   /*!< The level of the music, from 0 for silence to 10. */
    int sfxVolume = 10;     /*!< The level of the sound effects, from 0 for silence to 10. */
    bool speedInMph = true; /*!< Whether speeds show in miles an hour rather than kilometres. */
};

/** A season of the championship, as g_SeasonTable of the console records it. */
struct MenuSeason {
    std::string name;                          /*!< The name, the title of the track menu. */
    std::string subtitle;                      /*!< The heading of the records of the season. */
    int raceCount = 0;                         /*!< The number of races. */
    int pointsNeeded = 0;                      /*!< The points that advance to the next season. */
    int unlocks = 0;                           /*!< The cars passing the season unlocks. */
    std::array<int, 2> cars{};                 /*!< The two cars, shown in the records. */
    std::array<std::array<int, 2>, 4> races{}; /*!< The track and the weather of each race. */
};

/** A car the players can choose, as g_aCarModels of the console records it. */
struct MenuCar {
    std::string name;             /*!< The name, in capitals. */
    std::string directory;        /*!< The directory of its model in the import of the release. */
    bool exists = false;          /*!< Whether the car has a model. */
    int driveType = 0;            /*!< The wheels driven, as the index of their name. */
    int engineLayout = 0;         /*!< The layout of the engine, as the index of its name. */
    float acceleration = 0.0f;    /*!< The acceleration shown, a full bar at 500. */
    float maxSpeed = 0.0f;        /*!< The top speed shown, a full bar at 10. */
    int gearCount = 0;            /*!< The gears of the gearbox. */
    float shadowLength = 0.0f;    /*!< The length of its shadow on the stage of the cars. */
    float shadowHalfWidth = 0.0f; /*!< Half the width of its shadow on the stage of the cars. */
};

/** The shaded triangles of a display list of the stage of the cars. */
struct StageMesh {
    std::vector<StageVertex> vertices;         /*!< The vertices. */
    std::vector<std::array<int, 3>> triangles; /*!< The vertices of each triangle. */
};

/** The choices of a player on the car select screen, the per-player state of the console. */
struct CarSelectPlayer {
    int state = 0;                                  /*!< The step, by the CAR_SELECT_ values. */
    int cursor = 0;                                 /*!< The option under the cursor. */
    int prevCursor = 0;                             /*!< The option the cursor departed. */
    int optionCount = 0;                            /*!< The options of the step. */
    const std::vector<MenuItem> *options = nullptr; /*!< The options of the step. */
    std::string category;                           /*!< The name of the step. */
    float slide = 0.0f;                             /*!< How far the cars or options slide. */
    int slot = 0;                                   /*!< The slot of the car shown. */
    int selectedCar = 0;                            /*!< The car under the cursor. */
    int carModel = 0;                               /*!< The car chosen. */
    bool customDecals = false;                      /*!< Whether the custom decals were chosen. */
};

/** Where the menus go next. */
enum class MenusEnd {
    Running, /*!< A menu screen is still showing. */
    Splash,  /*!< The player went back from the main menu, or it timed out. */
    Race,    /*!< The race of Menus::ChosenRace() starts. */
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
     * @param state The state directory of the game, with the Controller Pak.
     * @param cache The cache directory of the game, for the images of the painted cars.
     * @return The menus, or nothing with the reason logged.
     */
    static std::unique_ptr<Menus> Load(SceneRenderer &renderer,
                                       const std::filesystem::path &release,
                                       const std::filesystem::path &state,
                                       const std::filesystem::path &cache);
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
     * @param players The presses of the first and the second controller.
     * @param secondController Whether a device is bound to the second port.
     * @param pointer The buttons, the stick, and the mouse of the first player, as the paint shop
     * and the dialogs of the Controller Pak read them.
     * @return Where the menus go.
     */
    MenusEnd Frame(SceneRenderer &renderer,
                   float seconds,
                   const std::array<MenuButtons, 2> &players,
                   bool secondController,
                   const PaintShopInput &pointer);

    /** @return Whether the screen that shows draws a cursor for the mouse. */
    bool DrawsPointer() const;

    /** @return Whether the screen that shows takes the text typed on a keyboard. */
    bool TakesText() const;

    /** @return The race the menus chose, in the terms of the race engine. */
    RaceSetup ChosenRace() const;

    /**
     * Go on from a race that Frame() started, as FinishRace() of the console goes on: to the
     * results screen, to the next season of the championship, or to the main menu.
     *
     * @param end How the race ended.
     * @param results The results of the race, read when it ended with RaceEnd::Results.
     */
    void FinishRace(RaceEnd end, const RaceResults &results);

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
        RaceResultsScreen,
        PaintShopScreen,
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
    bool LoadCarSelect(SceneRenderer &renderer, const std::filesystem::path &release);
    MenuProfile &ProfileOf(int player);
    bool IsCarUnlocked(int car) const;
    void LoadCarModel(SceneRenderer &renderer, int slot, int car);
    void StartCarSelect(SceneRenderer &renderer);
    void SetCarSelectOptions(int player,
                             const std::vector<MenuItem> &options,
                             const char *name,
                             int cursor);
    void StepCarSelectOption(int player, const MenuButtons &buttons);
    void HandleCarSelectInput(SceneRenderer &renderer, int player, const MenuButtons &buttons);
    void DrawCarStage(SceneRenderer &renderer, const Matrix4 &camera, bool hideChoiceMarker) const;
    void DrawStageCar(SceneRenderer &renderer,
                      const Matrix4 &camera,
                      const std::array<float, 3> &eye,
                      int slot,
                      float x,
                      float y,
                      float angle,
                      const SDL_Rect &scissor) const;
    void DrawPlayerStage(SceneRenderer &renderer, int player, int y, int height, float spread);
    void DrawCarStats(SceneRenderer &renderer, int player, int y, int barY);
    void DrawCarSelectChoices(SceneRenderer &renderer);
    void RunPakDialog(SceneRenderer &renderer, float seconds);
    void LeaveCarSelect();
    void ProcessCarSelectScreen(SceneRenderer &renderer,
                                float seconds,
                                const std::array<MenuButtons, 2> &players);
    void StartRaceResults(SceneRenderer &renderer);
    void ProcessSeasonResults();
    void DrawRaceResultsTitle(SceneRenderer &renderer);
    void DrawPlayerRaceResults(SceneRenderer &renderer, int player);
    void DrawSeasonResults(SceneRenderer &renderer);
    void DrawSeasonMessage(SceneRenderer &renderer);
    void HandleRaceResultsInput(const MenuButtons &buttons);
    void LeaveRaceResults();
    void ProcessRaceResultsScreen(SceneRenderer &renderer, float seconds, const MenuButtons &first);
    static int RaceVolumeLevel(int level);
    bool LoadOptionsItems(SceneRenderer &renderer, const std::filesystem::path &release);
    void LoadPakData();
    void ApplyVolumeSettings() const;
    bool SaveOptionsNote(int player) const;
    void DrawMessageBox(int x, int y, int width, int height);
    void SetMessageText();
    bool ProcessPakDataScreen(SceneRenderer &renderer, int dataType);
    void OptionsMenuPostDraw(SceneRenderer &renderer, int item);
    void ProcessOptionsMenu(SceneRenderer &renderer, float seconds, const MenuButtons &first);
    // The screens that open the save and load menu, g_bSaveLoadEntryPoint of the console.
    enum class SaveLoadEntry { MainMenu, Season, TimeAttack };
    void StartDataSaveLoadMenu(SaveLoadEntry entry);
    void DrawShortMessage(int boxX, int boxWidth, int textX, const char *line1, const char *line2);
    void DrawSaveLoadChoices();
    void DrawSaveLoadError(int noteSize);
    void DrawSaveLoadDone();
    void DrawConfirmOverwrite();
    bool ReadSeasonNote(const std::vector<u8> &note, int player, bool otherRetainsEquipment);
    bool ReadGhostNote(const std::vector<u8> &note);
    bool LoadSaveLoadNote(int player);
    int SaveSaveLoadNote(int player);
    void LeaveDataSaveLoadMenu();
    void ProcessDataSaveLoadMenu(SceneRenderer &renderer, float seconds, const MenuButtons &first);

    std::unique_ptr<MenuScreen> m_screen;
    std::unique_ptr<PaintShop> m_paintShop;
    std::filesystem::path m_cache;
    PaintShopInput m_pointer;
    bool m_escapeDown = false;
    // The decals each player loaded from the Controller Pak on the car select screen, the images
    // of the car with them, and the model of the car that shows them.
    std::array<std::optional<DecalCar>, 2> m_customDecals;
    std::array<std::optional<std::filesystem::path>, 2> m_customDecalImages;
    std::array<std::unique_ptr<RenderModel>, 2> m_customModels;
    std::unique_ptr<RenderModel> m_retiredCustomModel;
    int m_pakDialogPlayer = -1;
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
    std::vector<MenuCar> m_cars;
    std::vector<MenuSeason> m_seasons;
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
    // The console boots with a recording of its header alone.
    int m_ghostRecordingSize = 8;

    std::vector<MenuItem> m_weatherItems;
    int m_weatherSelectState = 0;
    int m_weatherMenuSelection = 0;

    std::vector<std::string> m_engineLayouts;
    std::vector<std::string> m_driveTypes;
    std::vector<MenuItem> m_handlingOptions;
    std::vector<MenuItem> m_transmissionOptions;
    std::vector<MenuItem> m_tiresOptions;
    std::vector<MenuItem> m_suspensionOptions;
    std::vector<MenuItem> m_decalOptions;
    MenuProfile m_secondProfile;
    std::array<CarSelectPlayer, 2> m_carSelect;
    std::array<float, 2> m_carSelectAngle{};
    // The car loaded into each of the four slots of the console, and the models of the cars in the
    // colour of each of the two vehicles that draw them.
    std::array<int, 4> m_slotCars{};
    std::array<std::vector<std::unique_ptr<RenderModel>>, 2> m_carModels;
    // The choice marker of the stage of the cars, the shadow of a car on it, and the reflection of
    // the sky on the cars.
    StageMesh m_stageChoiceMarker;
    StageMesh m_stageShadow;
    std::unique_ptr<RenderTexture> m_stageReflection;
    bool m_carSelectInitialized = false;
    bool m_carSelectDone = false;

    // The results screen, the car select screen of the console in its review mode.
    RaceResults m_results;
    bool m_afterSeasonRace = false;
    bool m_resultsInitialized = false;
    bool m_resultsDone = false;
    bool m_seasonResultsShown = false;
    int m_resultsPage = 0;
    int m_resultsSeason = 0;
    int m_resultsRound = 0;
    int m_resultsTrack = 0;
    int m_resultsWeather = 0;
    s32 m_resultsChampionshipFlags = 0;
    std::vector<std::string> m_seasonMessage;

    // The Controller Pak of the first controller, a directory of the state of the game.
    ControllerPak m_pak{std::filesystem::path()};
    MenuOptions m_options;
    std::vector<MenuItem> m_optionsMenuItems;
    int m_optionsMenuState = 0;
    int m_optionsMenuSelection = 0;
    bool m_optionsChanged = false;
    bool m_loadConfigurationPending = false;
    bool m_saveConfigurationPending = false;

    // The screen of the Controller Pak data, and the presses of the frame it reads in place of the
    // menu under it.
    MenuButtons m_pakInput;
    bool m_pakDataInitialized = false;
    int m_pakDataStep = 0;
    int m_pakDataWaitFrames = 0;
    int m_pakError = 0;
    std::optional<std::vector<u8>> m_pakNote;

    // The save and load menu.
    SaveLoadEntry m_saveLoadEntry = SaveLoadEntry::MainMenu;
    bool m_saveLoadInitialized = false;
    bool m_saveLoadInputEnabled = false;
    bool m_saveLoadDataWasLoaded = false;
    int m_saveLoadStep = 0;
    int m_saveLoadChoice = 0;
    int m_saveLoadFadeDelayFrames = 0;
    // The ghost recording of the console, its track and its car first, g_abGhostRecording.
    std::vector<u8> m_ghostRecording = std::vector<u8>(8);
};

} // namespace coastline
