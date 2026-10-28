// Host check of the XDG directories and the BossRally.ini reader of the SDL game.

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>

#include "config.h"
#include "paths.h"

namespace {

int g_failures = 0;

void Check(bool condition, const char *description) {
    if (!condition) {
        std::cerr << "FAIL: " << description << '\n';
        ++g_failures;
    }
}

coastline::EnvironmentReader Environment(std::map<std::string, std::string> values) {
    return [values](const char *name) -> std::optional<std::string> {
        auto found = values.find(name);
        if (found == values.end() || found->second.empty()) {
            return std::nullopt;
        }
        return found->second;
    };
}

} // namespace

int main() {
    auto defaults =
        coastline::ResolveXdgPaths(Environment({{"HOME", "/home/player"}}), "coastline");
    Check(defaults.has_value(), "HOME alone resolves the directories");
    Check(defaults->config == "/home/player/.config/coastline", "default configuration directory");
    Check(defaults->data == "/home/player/.local/share/coastline", "default data directory");
    Check(defaults->state == "/home/player/.local/state/coastline", "default state directory");
    Check(defaults->cache == "/home/player/.cache/coastline", "default cache directory");

    auto overridden =
        coastline::ResolveXdgPaths(Environment({{"HOME", "/home/player"},
                                                {"XDG_CONFIG_HOME", "/etc/xdg-user"},
                                                {"XDG_CACHE_HOME", "relative/cache"}}),
                                   "coastline");
    Check(overridden->config == "/etc/xdg-user/coastline", "XDG_CONFIG_HOME is used");
    Check(overridden->cache == "/home/player/.cache/coastline", "a relative XDG value is ignored");
    Check(!coastline::ResolveXdgPaths(Environment({}), "coastline"), "no base gives nothing");

    auto config = coastline::GameConfig::Parse("; comment\r\n[Video]\r\nDriver=D3D\r\n"
                                               "Width = 1920\r\n\r\n[game]\r\nPlayMusic=0\r\n"
                                               "PlaySFX=yes\r\nbroken\r\nWidth=wide\r\n");
    Check(config.Get("video", "DRIVER") == "D3D", "names compare without case");
    Check(config.GetInteger("Video", "Width", 0) == 1920, "values are trimmed");
    Check(!config.GetBoolean("Game", "PlayMusic", true), "0 is false");
    Check(config.GetBoolean("Game", "PlaySFX", false), "yes is true");
    Check(config.GetInteger("Game", "Width", 7) == 7, "a malformed number gives the fallback");
    Check(!config.Get("Game", "broken"), "a line without = is skipped");
    config.Set("Video", "driver", "SDL");
    Check(config.Get("Video", "Driver") == "SDL", "Set replaces the key of any case");

    return g_failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
