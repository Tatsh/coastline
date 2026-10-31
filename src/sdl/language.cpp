#include "language.h"

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
