#include "videoconvert.h"

#include <array>
#include <fstream>
#include <memory>
#include <string_view>
#include <vector>

#include <SDL3/SDL.h>

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef _WIN32
#include <bcrypt.h>
#elif defined(__APPLE__)
#include <CommonCrypto/CommonDigest.h>
#endif

#include "paths.h"

namespace coastline {

#if !defined(_WIN32) && !defined(__APPLE__) && COASTLINE_VIDEO
std::optional<std::string> ConvertIntroMovieWithLibav(const std::filesystem::path &source,
                                                      const std::filesystem::path &output,
                                                      const ImportProgress &progress);
#endif

namespace {

#if defined(_WIN32) || defined(__APPLE__)
bool VerifySha256(const std::filesystem::path &path, std::string_view expected) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }
#ifdef _WIN32
    BCRYPT_ALG_HANDLE algorithm = nullptr;
    if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) < 0) {
        return false;
    }
    const auto closeAlgorithm = [](void *handle) { BCryptCloseAlgorithmProvider(handle, 0); };
    const std::unique_ptr<void, decltype(closeAlgorithm)> provider(algorithm, closeAlgorithm);
    BCRYPT_HASH_HANDLE handle = nullptr;
    if (BCryptCreateHash(algorithm, &handle, nullptr, 0, nullptr, 0, 0) < 0) {
        return false;
    }
    const std::unique_ptr<void, decltype(&BCryptDestroyHash)> hash(handle, BCryptDestroyHash);
#else
    CC_SHA256_CTX hash;
    if (!CC_SHA256_Init(&hash)) {
        return false;
    }
#endif
    std::array<unsigned char, 65536> buffer{};
    while (file.read(reinterpret_cast<char *>(buffer.data()), buffer.size()) || file.gcount() > 0) {
#ifdef _WIN32
        if (BCryptHashData(handle, buffer.data(), static_cast<ULONG>(file.gcount()), 0) < 0) {
            return false;
        }
#else
        if (!CC_SHA256_Update(&hash, buffer.data(), static_cast<CC_LONG>(file.gcount()))) {
            return false;
        }
#endif
    }
    if (!file.eof()) {
        return false;
    }
    std::array<unsigned char, 32> digest{};
#ifdef _WIN32
    if (BCryptFinishHash(handle, digest.data(), static_cast<ULONG>(digest.size()), 0) < 0) {
        return false;
    }
#else
    if (!CC_SHA256_Final(digest.data(), &hash)) {
        return false;
    }
#endif
    constexpr char HEX[] = "0123456789abcdef";
    std::string actual;
    for (const auto byte : digest) {
        actual += HEX[byte >> 4];
        actual += HEX[byte & 15];
    }
    return actual == expected;
}
#endif

#if defined(_WIN32) || defined(__APPLE__)
using Process = std::unique_ptr<SDL_Process, decltype(&SDL_DestroyProcess)>;

bool Run(const std::vector<std::string> &arguments, const ImportProgress &progress) {
    std::vector<const char *> argv;
    for (const auto &argument : arguments) {
        argv.push_back(argument.c_str());
    }
    argv.push_back(nullptr);
    const auto properties = SDL_CreateProperties();
    SDL_SetPointerProperty(properties, SDL_PROP_PROCESS_CREATE_ARGS_POINTER, argv.data());
    SDL_SetNumberProperty(properties, SDL_PROP_PROCESS_CREATE_STDIN_NUMBER, SDL_PROCESS_STDIO_NULL);
    SDL_SetNumberProperty(
        properties, SDL_PROP_PROCESS_CREATE_STDOUT_NUMBER, SDL_PROCESS_STDIO_NULL);
    SDL_SetNumberProperty(
        properties, SDL_PROP_PROCESS_CREATE_STDERR_NUMBER, SDL_PROCESS_STDIO_NULL);
    Process process(SDL_CreateProcessWithProperties(properties), SDL_DestroyProcess);
    SDL_DestroyProperties(properties);
    if (!process) {
        return false;
    }
    int result = -1;
    const auto start = SDL_GetTicks();
    while (!SDL_WaitProcess(process.get(), false, &result)) {
        if (SDL_GetTicks() - start > 600000 ||
            (progress && !progress("Converting the intro movie...", 0.5))) {
            SDL_KillProcess(process.get(), true);
            SDL_WaitProcess(process.get(), true, &result);
            return false;
        }
        SDL_Delay(50);
    }
    return result == 0;
}

