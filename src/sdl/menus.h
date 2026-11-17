#pragma once

/**
 * @file
 * The menu screens of the Nintendo 64 release and the order the console runs them in, each screen a
 * routine SetCurrentScreenCallback() of the console would switch to.
 */

#include <array>
#include <filesystem>
#include <functional>
#include <future>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "config.h"
#include "configsaver.h"
#include "frontend.h"
#include "gamesettings.h"
#include "menuscreen.h"
#include "paintshop.h"
#include "racesession.h"
#include "trackcatalog.h"

namespace coastline {

class GameConfig;
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
    std::array<f32, MENU_TRACK_COUNT>
        lapRecords{}; /*!< The best lap on each track, or 0 for none. */
    std::array<f32, MENU_TRACK_COUNT>
        raceRecords{};        /*!< The best race on each track, or 0 for none. */
    u16 unlocks = 0x102;      /*!< The unlocked cars, a bit for each. */
    u16 unlockedTracks = 4;   /*!< The unlocked tracks, a bit for each. */
    u16 unlockedWeathers = 1; /*!< The weathers the championship has raced in. */
    int controllerLayout = 0; /*!< The button layout of the controller screen. */
};

/** The settings of the options menu, as OptionSettings of the console. */
struct MenuOptions {
    int musicVolume = 10;   /*!< The level of the music, from 0 for silence to 10. */
    int sfxVolume = 4;      /*!< The level of the sound effects, from 0 for silence to 10. */
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
    std::string name;              /*!< The name, in capitals. */
    std::string directory;         /*!< The directory of its model in the import of the release. */
    std::filesystem::path preview; /*!< The model displayed in the car-selection stage. */
    int engineCar = 0;             /*!< The car index of the PC race engine. */
    bool exists = false;           /*!< Whether the car has a model. */
    int driveType = 0;             /*!< The wheels driven, as the index of their name. */
    int engineLayout = 0;          /*!< The layout of the engine, as the index of its name. */
    float acceleration = 0.0f;     /*!< The acceleration shown, a full bar at 500. */
    float maxSpeed = 0.0f;         /*!< The top speed shown, a full bar at 10. */
    int gearCount = 0;             /*!< The gears of the gearbox. */
    float shadowLength = 0.0f;     /*!< The length of its shadow on the stage of the cars. */
    float shadowHalfWidth = 0.0f;  /*!< Half the width of its shadow on the stage of the cars. */
    bool championship = true;      /*!< Whether the selected cartridge offers the car in seasons. */
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
    std::array<u8, 3> paint{}; /*!< The red, green, and blue of the paint of the car. */
    bool otherLook = false;    /*!< Whether the car shows its other look, with its highlighted parts
                                    reflecting the sky in place of its textured parts. */
    /** The milliseconds of the stick held up and down before it presses again, as the repeatUp
     * and repeatDown of the input of the console: positive before the first repeat, negative
     * after it, and 0 while the stick is at rest. */
    std::array<int, 2> stickRepeat{};
};

/** Where the menus go next. */
enum class MenusEnd {
    Running,       /*!< A menu screen is still showing. */
    Quit,          /*!< Two Escape presses request an application exit. */
    Splash,        /*!< The player went back from the main menu, or it timed out. */
    AttractDemo,   /*!< The asset pack starts a demonstration after main-menu inactivity. */
    Race,          /*!< The race of Menus::ChosenRace() starts. */
    Credits,       /*!< The race of the credits of the options menu starts. */
    ApplySettings, /*!< Apply pending settings after the options fade out. */
    Reinitialize,  /*!< Reload game resources and return to the logos. */
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
     * @param release The selected cartridge directory, n64/base of the prepared assets.
     * @param saves The directory of the saves, the data directory of the game, with the
     * Controller Pak and the frames of the ghost car.
     * @param cache The cache directory of the game, for the images of the painted cars.
     * @param config The settings of BossRally.ini, with the cars the players chose last. The
     * settings outlive the menus.
     * @param configPath The file of the settings, written whenever a player chooses a car.
     * @param configSaver The thread that writes the settings. The thread outlives the menus.
     * @param dataRoots The imported data roots for available packs and cartridge regions.
     * @param assetPack The active pack, or empty to use the configured pack.
     * @param unitsRegion The explicit CLI region for automatic units, or empty to use locale.
     * @param packSettings Asset-pack menu heading, idle timeout, and attract-demo preferences.
     * @return The menus, or nothing with the reason logged.
     */
    static std::unique_ptr<Menus>
    Load(SceneRenderer &renderer,
         const std::filesystem::path &release,
         const std::filesystem::path &saves,
         const std::filesystem::path &cache,
         GameConfig &config,
         const std::filesystem::path &configPath,
         ConfigSaver &configSaver,
         std::span<const std::filesystem::path> dataRoots = {},
         std::string_view assetPack = {},
         std::string_view unitsRegion = {},
         const nlohmann::json &packSettings = nlohmann::json::object());
    /** Release front-end resources before the configuration and renderer are destroyed. */
    ~Menus();
    /** Prevent copying active front-end resources and screen progress. */
    Menus(const Menus &) = delete;
    /**
     * Prevent assignment of active front-end resources and screen progress.
     *
     * @return This object. The operator is deleted and is never called.
     */
    Menus &operator=(const Menus &) = delete;

