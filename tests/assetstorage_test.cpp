#include <cassert>
#include <fstream>
#include <iterator>
#include <map>
#include <string>

#include "assetstorage.h"
#include "paths.h"

namespace {

void Write(const std::filesystem::path &path, const std::string &bytes) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::binary);
    file << bytes;
    assert(file.good());
}

std::string Read(const std::filesystem::path &path) {
    std::ifstream file(path, std::ios::binary);
    assert(file.good());
    return std::string(std::istreambuf_iterator<char>(file), {});
}

using Files = std::map<std::filesystem::path, std::string>;

Files Snapshot(const std::filesystem::path &directory) {
    Files files;
    for (const auto &entry : std::filesystem::recursive_directory_iterator(directory)) {
        if (entry.is_regular_file()) {
            files[entry.path().lexically_relative(directory)] = Read(entry.path());
        }
    }
    return files;
}

void CheckFiles(const std::filesystem::path &directory, const Files &files) {
    for (const auto &[path, bytes] : files) {
        assert(Read(directory / path) == bytes);
    }
}

void CheckInvalidTables(const std::filesystem::path &pack, const std::filesystem::path &defaults) {
    Write(pack / "target.bin", "target");
    for (const std::string &text :
         {"coastline-asset-aliases 2\nend\n",
          "coastline-asset-aliases 1\n",
          "coastline-asset-aliases 1\nend\ntrailing\n",
          "coastline-asset-aliases 1\nself \"../outside\" \"target.bin\"\nend\n",
          "coastline-asset-aliases 1\nself \"alias\" \"../outside\"\nend\n",
          "coastline-asset-aliases 1\nself \"/absolute\" \"target.bin\"\nend\n",
          "coastline-asset-aliases 1\nboss \"alias\" \"target.bin\"\nend\n",
          "coastline-asset-aliases 1\nself \"alias\" \"missing.bin\"\nend\n",
          "coastline-asset-aliases 1\nself \"target.bin\" \"target.bin\"\nend\n",
          "coastline-asset-aliases 1\nself \".layer-stamp\" \"target.bin\"\nend\n",
          "coastline-asset-aliases 1\nself \"a\" \"target.bin\"\n"
          "self \"a\" \"target.bin\"\nend\n",
          "coastline-asset-aliases 1\nself \"a/b\" \"target.bin\"\n"
          "self \"a\" \"target.bin\"\nend\n"}) {
        Write(pack / coastline::ASSET_ALIASES_FILE, text);
        assert(!coastline::ReadAssetFileAliases(pack, defaults));
    }
}

} // namespace

