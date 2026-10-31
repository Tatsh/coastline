#include <algorithm>
#include <array>
#include <atomic>
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
}

bool Enabled(const SetupWizard &wizard, WizardControlId id) {
    auto controls = wizard.Controls();
    auto control = std::ranges::find(controls, id, &WizardControl::id);
    return control != controls.end() && control->enabled;
}

void Wait(SetupWizard &wizard) {
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (wizard.Screen() == WizardScreen::Import && !Enabled(wizard, Back)) {
        Check(std::chrono::steady_clock::now() < deadline, "The import must finish.");
        wizard.Update();
        std::this_thread::yield();
    }
}

WizardServices Services(const std::filesystem::path &root) {
    WizardServices services;
    services.home = root;
    services.checkDisc = [](const auto &) { return std::optional<std::string>{}; };
    services.identifyCartridge = [](const std::filesystem::path &path) {
        return path.stem() == "usa" ? WizardCartridge::Usa :
               path.stem() == "eur" ? WizardCartridge::Eur :
                                      WizardCartridge::Jpn;
    };
    services.import = [](const auto &, const auto &) { return std::optional<std::string>{}; };
    return services;
}

void Disc(SetupWizard &wizard, const std::filesystem::path &root) {
    Check(wizard.Focus() == DiscText, "The disc path must have initial focus.");
    Check(!Enabled(wizard, Next), "An empty disc path must disable Next.");
    Type(wizard, Utf8FromPath(root / "disc.iso"));
    Check(Enabled(wizard, Next), "A valid disc must enable Next.");
    Key(wizard, WizardKey::Enter);
    Check(wizard.Screen() == WizardScreen::Cartridges, "Enter must advance from a text box.");
}

void TestCombinations(const std::filesystem::path &root) {
    constexpr std::array checks{CheckUsa, CheckEur, CheckJpn};
    constexpr std::array fields{UsaText, EurText, JpnText};
    constexpr std::array names{"usa.z64", "eur.v64", "jpn.n64"};
    for (unsigned bits = 0; bits < 8; ++bits) {
        std::optional<WizardImport> imported;
        auto services = Services(root);
        services.import = [&](const WizardImport &files, const WizardProgress &progress) {
            imported = files;
            progress("Importing", 0.5);
            return std::optional<std::string>{};
        };
        SetupWizard wizard(std::move(services));
        Disc(wizard, root);
        std::array<bool, 3> checked{};
        for (unsigned i = 0; i < 3; ++i) {
            checked[i] = (bits & (1u << i)) != 0;
            if (checked[i]) {
                Click(wizard, checks[i]);
            }
        }
        Check(Enabled(wizard, Next) == (bits != 0), "At least one release must be checked.");
        if (!bits) {
            Check(!BaseCartridge(checked), "An empty selection must have no base.");
            continue;
        }
        const auto expected = (bits & 1) ? WizardCartridge::Usa :
                              (bits & 2) ? WizardCartridge::Eur :
                                           WizardCartridge::Jpn;
        Check(BaseCartridge(checked) == expected, "The base priority must be USA, Europe, Japan.");
        Click(wizard, Next);
        Check(!Enabled(wizard, Next), "Every checked release must have a valid path.");
        for (unsigned i = 0; i < 3; ++i) {
            if (checked[i]) {
                Click(wizard, fields[i]);
                Type(wizard, Utf8FromPath(root / names[i]));
            }
        }
        Check(Enabled(wizard, Next), "All valid rows must enable Next.");
        Click(wizard, Next);
        Wait(wizard);
        Check(wizard.Screen() == WizardScreen::Done, "A successful import must show Done.");
        Check(imported && imported->cartridges.front().first == BaseCartridge(checked),
              "The base must be imported first for every combination.");
        Check(imported->cartridges.size() ==
                  static_cast<std::size_t>(std::ranges::count(checked, true)),
              "Only checked cartridges must be imported.");
        Check(wizard.Focus() == Finish, "Done must focus Finish.");
        Key(wizard, WizardKey::Enter);
        Check(wizard.End() == WizardEnd::Finished, "Finish must start the game.");
    }
}

