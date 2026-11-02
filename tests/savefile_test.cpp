#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <csignal>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

#include "savefile.h"

namespace {

std::string Read(const std::filesystem::path &path) {
    const auto bytes = coastline::ReadSaveFile(path, 1024 * 1024);
    assert(bytes);
    return {bytes->begin(), bytes->end()};
}

pid_t Spawn(const char *executable, const std::filesystem::path &path, const char *mode) {
    const auto child = fork();
    assert(child >= 0);
    if (child == 0) {
        execl(executable, executable, mode, path.c_str(), nullptr);
        _exit(127);
    }
    return child;
}

void Await(const std::filesystem::path &path) {
    for (int attempt = 0; attempt < 1000 && !std::filesystem::exists(path); ++attempt) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    assert(std::filesystem::exists(path));
}

void CheckVerified(const std::filesystem::path &root) {
    using namespace coastline;
    const auto path = root / "verified.dat";
    const std::vector<std::uint8_t> payload{0, 1, 2, 3, 255};
    assert(WriteVerifiedSaveFile(path, payload));
    assert(ReadVerifiedSaveFile(path, payload.size()) == payload);
    const auto original = ReadSaveFile(path, 1024);
    assert(original && original->size() > payload.size());
    for (std::size_t index = 0; index < original->size(); ++index) {
        auto damaged = *original;
        damaged[index] ^= 1;
        assert(WriteSaveFile(path, damaged));
        assert(!ReadVerifiedSaveFile(path, payload.size()));
        assert(!std::filesystem::exists(path));
    }
    for (std::size_t size = 0; size < original->size(); ++size) {
        assert(WriteSaveFile(path, std::span<const std::uint8_t>(*original).first(size)));
        assert(!ReadVerifiedSaveFile(path, payload.size()));
        assert(!std::filesystem::exists(path));
    }
    assert(WriteVerifiedSaveFile(path, payload));
    assert(!ReadVerifiedSaveFile(path, payload.size() - 1));
    assert(!std::filesystem::exists(path));
    assert(WriteVerifiedSaveFile(path, {}));
    const auto empty = ReadVerifiedSaveFile(path, 0);
    assert(empty && empty->empty());
    assert(!ReadVerifiedSaveFile(path, std::numeric_limits<std::size_t>::max()));
    assert(std::filesystem::exists(path));
    assert(WriteVerifiedSaveFile(path, payload));
    assert(!ReadVerifiedSaveFile(
        path, payload.size(), true, [](std::span<const std::uint8_t>) { return false; }));
    assert(!std::filesystem::exists(path));
    assert(WriteSaveFile(path, "unrelated file"));
    assert(!ReadVerifiedSaveFile(path, 1024, false));
    assert(Read(path) == "unrelated file");
}

} // namespace

int main(int argc, char **argv) {
    using namespace coastline;
    if (argc == 3) {
        const std::filesystem::path path(argv[2]);
        if (std::string_view(argv[1]) == "write" || std::string_view(argv[1]) == "verified-write") {
            for (int iteration = 0; iteration < 30; ++iteration) {
                const std::string text(256 * 1024, iteration % 2 ? 'a' : 'b');
                if (std::string_view(argv[1]) == "verified-write") {
                    assert(WriteVerifiedSaveFile(
                        path,
                        std::span(reinterpret_cast<const std::uint8_t *>(text.data()),
                                  text.size())));
                } else {
                    assert(WriteSaveFile(path, text));
                }
            }
            return 0;
        }
        auto save = SaveFile::Open(path);
        assert(save);
        assert(WriteSaveFile(path.parent_path() / "locked", "ready"));
        for (;;) {
            pause();
        }
    }
    const auto root = std::filesystem::temp_directory_path() / "coastline-savefile-test";
    std::filesystem::remove_all(root);
    const auto path = root / "save.dat";
    CheckVerified(root);
    assert(WriteSaveFile(path, "first"));
    assert(Read(path) == "first");
    assert(!ReadSaveFile(path, 4));
    assert(WriteSaveFile(path, "second"));
    assert(Read(path) == "second");
    const auto alias = root / "alias";
    std::filesystem::create_symlink(path, alias);
    assert(WriteSaveFile(alias, "third"));
    assert(std::filesystem::is_symlink(alias) && Read(path) == "third");
    const auto directory = root / "directory";
    std::filesystem::create_directory(directory);
    assert(!WriteSaveFile(directory, "not a file"));
    assert(std::filesystem::is_directory(directory));

    assert(WriteSaveFile(path, std::string(256 * 1024, 'a')));
    const auto writer = Spawn(argv[0], path, "write");
    int status = 0;
    while (waitpid(writer, &status, WNOHANG) == 0) {
        const auto text = Read(path);
        assert(text == std::string(256 * 1024, 'a') || text == std::string(256 * 1024, 'b'));
    }
    assert(WIFEXITED(status) && WEXITSTATUS(status) == 0);

    const std::vector<std::uint8_t> first(256 * 1024, 'a');
    const std::vector<std::uint8_t> second(256 * 1024, 'b');
    assert(WriteVerifiedSaveFile(path, first));
    const auto verifiedWriter = Spawn(argv[0], path, "verified-write");
    while (waitpid(verifiedWriter, &status, WNOHANG) == 0) {
        const auto bytes = ReadVerifiedSaveFile(path, first.size());
        assert(bytes && (*bytes == first || *bytes == second));
    }
    assert(WIFEXITED(status) && WEXITSTATUS(status) == 0);

    const auto holder = Spawn(argv[0], path, "hold");
    Await(root / "locked");
    std::atomic<bool> acquired = false;
    std::thread waiting([&] {
        auto save = SaveFile::Open(path);
        assert(save);
        acquired = true;
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    assert(!acquired);
    assert(kill(holder, SIGKILL) == 0);
    assert(waitpid(holder, &status, 0) == holder && WIFSIGNALED(status));
    waiting.join();
    assert(acquired);
    for (const auto &entry : std::filesystem::directory_iterator(root)) {
        assert(!entry.path().filename().string().starts_with(".coastline-save-"));
    }
    std::filesystem::remove_all(root);
}