    /** Show the main menu again, as the splash screens and the attract demo switch to it. */
    void Restart();

    /**
     * Report the screen that shows.
     *
     * @return Whether the root main menu is active.
     */
    bool IsMainMenu() const {
        return m_current == Screen::MainMenu;
    }

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

    /**
     * Report whether the mouse cursor is drawn. Only the paint shop draws it.
     *
     * @return Whether the screen that shows draws a cursor for the mouse.
     */
    bool DrawsPointer() const;

    /**
     * Report whether keyboard text goes to the menus. Only a text field of the paint shop takes
     * it.
     *
     * @return Whether the screen that shows takes the text typed on a keyboard.
     */
    bool TakesText() const;

    /**
     * Report how many controllers the screen that shows reads. Every screen but the car select
     * reads the first controller alone.
     *
     * @return The number of players the screen that shows reads, 2 on the car select of two.
     */
    int ControlledPlayers() const;

    /**
     * Build the setup of the race the menus chose: the mode, the track, the weather, the laps, the
     * volumes, the car and equipment of each player, the records, and the ghost of a time attack
     * on the track of the ghost. The race after the last season races the PC track 12.
     *
     * @return The race the menus chose, in the terms of the race engine.
     */
    RaceSetup ChosenRace() const;

    /**
     * Find the model of a car for the race engine.
     *
     * @param car The race-engine car index.
     * @return The car's preview-document path for the selected region and asset pack, or an
     * empty optional when the car is unavailable.
     */
    std::optional<std::filesystem::path> CarImages(int car) const;

    /**
     * Go on from a race that Frame() started, as FinishRace() of the console goes on: to the
     * results screen, to the next season of the championship, or to the main menu.
     *
     * @param end How the race ended.
     * @param results The results of the race, read when it ended with RaceEnd::Results.
     * @param sfxVolumeAdjusted Whether the pause menu changed SFX volume during the race,
     * even if later restored to its initial level.
     */
    void FinishRace(RaceEnd end, const RaceResults &results, bool sfxVolumeAdjusted = false);

    /** Show the options menu again after the race of the credits it started. */
    void FinishCredits();

    /**
     * Set the routine that starts the music module of the menus. The main menu calls the routine
     * as it opens after a module of fewer channels, the module of a race.
     *
     * @param play The routine, or empty for none.
     */
    void SetMenuMusic(std::function<void()> play) {
        m_playMenuMusic = std::move(play);
    }

    /**
     * Display temporary command-line values and disable matching Options entries.
     *
     * @param overrides Per-run settings that are never written to the user's INI file.
     */
    void SetOptionOverrides(GameConfig overrides) {
        if (m_config) {
            const auto source = overrides.Get("Game", "SoundEffects")
                                    .value_or(m_config->Get("Game", "SoundEffects").value_or("PC"));
            m_options.sfxVolume = SoundEffectsVolume(*m_config, source == "N64");
            ApplyVolumeSettings();
        }
        if (m_gameSettings) {
            m_gameSettings->SetOverrides(std::move(overrides));
        }
    }

private:
    // Each screen is processed by its Process routine.
    enum class Screen {
        MainMenu,
        PlayerCountSelect,
        TrackMenu,
        WeatherSelect,
        CarSelect,
        DataSaveLoadMenu, // Controller Pak save and load.
        OptionsMenu,
        RaceResultsScreen,
        PaintShopScreen,
        ControllerConfigScreen
    };

    // Load() reads everything except the menu screen.
    explicit Menus(std::unique_ptr<MenuScreen> screen);

    // Switch on the next frame. Departing the results screen waits for the automatic save of the
    // race first.
    void SetCurrentScreen(Screen screen);

    // Run a frame of the main menu, with the code that unlocks Season Champion. Choosing an item
    // starts its mode or screen, and B or the idle timeout ends the menus at the splash screens or
    // at the attract demo.
    void ProcessMainMenu(SceneRenderer &renderer, float seconds, const MenuButtons &first);

    // Enter the championship for one player on the track menu, at the track and the weather of the
    // next race of the profile.
    void StartChampionshipRace();

    // Enter the time attack for one player on the track menu, with the weather reset to 1.
    void StartTimeAttack();

    // Read the items of the player-count screen from `menu-items/playercount`.
    bool LoadPlayerCountItems(SceneRenderer &renderer, const std::filesystem::path &release);

    // Draw the note of the second controller over the player-count screen while the item under the
    // cursor is unavailable.
    void TwoControllersPostDraw(SceneRenderer &renderer, int item) const;

    // The two-player item is unavailable unless secondController reports a device bound to the
    // second port, and a choice goes on to the track menu.
    void ProcessPlayerCountSelect(SceneRenderer &renderer,
                                  float seconds,
                                  const MenuButtons &first,
                                  bool secondController);

