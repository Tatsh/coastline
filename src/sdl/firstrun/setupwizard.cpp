#include "setupwizard.h"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cmath>
#include <mutex>
#include <string_view>
#include <system_error>

namespace coastline {

namespace {

// The dialog of every screen and of the browser, the frame included.
constexpr int DIALOG_COLUMN = 4;
constexpr int DIALOG_ROW = 2;
constexpr int DIALOG_WIDTH = 72;
constexpr int DIALOG_HEIGHT = 20;
constexpr int CONTENT_COLUMN = DIALOG_COLUMN + 3;
// The line above the buttons, and the buttons.
constexpr int SEPARATOR_ROW = DIALOG_ROW + DIALOG_HEIGHT - 4;
constexpr int BUTTON_ROW = DIALOG_ROW + DIALOG_HEIGHT - 3;
constexpr int TEXT_WIDTH = 50;
constexpr int BROWSE_COLUMN = CONTENT_COLUMN + TEXT_WIDTH + 2;
// The list of the browser, inside its frame, its last column the scroll bar.
constexpr int LIST_COLUMN = CONTENT_COLUMN;
constexpr int LIST_ROW = DIALOG_ROW + 5;
constexpr int LIST_WIDTH = DIALOG_WIDTH - 8;
constexpr int LIST_HEIGHT = 9;
constexpr int FILTER_ROW = LIST_ROW + LIST_HEIGHT + 1;
// A second click on the same entry within the threshold opens the entry. Typing another letter
// within the threshold extends the list search prefix.
constexpr std::uint64_t DOUBLE_CLICK_MILLISECONDS = 500;
constexpr std::uint64_t TYPING_MILLISECONDS = 1000;
constexpr int WHEEL_ROWS = 3;

constexpr std::array<std::string_view, 2> DISC_EXTENSIONS{".cue", ".iso"};
constexpr std::array<std::string_view, 3> CARTRIDGE_EXTENSIONS{".n64", ".v64", ".z64"};

constexpr std::array<WizardControlId, WIZARD_CARTRIDGES> CHECK_IDS{
    WizardControlId::CheckUsa, WizardControlId::CheckEur, WizardControlId::CheckJpn};
constexpr std::array<WizardControlId, WIZARD_CARTRIDGES> TEXT_IDS{
    WizardControlId::UsaText, WizardControlId::EurText, WizardControlId::JpnText};
constexpr std::array<WizardControlId, WIZARD_CARTRIDGES> BROWSE_IDS{
    WizardControlId::UsaBrowse, WizardControlId::EurBrowse, WizardControlId::JpnBrowse};
constexpr std::array<std::string_view, WIZARD_CARTRIDGES> CHECK_LABELS{"&USA", "&Europe", "&Japan"};
constexpr std::array<std::string_view, WIZARD_CARTRIDGES> TEXT_LABELS{
    "&USA ROM image:", "&Europe ROM image:", "&Japan ROM image:"};
// The Browse button of each row of the cartridge paths, by its place on the screen.
constexpr std::array<std::string_view, WIZARD_CARTRIDGES> BROWSE_LABELS{
    "B&rowse...", "Bro&wse...", "Brow&se..."};
constexpr std::array<std::string_view, WIZARD_CARTRIDGES> RELEASE_NAMES{
    "USA", "European", "Japanese"};

int CartridgeIndex(WizardControlId id) {
    for (int i = 0; i < WIZARD_CARTRIDGES; ++i) {
        if (TEXT_IDS[i] == id || BROWSE_IDS[i] == id || CHECK_IDS[i] == id) {
            return i;
        }
    }
    return -1;
}

int LabelWidth(const std::string &label) {
    int width = static_cast<int>(DecodeUtf8(label).size());
    return label.find('&') == std::string::npos ? width : width - 1;
}

WizardControl MakeButton(WizardControlId id, std::string label, int column, int row) {
    WizardControl control;
    control.id = id;
    control.kind = WizardControl::Kind::Button;
    control.column = column;
    control.row = row;
    control.width = LabelWidth(label) + 4;
    control.label = std::move(label);
    return control;
}

// Buttons centred along the row of the buttons, two columns apart.
void CentreButtons(std::vector<WizardControl> &controls, std::size_t first) {
    int total = 0;
    for (std::size_t i = first; i < controls.size(); ++i) {
        total += controls[i].width + (i > first ? 2 : 0);
    }
    int column = DIALOG_COLUMN + (DIALOG_WIDTH - total) / 2;
    for (std::size_t i = first; i < controls.size(); ++i) {
        controls[i].column = column;
        column += controls[i].width + 2;
    }
}

bool Contains(const WizardControl &control, int column, int row) {
    return column >= control.column && column < control.column + control.width &&
           row >= control.row && row < control.row + control.height;
}

std::string Lowercase(std::string text) {
    std::ranges::transform(
        text, text.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return text;
}

// The words of a text in lines of at most @p width code points.
std::vector<std::u32string> Wrap(const std::string &text, std::size_t width) {
    std::vector<std::u32string> lines;
    std::u32string line;
    std::u32string all = DecodeUtf8(text);
    std::size_t at = 0;
    while (at < all.size()) {
        std::size_t end = all.find(U' ', at);
        std::u32string word = all.substr(at, end == std::u32string::npos ? end : end - at);
        at = end == std::u32string::npos ? all.size() : end + 1;
        while (word.size() > width) {
            if (!line.empty()) {
                lines.push_back(line);
                line.clear();
            }
            lines.push_back(word.substr(0, width));
            word.erase(0, width);
        }
        if (!line.empty() && line.size() + 1 + word.size() > width) {
            lines.push_back(line);
            line.clear();
        }
        line += (line.empty() ? U"" : U" ") + word;
    }
    if (!line.empty()) {
        lines.push_back(line);
    }
    return lines;
}

// Write a label, its accelerator letter in @p accent.
void WriteLabel(TextGrid &grid,
                int column,
                int row,
                const std::string &label,
                CgaColor foreground,
                CgaColor background,
                CgaColor accent) {
    std::u32string text = DecodeUtf8(label);
    int x = column;
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (text[i] == U'&' && i + 1 < text.size()) {
            grid.Write(x++, row, text.substr(i + 1, 1), accent, background);
            ++i;
        } else {
            grid.Write(x++, row, text.substr(i, 1), foreground, background);
        }
    }
}

} // namespace

struct SetupWizard::ImportState {
    std::mutex mutex;
    std::string step;
    double fraction = 0.0;
    bool done = false;
    std::optional<std::string> failure;
    std::atomic<bool> cancel{false};
};

std::optional<WizardCartridge> BaseCartridge(const std::array<bool, WIZARD_CARTRIDGES> &checked) {
    for (WizardCartridge release :
         {WizardCartridge::Usa, WizardCartridge::Eur, WizardCartridge::Jpn}) {
        if (checked[static_cast<std::size_t>(release)]) {
            return release;
        }
    }
    return std::nullopt;
}

char AcceleratorOf(const std::string &label) {
    std::size_t at = label.find('&');
    if (at == std::string::npos || at + 1 >= label.size()) {
        return 0;
    }
    return static_cast<char>(std::tolower(static_cast<unsigned char>(label[at + 1])));
}

SetupWizard::SetupWizard(WizardServices services)
    : m_services(std::move(services)), m_lastDirectory(m_services.lastDirectory) {
    FocusFirst();
}

SetupWizard::~SetupWizard() {
    if (m_thread.joinable()) {
        m_import->cancel = true;
        m_thread.join();
    }
}

WizardScreen SetupWizard::Screen() const {
    return m_screen;
}

WizardEnd SetupWizard::End() const {
    return m_end;
}

bool SetupWizard::BrowserOpen() const {
    return m_browser.has_value();
}

const std::vector<BrowserEntry> &SetupWizard::BrowserEntries() const {
    static const std::vector<BrowserEntry> NONE;
    return m_browser ? m_browser->entries : NONE;
}

int SetupWizard::BrowserSelection() const {
    return m_browser ? m_browser->selected : -1;
}

std::vector<std::string> SetupWizard::MessageLines() const {
    return m_message ? m_message->lines : std::vector<std::string>{};
}

const std::filesystem::path &SetupWizard::LastDirectory() const {
    return m_lastDirectory;
}

SetupWizard::TextField &SetupWizard::Field(WizardControlId id) {
    int index = CartridgeIndex(id);
    return index < 0 ? m_disc : m_cartridges[static_cast<std::size_t>(index)];
}

const SetupWizard::TextField &SetupWizard::Field(WizardControlId id) const {
    int index = CartridgeIndex(id);
    return index < 0 ? m_disc : m_cartridges[static_cast<std::size_t>(index)];
}

std::string SetupWizard::Text(WizardControlId id) const {
    return EncodeUtf8(Field(id).text);
}

std::string SetupWizard::Reason(WizardControlId id) const {
    return Field(id).reason;
}

std::array<bool, WIZARD_CARTRIDGES> SetupWizard::Checked() const {
    return m_checked;
}

bool SetupWizard::NextEnabled() const {
    switch (m_screen) {
    case WizardScreen::Disc:
        return m_disc.valid;
    case WizardScreen::Cartridges:
        return BaseCartridge(m_checked).has_value();
    case WizardScreen::Paths:
        for (int i = 0; i < WIZARD_CARTRIDGES; ++i) {
            if (m_checked[i] && !m_cartridges[i].valid) {
                return false;
            }
        }
        return BaseCartridge(m_checked).has_value();
    case WizardScreen::Import:
    case WizardScreen::Done:
        return false;
    }
    return false;
}

std::vector<WizardControl> SetupWizard::ScreenControls() const {
    std::vector<WizardControl> controls;
    auto textBox = [&](WizardControlId id, std::string label, int row) {
        WizardControl control;
        control.id = id;
        control.kind = WizardControl::Kind::TextBox;
        control.column = CONTENT_COLUMN;
        control.row = row;
        control.width = TEXT_WIDTH;
        control.label = std::move(label);
        const TextField &field = Field(id);
        control.bad = !field.valid && !field.text.empty();
        controls.push_back(control);
    };
    switch (m_screen) {
    case WizardScreen::Disc:
        textBox(WizardControlId::DiscText, "&Disc image:", DIALOG_ROW + 6);
        controls.push_back(
            MakeButton(WizardControlId::DiscBrowse, "B&rowse...", BROWSE_COLUMN, DIALOG_ROW + 6));
        break;
    case WizardScreen::Cartridges:
        for (int i = 0; i < WIZARD_CARTRIDGES; ++i) {
            WizardControl check;
            check.id = CHECK_IDS[i];
            check.kind = WizardControl::Kind::CheckBox;
            check.column = CONTENT_COLUMN + 2;
            check.row = DIALOG_ROW + 6 + 2 * i;
            check.label = std::string(CHECK_LABELS[i]);
            check.width = LabelWidth(check.label) + 4;
            check.checked = m_checked[i];
            controls.push_back(check);
        }
        break;
    case WizardScreen::Paths: {
        int row = DIALOG_ROW + 5;
        int place = 0;
        for (int i = 0; i < WIZARD_CARTRIDGES; ++i) {
            if (!m_checked[i]) {
                continue;
            }
            textBox(TEXT_IDS[i], std::string(TEXT_LABELS[i]), row);
            controls.push_back(
                MakeButton(BROWSE_IDS[i], std::string(BROWSE_LABELS[place]), BROWSE_COLUMN, row));
            row += 3;
            ++place;
        }
        break;
    }
    case WizardScreen::Import:
    case WizardScreen::Done:
        break;
    }
    std::size_t first = controls.size();
    if (m_screen != WizardScreen::Disc) {
        WizardControl back = MakeButton(WizardControlId::Back, "&Back", 0, BUTTON_ROW);
        back.enabled = m_screen != WizardScreen::Import || m_importFailure.has_value();
        controls.push_back(back);
    }
    if (m_screen == WizardScreen::Import) {
        WizardControl cancel = MakeButton(WizardControlId::Cancel, "&Cancel", 0, BUTTON_ROW);
        cancel.enabled = !m_importFailure.has_value();
        controls.push_back(cancel);
    } else if (m_screen == WizardScreen::Done) {
        controls.push_back(MakeButton(WizardControlId::Finish, "&Finish", 0, BUTTON_ROW));
    } else {
        WizardControl next = MakeButton(WizardControlId::Next, "&Next", 0, BUTTON_ROW);
        next.enabled = NextEnabled();
        controls.push_back(next);
    }
    CentreButtons(controls, first);
    return controls;
}

std::vector<WizardControl> SetupWizard::BrowserControls() const {
    std::vector<WizardControl> controls;
    WizardControl list;
    list.id = WizardControlId::BrowserList;
    list.kind = WizardControl::Kind::List;
    list.column = LIST_COLUMN;
    list.row = LIST_ROW;
    list.width = LIST_WIDTH;
    list.height = LIST_HEIGHT;
    list.label = "&Files:";
    controls.push_back(list);
    WizardControl filter;
    filter.id = WizardControlId::BrowserFilter;
    filter.kind = WizardControl::Kind::Choice;
    filter.label = "File &type:";
    filter.column = CONTENT_COLUMN + LabelWidth(filter.label) + 1;
    filter.row = FILTER_ROW;
    filter.width = 44;
    filter.value = m_browser->allFiles ? "All files (*.*)" :
                   m_browser->target == WizardControlId::DiscText ?
                                         "Disc images (*.cue, *.iso)" :
                                         "ROM images (*.z64, *.n64, *.v64)";
    controls.push_back(filter);
    std::size_t first = controls.size();
    controls.push_back(MakeButton(WizardControlId::BrowserOk, "&OK", 0, BUTTON_ROW));
    controls.push_back(MakeButton(WizardControlId::BrowserCancel, "&Cancel", 0, BUTTON_ROW));
    CentreButtons(controls, first);
    return controls;
}

std::vector<WizardControl> SetupWizard::MessageControls() const {
    std::vector<WizardControl> controls;
    int row = 12 + static_cast<int>(m_message->lines.size()) / 2 + 1;
    if (m_message->question) {
        controls.push_back(MakeButton(WizardControlId::MessageYes, "&Yes", 0, row));
        controls.push_back(MakeButton(WizardControlId::MessageNo, "&No", 0, row));
    } else {
        controls.push_back(MakeButton(WizardControlId::MessageOk, "&OK", 0, row));
    }
    CentreButtons(controls, 0);
    return controls;
}

std::vector<WizardControl> SetupWizard::Controls() const {
    if (m_message) {
        return MessageControls();
    }
    if (m_browser) {
        return BrowserControls();
    }
    return ScreenControls();
}

std::optional<WizardControlId> &SetupWizard::FocusSlot() {
    return m_message ? m_messageFocus : m_browser ? m_browserFocus : m_screenFocus;
}

std::optional<WizardControlId> SetupWizard::Focus() const {
    return m_message ? m_messageFocus : m_browser ? m_browserFocus : m_screenFocus;
}

void SetupWizard::FocusFirst() {
    std::optional<WizardControlId> &focus = FocusSlot();
    focus.reset();
    for (const WizardControl &control : Controls()) {
        if (control.enabled) {
            focus = control.id;
            return;
        }
    }
}

void SetupWizard::MoveFocus(bool backward) {
    std::vector<WizardControl> enabled;
    std::ranges::copy_if(Controls(), std::back_inserter(enabled), &WizardControl::enabled);
    if (enabled.empty()) {
        return;
    }
    std::optional<WizardControlId> &focus = FocusSlot();
    auto at = std::ranges::find(
        enabled, focus, [](const WizardControl &control) { return std::optional(control.id); });
    auto count = static_cast<std::ptrdiff_t>(enabled.size());
    std::ptrdiff_t index = at == enabled.end() ? (backward ? 0 : -1) : at - enabled.begin();
    index = ((index + (backward ? -1 : 1)) % count + count) % count;
    focus = enabled[static_cast<std::size_t>(index)].id;
}

void SetupWizard::SetScreen(WizardScreen screen) {
    m_screen = screen;
    if (screen == WizardScreen::Paths) {
        for (WizardControlId id : TEXT_IDS) {
            Validate(id, true);
        }
    }
    FocusFirst();
    if (screen == WizardScreen::Done) {
        m_screenFocus = WizardControlId::Finish;
    }
    if (screen == WizardScreen::Import) {
        StartImport();
        FocusFirst();
    }
}

void SetupWizard::ShowMessage(std::vector<std::string> lines, bool question) {
    std::vector<std::string> wrapped;
    for (const auto &line : lines) {
        for (const auto &part : Wrap(line, DIALOG_WIDTH - 6)) {
            wrapped.push_back(EncodeUtf8(part));
        }
    }
    if (wrapped.size() > 15) {
        wrapped.resize(15);
        wrapped.back() = "...";
    }
    m_message = Message{std::move(wrapped), question};
    FocusFirst();
}

void SetupWizard::Validate(WizardControlId id, bool force) {
    TextField &field = Field(id);
    const std::filesystem::path path = PathFromUtf8(EncodeUtf8(field.text));
    std::error_code error;
    const bool exists = !field.text.empty() && std::filesystem::is_regular_file(path, error);
    std::uintmax_t size = exists ? std::filesystem::file_size(path, error) : 0;
    auto time =
        exists ? std::filesystem::last_write_time(path, error) : std::filesystem::file_time_type{};
    if (!force && field.checkedOnce && field.checked == field.text && field.size == size &&
        field.time == time) {
        return;
    }
    field.checkedOnce = true;
    field.checked = field.text;
    field.size = size;
    field.time = time;
    field.valid = false;
    field.reason.clear();
    if (field.text.empty()) {
        return;
    }
    if (!exists) {
        field.reason = "File not found";
        return;
    }
    int index = CartridgeIndex(id);
    if (index < 0) {
        if (!std::ranges::contains(DISC_EXTENSIONS, LowercaseExtension(path))) {
            field.reason = "Not a disc image (.cue or .iso)";
            return;
        }
        if (auto failure = m_services.checkDisc(path)) {
            field.reason = *failure;
            return;
        }
        field.valid = true;
        return;
    }
    auto release = m_services.identifyCartridge(path);
    if (!release) {
        field.reason = "Not a Top Gear Rally ROM";
    } else if (static_cast<int>(*release) != index) {
        field.reason = "Not the " + std::string(RELEASE_NAMES[index]) + " release";
    } else {
        field.valid = true;
    }
}

void SetupWizard::OpenBrowser(WizardControlId target) {
    Browser browser;
    browser.target = target;
    m_browser = std::move(browser);
    std::error_code error;
    std::filesystem::path typed = PathFromUtf8(EncodeUtf8(Field(target).text));
    if (!typed.empty()) {
        typed = std::filesystem::absolute(typed, error);
    }
    std::filesystem::path start;
    for (const std::filesystem::path &candidate :
         {typed, typed.parent_path(), m_lastDirectory, m_services.home}) {
        if (!candidate.empty() && std::filesystem::is_directory(candidate, error)) {
            start = candidate;
            break;
        }
    }
    m_browser->directory = start;
    ChangeDirectory(start);
    m_browserFocus = WizardControlId::BrowserList;
}

void SetupWizard::ChangeDirectory(const std::filesystem::path &directory) {
    std::span<const std::string_view> extensions;
    if (!m_browser->allFiles) {
        extensions = m_browser->target == WizardControlId::DiscText ?
                         std::span<const std::string_view>(DISC_EXTENSIONS) :
                         std::span<const std::string_view>(CARTRIDGE_EXTENSIONS);
    }
    auto entries = ListBrowserDirectory(directory, extensions);
    if (!entries) {
        ShowMessage({Utf8FromPath(directory) + " cannot be read."}, false);
        return;
    }
    std::filesystem::path normal = directory.lexically_normal();
    if (!normal.has_filename() && normal.has_relative_path()) {
        normal = normal.parent_path();
    }
    m_browser->directory = normal;
    m_lastDirectory = normal;
    m_browser->entries = std::move(*entries);
    m_browser->selected = 0;
    m_browser->top = 0;
    m_browser->clicked = -1;
}

void SetupWizard::OpenSelected() {
    if (m_browser->entries.empty()) {
        return;
    }
    const BrowserEntry entry = m_browser->entries[static_cast<std::size_t>(m_browser->selected)];
    if (entry.kind != BrowserEntry::Kind::File) {
        ChangeDirectory(entry.path);
        return;
    }
    TextField &field = Field(m_browser->target);
    field.text = DecodeUtf8(Utf8FromPath(entry.path));
    field.caret = field.text.size();
    field.scroll = field.caret > TEXT_WIDTH - 1 ? field.caret - (TEXT_WIDTH - 1) : 0;
    m_lastDirectory = m_browser->directory;
    m_screenFocus = m_browser->target;
    Validate(m_browser->target, true);
    m_browser.reset();
}

void SetupWizard::SetSelection(int selected) {
    if (m_browser->entries.empty()) {
        return;
    }
    int count = static_cast<int>(m_browser->entries.size());
    m_browser->selected = std::clamp(selected, 0, count - 1);
    if (m_browser->selected < m_browser->top) {
        m_browser->top = m_browser->selected;
    } else if (m_browser->selected >= m_browser->top + LIST_HEIGHT) {
        m_browser->top = m_browser->selected - LIST_HEIGHT + 1;
    }
}

void SetupWizard::StartImport() {
    WizardImport files;
    files.disc = PathFromUtf8(EncodeUtf8(m_disc.text));
    auto base = BaseCartridge(m_checked);
    for (int pass = 0; pass < 2; ++pass) {
        for (int i = 0; i < WIZARD_CARTRIDGES; ++i) {
            bool isBase = base && static_cast<int>(*base) == i;
            if (m_checked[i] && isBase == (pass == 0)) {
                files.cartridges.emplace_back(static_cast<WizardCartridge>(i),
                                              PathFromUtf8(EncodeUtf8(m_cartridges[i].text)));
            }
        }
    }
    m_importFailure.reset();
    m_importStep.clear();
    m_importFraction = 0.0;
    m_import = std::make_shared<ImportState>();
    std::shared_ptr<ImportState> state = m_import;
    auto import = m_services.import;
    m_thread = std::thread([state, import, files]() {
        std::optional<std::string> failure;
        try {
            failure = import(files, [state](const std::string &step, double fraction) {
                std::lock_guard lock(state->mutex);
                state->step = step;
                state->fraction = fraction;
                return !state->cancel.load();
            });
        } catch (const std::exception &error) {
            failure = error.what();
        } catch (...) {
            failure = "The import failed.";
        }
        std::lock_guard lock(state->mutex);
        state->failure = std::move(failure);
        state->done = true;
    });
}

void SetupWizard::Update() {
    if (!m_import) {
        return;
    }
    bool done = false;
    {
        std::lock_guard lock(m_import->mutex);
        m_importStep = m_import->step;
        m_importFraction = m_import->fraction;
        done = m_import->done;
        if (done) {
            m_importFailure = m_import->failure;
        }
    }
    if (!done) {
        return;
    }
    m_thread.join();
    bool cancelled = m_import->cancel;
    m_import.reset();
    if (cancelled) {
        m_importFailure.reset();
        SetScreen(WizardScreen::Disc);
    } else if (m_importFailure) {
        FocusFirst();
    } else {
        SetScreen(WizardScreen::Done);
    }
}

void SetupWizard::Activate(WizardControlId id) {
    switch (id) {
    case WizardControlId::DiscBrowse:
        OpenBrowser(WizardControlId::DiscText);
        break;
    case WizardControlId::UsaBrowse:
    case WizardControlId::EurBrowse:
    case WizardControlId::JpnBrowse:
        OpenBrowser(TEXT_IDS[CartridgeIndex(id)]);
        break;
    case WizardControlId::CheckUsa:
    case WizardControlId::CheckEur:
    case WizardControlId::CheckJpn: {
        bool &checked = m_checked[static_cast<std::size_t>(CartridgeIndex(id))];
        checked = !checked;
        break;
    }
    case WizardControlId::Next:
        if (NextEnabled()) {
            SetScreen(static_cast<WizardScreen>(static_cast<int>(m_screen) + 1));
        }
        break;
    case WizardControlId::Back:
        if (m_screen == WizardScreen::Cartridges) {
            SetScreen(WizardScreen::Disc);
        } else if (m_screen == WizardScreen::Paths) {
            SetScreen(WizardScreen::Cartridges);
        } else if (m_screen == WizardScreen::Done ||
                   (m_screen == WizardScreen::Import && m_importFailure)) {
            m_importFailure.reset();
            SetScreen(WizardScreen::Paths);
        }
        break;
    case WizardControlId::Cancel:
        if (m_import) {
            m_import->cancel = true;
        }
        break;
    case WizardControlId::Finish:
        m_end = WizardEnd::Finished;
        break;
    case WizardControlId::BrowserOk:
        OpenSelected();
        break;
    case WizardControlId::BrowserCancel:
        m_browser.reset();
        break;
    case WizardControlId::BrowserFilter:
        m_browser->allFiles = !m_browser->allFiles;
        ChangeDirectory(m_browser->directory);
        break;
    case WizardControlId::MessageOk:
    case WizardControlId::MessageNo:
        m_message.reset();
        break;
    case WizardControlId::MessageYes:
        m_message.reset();
        m_end = WizardEnd::Quit;
        break;
    case WizardControlId::DiscText:
    case WizardControlId::UsaText:
    case WizardControlId::EurText:
    case WizardControlId::JpnText:
    case WizardControlId::BrowserList:
        break;
    }
}

void SetupWizard::PressDefault() {
    if (m_message) {
        Activate(m_message->question ? WizardControlId::MessageYes : WizardControlId::MessageOk);
    } else if (m_browser) {
        OpenSelected();
    } else if (m_screen == WizardScreen::Done) {
        Activate(WizardControlId::Finish);
    } else {
        Activate(WizardControlId::Next);
    }
}

void SetupWizard::Escape() {
    if (m_message) {
        m_message.reset();
    } else if (m_browser) {
        m_browser.reset();
    } else if (m_screen == WizardScreen::Disc) {
        ShowMessage({"Quit?", "The game cannot start without its data."}, true);
    } else if (m_screen == WizardScreen::Import && !m_importFailure) {
        Activate(WizardControlId::Cancel);
    } else {
        Activate(WizardControlId::Back);
    }
}

void SetupWizard::EditText(TextField &field, const WizardEvent &event, WizardControlId id) {
    if (event.type == WizardEvent::Type::Text) {
        std::u32string typed;
        for (char32_t c : DecodeUtf8(event.text)) {
            if (c >= 0x20 && c != 0x7F) {
                typed += c;
            }
        }
        field.text.insert(field.caret, typed);
        field.caret += typed.size();
    } else {
        switch (event.key) {
        case WizardKey::Left:
            field.caret -= field.caret > 0 ? 1 : 0;
            break;
        case WizardKey::Right:
            field.caret += field.caret < field.text.size() ? 1 : 0;
            break;
        case WizardKey::Home:
            field.caret = 0;
            break;
        case WizardKey::End:
            field.caret = field.text.size();
            break;
        case WizardKey::Backspace:
            if (field.caret > 0) {
                field.text.erase(--field.caret, 1);
            }
            break;
        case WizardKey::Delete:
            if (field.caret < field.text.size()) {
                field.text.erase(field.caret, 1);
            }
            break;
        default:
            return;
        }
    }
    if (field.caret < field.scroll) {
        field.scroll = field.caret;
    } else if (field.caret >= field.scroll + TEXT_WIDTH) {
        field.scroll = field.caret - TEXT_WIDTH + 1;
    }
    Validate(id, false);
}

void SetupWizard::HandleListKey(WizardKey key) {
    switch (key) {
    case WizardKey::Up:
        SetSelection(m_browser->selected - 1);
        break;
    case WizardKey::Down:
        SetSelection(m_browser->selected + 1);
        break;
    case WizardKey::PageUp:
        SetSelection(m_browser->selected - (LIST_HEIGHT - 1));
        break;
    case WizardKey::PageDown:
        SetSelection(m_browser->selected + (LIST_HEIGHT - 1));
        break;
    case WizardKey::Home:
        SetSelection(0);
        break;
    case WizardKey::End:
        SetSelection(static_cast<int>(m_browser->entries.size()) - 1);
        break;
    default:
        break;
    }
}

void SetupWizard::HandleListText(const std::string &text, std::uint64_t milliseconds) {
    std::string letters = Lowercase(text);
    std::erase_if(letters, [](char c) { return std::isspace(static_cast<unsigned char>(c)); });
    if (letters.empty()) {
        return;
    }
    if (milliseconds - m_browser->typedAt > TYPING_MILLISECONDS) {
        m_browser->typed.clear();
    }
    m_browser->typedAt = milliseconds;
    m_browser->typed += letters;
    for (const std::string &prefix : {m_browser->typed, letters}) {
        for (std::size_t i = 0; i < m_browser->entries.size(); ++i) {
            const BrowserEntry &entry = m_browser->entries[i];
            if (entry.kind != BrowserEntry::Kind::Parent &&
                Lowercase(entry.name).starts_with(prefix)) {
                SetSelection(static_cast<int>(i));
                return;
            }
        }
    }
}

void SetupWizard::HandleMouse(const WizardEvent &event) {
    const int column = event.x / CELL_WIDTH;
    const int row = event.y / CELL_HEIGHT;
    // The scroll bar of the list: its arrows, its track, and its thumb.
    const int bar = LIST_COLUMN + LIST_WIDTH - 1;
    const int count = m_browser ? static_cast<int>(m_browser->entries.size()) : 0;
    const int range = std::max(count - LIST_HEIGHT, 0);
    const int track = LIST_HEIGHT - 2;
    auto thumbRow = [&]() {
        return LIST_ROW + 1 + (range == 0 ? 0 : m_browser->top * (track - 1) / range);
    };
    auto scrollTo = [&](int top) { m_browser->top = std::clamp(top, 0, range); };
    if (event.type == WizardEvent::Type::MouseUp) {
        if (m_browser) {
            m_browser->dragging = false;
        }
        return;
    }
    if (event.type == WizardEvent::Type::MouseMove) {
        if (m_browser && m_browser->dragging && track > 1) {
            scrollTo(((row - LIST_ROW - 1) * range + (track - 1) / 2) / (track - 1));
        }
        return;
    }
    if (event.type == WizardEvent::Type::Wheel) {
        if (m_browser && !m_message) {
            scrollTo(m_browser->top - event.wheel * WHEEL_ROWS);
        }
        return;
    }
    for (const WizardControl &control : Controls()) {
        if (!control.enabled || !Contains(control, column, row)) {
            continue;
        }
        FocusSlot() = control.id;
        switch (control.kind) {
        case WizardControl::Kind::Button:
        case WizardControl::Kind::CheckBox:
        case WizardControl::Kind::Choice:
            Activate(control.id);
            break;
        case WizardControl::Kind::TextBox: {
            TextField &field = Field(control.id);
            field.caret = std::min(field.scroll + static_cast<std::size_t>(column - control.column),
                                   field.text.size());
            break;
        }
        case WizardControl::Kind::List:
            if (column == bar) {
                if (row == LIST_ROW) {
                    scrollTo(m_browser->top - 1);
                } else if (row == LIST_ROW + LIST_HEIGHT - 1) {
                    scrollTo(m_browser->top + 1);
                } else if (row == thumbRow()) {
                    m_browser->dragging = true;
                } else {
                    scrollTo(m_browser->top +
                             (row < thumbRow() ? -(LIST_HEIGHT - 1) : LIST_HEIGHT - 1));
                }
            } else {
                int index = m_browser->top + row - LIST_ROW;
                if (index < count) {
                    bool twice =
                        index == m_browser->clicked &&
                        event.milliseconds - m_browser->clickedAt <= DOUBLE_CLICK_MILLISECONDS;
                    m_browser->selected = index;
                    m_browser->clicked = index;
                    m_browser->clickedAt = event.milliseconds;
                    if (twice) {
                        OpenSelected();
                    }
                }
            }
            break;
        }
        return;
    }
}

void SetupWizard::Handle(const WizardEvent &event) {
    if (m_end != WizardEnd::Running) {
        return;
    }
    if (event.type != WizardEvent::Type::Key && event.type != WizardEvent::Type::Text &&
        event.type != WizardEvent::Type::Accelerator) {
        HandleMouse(event);
        return;
    }
    std::vector<WizardControl> controls = Controls();
    std::optional<WizardControlId> focus = Focus();
    auto focused = std::ranges::find_if(controls, [&](const WizardControl &control) {
        return focus == control.id && control.enabled;
    });
    const WizardControl *control = focused == controls.end() ? nullptr : &*focused;
    const auto kind = control ? control->kind : WizardControl::Kind::Button;
    if (event.type == WizardEvent::Type::Accelerator) {
        for (const WizardControl &candidate : controls) {
            if (candidate.enabled && AcceleratorOf(candidate.label) == event.letter) {
                FocusSlot() = candidate.id;
                Activate(candidate.id);
                return;
            }
        }
        return;
    }
    if (event.type == WizardEvent::Type::Text) {
        if (control && kind == WizardControl::Kind::TextBox) {
            EditText(Field(control->id), event, control->id);
        } else if (control && kind == WizardControl::Kind::List) {
            HandleListText(event.text, event.milliseconds);
        }
        return;
    }
    switch (event.key) {
    case WizardKey::Tab:
        MoveFocus(event.shift);
        return;
    case WizardKey::Escape:
        Escape();
        return;
    case WizardKey::Enter:
        if (control && kind == WizardControl::Kind::Button) {
            Activate(control->id);
        } else {
            PressDefault();
        }
        return;
    case WizardKey::Space:
        if (control && kind != WizardControl::Kind::TextBox && kind != WizardControl::Kind::List) {
            Activate(control->id);
        }
        return;
    default:
        break;
    }
    if (control && kind == WizardControl::Kind::TextBox) {
        EditText(Field(control->id), event, control->id);
    } else if (control && kind == WizardControl::Kind::List) {
        HandleListKey(event.key);
    } else if (control && kind == WizardControl::Kind::Choice &&
               (event.key == WizardKey::Up || event.key == WizardKey::Down ||
                event.key == WizardKey::Left || event.key == WizardKey::Right)) {
        Activate(control->id);
    }
}

void SetupWizard::Draw(TextGrid &grid, double seconds) const {
    using enum CgaColor;
    grid.cursor.reset();
    const bool blink = std::fmod(seconds, 1.0) < 0.5;
    grid.Fill(0, 0, GRID_COLUMNS, GRID_ROWS, ' ', LightGray, Blue);
    grid.Fill(0, 0, GRID_COLUMNS, 1, ' ', Black, LightGray);
    grid.WriteUtf8(1, 0, "Coastline Setup", Black, LightGray);

    auto dialog = [&](const std::string &title) {
        grid.Box(DIALOG_COLUMN, DIALOG_ROW, DIALOG_WIDTH, DIALOG_HEIGHT, Black, LightGray, false);
        grid.Shadow(DIALOG_COLUMN, DIALOG_ROW, DIALOG_WIDTH, DIALOG_HEIGHT);
        std::string text = " " + title + " ";
        grid.WriteUtf8(DIALOG_COLUMN + (DIALOG_WIDTH - LabelWidth(text)) / 2,
                       DIALOG_ROW,
                       text,
                       Black,
                       LightGray);
        grid.Fill(DIALOG_COLUMN + 1, SEPARATOR_ROW, DIALOG_WIDTH - 2, 1, 0xC4, Black, LightGray);
        grid.At(DIALOG_COLUMN, SEPARATOR_ROW).glyph = 0xC3;
        grid.At(DIALOG_COLUMN + DIALOG_WIDTH - 1, SEPARATOR_ROW).glyph = 0xB4;
    };
    auto drawControl = [&](const WizardControl &control, bool focused) {
        switch (control.kind) {
        case WizardControl::Kind::Button: {
            CgaColor foreground = !control.enabled ? DarkGray : focused ? LightGray : Black;
            CgaColor background = focused ? Black : LightGray;
            CgaColor accent = control.enabled ? White : DarkGray;
            std::string label = control.label;
            if (!control.enabled) {
                std::erase(label, '&');
            }
            grid.WriteUtf8(control.column, control.row, "< ", foreground, background);
            WriteLabel(
                grid, control.column + 2, control.row, label, foreground, background, accent);
            grid.WriteUtf8(
                control.column + control.width - 2, control.row, " >", foreground, background);
            break;
        }
        case WizardControl::Kind::CheckBox: {
            CgaColor foreground = focused ? LightGray : Black;
            CgaColor background = focused ? Black : LightGray;
            grid.WriteUtf8(control.column,
                           control.row,
                           control.checked ? "[X] " : "[ ] ",
                           foreground,
                           background);
            WriteLabel(grid,
                       control.column + 4,
                       control.row,
                       control.label,
                       foreground,
                       background,
                       White);
            break;
        }
        case WizardControl::Kind::TextBox: {
            const TextField &field = Field(control.id);
            CgaColor labelColor = control.bad ? Red : Black;
            WriteLabel(
                grid, control.column, control.row - 1, control.label, labelColor, LightGray, White);
            CgaColor foreground = control.bad ? (focused ? Yellow : White) :
                                  focused     ? White :
                                                Black;
            CgaColor background = control.bad ? Red : focused ? Black : Cyan;
            grid.Fill(control.column, control.row, control.width, 1, ' ', foreground, background);
            std::u32string_view text = field.text;
            if (field.scroll < text.size()) {
                grid.Write(control.column,
                           control.row,
                           text.substr(field.scroll),
                           foreground,
                           background,
                           control.width);
            }
            if (focused && blink && Focus() == control.id) {
                grid.cursor = std::array<int, 2>{
                    control.column + static_cast<int>(field.caret - field.scroll), control.row};
            }
            if (control.bad) {
                grid.WriteUtf8(
                    control.column, control.row + 1, field.reason, Red, LightGray, TEXT_WIDTH + 14);
            }
            break;
        }
        case WizardControl::Kind::List: {
            WriteLabel(
                grid, control.column - 1, control.row - 2, control.label, Black, LightGray, White);
            grid.Box(control.column - 1,
                     control.row - 1,
                     control.width + 2,
                     control.height + 2,
                     Black,
                     LightGray,
                     false);
            const auto &entries = m_browser->entries;
            for (int line = 0; line < control.height; ++line) {
                int index = m_browser->top + line;
                if (index >= static_cast<int>(entries.size())) {
                    break;
                }
                bool selected = index == m_browser->selected;
                CgaColor foreground = selected && focused ? LightGray : Black;
                CgaColor background = !selected ? LightGray : focused ? Black : Cyan;
                grid.Fill(control.column,
                          control.row + line,
                          control.width - 1,
                          1,
                          ' ',
                          foreground,
                          background);
                grid.WriteUtf8(control.column + 1,
                               control.row + line,
                               entries[static_cast<std::size_t>(index)].label,
                               foreground,
                               background,
                               control.width - 3);
            }
            const int bar = control.column + control.width - 1;
            const int range = std::max(static_cast<int>(entries.size()) - control.height, 0);
            const int track = control.height - 2;
            grid.Fill(bar, control.row + 1, 1, track, 0xB0, Black, LightGray);
            grid.At(bar, control.row) = {0x18, Black, LightGray};
            grid.At(bar, control.row + control.height - 1) = {0x19, Black, LightGray};
            int thumb = control.row + 1 + (range == 0 ? 0 : m_browser->top * (track - 1) / range);
            grid.At(bar, thumb) = {0xDB, Black, LightGray};
            break;
        }
        case WizardControl::Kind::Choice: {
            WriteLabel(grid, CONTENT_COLUMN, control.row, control.label, Black, LightGray, White);
            CgaColor foreground = focused ? LightGray : Black;
            CgaColor background = focused ? Black : Cyan;
            grid.Fill(control.column, control.row, control.width, 1, ' ', foreground, background);
            grid.WriteUtf8(control.column + 1,
                           control.row,
                           control.value,
                           foreground,
                           background,
                           control.width - 3);
            grid.At(control.column + control.width - 2, control.row).glyph = 0x19;
            break;
        }
        }
    };
    auto drawControls = [&](const std::vector<WizardControl> &controls,
                            std::optional<WizardControlId> focus) {
        for (const WizardControl &control : controls) {
            drawControl(control, focus == control.id && control.enabled);
        }
    };
    auto writeLines = [&](int row, const std::string &text, CgaColor color) {
        for (const std::u32string &line : Wrap(text, DIALOG_WIDTH - 6)) {
            grid.Write(CONTENT_COLUMN, row++, line, color, LightGray);
        }
        return row;
    };

    const bool screenOnTop = !m_browser && !m_message;
    std::string status = " <Tab=Next Field>  <Alt+Letter=Choose>  <Enter=Next>  <Esc=Back>";
    switch (m_screen) {
    case WizardScreen::Disc:
        dialog("Step 1 of 5: Disc");
        writeLines(DIALOG_ROW + 2,
                   "Please locate your copy of the Boss Rally disc image (.cue or .iso).",
                   Black);
        if (m_disc.valid && LowercaseExtension(PathFromUtf8(EncodeUtf8(m_disc.text))) == ".iso") {
            writeLines(DIALOG_ROW + 10,
                       "ISO images do not include PC music. Choose a CUE sheet for disc audio.",
                       Black);
        }
        status = " <Tab=Next Field>  <Alt+Letter=Choose>  <Enter=Next>  <Esc=Quit>";
        break;
    case WizardScreen::Cartridges:
        dialog("Step 2 of 5: ROM releases");
        writeLines(DIALOG_ROW + 2, "Which Top Gear Rally N64 ROM images do you have?", Black);
        writeLines(DIALOG_ROW + 3, "Check every release you have with Space.", Black);
        break;
    case WizardScreen::Paths: {
        dialog("Step 3 of 5: ROM images");
        writeLines(DIALOG_ROW + 2, "Please locate each ROM image.", Black);
        if (auto base = BaseCartridge(m_checked)) {
            writeLines(SEPARATOR_ROW - 1,
                       "The default pack is built on the " +
                           std::string(RELEASE_NAMES[static_cast<int>(*base)]) + " release.",
                       DarkGray);
        }
        break;
    }
    case WizardScreen::Import: {
        dialog("Step 4 of 5: Import");
        if (m_importFailure) {
            int row = writeLines(DIALOG_ROW + 2, "The import failed.", Red);
            writeLines(row + 1, *m_importFailure, Red);
            status = " <Enter=Back>  <Esc=Back>";
        } else {
            writeLines(DIALOG_ROW + 2, "Importing the game data. This takes a few minutes.", Black);
            grid.WriteUtf8(
                CONTENT_COLUMN, DIALOG_ROW + 5, m_importStep, Black, LightGray, DIALOG_WIDTH - 6);
            const int width = DIALOG_WIDTH - 6;
            const int filled = static_cast<int>(std::clamp(m_importFraction, 0.0, 1.0) * width);
            grid.Fill(CONTENT_COLUMN, DIALOG_ROW + 7, width, 1, 0xB0, DarkGray, LightGray);
            grid.Fill(CONTENT_COLUMN, DIALOG_ROW + 7, filled, 1, 0xDB, Blue, LightGray);
            grid.WriteUtf8(CONTENT_COLUMN,
                           DIALOG_ROW + 8,
                           std::to_string(static_cast<int>(m_importFraction * 100.0)) + "%",
                           Black,
                           LightGray);
            status = " <Esc=Cancel>";
        }
        break;
    }
    case WizardScreen::Done:
        dialog("Step 5 of 5: Done");
        grid.WriteUtf8(DIALOG_COLUMN + (DIALOG_WIDTH - 21) / 2,
                       DIALOG_ROW + 7,
                       "Enjoy Top Gear Rally!",
                       Black,
                       LightGray);
        writeLines(DIALOG_ROW + 10, "Press Finish to start the game.", Black);
        status = " <Enter=Finish>  <Esc=Back>";
        break;
    }
    drawControls(ScreenControls(), screenOnTop ? m_screenFocus : std::nullopt);
    if (m_browser) {
        grid.Box(DIALOG_COLUMN, DIALOG_ROW, DIALOG_WIDTH, DIALOG_HEIGHT, Black, LightGray, true);
        grid.Shadow(DIALOG_COLUMN, DIALOG_ROW, DIALOG_WIDTH, DIALOG_HEIGHT);
        grid.WriteUtf8(
            DIALOG_COLUMN + (DIALOG_WIDTH - 8) / 2, DIALOG_ROW, " Browse ", Black, LightGray);
        std::u32string directory = DecodeUtf8(Utf8FromPath(m_browser->directory));
        const std::size_t room = DIALOG_WIDTH - 17;
        if (directory.size() > room) {
            directory = U"..." + directory.substr(directory.size() - room + 3);
        }
        grid.WriteUtf8(CONTENT_COLUMN - 1, DIALOG_ROW + 2, "Directory:", Black, LightGray);
        grid.Write(CONTENT_COLUMN + 10, DIALOG_ROW + 2, directory, Black, LightGray);
        grid.Fill(DIALOG_COLUMN + 1, SEPARATOR_ROW, DIALOG_WIDTH - 2, 1, 0xC4, Black, LightGray);
        grid.At(DIALOG_COLUMN, SEPARATOR_ROW).glyph = 0xC7;
        grid.At(DIALOG_COLUMN + DIALOG_WIDTH - 1, SEPARATOR_ROW).glyph = 0xB6;
        drawControls(BrowserControls(), m_message ? std::nullopt : m_browserFocus);
        status = " <Enter=Open>  <Tab=Next Field>  <Alt+Letter=Choose>  <Esc=Cancel>";
    }
    if (m_message) {
        int width = 20;
        for (const std::string &line : m_message->lines) {
            width = std::max(width, LabelWidth(line) + 6);
        }
        const int height = static_cast<int>(m_message->lines.size()) + 4;
        std::vector<WizardControl> controls = MessageControls();
        const int row = controls.front().row - height + 2;
        const int column = (GRID_COLUMNS - width) / 2;
        grid.Box(column, row, width, height, Black, LightGray, true);
        grid.Shadow(column, row, width, height);
        for (std::size_t i = 0; i < m_message->lines.size(); ++i) {
            const std::string &line = m_message->lines[i];
            grid.WriteUtf8(column + (width - LabelWidth(line)) / 2,
                           row + 1 + static_cast<int>(i),
                           line,
                           Black,
                           LightGray);
        }
        drawControls(controls, m_messageFocus);
        status =
            m_message->question ? " <Alt+Y=Yes>  <Alt+N=No>  <Esc=No>" : " <Enter=OK>  <Esc=OK>";
    }
    grid.Fill(0, GRID_ROWS - 1, GRID_COLUMNS, 1, ' ', Black, Cyan);
    grid.WriteUtf8(0, GRID_ROWS - 1, status, Black, Cyan);
}

} // namespace coastline
