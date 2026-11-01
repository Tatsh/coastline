#pragma once

/**
 * @file
 * The first-run setup wizard locates and imports disc and ROM images on screens drawn on a
 * TextGrid with the blue and grey palette of text-mode Windows Setup. The wizard processes events
 * and produces a grid without calling SDL.
 */

#include <array>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "filebrowser.h"
#include "textgrid.h"

namespace coastline {

/** A supported ROM release. */
enum class WizardCartridge {
    Usa, /*!< The USA release. */
    Eur, /*!< The European release. */
    Jpn, /*!< The Japanese release. */
};

/** The number of supported ROM releases. */
inline constexpr int WIZARD_CARTRIDGES = 3;

/**
 * Choose the base release for the default pack. Prefer the USA release when detected, then the
 * European release, then the Japanese release.
 *
 * @param checked Whether each release is available, in the order of WizardCartridge.
 * @return The release, or nothing when no release is available.
 */
std::optional<WizardCartridge> BaseCartridge(const std::array<bool, WIZARD_CARTRIDGES> &checked);

/** The files an import of the wizard reads. */
struct WizardImport {
    std::filesystem::path disc; /*!< The CUE sheet, ISO image, or BIN image of the disc. */
    /** The ROM images by release, with the base of the default pack first. */
    std::vector<std::pair<WizardCartridge, std::filesystem::path>> cartridges;
};

/** Reports the step of an import and its share done from 0 to 1, and returns false to cancel. */
using WizardProgress = std::function<bool(const std::string &step, double fraction)>;

/** The routines the wizard calls to check and import its files. */
struct WizardServices {
    /** The reason a disc image is not Boss Rally, or nothing when it is. */
    std::function<std::optional<std::string>(const std::filesystem::path &)> checkDisc;
    /** The release of a ROM image by its SHA-1, or nothing when it is not Top Gear Rally. */
    std::function<std::optional<WizardCartridge>(const std::filesystem::path &)> identifyCartridge;
    /** Search ZIP entries and report validated ROM images cached outside the archive. */
    std::function<void(const std::filesystem::path &,
                       const std::function<bool(WizardCartridge, const std::filesystem::path &)> &,
                       const WizardProgress &)>
        scanArchive;
    /** Import files on a separate thread and return a message on failure. */
    std::function<std::optional<std::string>(const WizardImport &, const WizardProgress &)> import;
    std::filesystem::path home;          /*!< The directory the file browser starts in at first. */
    std::filesystem::path lastDirectory; /*!< The directory last browsed, or empty. */
    std::string buildRevision;           /*!< A version tag or short source revision. */
    std::string buildDate;               /*!< The date the executable was built. */
};

/** The images found during a directory search, indexed by detected release. */
struct WizardSearchResult {
    std::filesystem::path disc;
    std::array<std::filesystem::path, WIZARD_CARTRIDGES> roms;
    std::size_t examined = 0;
    std::size_t candidates = 0;
    std::optional<std::string> failure;
};

/** Report search progress and return false to cancel. */
using WizardSearchProgress =
    std::function<bool(const WizardSearchResult &, const std::string &, double)>;

/** Search a directory recursively for validated disc and ROM images. */
WizardSearchResult SearchWizardDirectory(const std::filesystem::path &directory,
                                         const WizardServices &services,
                                         const WizardSearchProgress &progress);

/** A key of the keyboard the wizard reads. */
enum class WizardKey {
    Tab,            /*!< Tab, or Shift and Tab. */
    Enter,          /*!< Enter. */
    Space,          /*!< Space. */
    Escape,         /*!< Escape. */
    Up,             /*!< The up arrow. */
    Down,           /*!< The down arrow. */
    Left,           /*!< The left arrow. */
    Right,          /*!< The right arrow. */
    PageUp,         /*!< Page Up. */
    PageDown,       /*!< Page Down. */
    Home,           /*!< Home. */
    End,            /*!< End. */
    Backspace,      /*!< Backspace. */
    Delete,         /*!< Delete. */
    SelectAll,      /*!< Select the entire text. */
    DeleteWord,     /*!< Delete the previous word. */
    DeleteNextWord, /*!< Delete the next word. */
    WordLeft,       /*!< Move to the previous word. */
    WordRight,      /*!< Move to the next word. */
};

/** An event of the keyboard or the mouse. */
struct WizardEvent {
    /** The kind of event. */
    enum class Type {
        Key,         /*!< A press of #key. */
        Text,        /*!< #text typed or pasted. */
        Accelerator, /*!< Alt and #letter. */
        MouseDown,   /*!< The left button pressed at #x and #y. */
        MouseUp,     /*!< The left button released at #x and #y. */
        MouseMove,   /*!< The mouse moved to #x and #y. */
        Wheel,       /*!< The wheel turned #wheel notches at #x and #y. */
    };
    Type type = Type::Key;          /*!< The kind of event. */
    WizardKey key = WizardKey::Tab; /*!< The key of a Key event. */
    bool shift = false;             /*!< Whether Shift is pressed with the key. */
    std::string text;               /*!< The UTF-8 text of a Text event. */
    char letter = 0;                /*!< The lowercase letter of an Accelerator event. */
    int x = 0;                      /*!< The mouse across the 640 by 400 grid, in pixels. */
    int y = 0;                      /*!< The mouse down the 640 by 400 grid, in pixels. */
    int wheel = 0; /*!< The notches of a Wheel event, above 0 away from the user. */
    std::uint64_t milliseconds = 0; /*!< The time of the event, for double-clicks and typing. */
};

/** A screen of the wizard, in order. */
enum class WizardScreen {
    Files,           /*!< The disc and three interchangeable ROM paths. */
    SearchDirectory, /*!< The directory to search. */
    Search,          /*!< Search progress and discovered images. */
    Import,          /*!< The import and its progress. */
    Done,            /*!< The end, with Finish. */
};

/** How the wizard ended. */
enum class WizardEnd {
    Running,  /*!< The wizard shows. */
    Quit,     /*!< The user quit. */
    Finished, /*!< The data was imported and Finish was pressed. */
};

/** A control of a screen or a dialog of the wizard. */
enum class WizardControlId {
    DiscText,      /*!< The path of the disc image. */
    DiscBrowse,    /*!< Browse for the disc image. */
    DiscClear,     /*!< Clear the disc path. */
    Rom1Text,      /*!< The first ROM path. */
    Rom2Text,      /*!< The second ROM path. */
    Rom3Text,      /*!< The third ROM path. */
    Rom1Browse,    /*!< Browse for the first ROM image. */
    Rom2Browse,    /*!< Browse for the second ROM image. */
    Rom3Browse,    /*!< Browse for the third ROM image. */
    Rom1Clear,     /*!< Clear the first ROM path. */
    Rom2Clear,     /*!< Clear the second ROM path. */
    Rom3Clear,     /*!< Clear the third ROM path. */
    SearchAssets,  /*!< Open the directory search. */
    SearchText,    /*!< The directory path. */
    SearchBrowse,  /*!< Browse for a directory. */
    SearchClear,   /*!< Clear the directory path. */
    StartSearch,   /*!< Start searching. */
    RetrySearch,   /*!< Search another directory. */
    UseFound,      /*!< Return to the setup fields. */
    Back,          /*!< The previous screen. */
    Next,          /*!< The next screen. */
    Cancel,        /*!< Cancel the import. */
    Finish,        /*!< Start the game. */
    BrowserList,   /*!< The entries of the directory of the file browser. */
    BrowserFilter, /*!< The kinds of file the browser lists. */
    BrowserOk,     /*!< Open or choose the entry of the browser. */
    BrowserCancel, /*!< Close the browser. */
    MessageOk,     /*!< Close a message. */
    MessageYes,    /*!< Quit the wizard. */
    MessageNo,     /*!< Do not quit the wizard. */
};

/** A control positioned on the grid. */
struct WizardControl {
    /** The kind of control. */
    enum class Kind {
        Button,   /*!< A button, `< Label >`. */
        CheckBox, /*!< A check box, `[X] Label`. */
        TextBox,  /*!< A line of text, with its label above it. */
        List,     /*!< A list with a scroll bar, with its label above its frame. */
        Choice,   /*!< One of a few values, with its label at its left. */
    };
    WizardControlId id = WizardControlId::Next; /*!< The control. */
    Kind kind = Kind::Button;                   /*!< The kind of control. */
    int column = 0;                             /*!< The left column. */
    int row = 0;                                /*!< The top row. */
    int width = 0;                              /*!< The columns. */
    int height = 1;                             /*!< The rows. */
    /** The label, with & before the letter Alt chooses it by. */
    std::string label;
    std::string value;    /*!< The value of a Choice. */
    bool enabled = true;  /*!< Whether the control accepts focus and presses. */
    bool checked = false; /*!< Whether a check box is checked. */
    bool bad = false;     /*!< Whether a text box stores a path that failed its check. */
};

/**
 * @param label A label with & before the letter of its accelerator.
 * @return The letter in lowercase, or 0 when the label has none.
 */
char AcceleratorOf(const std::string &label);

/** Manage setup screens, their controls and focus, the file browser, and the import. */
class SetupWizard {
public:
    /**
     * @param services The checks and the import of the files.
     */
    explicit SetupWizard(WizardServices services);

