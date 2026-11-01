#include "language.h"

#include <algorithm>
#include <set>

#include "config.h"

namespace coastline {

StringTable MergeStrings(StringTable fallback, const StringTable &language) {
    for (const auto &[key, text] : language) {
        fallback.insert_or_assign(key, text);
    }
    return fallback;
}

std::filesystem::path ReleaseDirectory(const std::filesystem::path &assets) {
    return assets / "n64" / "base";
}

std::string CartridgeRegion(const std::filesystem::path &release) {
    auto region = GameConfig::Load(release / "cartridge.ini").Get("Cartridge", "Region");
    return region && (*region == "usa" || *region == "eur" || *region == "jpn") ? *region : "";
}

std::vector<std::string>
AvailableCartridgeRegions(std::span<const std::filesystem::path> dataRoots) {
    std::set<std::string> regions;
    for (const auto &root : dataRoots) {
        for (const auto pack : {"default", "usa", "eur", "jpn"}) {
            const auto region = CartridgeRegion(ReleaseDirectory(root / "assets" / pack));
            if (!region.empty() && (std::string_view(pack) == "default" || region == pack)) {
                regions.insert(region);
            }
        }
    }
    return {regions.begin(), regions.end()};
}

std::optional<std::string> TitleScreenRegion(std::string_view choice,
                                             std::string_view currentRegion,
                                             std::span<const std::string> available) {
    auto present = [&](std::string_view region) {
        return region == currentRegion || std::ranges::find(available, region) != available.end();
    };
    if (choice == "regional") {
        return std::string(currentRegion);
    }
    if (choice == "japanese" && present("jpn")) {
        return "jpn";
    }
    if (choice == "international") {
        if (currentRegion == "usa" || currentRegion == "eur") {
            return std::string(currentRegion);
        }
        for (const auto region : {"usa", "eur"}) {
            if (present(region)) {
                return region;
            }
        }
    }
    return std::nullopt;
}

std::filesystem::path FontDirectory(const std::filesystem::path &assets,
                                    std::string_view language) {
    auto fonts = assets / "fonts" /
                 (language == "ja"    ? "jpn" :
                  language == "en-gb" ? "eur" :
                                        "usa");
    std::error_code error;
    return std::filesystem::is_regular_file(fonts / "fonts.json", error) ?
               fonts :
               assets / "fonts" / "base";
}

} // namespace coastline