void TestValidation(const std::filesystem::path &root) {
    SetupWizard wizard(Services(root));
    Type(wizard, Utf8FromPath(root / "missing.iso"));
    Check(wizard.Reason(DiscText) == "File not found", "Missing files must fail validation.");
    Click(wizard, Next);
    Check(wizard.Screen() == WizardScreen::Disc, "Disabled Next must not advance.");
    Key(wizard, WizardKey::Home);
    Key(wizard, WizardKey::Delete);
    Check(!Enabled(wizard, Next), "Editing a missing file must not validate it.");
    SetupWizard wrong(Services(root));
    Disc(wrong, root);
    Key(wrong, WizardKey::Space);
    Click(wrong, Next);
    Type(wrong, Utf8FromPath(root / "eur.v64"));
    Check(wrong.Reason(UsaText) == "Not the USA release", "Wrong-release rows must fail.");
    Check(!Enabled(wrong, Next), "A wrong-release row must disable Next.");
    TextGrid grid;
    wrong.Draw(grid, 0.0);
    Check(grid.cursor && grid.At((*grid.cursor)[0], (*grid.cursor)[1]).background == CgaColor::Red,
          "Invalid input must draw in red.");
}

void TestBrowser(const std::filesystem::path &root) {
    constexpr std::array<std::string_view, 1> extensions{".iso"};
    auto entries = ListBrowserDirectory(root, extensions);
    Check(entries && entries->front().kind == BrowserEntry::Kind::Parent,
          "The parent directory must be first.");
    Check((*entries)[1].name == "Alpha" && (*entries)[2].name == "zeta",
          "Directories must precede files in case-insensitive order.");
    Check(entries->size() == 4 && entries->back().name == "disc.iso",
          "Hidden entries and other extensions must be filtered.");
    Check(!ListBrowserDirectory(root / "missing", extensions),
          "An unreadable directory must fail without throwing.");
    SetupWizard wizard(Services(root));
    Key(wizard, WizardKey::Tab, true);
    Check(wizard.Focus() == DiscBrowse, "Shift and Tab must wrap past disabled controls.");
    Key(wizard, WizardKey::Enter);
    Check(wizard.BrowserOpen(), "Enter on Browse must open the browser.");
    Type(wizard, "disc");
    Check(wizard.BrowserEntries()[wizard.BrowserSelection()].name == "disc.iso",
          "Typing must select a matching entry.");
    Key(wizard, WizardKey::Enter);
    Check(!wizard.BrowserOpen() && Enabled(wizard, Next),
          "Choosing a file must close the browser and validate the path.");
    Check(wizard.LastDirectory() == root, "The browser must remember its directory.");
    Key(wizard, WizardKey::Escape);
    Check(!wizard.MessageLines().empty(), "Escape on the first screen must confirm quitting.");
    Click(wizard, MessageNo);
    Check(wizard.End() == WizardEnd::Running, "No must return to setup.");
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
        Disc(wizard, root);
        Click(wizard, CheckUsa);
        Click(wizard, Next);
        Type(wizard, Utf8FromPath(root / "usa.z64"));
        Click(wizard, Next);
        if (cancel) {
            Key(wizard, WizardKey::Escape);
        }
        Wait(wizard);
        Check(wizard.Screen() == (cancel ? WizardScreen::Disc : WizardScreen::Import),
              "Cancel must return to Disc, and failure must retain Import.");
        if (!cancel) {
            Check(Enabled(wizard, Back), "Import failures must enable Back.");
            Click(wizard, Back);
            Check(wizard.Screen() == WizardScreen::Paths, "Back must allow correction.");
        }
    }
}

} // namespace

int main() {
    auto root = std::filesystem::temp_directory_path() /
                ("coastline-firstrun-" +
                 std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    try {
        std::filesystem::create_directories(root / "Alpha");
        std::filesystem::create_directories(root / "zeta");
        for (const char *name : {"disc.iso", "usa.z64", "eur.v64", "jpn.n64", ".hidden.iso"}) {
            std::ofstream(root / name).put('x');
        }
        TestCombinations(root);
        TestValidation(root);
        TestBrowser(root);
        TestImportFailureAndCancel(root);
        Check(EncodeUtf8(DecodeUtf8("é 日本")) == "é 日本", "UTF-8 paths must round-trip.");
        Check(Cp437FromCodepoint(U'日') == REPLACEMENT_GLYPH,
              "Unsupported code points must draw a replacement glyph.");
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        std::filesystem::remove_all(root);
        return 1;
    }
    std::filesystem::remove_all(root);
    return 0;
}