    /** Cancel an import that runs and wait for it. */
    ~SetupWizard();
    SetupWizard(const SetupWizard &) = delete;
    SetupWizard &operator=(const SetupWizard &) = delete;

    /**
     * Act on an event of the keyboard or the mouse.
     *
     * @param event The event.
     */
    void Handle(const WizardEvent &event);

    /** Take the progress of the import, and move to the next screen when it ends. */
    void Update();

    /**
     * Draw the wizard.
     *
     * @param grid The grid, every cell of which is written.
     * @param seconds The time, for the blink of the text cursor.
     */
    void Draw(TextGrid &grid, double seconds) const;

    /** @return The screen that shows. */
    WizardScreen Screen() const;

    /** @return How the wizard ended, or WizardEnd::Running. */
    WizardEnd End() const;

    /** @return The controls of the dialog on top: the message, the browser, or the screen. */
    std::vector<WizardControl> Controls() const;

    /** @return The control with the focus in the dialog on top, or nothing. */
    std::optional<WizardControlId> Focus() const;

    /** @return Whether the file browser shows. */
    bool BrowserOpen() const;

    /** @return The entries the file browser lists. */
    const std::vector<BrowserEntry> &BrowserEntries() const;

    /** @return The entry of the browser chosen, from 0. */
    int BrowserSelection() const;

