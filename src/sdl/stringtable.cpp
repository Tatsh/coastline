// LoadStringTableFromDll() of the SDL game, which reads the strings of BRString.dll that the import
// writes as JSON.

#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/fileio.h"
#include "common/strings.h"
#include "sdl/stringresources.h"
#include "sdl/win32types.h"

namespace {

constexpr const char *STRING_FILE = "strings/default/pc.json";

} // namespace

void LoadStringTableFromDll(void) {
    std::memset(&g_apszStringResourceTable[1], 0, STRING_RESOURCE_COUNT * sizeof(char *));
    if (g_pStringTableBuffer != nullptr) {
        return;
    }
    GameFile *file = CHK_FReadOpen(STRING_FILE);
    std::string text(static_cast<std::size_t>(GetFileLength(file)), '\0');
    CHK_FRead(text.data(), 1, text.size(), file);
    CHK_FClose(file);
    auto strings = nlohmann::json::parse(text, nullptr, false);
    if (!strings.is_object()) {
        return;
    }
    // The game frees the buffer of the strings with free().
    std::vector<std::pair<unsigned, std::string>> converted;
    std::size_t size = 0;
    for (const auto &[key, value] : strings.items()) {
        unsigned id = static_cast<unsigned>(std::strtoul(key.c_str(), nullptr, 10));
        if (id >= 1 && id <= STRING_RESOURCE_COUNT && value.is_string()) {
            converted.emplace_back(id, coastline::ToWindows1252(value.get<std::string>()));
            size += converted.back().second.size() + 1;
        }
    }
    g_pStringTableBuffer = static_cast<char *>(std::malloc(size));
    if (g_pStringTableBuffer == nullptr) {
        return;
    }
    g_dwStringTableBufferSize = size;
    for (const auto &[id, value] : converted) {
        char *entry = g_pStringTableBuffer + g_dwStringTableUsedBytes;
        std::memcpy(entry, value.c_str(), value.size() + 1);
        g_apszStringResourceTable[id] = entry;
        g_dwStringTableUsedBytes += value.size() + 1;
    }
}
