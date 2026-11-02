#include <array>
#include <bit>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>
#include <tuple>

#include "config.h"

namespace {
void Write(const std::filesystem::path &file, const std::string &text) {
    std::ofstream output(file, std::ios::binary);
    output << text;
    assert(output.good());
}
} // namespace

int main() {
    const auto root = std::filesystem::temp_directory_path() / "coastline-config-safety-test";
    std::filesystem::create_directories(root);
    const auto file = root / "settings.ini";
    for (const auto &[section, key, value] :
         {std::tuple{"Video", "Width", "2147483648"},
          {"Video", "Width", "-1"},
          {"Video", "Width", "0"},
          {"Video", "Height", "16385"},
          {"Video", "Width", "9223372036854775808"},
          {"Video", "RefreshRate", "2147483648"},
          {"Video", "FrameRateLimit", "-1"},
          {"Video", "InternalWidth", "16385"},
          {"Video", "InternalHeight", "-1"},
          {"Video", "RenderScale", "nan"},
          {"Video", "RenderScale", "inf"},
          {"Video", "RenderScale", "3.5e38"},
          {"Video", "RenderScale", "0"},
          {"Video", "FieldOfView", "1e30"},
          {"Video", "FieldOfView", "-1"},
          {"Video", "DrawDistance", "-inf"},
          {"Video", "DrawDistance", "0.5"},
          {"Video", "DrawDistance", "4.01"},
          {"Video", "Antialias", "3"},
          {"Video", "AnisotropicFiltering", "99999999999999999999999"},
          {"Video", "AspectRatio", "nan"},
          {"Video", "AspectRatio", "4:0"},
          {"Game", "XmMusicGain", "8.1"},
          {"Game", "XmMusicGain", "nan"},
          {"Game", "XmMusicGain", "-1"}}) {
        Write(file,
              "[Paths]\nDisc=retained\n[" + std::string(section) + "]\n" + key + "=" + value +
                  "\n");
        const auto config = coastline::GameConfig::Load(file);
        assert(!config.Get(section, key));
        assert(config.Get("Paths", "Disc") == "retained");
        assert(config.Save(file));
        assert(!coastline::GameConfig::Load(file).Get(section, key));
    }
    for (const auto &[width, height] : {std::pair{"320", "0"},
                                        {"0", "240"},
                                        {"319", "240"},
                                        {"320", "239"},
                                        {"2147483648", "240"},
                                        {"320", "99999999999999999999999999"}}) {
        Write(file,
              "[Video]\nInternalWidth=" + std::string(width) + "\nInternalHeight=" + height + "\n");
        const auto config = coastline::GameConfig::Load(file);
        assert(config.GetInteger("Video", "InternalWidth", 0) == 0);
        assert(config.GetInteger("Video", "InternalHeight", 0) == 0);
    }
    Write(file,
          "[Video]\nWidth=16384\nHeight=1\nInternalWidth=640\nInternalHeight=360\n"
          "RenderScale=4\nFieldOfView=2\nDrawDistance=4\nAntialias=8\n"
          "AnisotropicFiltering=16\nAspectRatio=32:9\n[Game]\nXmMusicGain=0\n");
    auto config = coastline::GameConfig::Load(file);
    assert(config.GetInteger("Video", "Width", 0) == 16384);
    assert(config.GetInteger("Video", "Height", 0) == 1);
    assert(config.GetInteger("Video", "InternalWidth", 0) == 640);
    assert(config.GetInteger("Video", "InternalHeight", 0) == 360);
    assert(config.GetFloat("Video", "RenderScale", 0) == 4);
    assert(config.GetFloat("Video", "FieldOfView", 0) == 2);
    assert(config.GetFloat("Game", "XmMusicGain", 1) == 0);
    for (const auto *text : {"nan", "NaN", "inf", "-inf", "1e1000", "-1e1000"}) {
        config.Set("Numeric", "Value", text);
        assert(config.GetFloat("Numeric", "Value", 7) == 7);
    }
    using coastline::ScaleFrameResolution;
    assert((ScaleFrameResolution(1920, 1080, 1, 0) == std::array{1920, 1080}));
    assert((ScaleFrameResolution(1920, 1080, 1, 4.0f / 3.0f) == std::array{1440, 1080}));
    assert((ScaleFrameResolution(640, 480, 1.5f, 16.0f / 9.0f) == std::array{960, 540}));
    assert((ScaleFrameResolution(0, -1, 1, 0) == std::array{1, 1}));
    assert((ScaleFrameResolution(
                std::numeric_limits<int>::max(), std::numeric_limits<int>::max(), 4, 0) ==
            std::array{16384, 16384}));
    const auto nan = std::numeric_limits<float>::quiet_NaN();
    assert((ScaleFrameResolution(640, 480, nan, nan) == std::array{640, 480}));
    std::uint32_t random = 0x12345678;
    for (unsigned index = 0; index < 100000; ++index) {
        random = random * 1664525 + 1013904223;
        const auto scale = std::bit_cast<float>(random);
        const auto aspect = std::bit_cast<float>(random ^ 0x9e3779b9);
        for (const auto dimension :
             ScaleFrameResolution(std::bit_cast<std::int32_t>(random),
                                  std::bit_cast<std::int32_t>(random ^ 0xdeadbeef),
                                  scale,
                                  aspect)) {
            assert(dimension >= 1 && dimension <= 16384);
        }
    }
    std::filesystem::remove_all(root);
}
