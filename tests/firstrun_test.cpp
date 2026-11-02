#include <algorithm>
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>

#include "setupwizard.h"

namespace {
using namespace coastline;
using enum WizardControlId;

void Check(bool condition, const char *description) {
    if (!condition) {
        throw std::runtime_error(description);
    }
}

void Key(SetupWizard &wizard, WizardKey key, bool shift = false) {
    WizardEvent event;
    event.key = key;
    event.shift = shift;
    wizard.Handle(event);
}

void Type(SetupWizard &wizard, const std::string &text) {
    WizardEvent event;
    event.type = WizardEvent::Type::Text;
    event.text = text;
    wizard.Handle(event);
}

void Click(SetupWizard &wizard, WizardControlId id) {
    auto controls = wizard.Controls();
    auto control = std::ranges::find(controls, id, &WizardControl::id);
    Check(control != controls.end(), "The control must be visible.");
    WizardEvent event;
    event.type = WizardEvent::Type::MouseDown;
    event.x = control->column * CELL_WIDTH;
    event.y = control->row * CELL_HEIGHT;
    wizard.Handle(event);
    event.type = WizardEvent::Type::MouseUp;
    wizard.Handle(event);
}

bool Enabled(const SetupWizard &wizard, WizardControlId id) {
    auto controls = wizard.Controls();
    auto control = std::ranges::find(controls, id, &WizardControl::id);
    return control != controls.end() && control->enabled;
}

void Set(SetupWizard &wizard, WizardControlId id, const std::string &text) {
    Click(wizard, id);
    Key(wizard, WizardKey::SelectAll);
    Type(wizard, text);
}

void Wait(SetupWizard &wizard) {
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while ((wizard.Screen() == WizardScreen::Import && !Enabled(wizard, Back)) ||
           (wizard.Screen() == WizardScreen::Search && !Enabled(wizard, UseFound) &&
            wizard.MessageLines().empty())) {
        Check(std::chrono::steady_clock::now() < deadline, "The worker must finish.");
        wizard.Update();
        std::this_thread::yield();
    }
}

WizardServices Services(const std::filesystem::path &root) {
    WizardServices services;
    services.home = root;
    services.buildRevision = "v1.2.3";
    services.buildDate = "2026-11-01T14:32:05Z";
    services.checkDisc = [](const std::filesystem::path &path) {
        return path.stem() == "disc" ? std::optional<std::string>{} :
                                       std::optional<std::string>{"Not Boss Rally"};
    };
    services.identifyCartridge =
        [](const std::filesystem::path &path) -> std::optional<WizardCartridge> {
        if (path.stem() == "usa") {
            return WizardCartridge::Usa;
        }
        if (path.stem() == "eur") {
            return WizardCartridge::Eur;
        }
        if (path.stem() == "jpn") {
            return WizardCartridge::Jpn;
        }
        return std::nullopt;
    };
    services.import = [](const auto &, const auto &) { return std::optional<std::string>{}; };
    return services;
}

void TestCombinations(const std::filesystem::path &root) {
    constexpr std::array fields{Rom1Text, Rom2Text, Rom3Text};
    constexpr std::array names{"usa.z64", "eur.v64", "jpn.n64"};
    for (unsigned bits = 1; bits < 8; ++bits) {
        for (unsigned rotation = 0; rotation < 3; ++rotation) {
            std::optional<WizardImport> imported;
            auto services = Services(root);
            services.import = [&](const WizardImport &files, const WizardProgress &progress) {
                imported = files;
                progress("Importing", 0.5);
                return std::optional<std::string>{};
            };
            SetupWizard wizard(std::move(services));
            Type(wizard, Utf8FromPath(root / "disc.iso"));
            Check(!Enabled(wizard, Next), "A disc without ROM images must disable Next.");
            std::array<bool, 3> selected{};
            for (unsigned i = 0; i < 3; ++i) {
                selected[i] = (bits & (1u << i)) != 0;
                if (selected[i]) {
                    Set(wizard, fields[(i + rotation) % 3], Utf8FromPath(root / names[i]));
                }
            }
            Check(Enabled(wizard, Next), "Any slot must accept every supported release.");
            Click(wizard, Next);
            Wait(wizard);
            Check(wizard.Screen() == WizardScreen::Done, "A successful import must show Done.");
            Check(!Enabled(wizard, Back), "A successful import must disable Back.");
            Key(wizard, WizardKey::Escape);
            Check(wizard.Screen() == WizardScreen::Done,
                  "Escape must not return to file selection after a successful import.");
            Check(imported && imported->cartridges.front().first == BaseCartridge(selected),
                  "The base priority must be USA, Europe, Japan.");
            Check(imported->cartridges.size() ==
                      static_cast<std::size_t>(std::ranges::count(selected, true)),
                  "Only selected releases must be imported.");
            Key(wizard, WizardKey::Enter);
            Check(wizard.End() == WizardEnd::Finished, "Finish must start the game.");
        }
    }
    auto services = Services(root);
    services.import = [](const WizardImport &files, const auto &) {
        Check(files.cartridges.size() == 1, "Duplicate releases must be imported once.");
        Check(files.disc.is_absolute(), "Tilde paths must resolve before import.");
        return std::optional<std::string>{};
    };
    SetupWizard duplicate(std::move(services));
    Type(duplicate, "~/disc.iso");
    Set(duplicate, Rom1Text, "~/usa.z64");
    Set(duplicate, Rom3Text, "~/usa.z64");
    Click(duplicate, Next);
    Wait(duplicate);
    Check(duplicate.Screen() == WizardScreen::Done, "Tilde paths must import successfully.");
}

void TestRestoredPaths(const std::filesystem::path &root) {
    auto services = Services(root);
    services.disc = "~/disc.iso";
    services.roms = {"~/jpn.n64", "", "~/usa.z64"};
    services.searchDirectory = root / ".hidden";
    SetupWizard wizard(services);
    Check(wizard.Text(DiscText) == "~/disc.iso", "Restore the entered disc path.");
    Check(wizard.Text(Rom1Text) == "~/jpn.n64" && wizard.Text(Rom2Text).empty() &&
              wizard.Text(Rom3Text) == "~/usa.z64",
          "Restore ROM slots without changing their order or filling empty slots.");
    Check(Enabled(wizard, Next), "Validate restored images before enabling Next.");
    Click(wizard, SearchAssets);
    Check(wizard.Text(SearchText) == Utf8FromPath(services.searchDirectory),
          "Restore the search directory instead of the Downloads default.");
    Check(Enabled(wizard, StartSearch), "Validate the restored search directory.");

    services.roms[2] = "~/missing.z64";
    SetupWizard missing(services);
    Check(!Enabled(missing, Next) && missing.Reason(Rom3Text) == "Invalid ROM image.",
          "A missing restored ROM must disable Next and display its validation error.");
    Click(missing, Rom3Clear);
    Check(Enabled(missing, Next), "Clearing a missing path must permit the remaining valid ROM.");
}

void TestRepositoryLink(const std::filesystem::path &root) {
    auto services = Services(root);
    int opened = 0;
    services.openUrl = [&](const std::string &url) {
        Check(url == "https://github.com/Tatsh/coastline",
              "The repository link must open its URI.");
        ++opened;
    };
    SetupWizard wizard(std::move(services));
    WizardEvent event;
    event.x = GRID_COLUMNS * CELL_WIDTH / 2;
    event.y = (GRID_ROWS - 3) * CELL_HEIGHT + 4;
    event.type = WizardEvent::Type::MouseMove;
    wizard.Handle(event);
    TextGrid grid;
    wizard.Draw(grid, 0.0);
    Check(grid.At(GRID_COLUMNS / 2, GRID_ROWS - 3).underline,
          "Hovering the repository link must underline it.");
    event.type = WizardEvent::Type::MouseUp;
    wizard.Handle(event);
    Check(opened == 0, "Releasing an unrelated click must not open the browser.");
    event.type = WizardEvent::Type::MouseDown;
    wizard.Handle(event);
    event.type = WizardEvent::Type::MouseUp;
    wizard.Handle(event);
    Check(opened == 1, "Clicking the repository link must open the browser once.");
    event.type = WizardEvent::Type::MouseDown;
    wizard.Handle(event);
    event.x = 0;
    event.type = WizardEvent::Type::MouseMove;
    wizard.Handle(event);
    event.type = WizardEvent::Type::MouseUp;
    wizard.Handle(event);
    wizard.Draw(grid, 0.0);
    Check(opened == 1, "Dragging outside the link must cancel browser activation.");
    Check(!grid.At(GRID_COLUMNS / 2, GRID_ROWS - 3).underline,
          "Moving outside the repository link must remove its underline.");
}

void TestReimport(const std::filesystem::path &root) {
    int imports = 0;
    auto services = Services(root);
    services.reimportReason = "Existing assets require another import.";
    services.import = [&](const auto &, const auto &) {
        ++imports;
        return std::optional<std::string>{};
    };
    SetupWizard wizard(services);
    Check(wizard.Screen() == WizardScreen::Reimport, "Old assets must open the re-import screen.");
    const auto controls = wizard.Controls();
    Check(controls.size() == 2 && Enabled(wizard, Next) && Enabled(wizard, Quit),
          "Re-import must offer only Start setup and Quit.");
    TextGrid grid;
    wizard.Draw(grid, 0);
    std::string text;
    for (int row = 0; row < GRID_ROWS; ++row) {
        for (int column = 0; column < GRID_COLUMNS; ++column) {
            text += static_cast<char>(grid.At(column, row).glyph);
        }
    }
    Check(text.find(services.reimportReason) != std::string::npos,
          "Re-import must explain why setup is required.");
    Key(wizard, WizardKey::Enter);
    Check(wizard.Screen() == WizardScreen::Files && wizard.End() == WizardEnd::Running,
          "Start setup must open game-file selection.");
    Check(!Enabled(wizard, Next) && imports == 0,
          "Starting setup must not replace assets before valid files are selected.");
    SetupWizard quit(services);
    Click(quit, Quit);
    Check(quit.End() == WizardEnd::Quit && imports == 0,
          "Quit must exit without importing assets.");
    SetupWizard escape(services);
    Key(escape, WizardKey::Escape);
    Check(escape.End() == WizardEnd::Quit && imports == 0,
          "Escape must quit the re-import prompt without changing assets.");
}

void TestEditing(const std::filesystem::path &root) {
    SetupWizard wizard(Services(root));
    Type(wizard, "~/disc.iso");
    Set(wizard, Rom2Text, "~/eur.v64");
    Check(Enabled(wizard, Next), "Disc and one ROM must enable Next.");
    Set(wizard, Rom1Text, "~/bad.z64");
    Check(wizard.Reason(Rom1Text) == "Invalid ROM image.", "Wrong ROMs need the exact error.");
    Check(!Enabled(wizard, Next), "An invalid optional slot must disable Next.");
    TextGrid grid;
    wizard.Draw(grid, 0.0);
    auto rowText = [&](int row) {
        std::string text;
        for (int column = 0; column < GRID_COLUMNS; ++column) {
            text.push_back(static_cast<char>(grid.At(column, row).glyph));
        }
        return text;
    };
    Check(rowText(GRID_ROWS - 3).contains("https://github.com/Tatsh/coastline"),
          "Setup must display its repository URI.");
    Check(rowText(GRID_ROWS - 2).contains("v1.2.3 | Built 2026-11-01T14:32:05Z"),
          "Setup must display the build revision and date.");
    const auto controls = wizard.Controls();
    const auto field = std::ranges::find(controls, Rom1Text, &WizardControl::id);
    Check(grid.At(field->column, field->row).background == CgaColor::Red,
          "Invalid ROM fields must be red.");
    Click(wizard, Rom1Clear);
    Check(Enabled(wizard, Next), "Clearing an invalid slot must restore Next.");
    Set(wizard, Rom3Text, "~/missing.z64");
    Check(!Enabled(wizard, Next), "A missing optional image must disable Next.");
    Click(wizard, Rom3Clear);
    Click(wizard, Rom2Clear);
    Check(!Enabled(wizard, Next), "Clearing the only ROM must disable Next.");
    Click(wizard, DiscClear);
    Type(wizard, "/one/two three");
    Key(wizard, WizardKey::DeleteWord);
    Key(wizard, WizardKey::SelectAll);
    Check(wizard.SelectedText() == "/one/two ", "DeleteWord must remove the previous word.");
    Type(wizard, "é 日本");
    Key(wizard, WizardKey::Left, true);
    Check(wizard.SelectedText() == "本", "Shift arrows must select Unicode characters.");
    Key(wizard, WizardKey::Backspace);
    Key(wizard, WizardKey::SelectAll);
    Check(wizard.SelectedText() == "é 日", "Backspace must erase the selection.");
    Key(wizard, WizardKey::Delete);
    Type(wizard, "alpha beta");
    Key(wizard, WizardKey::Home);
    Key(wizard, WizardKey::DeleteNextWord);
    Key(wizard, WizardKey::SelectAll);
    Check(wizard.SelectedText() == "beta", "DeleteNextWord must erase the next word.");
    Type(wizard, "~/disc.iso");
    Check(wizard.Reason(DiscText).empty(), "Typing must replace selected text and revalidate.");
}

void TestBrowser(const std::filesystem::path &root) {
    Check(ResolveUserPath("~/usa.z64", root) == root / "usa.z64" &&
              ResolveUserPath("~\\usa.z64", root) == root / "usa.z64",
          "Both home-directory prefixes must resolve.");
#ifdef _WIN32
    Check(Utf8FromPath(PathFromUtf8("C:/games/assets")) == "C:\\games\\assets",
          "Windows paths must display native separators.");
    Check(Utf8FromPath(ResolveUserPath("~\\games\\assets", root)).find('/') == std::string::npos,
          "Windows home expansion must use backslashes.");
#endif
    constexpr std::array<std::string_view, 1> extensions{".iso"};
    auto entries = ListBrowserDirectory(root, extensions);
    Check(entries && entries->front().kind == BrowserEntry::Kind::Parent,
          "The parent directory must be first.");
    Check((*entries)[1].name == "Alpha" && (*entries)[2].name == "zeta",
          "Directories must precede files in case-insensitive order.");
    Check(entries->size() == 4 && entries->back().name == "disc.iso",
          "Hidden entries and other extensions must be filtered.");
    auto folders = ListBrowserDirectory(root, extensions, true);
    Check(folders && folders->size() == 3, "Directory selection must exclude files.");
    SetupWizard wizard(Services(root));
    Click(wizard, DiscBrowse);
    Type(wizard, "disc.iso");
    Key(wizard, WizardKey::Enter);
    Check(!wizard.BrowserOpen() && wizard.Reason(DiscText).empty(),
          "Choosing a file must validate the path.");
    Click(wizard, SearchAssets);
    Click(wizard, SearchClear);
    Check(!Enabled(wizard, StartSearch), "An empty directory must disable searching.");
    Type(wizard, "~");
    Check(Enabled(wizard, StartSearch), "A home directory must resolve tilde.");
    Click(wizard, SearchBrowse);
    Click(wizard, BrowserOk);
    Check(!wizard.BrowserOpen() && Enabled(wizard, StartSearch),
          "The folder browser must select its current directory.");
    Click(wizard, Back);
    Key(wizard, WizardKey::Escape);
    Click(wizard, MessageNo);
    Check(wizard.End() == WizardEnd::Running, "No must return from the quit prompt.");
}

void TestSearch(const std::filesystem::path &root) {
    auto services = Services(root);
    bool sawDiscBeforeRom = false;
    bool sawAll = false;
    auto result =
        SearchWizardDirectory(root, services, [&](const auto &found, const auto &, double f) {
            Check(f >= 0 && f <= 1, "Search progress must be a fraction.");
            sawDiscBeforeRom |= !found.disc.empty() && found.roms[0].empty();
            sawAll |= !found.disc.empty() && std::ranges::all_of(found.roms, [](const auto &path) {
                return !path.empty();
            });
            return true;
        });
    Check(result.disc == root / "Alpha" / "disc.cue", "Search must prefer a CUE sheet.");
    Check(sawDiscBeforeRom && sawAll, "Progress snapshots must update discovered images.");
    Check(result.roms[0] == root / ".hidden" / "sub" / "usa.z64",
          "Search must include hidden directories recursively.");
    unsigned callbacks = 0;
    SearchWizardDirectory(
        root, services, [&](const auto &, const auto &, double) { return ++callbacks < 2; });
    Check(callbacks == 2, "Search must stop when cancelled.");
    Check(SearchWizardDirectory(root / "missing", services, {}).failure.has_value(),
          "Unreadable directories must report a failure.");
    SetupWizard wizard(services);
    WizardEvent search;
    search.type = WizardEvent::Type::Accelerator;
    search.letter = 's';
    wizard.Handle(search);
    Check(wizard.Screen() == WizardScreen::SearchDirectory, "Alt+S must open asset search.");
    Set(wizard, SearchText, "~");
    Click(wizard, StartSearch);
    Wait(wizard);
    Check(wizard.MessageLines().empty(), "A complete search must not warn.");
    Click(wizard, UseFound);
    Check(Enabled(wizard, Next), "A complete search must populate valid setup fields.");
    Check(wizard.Text(DiscText) == Utf8FromPath(std::filesystem::path("~") / "Alpha" / "disc.cue"),
          "Search results inside home must display a tilde prefix.");
    for (auto id : {Rom1Text, Rom2Text, Rom3Text}) {
        Check(wizard.Text(id).starts_with("~"), "Found ROM paths must abbreviate home.");
    }
    Click(wizard, SearchAssets);
    Set(wizard, SearchText, "~/zeta");
    Click(wizard, StartSearch);
    Wait(wizard);
    Check(!wizard.MessageLines().empty(), "A partial search must offer another directory.");
    Click(wizard, MessageNo);
    Check(wizard.Focus() == DiscText && !Enabled(wizard, Next),
          "ROM-only results must focus the missing disc and remove previous disc paths.");
    Click(wizard, SearchAssets);
    Set(wizard, SearchText, "~/Alpha");
    Click(wizard, StartSearch);
    Wait(wizard);
    Click(wizard, MessageNo);
    Check(wizard.Focus() == Rom1Text && !Enabled(wizard, Next),
          "Disc-only results must focus a ROM slot and remove previous ROM paths.");
    Click(wizard, SearchAssets);
    Click(wizard, StartSearch);
    Wait(wizard);
    Click(wizard, MessageYes);
    Check(wizard.Screen() == WizardScreen::SearchDirectory,
          "Retry must return to directory selection.");
    std::ofstream(root / "zeta" / "disc.iso").put('x');
    Set(wizard, SearchText, "~/zeta");
    Click(wizard, StartSearch);
    Wait(wizard);
    Click(wizard, MessageNo);
    Check(Enabled(wizard, Next), "A partial search with disc and one ROM must permit import.");
    Click(wizard, SearchAssets);
    Set(wizard, SearchText, "~");
    Click(wizard, StartSearch);
    Key(wizard, WizardKey::Escape);
    Wait(wizard);
    Check(wizard.Screen() == WizardScreen::SearchDirectory,
          "Cancelling a search must return to directory selection.");
}

void TestImportFailureAndCancel(const std::filesystem::path &root) {
    for (bool cancel : {false, true}) {
        auto services = Services(root);
        services.import = [cancel](const auto &, const WizardProgress &progress) {
            if (cancel) {
                while (progress("Importing", 0.5)) {
                    std::this_thread::yield();
                }
            }
            return std::optional<std::string>("Import failed");
        };
        SetupWizard wizard(std::move(services));
        Type(wizard, "~/disc.iso");
        Set(wizard, Rom1Text, "~/usa.z64");
        Click(wizard, Next);
        if (cancel) {
            Key(wizard, WizardKey::Escape);
        }
        Wait(wizard);
        Check(wizard.Screen() == (cancel ? WizardScreen::Files : WizardScreen::Import),
              "Cancel must return to Files, and failure must retain Import.");
        if (!cancel) {
            Click(wizard, Back);
            Check(Enabled(wizard, Next), "Back must permit correction and retry.");
        }
    }
}
} // namespace