    // Read the tracks, the weathers, the cars, and the seasons from `data/menus.json`, add the
    // tracks of the PC release, and add the load and reset items. Each failure is logged. With
    // pcPreviews the cars show the models of the PC release, as in the `boss` pack. Returns whether
    // the menu loaded with every console track and at least one season.
    bool
    LoadTrackMenu(SceneRenderer &renderer, const std::filesystem::path &release, bool pcPreviews);

    // Add the Japanese cars TYPE-PJ, TYPE-TR, and TYPE-MN outside the championship when the
    // release lacks them. Their models come from a Japanese cartridge imported under one of
    // dataRoots, except in the `boss` pack, and otherwise from the PC models. The packs are layered
    // in cache.
    void AddRegionalCars(const std::filesystem::path &cache,
                         std::span<const std::filesystem::path> dataRoots,
                         std::string_view assetPack);

    // Select the track and the weather of the next race of the season of the profile, the mirrored
    // track while bit 0 of the championship flags is set.
    void SetTrackFromSeason();

    // Load the two cars of the season of the profile into the first two slots, in the colours of
    // the players.
    void LoadSeasonCars(SceneRenderer &renderer);

    // The championship offers only the track of the next race and, after the records, the load and
    // reset items. A time attack also offers its load and reset items. The PC tracks that are not
    // selectable need the Season Champion code.
    bool IsTrackUnlocked(int track) const;

    // Read the records of the PC tracks from the `Records` section of the settings. An invalid
    // record is reset to 0 and the settings are saved.
    void LoadExtraTrackRecords();

    // Write the records of the PC tracks to the `Records` section of the settings in memory.
    void SetExtraTrackRecords();

    // Write the records of the PC tracks to the settings file and wait for the write. Returns false
    // without settings.
    bool SaveExtraTrackRecords();

    // Reset the championship of the profile to the first round, season, or year when a loaded value
    // is out of range.
    void ValidateChampionshipProgress();

    // Draw the notes of the track menu over it (the laps of the arcade, the points of the
    // championship), and run the Controller Pak screen of its load item. A loaded season or ghost
    // car restarts its mode on its track.
    void TrackMenuPostDraw(SceneRenderer &renderer, int item);

    // Draw the records of the season before the first race of a championship session: the races
    // with their records, the race under way blinking, and the two cars of the season on their
    // stages.
    void SeasonRecordsPreDraw(SceneRenderer &renderer);

    // A track goes on to the car select, or to the weather select in the arcade and practice. The
    // extra items load the season or the ghost car, or reset the championship or the ghost, and up
    // and down change the laps of the arcade.
    void ProcessTrackMenu(SceneRenderer &renderer, float seconds, const MenuButtons &first);

    // Read the items of the weather select from `menu-items/weather`.
    bool LoadWeatherItems(SceneRenderer &renderer, const std::filesystem::path &release);

    // Sunny is offered only to one player and off Strip Mine and its mirror.
    bool IsWeatherSelectable(int weather) const;

    // A choice goes on to the car select, and B returns to the track menu.
    void ProcessWeatherSelect(SceneRenderer &renderer, float seconds, const MenuButtons &first);
    // Read the engine layouts, the drive types, and the options of each step from
    // `data/menus.json`, the meshes of the stage from `data/car-stage.json`, and the reflection of
    // the sky. Each failure is logged.
    bool LoadCarSelect(SceneRenderer &renderer, const std::filesystem::path &release);

    // Returns the profile of the first player for 0, otherwise the profile of the second.
    MenuProfile &ProfileOf(int player);

    // The paint shop offers only the cars it paints, and the championship only the cars of the
    // cartridge the profile unlocked. A car must also be in range and have a model.
    bool IsCarUnlocked(int car) const;

    // The paint shop paints the first 9 cars and the Helmet, Milk Truck, Cupra, and Beach Ball.
    bool CanPaintCar(int car) const;

    // Load the model of a car into a slot (0 to 3) in the paint and the look of the player of the
    // slot, as LoadVehicleModel() of the console. Even slots draw the first player and odd slots
    // the second. A model already loaded in that paint and look is reused, and a replaced model is
    // released at the start of the next frame.
    void LoadCarModel(SceneRenderer &renderer, int slot, int car);

    // Paint the car of a player in the colour of the player, in its usual look, as
    // InitVehicleColor() of the console.
    void ResetCarPaint(int player);

    // C-down switches the look once the mirrored cars are unlocked, and up or down raises or lowers
    // the red, green, or blue by 4 while C-left, C-up, or C-right is down. stick records whether
    // the stick pressed up and down on this console frame. The cars of the player are repainted
    // when the paint changes.
    void AdjustCarPaint(SceneRenderer &renderer,
                        int player,
                        const MenuButtons &buttons,
                        std::array<bool, 2> stick);

    // Reload the models of both slots of a player, and the model with the custom decals, in the
    // paint and the look of the player.
    void RepaintCars(SceneRenderer &renderer, int player);

