#include <cassert>
#include <filesystem>
#include <string>

#include "config.h"
#include "configsaver.h"

using coastline::ConfigSaver;
using coastline::GameConfig;

int main() {
    const auto directory = std::filesystem::temp_directory_path() / "coastline-configsaver-test";
    std::filesystem::remove_all(directory);
    std::filesystem::create_directories(directory);
    const auto path = directory / "BossRally.ini";
    {
        ConfigSaver saver;
        assert(saver.Flush());
        GameConfig config;
        for (int value = 0; value < 50; ++value) {
            config.Set("Game", "Value", std::to_string(value));
            saver.Save(config, path);
        }
        assert(saver.Flush());
        assert(!saver.Failed());
        assert(GameConfig::Load(path).Get("Game", "Value") == "49");
        // A file cannot replace the directory.
        saver.Save(config, directory);
        assert(!saver.Flush());
        assert(saver.Failed());
        saver.Save(config, path);
        assert(saver.Flush());
        assert(!saver.Failed());
        config.Set("Game", "Value", "last");
        saver.Save(config, path);
    }
    // The destructor writes the copy that waits.
    assert(GameConfig::Load(path).Get("Game", "Value") == "last");
    std::filesystem::remove_all(directory);
    return 0;
}