void TestRowNavigation(const std::filesystem::path &root) {
    SetupWizard wizard(Services(root));
    Key(wizard, WizardKey::Down);
    Check(wizard.Focus() == Rom1Text, "Down must move from the disc to the first ROM row.");
    Key(wizard, WizardKey::Down);
    Check(wizard.Focus() == Rom2Text, "Down must move to the second ROM row.");
    Key(wizard, WizardKey::Down);
    Check(wizard.Focus() == Rom3Text, "Down must move to the third ROM row.");
    Key(wizard, WizardKey::Down);
    Check(wizard.Focus() == SearchAssets, "Down must enter the bottom button row.");
    Key(wizard, WizardKey::Up);
    Check(wizard.Focus() == Rom3Text, "Up must return from buttons to the text fields.");
    for (int attempt = 0; attempt < 30 && wizard.Focus() != DiscBrowse; ++attempt) {
        Key(wizard, WizardKey::Tab);
    }
    Check(wizard.Focus() == DiscBrowse, "Tab must focus the disc browse button.");
    Key(wizard, WizardKey::Down);
    Check(wizard.Focus() == Rom1Browse, "Vertical movement must preserve the browse column.");
    Click(wizard, DiscBrowse);
    Key(wizard, WizardKey::End);
    Key(wizard, WizardKey::Down);
    Check(wizard.Focus() == BrowserFilter, "Down must exit the final directory entry.");
    Key(wizard, WizardKey::Down);
    Check(wizard.Focus() == BrowserOk || wizard.Focus() == BrowserUseDirectory ||
              wizard.Focus() == BrowserCancel,
          "Down must enter the browser buttons.");
    Key(wizard, WizardKey::Up);
    Check(wizard.Focus() == BrowserFilter, "Up must return to the browser filter.");
}