    // Load the model of the car of a player with the custom decals, in the paint and the look of
    // the player. The previous model is released at the start of the next frame, and no model
    // loads without decals and their images.
    void LoadCustomModel(SceneRenderer &renderer, int player);

    // Set the paint of a player to the paint colour of the custom decals loaded, as
    // ExtractVehicleBaseColor() of the console. Without loaded decals the paint is unchanged.
    void TakeDecalPaint(int player);

    // Fade in, and put each player on the car step with the car chosen last, or the first car of
    // the season in the championship, moved on to the next unlocked car.
    void StartCarSelect(SceneRenderer &renderer);

    // The menus retain the options list. name is shown before the option.
    void SetCarSelectOptions(int player,
                             const std::vector<MenuItem> &options,
                             const char *name,
                             int cursor);

    // Move the cursor of a player to the previous or the next option on left or right, wrapping,
    // and start the slide of the options.
    void StepCarSelectOption(int player, const MenuButtons &buttons);

    // As HandleCarSelectInput() of the console. The player chooses a car, then the handling, the
    // transmission, the tyres, the suspension, and the decals, and B steps back. Start or the paint
    // shop makes the player ready at once, and the screen fades out once every player is ready.
    // The equipment chosen is saved.
    void HandleCarSelectInput(SceneRenderer &renderer, int player, const MenuButtons &buttons);

    // Draw the white disc of the stage and its choice marker without a depth test, as
    // DrawCarStageDisc() and DrawCarStageChoiceMarker() of the console.
    void DrawCarStage(SceneRenderer &renderer, const Matrix4 &camera, bool hideChoiceMarker) const;

    // Draw the car of a slot (0 to 3) on its stage with its shadow under it and the sky reflected
    // from eye on its painted body, as DRAW_STAGE_VEHICLE() of the console. x runs across the stage
    // and y along it, angle is in radians, and scissor is in pixels. A slot of a player with custom
    // decals shows them on the decal and ready steps, and a slot without a model draws nothing.
    void DrawStageCar(SceneRenderer &renderer,
                      const Matrix4 &camera,
                      const std::array<float, 3> &eye,
                      int slot,
                      float x,
                      float y,
                      float angle,
                      const SDL_Rect &scissor) const;

    // As RenderCameraView() of the console: the car or the option under the cursor and, while they
    // slide, the one the cursor departed coming in from the far side. y and height are on the 320
    // by 240 screen, and spread is the distance the slide moves the cars apart.
    void DrawPlayerStage(SceneRenderer &renderer, int player, int y, int height, float spread);

    // Draw the engine layout, the gears, the drive type, and the bars of the acceleration and the
    // top speed of the car under the cursor of a player, on the car select of two players. y is the
    // line of the text and barY the bottom of the bars, both on the 320 by 240 screen.
    void DrawCarStats(SceneRenderer &renderer, int player, int y, int barY);

    // Draw what each player is choosing (the car, the step and its option, or ready), the note of
    // the decals, and the statistics of the car on the car step.
    void DrawCarSelectChoices(SceneRenderer &renderer);

    // Write the images of the car of a player with the custom decals to the cache, take the paint
    // of the decals, and load the models that show them.
    void UseCustomDecals(SceneRenderer &renderer, int player);

    // Load the custom decals of the car a player chose from the Controller Pak without the dialog,
    // or reuse the decals already loaded for the car. Returns whether the car has custom decals
    // with their images, false without the paint shop or for a car the paint shop does not paint.
    bool RestoreCustomDecals(SceneRenderer &renderer, int player);

    // Run a frame of the Controller Pak dialog that loads the decals of a player, as RunPakDialog()
    // of the console. Loaded decals add the custom choice and move the cursor to it.
    void RunPakDialog(SceneRenderer &renderer, float seconds);

    // Read the car and the custom decals each player chose last, and the equipment of the second
    // player, from the `Player1` and `Player2` sections of the settings.
    void LoadCarChoices();

    // Save the car and the custom decals of a player to the settings. The equipment of the first
    // player goes to the note of the options, and the equipment of the second to the settings.
    void SaveCarChoice(int player);

    // Depart the car select once it fades out: to the race, to the paint shop, or back to the
    // screen before it.
    void LeaveCarSelect();

    // The console frames of the screen are counted at 30 each second for the paint and the stick
    // repeat, the stages turn, and each player reads their controller.
    void ProcessCarSelectScreen(SceneRenderer &renderer,
                                float seconds,
                                const std::array<MenuButtons, 2> &players);

    // Fade in, record the season, the round, the track, and the weather of the race, and put the
    // car of each player on its stage.
    void StartRaceResults(SceneRenderer &renderer);

    // Advance the season of the profile past the race, and retain the message of the season.
    void ProcessSeasonResults();

    // Advance a profile past the race of the championship, as ProcessSeasonResults() of the
    // console: to the next race, or after the last race to the next season with its unlocks and
    // bonuses, or to the same season again without the points. Other modes change nothing. Returns
    // the lines of the message of the season, empty when there is none.
    std::vector<std::string> AdvanceSeason(MenuProfile &profile) const;