    /** @return The lines of the message that shows, or none. */
    std::vector<std::string> MessageLines() const;

    /**
     * @param id A text box.
     * @return The text of the text box in UTF-8.
     */
    std::string Text(WizardControlId id) const;

    /**
     * @param id A text box.
     * @return Why the path of the text box failed its check, or empty when it passed or is empty.
     */
    std::string Reason(WizardControlId id) const;

    /** @return The selection of the focused text field, in UTF-8. */
    std::string SelectedText() const;

    /** @return The images discovered during the directory search. */
    const WizardSearchResult &SearchResult() const;

    /** @return The directory last browsed, or empty. */
    const std::filesystem::path &LastDirectory() const;

private:
    struct TextField {
        std::u32string text;
        std::size_t caret = 0;
        std::size_t scroll = 0;
        std::optional<std::size_t> anchor;
        bool valid = false;
        std::optional<WizardCartridge> release;
        std::string reason;
        // The text and the size and time of its file at the last check.
        std::u32string checked;
        std::uintmax_t size = 0;
        std::filesystem::file_time_type time{};
        bool checkedOnce = false;
    };
    struct Browser {
        WizardControlId target = WizardControlId::DiscText;
        std::filesystem::path directory;
        std::vector<BrowserEntry> entries;
        int selected = 0;
        int top = 0;
        bool allFiles = false;
        std::string typed;
        std::uint64_t typedAt = 0;
        std::uint64_t clickedAt = 0;
        int clicked = -1;
        bool dragging = false;
    };
    struct Message {
        std::vector<std::string> lines;
        bool question = false;
    };
    struct ImportState;

    std::vector<WizardControl> ScreenControls() const;
    std::vector<WizardControl> BrowserControls() const;
    std::vector<WizardControl> MessageControls() const;
    std::optional<WizardControlId> &FocusSlot();
    void FocusFirst();
    void MoveFocus(bool backward);
    void Activate(WizardControlId id);
    void PressDefault();
    void Escape();
    void SetScreen(WizardScreen screen);
    void OpenBrowser(WizardControlId target);
    void ChangeDirectory(const std::filesystem::path &directory);
    void OpenSelected();
    void ChooseDirectory();
    void ShowMessage(std::vector<std::string> lines, bool question);
    void StartImport();
    void StartSearch();
    void ApplySearchResult();
    void SetText(WizardControlId id, const std::filesystem::path &path);
    void ClearText(WizardControlId id);
    void Validate(WizardControlId id, bool force);
    void EditText(TextField &field, const WizardEvent &event, WizardControlId id);
    void HandleListKey(WizardKey key);
    void HandleListText(const std::string &text, std::uint64_t milliseconds);
    void HandleMouse(const WizardEvent &event);
    void SetSelection(int selected);
    bool NextEnabled() const;
    TextField &Field(WizardControlId id);
    const TextField &Field(WizardControlId id) const;
    std::array<bool, WIZARD_CARTRIDGES> Detected() const;

    WizardServices m_services;
    WizardScreen m_screen = WizardScreen::Files;
    WizardEnd m_end = WizardEnd::Running;
    TextField m_disc;
    std::array<TextField, WIZARD_CARTRIDGES> m_cartridges;
    TextField m_directory;
    WizardSearchResult m_searchResult;
    bool m_searchComplete = false;
    std::optional<Browser> m_browser;
    std::optional<Message> m_message;
    std::optional<WizardControlId> m_screenFocus;
    std::optional<WizardControlId> m_browserFocus;
    std::optional<WizardControlId> m_messageFocus;
    std::filesystem::path m_lastDirectory;
    std::shared_ptr<ImportState> m_import;
    std::thread m_thread;
    std::string m_importStep;
    double m_importFraction = 0.0;
    std::optional<std::string> m_importFailure;
    std::optional<WizardControlId> m_selecting;
};

} // namespace coastline
