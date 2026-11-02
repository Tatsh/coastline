#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include <SDL3/SDL.h>

#include "log.h"

namespace {

void SDLCALL Capture(void *data, int, SDL_LogPriority, const char *message) {
    static_cast<std::vector<std::string> *>(data)->emplace_back(message);
}

} // namespace

int main() {
    using coastline::AbbreviateLogPaths;
    assert(AbbreviateLogPaths("Reading /home/player/assets/file.", "/home/player/") ==
           "Reading ~/assets/file.");
    assert(AbbreviateLogPaths("From '/home/player/a' to (/home/player/b).", "/home/player") ==
           "From '~/a' to (~/b).");
    assert(AbbreviateLogPaths("Using /home/player.", "/home/player") == "Using ~.");
    for (const auto text : {"/home/player2/a",
                            "/home/player-backup/a",
                            "/home/player.txt",
                            "/tmp/home/player/a",
                            "https://example.com/home/player/a"}) {
        assert(AbbreviateLogPaths(text, "/home/player") == text);
    }
    assert(AbbreviateLogPaths("/home/player/a", "") == "/home/player/a");
    assert(AbbreviateLogPaths("/tmp/a", "/") == "/tmp/a");
    assert(AbbreviateLogPaths("C:\\Users\\Player\\assets\\usa", "C:\\Users\\Player\\") ==
           "~\\assets\\usa");
    assert(AbbreviateLogPaths("c:/users/player/assets/jpn", "C:\\Users\\Player") == "~/assets/jpn");
    assert(AbbreviateLogPaths("C:\\Users\\Player2\\assets", "C:\\Users\\Player") ==
           "C:\\Users\\Player2\\assets");
    assert(AbbreviateLogPaths("\\\\Server\\Users\\Player\\assets", "\\\\server\\users\\player") ==
           "~\\assets");

    SDL_LogOutputFunction original = nullptr;
    void *originalData = nullptr;
    SDL_GetLogOutputFunction(&original, &originalData);
    std::vector<std::string> captured;
    SDL_SetLogOutputFunction(Capture, &captured);
    const auto directory =
        std::filesystem::temp_directory_path() /
        ("coastline-log-" +
         std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(directory);
    const auto file = directory / "game.log";
    const char *home = SDL_GetUserFolder(SDL_FOLDER_HOME);
    assert(home && *home);
    const auto path = std::filesystem::path(home) / "assets" / "jpn" / "file";
    const auto message = "Reading " + path.string() + ".";
    const auto expected = AbbreviateLogPaths(message, home);
    {
        coastline::GameLog log(file);
        assert(log.IsOpen());
        SDL_Log("%s", message.c_str());
        assert(!captured.empty() && captured.back() == expected);
        coastline::StopGameLog();
        SDL_Log("After log shutdown.");
        assert(captured.back() == "After log shutdown.");
    }
    std::ifstream input(file);
    const std::string contents(std::istreambuf_iterator<char>(input), {});
    assert(contents.find(expected) != std::string::npos);
    assert(contents.find(message) == std::string::npos);
    assert(contents.find("After log shutdown.") == std::string::npos);
    SDL_SetLogOutputFunction(original, originalData);
    input.close();
    std::filesystem::remove_all(directory);
}