    // Draw the title of the page of the results (the track and the weather, or the season), and the
    // warning of a failed automatic save.
    void DrawRaceResultsTitle(SceneRenderer &renderer);

    // Draw the place of a player and the times of the laps and the race, as DrawRaceResults() of
    // the console.
    void DrawPlayerRaceResults(SceneRenderer &renderer, int player);

    // Draw the races of the season with their times, places, and points, and the total, as
    // DrawSeasonResults() of the console.
    void DrawSeasonResults(SceneRenderer &renderer);

    // The message of the season is centred.
    void DrawSeasonMessage(SceneRenderer &renderer);

    // B exits, and A or Start turns the page, past the last page out of the screen.
    void HandleRaceResultsInput(const MenuButtons &buttons);

    // Depart the results screen once it fades out: to the main menu, to the save of a new ghost,
    // to the save of the season, or to the race after the last season.
    void LeaveRaceResults();

    void ProcessRaceResultsScreen(SceneRenderer &renderer, float seconds, const MenuButtons &first);

    // Read the items of the options menu from `menu-items/options`. Only the USA release has the
    // save and load configuration items.
    bool LoadOptionsItems(SceneRenderer &renderer, const std::filesystem::path &release);

    // Read the notes of the season and the options as the console boots, and set the volumes.
    // Invalid notes are ignored.
    void LoadPakData();

    // Set the music and sound-effect volumes of the audio from the levels of the options.
    void ApplyVolumeSettings() const;

    // Queue a write of the settings to their file, without waiting.
    void SaveConfig();

    // Record the speed units of the options as `Units` of the settings, and save them.
    void RememberSpeedUnits();

    // Record the sound-effect volume as `SfxVolume` of the settings, or `auto` when automatic is
    // set, and save them.
    void RememberSfxVolume(bool automatic = false);

    // Switch between miles and kilometres an hour, and record the choice.
    void ToggleSpeedUnits();

    // Write the note of the options: the volumes, the speed units, and the equipment of a player.
    bool SaveOptionsNote(int player) const;

    // Build the 0x200-byte note of a season from a profile, over the bytes of the saved note.
    std::vector<u8> SeasonNote(const MenuProfile &profile) const;

    bool SaveSeasonNote(const MenuProfile &profile) const;

    // Write the note of the ghost car and the frames of the race engine beside it. Returns 0 on
    // success, 3 when a write failed, or 4 when the recording does not fit a note.
    int SaveGhostNote() const;

    // Save the progress of the race on a thread when `AutoSave` of the settings is on: the season
    // as it stands after the race, the records, and a new ghost of a time attack. A save already
    // running is waited for first.
    void StartAutoSave();

    // Collect the result of the automatic save and set its flags. Without wait, return while the
    // save runs.
    void FinishAutoSave(bool wait);

    // Draw the grey box of a message with a raised border 3 pixels wide, with the paint shop. The
    // box is in pixels of the 640 by 480 screen.
    void DrawMessageBox(int x, int y, int width, int height);

    // Set the text of the paint shop to the size and the white and yellow of a message.
    void SetMessageText();

    // Run a frame of the Controller Pak screen of the track menu and the options menu. The screen
    // loads or saves one note and shows its result or error. dataType is 0 to load the season, 1
    // the ghost car, 2 the configuration, or 3 to save the configuration. Returns whether the
    // screen finished, at once without the paint shop.
    bool ProcessPakDataScreen(SceneRenderer &renderer, int dataType);

    // Draw the notes of the options menu over it, and run the Controller Pak screen of a pending
    // load or save of the configuration.
    void OptionsMenuPostDraw(SceneRenderer &renderer, int item);

    // Up and down change the volumes, the units, and the game settings, and the other items open
    // their screens. Backing out saves changed options and ends the menus to apply pending
    // settings.
    void ProcessOptionsMenu(SceneRenderer &renderer, float seconds, const MenuButtons &first);

    // Read the items of the controller configuration screen and the pictures of the layouts of the
    // console.
    bool LoadControllerScreen(SceneRenderer &renderer, const std::filesystem::path &release);

    // Draw the picture and the labels of a console layout, or the lines of a preset.
    void DrawControllerLayoutDiagram(SceneRenderer &renderer, int item) const;

    // A choice records the layout of the player and the preset of the `Controls` section and
    // returns to the options menu, as B does.
    void
    ProcessControllerConfigScreen(SceneRenderer &renderer, float seconds, const MenuButtons &first);
    // The screen that opens the console save and load menu.
    enum class SaveLoadEntry {
        MainMenu,
        Season,
        TimeAttack,
    };

    // After a race whose progress the automatic save stored, the menu is skipped for the track menu
    // or the main menu.
    void StartDataSaveLoadMenu(SaveLoadEntry entry);