struct TemporaryTools {
    std::filesystem::path path;
    ~TemporaryTools() {
        std::error_code error;
        std::filesystem::remove_all(path, error);
    }
};
#endif

std::optional<std::string> ConvertLegalImage(const std::filesystem::path &directory) {
    std::ifstream file(directory / "splash.img", std::ios::binary);
    if (!file) {
        return std::nullopt;
    }
    std::array<unsigned char, 12> header{};
    file.read(reinterpret_cast<char *>(header.data()), header.size());
    auto word = [&](int offset) {
        return static_cast<std::uint32_t>(header[offset]) |
               static_cast<std::uint32_t>(header[offset + 1]) << 8 |
               static_cast<std::uint32_t>(header[offset + 2]) << 16 |
               static_cast<std::uint32_t>(header[offset + 3]) << 24;
    };
    const auto width = word(0);
    const auto height = word(4);
    if (!file || width == 0 || height == 0 || width > 4096 || height > 4096 || word(8) != 0x1555) {
        return "Invalid Boss Rally legal image.";
    }
    std::vector<unsigned char> pixels(static_cast<std::size_t>(width) * height * 4);
    for (std::size_t i = 0; i < pixels.size(); i += 4) {
        const int low = file.get();
        const int high = file.get();
        if (low < 0 || high < 0) {
            return "Truncated Boss Rally legal image.";
        }
        const auto color = static_cast<unsigned>(low | high << 8);
        auto channel = [&](int shift) {
            const auto value = (color >> shift) & 31;
            return static_cast<unsigned char>((value << 3) | (value >> 2));
        };
        pixels[i] = channel(10);
        pixels[i + 1] = channel(5);
        pixels[i + 2] = channel(0);
        pixels[i + 3] = 255;
    }
    using Surface = std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)>;
    Surface image(
        SDL_CreateSurfaceFrom(width, height, SDL_PIXELFORMAT_RGBA32, pixels.data(), width * 4),
        SDL_DestroySurface);
    if (!image || !SDL_SavePNG(image.get(), Utf8FromPath(directory / "intro/legal.png").c_str())) {
        return "Cannot convert Boss Rally legal image.";
    }
    return std::nullopt;
}

} // namespace

std::optional<std::string> ConvertIntroMedia(const std::filesystem::path &directory,
                                             const ImportProgress &progress) {
    std::error_code error;
    std::filesystem::create_directories(directory / "intro", error);
    if (error) {
        return "Cannot create intro media directory.";
    }
    if (auto failure = ConvertLegalImage(directory)) {
        return failure;
    }
    const auto source = directory / "brally.avi";
    if (!std::filesystem::is_regular_file(source, error)) {
        return std::nullopt;
    }
    const auto output = directory / "intro/brally.mp4";
#if !defined(_WIN32) && !defined(__APPLE__)
#if COASTLINE_VIDEO
    if (auto failure = ConvertIntroMovieWithLibav(source, output, progress)) {
        std::filesystem::remove(output, error);
        return failure;
    }
    if (!std::filesystem::is_regular_file(output, error) ||
        std::filesystem::file_size(output, error) == 0 || error) {
        return "The converted intro movie is empty.";
    }
    std::filesystem::remove(source, error);
    std::filesystem::remove(directory / "splash.img", error);
    return std::nullopt;
#else
    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                "libav is unavailable; skipping optional intro movie conversion.");
    std::filesystem::remove(source, error);
    return std::nullopt;
