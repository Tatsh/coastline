#include "language.h"

namespace coastline {

namespace {

std::string g_language(DEFAULT_LANGUAGE);

} // namespace

void SelectLanguage(std::string_view language) {
    g_language = language;
}

const std::string &SelectedLanguage() {
    return g_language;
}

StringTable MergeStrings(StringTable fallback, const StringTable &language) {
    for (const auto &[key, text] : language) {
        fallback.insert_or_assign(key, text);
    }
    return fallback;
}

std::filesystem::path ReleaseDirectory(const std::filesystem::path &assets,
                                       std::string_view language) {
    auto release = assets / "n64" / (language == "ja" ? "jpn" : "usa");
    std::error_code error;
    if (!std::filesystem::exists(release / "segment.bin", error)) {
        return assets / "n64" / "usa";
    }
    return release;
}

} // namespace coastline