    // Draw a message of one box at the height of the short messages of the save and load menu.
    // boxX and boxWidth are in pixels of the 640 by 480 screen, and textX is on the 320 by 240
    // screen.
    void DrawShortMessage(int boxX, int boxWidth, int textX, std::string_view text);

    // The choice under the cursor is brighter, and every choice is dimmed while a message shows.
    // After a race the menu shows only whether to save.
    void DrawSaveLoadChoices();

    // noteSize is in bytes and selects the message of a full folder.
    void DrawSaveLoadError(int noteSize);

    // Draw the message of a finished load or save of the save and load menu.
    void DrawSaveLoadDone();

    // Draw the request to overwrite a saved note, with the buttons that confirm and cancel.
    void DrawConfirmOverwrite();

    // Read the note of a season into the profiles of both players. player loaded the note, and
    // otherRetainsEquipment preserves the car equipment of the other player. Returns whether the
    // note was valid and read.
    bool ReadSeasonNote(const std::vector<u8> &note, int player, bool otherRetainsEquipment);

    // The values must be in range, and the season and the race must exist with at least one
    // championship car unlocked. A rejected note is logged.
    bool SeasonNoteIsValid(std::span<const u8> note) const;

    // Read a note of the Controller Pak (the season, the options, or the ghost car) and check it by
    // its kind. Returns nothing when the note is missing, invalid, or of another name.
    std::optional<std::vector<u8>> ReadSavedNote(const std::string &name) const;

    // Read the note of the ghost car and the frames of the race engine saved beside it. The track
    // and the car of the ghost become unlocked. Returns whether the note included a valid
    // recording.
    bool ReadGhostNote(const std::vector<u8> &note);

    // Retain the run of a time attack as the new ghost when it finished in fewer steps than the
    // ghost, or the ghost is of another track or empty, as KeepTimeAttackRecording() of the
    // console. recording starts with its header, rewritten with the track, the car, and the
    // equipment of the menus, and frames are the frames of the race engine recorded with the run.
    void KeepTimeAttackRecording(const std::vector<u8> &recording, const std::vector<u8> &frames);

    // Read the note the save and load menu found for its choice. A season or a ghost car fades out
    // to its mode at once, and the options show the message of the load. Returns whether the note
    // was valid and read.
    bool LoadSaveLoadNote(int player);

    // Write the note of the choice of the save and load menu, with the equipment of player in the
    // options. A season also saves the records of the PC tracks. Returns 0 on success, 3 when a
    // write failed, or 4 when a ghost does not fit a note.
    int SaveSaveLoadNote(int player);

    // Depart the save and load menu once it fades out: to the championship or the time attack of
    // loaded data, or back to the screen that opened it.
    void LeaveDataSaveLoadMenu();

    // The save and load menu draws with the paint shop. Without the paint shop the main menu shows
    // at once.
    void ProcessDataSaveLoadMenu(SceneRenderer &renderer, float seconds, const MenuButtons &first);

    std::unique_ptr<MenuScreen> m_screen;
    // The Controller Pak screens draw with the paint shop, and finish at once when it failed to
    // load and is null.
    std::unique_ptr<PaintShop> m_paintShop;
    // The images of the custom decals are written here.
    std::filesystem::path m_cache;
    GameConfig *m_config = nullptr; // Null before Load() sets it.
    std::filesystem::path m_configPath;
    ConfigSaver *m_configSaver = nullptr; // Null before Load() sets it.
    // The buttons, the stick, and the mouse of the first player on the frame, for the dialogs.
    PaintShopInput m_pointer;
    bool m_escapeDown = false; // Escape on the previous frame, to detect a new press.
    // The decals each player loaded from the Controller Pak on the car select screen.
    std::array<std::optional<DecalCar>, 2> m_customDecals;
    std::array<std::optional<std::filesystem::path>, 2> m_customDecalImages;
    std::array<std::unique_ptr<RenderModel>, 2> m_customModels;
    // The paint and the look each model of m_customModels was loaded in, as m_carModelLooks.
    std::array<std::array<int, 4>, 2> m_customModelLooks{};
    // The player whose Controller Pak dialog loads decals, or -1 while no dialog runs.
    int m_pakDialogPlayer = -1;
    std::filesystem::path m_release; // n64/base of the prepared assets.
    Screen m_current = Screen::MainMenu;
    // Where the menus go after the frame, reset to MenusEnd::Running at the start of each.
    MenusEnd m_end = MenusEnd::Running;
    GameMode m_gameMode = GameMode::Championship;
    int m_numLocalPlayers = 1; // 1 or 2.
    // Whether the car select chooses a car for the paint shop rather than a race.
    bool m_paintMode = false;
    // The profile of the first player, with the progress of the championship.
    MenuProfile m_profile;
    // The time of the frame of the track menu, for the blink of the season records.
    float m_frameSeconds = 0.0f;
    // The console never turns the custom colours of the text off once set.
    std::optional<std::array<int, 6>> m_textCustomColors;

