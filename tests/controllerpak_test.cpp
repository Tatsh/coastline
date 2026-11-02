#include <algorithm>
#include <cassert>
#include <filesystem>
#include <vector>

#include "controllerpak.h"
#include "savefile.h"

int main() {
    using namespace coastline;
    const auto root = std::filesystem::temp_directory_path() / "coastline-controllerpak-test";
    std::filesystem::remove_all(root);
    ControllerPak pak(root / "notes", root / "settings");
    std::vector<std::uint8_t> options(PAK_PAGE_BYTES, 0x42);
    std::vector<std::uint8_t> season(2 * PAK_PAGE_BYTES, 0x23);
    assert(pak.WriteNote("TOP GEAR OPTIONS", options));
    assert(pak.WriteNote("TOP GEAR SEASON", season));
    assert(!pak.ReadNote("TOP GEAR SEASON", [](std::span<const std::uint8_t>) { return false; }));
    assert(!std::filesystem::exists(root / "notes" / "TOP GEAR SEASON"));
    assert(pak.WriteNote("TOP GEAR SEASON", season));
    assert(pak.ReadNote("TOP GEAR OPTIONS") == options);
    assert(pak.ReadNote("TOP GEAR SEASON") == season);
    assert(pak.Notes().size() == 2);
    assert(pak.FreeBytes() == (PAK_PAGES - 3) * PAK_PAGE_BYTES);
    for (const auto release : {PakRelease::Usa, PakRelease::Eur, PakRelease::Jpn}) {
        const auto image = root / "native.mpk";
        assert(!ExportPakImage(pak, image, release));
        assert(std::filesystem::file_size(image) == PAK_IMAGE_BYTES);
        ControllerPak imported(root / "imported");
        assert(!ImportPakImage(imported, image));
        assert(imported.ReadNote("TOP GEAR OPTIONS") == options);
        assert(imported.ReadNote("TOP GEAR SEASON") == season);
        const auto second = root / "roundtrip.mpk";
        assert(!ExportPakImage(imported, second, release));
        assert(ReadSaveFile(image, PAK_IMAGE_BYTES) == ReadSaveFile(second, PAK_IMAGE_BYTES));
    }
    std::vector<std::uint8_t> four(4 * PAK_IMAGE_BYTES, 0x71);
    const auto image = root / "four.mpk";
    assert(WriteSaveFile(image, four));
    assert(!ExportPakImage(pak, image));
    const auto exported = ReadSaveFile(image, four.size());
    assert(exported && exported->size() == four.size());
    assert(std::equal(
        exported->begin() + PAK_IMAGE_BYTES, exported->end(), four.begin() + PAK_IMAGE_BYTES));
    const auto adjacentImage = root / "notes" / "external.mpk";
    assert(WriteSaveFile(adjacentImage, four));
    assert(!ExportPakImage(pak, adjacentImage));
    assert(pak.Notes().size() == 2);
    assert(std::filesystem::file_size(adjacentImage) == four.size());
    const auto path = root / "notes" / "TOP GEAR SEASON";
    auto damaged = ReadSaveFile(path, 1024);
    assert(damaged);
    damaged->back() ^= 1;
    assert(WriteSaveFile(path, *damaged));
    assert(!pak.ReadNote("TOP GEAR SEASON"));
    assert(!std::filesystem::exists(path));
    assert(pak.Notes().size() == 1 && pak.FreeBytes() == (PAK_PAGES - 1) * PAK_PAGE_BYTES);
    assert(pak.WriteNote("TOP GEAR SEASON", season));
    const std::vector<std::uint8_t> bytes{1, 2, 3};
    for (const auto *name : {"../outside",
                             "..\\outside",
                             "C:outside",
                             ".",
                             "..",
                             "TOP GEAR SEASON.lock",
                             "TOP GEAR SEASON.LOCK",
                             "TOP GEAR SEASON.lock.",
                             "TOP GEAR SEASON "}) {
        assert(!pak.WriteNote(name, bytes));
        assert(!pak.ReadNote(name));
        assert(!pak.DeleteNote(name));
    }
    assert(!std::filesystem::exists(root / "outside"));
    assert(pak.DeleteNote("TOP GEAR SEASON"));
    std::filesystem::remove_all(root);
}