int main() {
    using namespace coastline;
    const auto root = std::filesystem::temp_directory_path() / "coastline-assetstorage-test";
    std::filesystem::remove_all(root);
    const auto data = root / "data";
    const auto cache = root / "cache";
    const auto defaults = DefaultAssets(data);
    const auto boss = data / "assets" / "boss";
    const auto eur = data / "assets" / "eur";
    const auto jpn = data / "assets" / "jpn";
    const auto mod = data / "assets" / "mod";
    Write(defaults / "a.png", "base");
    Write(defaults / "textures" / "b.png", "base");
    Write(defaults / "virtual" / "only.png", "base");
    Write(defaults / "cars" / "a.gltf", "model");
    Write(defaults / "cars" / "b.gltf", "model");
    Write(defaults / "zero-a.bin", "");
    Write(defaults / "zero-b.bin", "");
    Write(defaults / std::filesystem::path(u8"images/日本.png"), "base");
    Write(defaults / "binary-a.bin", std::string("a\0b", 3));
    Write(defaults / "binary-b.bin", std::string("a\0b", 3));
    Write(boss / "a.png", "override");
    Write(boss / "base-copy.png", "base");
    Write(boss / "shared-a.bin", "optional common");
    Write(boss / "shared-b.bin", "optional common");
    Write(eur / "shared.bin", "optional common");
    Write(jpn / "shared.bin", "optional common");
    Write(mod / "cars" / "b.glb", "mod GLB");
    Write(mod / "keep.png", "base");
    const auto originalDefault = Snapshot(defaults);
    const auto originalBoss = Snapshot(boss);
    const auto originalEur = Snapshot(eur);
    const auto originalJpn = Snapshot(jpn);
    const auto originalMod = Snapshot(mod);
    auto savings = DeduplicateAssetFiles(defaults);
    assert(savings && savings->files == 10 && savings->bytes > 0);
    assert(Snapshot(mod) == originalMod);
    assert(!std::filesystem::exists(defaults / "virtual" / "only.png"));
    assert(!std::filesystem::exists(eur / "shared.bin"));
    auto aliases = ReadAssetFileAliases(boss, defaults);
    assert(aliases && aliases->size() == 3);
    for (const auto &alias : *aliases) {
        assert(alias.fromDefault);
    }
    const auto physicalDefault = Snapshot(defaults);
    auto expanded = PackAssets(data, cache, "default");
    assert(expanded && *expanded != defaults);
    CheckFiles(*expanded, originalDefault);
    assert(Snapshot(defaults) == physicalDefault);
    Write(*expanded / "cache-marker", "marker");
    expanded = PackAssets(data, cache, "default");
    assert(expanded && std::filesystem::exists(*expanded / "cache-marker"));

    auto layeredBoss = PackAssets(data, cache, "boss");
    assert(layeredBoss);
    auto bossFiles = originalDefault;
    for (const auto &[path, bytes] : originalBoss) {
        bossFiles[path] = bytes;
    }
    CheckFiles(*layeredBoss, bossFiles);
    assert(Read(*layeredBoss / "textures" / "b.png") == "base");
    assert(Read(*layeredBoss / "base-copy.png") == "base");
    assert(Read(*layeredBoss / "a.png") == "override");
    auto layeredEur = PackAssets(data, cache, "eur");
    auto layeredJpn = PackAssets(data, cache, "jpn");
    assert(layeredEur && layeredJpn);
    CheckFiles(*layeredEur, originalEur);
    CheckFiles(*layeredJpn, originalJpn);
    auto layeredMod = PackAssets(data, cache, "mod");
    assert(layeredMod && !std::filesystem::exists(*layeredMod / "cars" / "b.gltf"));
    assert(Read(*layeredMod / "cars" / "b.glb") == "mod GLB");
    assert(Read(*layeredMod / "cars" / "a.gltf") == "model");
    expanded = PackAssets(data, cache, "default");
    assert(expanded && std::filesystem::exists(*expanded / "cache-marker"));

    CheckInvalidTables(data / "assets" / "invalid", defaults);
    std::filesystem::remove(defaults / "a.png");
    assert(!ReadAssetFileAliases(defaults, defaults));
    assert(!PackAssets(data, cache, "default"));

    const GamePaths protectedPaths{
        root / "config", root / "protected", root / "state", root / "protected-cache"};
    const auto protectedPack = DefaultAssets(protectedPaths.data);
    const auto restricted = protectedPack / "restricted";
    const auto publicFiles = protectedPack / "public";
    Write(restricted / "file.bin", "restricted");
    Write(publicFiles / "file.bin", "public");
    Write(protectedPack / "shared.bin", "public");
    Write(protectedPaths.data / "assets/mod/file.bin", "mod");
    std::filesystem::create_directories(protectedPack / "empty/nested");
    assert(DeduplicateAssetFiles(protectedPack));
    constexpr auto writes = std::filesystem::perms::owner_write |
                            std::filesystem::perms::group_write |
                            std::filesystem::perms::others_write;
#ifndef _WIN32
    std::filesystem::permissions(restricted, std::filesystem::perms(0750));
    std::filesystem::permissions(restricted / "file.bin", std::filesystem::perms(0640));
    std::filesystem::permissions(publicFiles, std::filesystem::perms(0755));
    std::filesystem::permissions(publicFiles / "file.bin", std::filesystem::perms(0644));
#endif
    std::map<std::filesystem::path, std::filesystem::perms> originalModes;
    for (const auto &path :
         {restricted, publicFiles, restricted / "file.bin", publicFiles / "file.bin"}) {
        originalModes[path] = std::filesystem::status(path).permissions();
    }
    assert(!FinalizeGeneratedAssetPacks(protectedPack));
    assert(!std::filesystem::exists(protectedPack / "empty"));
    for (const auto &[path, permissions] : originalModes) {
        assert(std::filesystem::status(path).permissions() == (permissions & ~writes));
    }
    auto protectedCache = PackAssets(protectedPaths.data, protectedPaths.cache, "default");
    assert(protectedCache && Read(*protectedCache / "shared.bin") == "public");
    std::error_code removalError;
    RemoveAssetTree(protectedPaths.cache, removalError);
    assert(!removalError);
    for (const auto &[path, permissions] : originalModes) {
        assert(std::filesystem::status(path).permissions() == (permissions & ~writes));
    }
    assert(!ResetGamePaths(protectedPaths));
    assert(!std::filesystem::exists(protectedPack));
    assert(Read(protectedPaths.data / "assets/mod/file.bin") == "mod");
    assert(!ResetGamePaths(protectedPaths));
#ifndef _WIN32
    const auto outside = root / "outside";
    Write(outside / "file.bin", "external");
    std::filesystem::permissions(outside, std::filesystem::perms(0550));
    std::filesystem::create_directories(protectedPack);
    std::filesystem::create_directory_symlink(outside, protectedPack / "link");
    assert(!ResetGamePaths(protectedPaths));
    assert(std::filesystem::status(outside).permissions() == std::filesystem::perms(0550));
    assert(Read(outside / "file.bin") == "external");
    std::filesystem::permissions(outside, std::filesystem::perms(0750));
#endif
    std::filesystem::remove_all(root);
}