    // Starts the music module of the menus, or empty for none.
    std::function<void()> m_playMenuMusic;
    std::vector<MenuItem> m_mainMenuItems;
    // From the settings of the asset pack.
    std::string m_mainMenuTitle = "TOP GEAR RALLY";
    // The seconds without input before the main menu times out, 0 for never.
    float m_mainMenuIdleTimeout = 20;
    // Whether the timeout of the main menu starts the attract demo rather than the splashes.
    bool m_mainMenuAttractDemo = false;
    // 0 to start the main menu with the cursor reset to Championship, -1 to start it with the
    // cursor where it was, and 1 once started. A mode in m_mainMenuEntrySource moves the cursor
    // either way.
    int m_mainMenuInitState = 0;
    int m_mainMenuSelection = 0;
    // The mode that returns to its item of the main menu: 1 for the championship, 2 for the time
    // attack, or 0 for none. The main menu clears it as it starts.
    int m_mainMenuEntrySource = 0;
    // The presses of the Season Champion code entered so far.
    std::size_t m_gamewinCheatProgress = 0;
    // The buttons of the first controller down on the previous frame, to detect new presses.
    std::uint16_t m_gamewinCheatHeld = 0;
    // Whether the Season Champion code unlocked its track for the session.
    bool m_gamewinUnlocked = false;

    std::vector<MenuItem> m_playerCountItems;
    // 0 to start the player-count screen with the cursor on one player, -1 to start it with the
    // cursor where it was, and 1 once started.
    int m_playerCountSelectState = 0;
    int m_playerCountSelection = 0;

    // The tracks, then the load item and the three reset items.
    std::vector<MenuItem> m_trackMenuItems;
    // Whether each track of the track menu has its model imported.
    std::vector<bool> m_trackExists;
    std::vector<std::string> m_weatherNames;
    std::vector<MenuCar> m_cars;
    std::vector<MenuSeason> m_seasons;
    int m_numTracks = 0; // The items before the load item.
    // 0 or -1 to start the track menu on the selected track, and 1 once started. Choosing a track
    // sets -1, and backing out sets 0.
    int m_trackMenuInitState = 0;
    int m_trackMenuSelection = 0;
    int m_selectedTrackId = 2; // An item of the track menu.
    int m_currentWeather = 1;  // An item of the weather select.
    int m_totalLaps = 3;       // The laps of an arcade race, from 1 to 5.
    // Whether the track menu shows the records of the season before the next championship race.
    bool m_seasonFirstRace = false;
    // Whether the track menu returns from the car select or the results, without the records.
    bool m_continueFromResults = false;
    bool m_trackMenuChampionship = false;
    // Whether the Controller Pak screen of the load item of the track menu runs.
    bool m_trackMenuPakScreen = false;
    // The time into the blink of the race under way in the season records, in seconds.
    float m_seasonRecordsBlinkTime = 0.0f;
    // The bytes of m_ghostRecording in use. The console boots with a recording of its header.
    int m_ghostRecordingSize = 8;
    // Whether the last time attack recorded a new ghost, g_dwSaveAvailable of the console.
    bool m_ghostSaveAvailable = false;

    std::vector<MenuItem> m_weatherItems; // In the order of the weathers.
    // 0 to start the weather select with the cursor on the weather of the race, -1 to start it
    // with the cursor where it was, and 1 once started.
    int m_weatherSelectState = 0;
    int m_weatherMenuSelection = 0;

    std::vector<std::string> m_engineLayouts; // By MenuCar::engineLayout.
    std::vector<std::string> m_driveTypes;    // By MenuCar::driveType.
    std::vector<MenuItem> m_handlingOptions;
    std::vector<MenuItem> m_transmissionOptions;
    std::vector<MenuItem> m_tiresOptions;
    std::vector<MenuItem> m_suspensionOptions;
    // The default, a load from the pak, and the custom decals.
    std::vector<MenuItem> m_decalOptions;
    MenuProfile m_secondProfile;
    std::array<CarSelectPlayer, 2> m_carSelect;
    std::array<float, 2> m_carSelectAngle{}; // The turn of the stage of each player, in radians.
    std::array<int, 4> m_slotCars{};         // The car loaded into each slot of the console.
    // By car, in the colour of each of the two vehicles that draw them.
    std::array<std::vector<std::unique_ptr<RenderModel>>, 2> m_carModels;
    // The paint each model of m_carModels was loaded in, in the 5 bits a channel of
    // SetMaterialColor(), and 1 after it for the other look.
    std::array<std::vector<std::array<int, 4>>, 2> m_carModelLooks;
    // The highlighted parts of each model of m_carModels in the other look, or none.
    std::array<std::vector<std::unique_ptr<RenderModel>>, 2> m_carHighlightModels;
    // The models a frame drew and a load replaced, released at the start of the next frame.
    std::vector<std::unique_ptr<RenderModel>> m_retiredCarModels;
    // The document of each car the models of both vehicles take, or none before the first load.
    std::vector<std::shared_ptr<RenderDocument>> m_carDocuments;
    // The time towards the next frame of the console on the car select, in seconds.
    float m_carSelectFrameTime = 0.0f;
    // Whether the frame of the car select is a frame of the console.
    bool m_carSelectConsoleFrame = false;
    // The frames of the console since the car select started.
    std::uint64_t m_carSelectConsoleFrames = 0;
    int m_carSelectFrameMs = 0; // The last frame of the console, 33 or 34.
    StageMesh m_stageChoiceMarker;
    StageMesh m_stageShadow;
    std::unique_ptr<RenderTexture> m_stageReflection; // The sky reflected on the cars.
    bool m_carSelectInitialized = false;
    // Whether every player is ready as the car select fades out, rather than backing out.
    bool m_carSelectDone = false;