void TestDiscDirectory(const std::filesystem::path &root) {
    std::filesystem::create_directories(root / "disc");
    std::ofstream(root / "disc" / "BossRally.pod").put('x');
    auto services = Services(root);
    services.disc = root / "disc";
    services.roms[0] = root / "usa.z64";
    SetupWizard wizard(std::move(services));
    Check(Enabled(wizard, Next), "A disc directory and ROM must enable Next.");
    Click(wizard, DiscBrowse);
    Check(Enabled(wizard, BrowserUseDirectory), "Disc browsing must offer directory selection.");
    Click(wizard, BrowserUseDirectory);
    Check(Enabled(wizard, Next), "Selecting the current disc directory must preserve validation.");
    auto found = SearchWizardDirectory(root / "disc", Services(root), {});
    Check(found.disc == root / "disc", "Asset search must recognise a mounted disc directory.");
    Set(wizard, DiscText, Utf8FromPath(root / "Alpha"));
    Check(!Enabled(wizard, Next), "An unrelated directory must not enable Next.");
    std::filesystem::remove_all(root / "disc");
}

void TestDiscContainers(const std::filesystem::path &root) {
    for (const auto *extension : {".MDS",
                                  ".MDF",
                                  ".NRG",
                                  ".CDI",
                                  ".B5T",
                                  ".B5I",
                                  ".B6T",
                                  ".B6I",
                                  ".BWT",
                                  ".BWI",
                                  ".BWS",
                                  ".ISZ",
                                  ".PDI"}) {
        const auto directory = root / "containers";
        std::filesystem::create_directory(directory);
        const auto disc = directory / (std::string("disc") + extension);
        std::ofstream(disc).put('x');
        auto services = Services(root);
        services.disc = disc;
        services.roms[0] = root / "usa.z64";
        SetupWizard wizard(std::move(services));
        Check(Enabled(wizard, Next), "Container disc images must pass to disc validation.");
        Click(wizard, DiscBrowse);
        Check(std::ranges::any_of(
                  wizard.BrowserEntries(),
                  [&](const auto &entry) { return entry.name == disc.filename().string(); }),
              "Disc browsing must list container image extensions.");
        const auto found = SearchWizardDirectory(directory, Services(root), {});
        Check(found.disc == disc, "Recursive search must recognise container images.");
        std::filesystem::remove_all(directory);
    }
}

int main() {
    auto root = std::filesystem::temp_directory_path() /
                ("coastline-firstrun-" +
                 std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    try {
        std::filesystem::create_directories(root / "Alpha");
        std::filesystem::create_directories(root / "zeta");
        std::filesystem::create_directories(root / ".hidden" / "sub");
        for (const char *name : {"disc.iso",
                                 "usa.z64",
                                 "eur.v64",
                                 "jpn.n64",
                                 "bad.z64",
                                 ".hidden.iso",
                                 "Alpha/disc.cue",
                                 "zeta/usa.z64",
                                 ".hidden/sub/usa.z64"}) {
            std::ofstream(root / name).put('x');
        }
        TestCombinations(root);
        TestRowNavigation(root);
        TestDiscDirectory(root);
        TestRestoredPaths(root);
        TestRepositoryLink(root);
        TestReimport(root);
        TestEditing(root);
        TestBrowser(root);
        TestDiscContainers(root);
        TestSearch(root);
        TestImportFailureAndCancel(root);
        Check(EncodeUtf8(DecodeUtf8("é 日本")) == "é 日本", "UTF-8 paths must round-trip.");
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        std::filesystem::remove_all(root);
        return 1;
    }
    std::filesystem::remove_all(root);
    return 0;
}