#endif
#else
    TemporaryTools tools;
    std::string executable = "ffmpeg";
    if (progress && !progress("Converting the intro movie...", 0)) {
        return "Import cancelled.";
    }
    if (!Run({executable, "-version"}, {})) {
#if defined(_WIN32) || defined(__APPLE__)
        const ImportProgress toolProgress = [&](const std::string &, double) {
            return !progress || progress("Preparing temporary FFmpeg...", 0.1);
        };
        tools.path = directory / (".intro-tools-" + std::to_string(SDL_GetTicksNS()));
        if (!std::filesystem::create_directory(tools.path, error) || error) {
            return "Cannot create temporary FFmpeg directory.";
        }
        const auto archive = Utf8FromPath(tools.path / "ffmpeg.zip");
#ifdef _WIN32
        const char *url = "https://github.com/GyanD/codexffmpeg/releases/download/9.0.2/"
                          "ffmpeg-9.0.2-essentials_build.zip";
        const char *checksum = "60f467265b1e312373dbcd92200c2618a74850f98d3d078e94296bb3fa2047ba";
#elif defined(__aarch64__) || defined(__arm64__)
        const char *url = "https://www.osxexperts.net/ffmpeg902arm.zip";
        const char *checksum = "07e00300593799005b060c03a99fe648b1edda0f30c36b0c1adafe8e3247e586";
#else
        const char *url = "https://evermeet.cx/ffmpeg/ffmpeg-9.0.2.zip";
        const char *checksum = "4acc0be580f9b2788029eb7bd4d645ff87968911b0a62aeeb3940d42d54558d5";
#endif
        if (!Run({"curl",
                  "--fail",
                  "--location",
                  "--proto",
                  "=https",
                  "--proto-redir",
                  "=https",
                  "--connect-timeout",
                  "30",
                  "--max-time",
                  "300",
                  "--output",
                  archive,
                  url},
                 toolProgress)) {
            return "Cannot download temporary FFmpeg. Install FFmpeg in PATH and retry.";
        }
        if (!VerifySha256(tools.path / "ffmpeg.zip", checksum)) {
            return "Temporary FFmpeg checksum does not match. Install FFmpeg in PATH and retry.";
        }
#ifdef _WIN32
        const std::vector<std::string> unpack{
            "tar", "-xf", archive, "-C", Utf8FromPath(tools.path)};
#else
        const std::vector<std::string> unpack{
            "/usr/bin/ditto", "-x", "-k", archive, Utf8FromPath(tools.path)};
#endif
        if (!Run(unpack, toolProgress)) {
            return "Cannot unpack temporary FFmpeg.";
        }
        const std::filesystem::recursive_directory_iterator end;
        for (std::filesystem::recursive_directory_iterator item(tools.path, error);
             !error && item != end;
             item.increment(error)) {
            if (item->path().filename() == "ffmpeg" || item->path().filename() == "ffmpeg.exe") {
                executable = Utf8FromPath(item->path());
                std::filesystem::permissions(item->path(),
                                             std::filesystem::perms::owner_exec,
                                             std::filesystem::perm_options::add,
                                             error);
                break;
            }
        }
        if (executable == "ffmpeg") {
            return "Temporary FFmpeg executable is missing.";
        }
#endif
    }
    if (progress && !progress("Converting the intro movie...", 0.25)) {
        return "Import cancelled.";
    }
    if (!Run({executable,
              "-nostdin",
              "-y",
              "-v",
              "error",
              "-i",
              Utf8FromPath(source),
              "-map",
              "0:v:0",
              "-map",
              "0:a:0?",
              "-c:v",
              "libx264",
              "-crf",
              "18",
              "-threads",
              "2",
              "-pix_fmt",
              "yuv420p",
              "-c:a",
              "aac",
              "-b:a",
              "192k",
              "-ar",
              "48000",
              "-ac",
              "2",
              Utf8FromPath(output)},
             progress)) {
        return "Cannot convert intro movie. Check FFmpeg H.264 support or import cancellation.";
    }
    if (!std::filesystem::is_regular_file(output, error) ||
        std::filesystem::file_size(output, error) == 0 || error) {
        return "The converted intro movie is empty.";
    }
    std::filesystem::remove(source, error);
    std::filesystem::remove(directory / "splash.img", error);
    return std::nullopt;
#endif
}

} // namespace coastline
