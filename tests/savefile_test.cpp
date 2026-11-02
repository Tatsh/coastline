#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <csignal>
#include <filesystem>
#include <fstream>
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

} // namespace

int main(int argc, char **argv) {
    using namespace coastline;
    if (argc == 3) {
        const std::filesystem::path path(argv[2]);
        if (std::string_view(argv[1]) == "write") {
            for (int iteration = 0; iteration < 30; ++iteration) {
                assert(WriteSaveFile(path, std::string(256 * 1024, iteration % 2 ? 'a' : 'b')));
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
