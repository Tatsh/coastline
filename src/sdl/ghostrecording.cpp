#include <filesystem>
#include <fstream>

#include <nlohmann/json.hpp>

#include "sdl/gamefiles.h"
#include "sdl/ghostfile.h"

extern "C" FILE *OpenGhostRecording(const char *path) {
    auto json = std::filesystem::path(path).replace_extension(".json");
    if (auto found = coastline::FindGameFile(json.string().c_str())) {
        std::ifstream in(*found);
        auto parsed = nlohmann::ordered_json::parse(in, nullptr, false);
        auto bytes = parsed.is_discarded() ? std::nullopt : coastline::GhostFromJson(parsed);
        if (!bytes) {
            return nullptr;
        }
        // The race reads the recording as the file it was converted from.
        FILE *file = std::tmpfile();
        if (file != nullptr) {
            std::fwrite(bytes->data(), 1, bytes->size(), file);
            std::rewind(file);
        }
        return file;
    }
    return std::fopen(path, "rb");
}