    RaceResults m_results;
    // Whether the race to start is the race after the last season.
    bool m_afterSeasonRace = false;
    bool m_resultsInitialized = false;
    // Whether the results screen fades out past its last page, rather than on B.
    bool m_resultsDone = false;
    // Whether the results advanced the season of the profile.
    bool m_seasonResultsShown = false;
    int m_resultsPage = 0;
    int m_resultsSeason = 0; // Before the results advance it.
    int m_resultsRound = 0;  // Before the results advance it.
    int m_resultsTrack = 0;  // An item of the track menu.
    int m_resultsWeather = 0;
    // The championship flags of the profile at the race.
    s32 m_resultsChampionshipFlags = 0;
    std::vector<std::string> m_seasonMessage; // Empty for none.

    // The Controller Pak of the first controller, a directory of the state of the game.
    ControllerPak m_pak{std::filesystem::path()};
    MenuOptions m_options;
    // The game settings follow the items of the console.
    std::vector<MenuItem> m_optionsMenuItems;
    // 0 to start the options menu with the cursor on BGM Volume and the changes cleared, -1 to
    // start it with the cursor where it was, and 1 once started.
    int m_optionsMenuState = 0;
    int m_optionsMenuSelection = 0;
    // Whether the options changed, to save their note as the menu closes.
    bool m_optionsChanged = false;
    bool m_autoSaveFailed = false;
    // Whether the last automatic save stored the progress of the race.
    bool m_progressAutoSaved = false;
    // The thread that writes the automatic save of the last race. m_autoSaveFailed and
    // m_progressAutoSaved are set when the save finishes.
    std::future<bool> m_autoSave;
    // The game settings the options menu offers after the items of the console.
    std::unique_ptr<GameSettings> m_gameSettings;
    // The item of the options menu of the first game setting.
    int m_gameSettingsStart = 0;
    // Whether a game setting changed, to apply as the options menu closes.
    bool m_settingsPending = false;
    // Whether a changed game setting reinitialises the game as the options menu closes.
    bool m_reinitializePending = false;
    // Whether the Controller Pak screen loads the configuration.
    bool m_loadConfigurationPending = false;
    // Whether the Controller Pak screen saves the configuration.
    bool m_saveConfigurationPending = false;

    // The presses of the frame the Controller Pak screen reads in place of the menu under it.
    MenuButtons m_pakInput;
    bool m_pakDataInitialized = false;
    int m_pakDataStep = 0;
    // The frames counted after the sounds faded, before the Controller Pak screen goes on.
    int m_pakDataWaitFrames = 0;
    // The error of the Controller Pak screens: 0 for none, 1 for a missing note, 2 for a full
    // folder, 3 for a failed write, or 4 for a ghost too large for a note.
    int m_pakError = 0;
    // The note the Controller Pak screens found, or nothing.
    std::optional<std::vector<u8>> m_pakNote;

    std::vector<MenuItem> m_controllerScreenItems;
    // The pictures of the layouts of the console.
    std::array<std::unique_ptr<RenderTexture>, 5> m_controllerLayoutPictures;
    // 0 to start the controller configuration screen with the cursor on the preset of the settings
    // or the layout of the player, and 1 once started.
    int m_controllerScreenInitState = 0;
    int m_controllerScreenSelection = 0;

    SaveLoadEntry m_saveLoadEntry = SaveLoadEntry::MainMenu;
    bool m_saveLoadInitialized = false;
    // Whether the choices of the save and load menu are bright, false while a message shows.
    bool m_saveLoadInputEnabled = false;
    // Whether the save and load menu loaded a season or a ghost car to start.
    bool m_saveLoadDataWasLoaded = false;
    int m_saveLoadStep = 0;
    // The choice of the save and load menu, from loading the season to saving the options.
    int m_saveLoadChoice = 0;
    // The frames counted after the sounds faded, before the save and load menu goes on.
    int m_saveLoadFadeDelayFrames = 0;
    // The ghost recording of the console, its track and its car first, g_abGhostRecording.
    std::vector<u8> m_ghostRecording = std::vector<u8>(8);
    // The frames of the race engine the ghost car plays, recorded with m_ghostRecording. A ghost
    // recorded on the console has none.
    std::vector<u8> m_ghostFrames;
    // The file beside the pak that the frames of the ghost are saved to.
    std::filesystem::path m_ghostFramesFile;
};

} // namespace coastline
