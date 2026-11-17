#include "decodeddisc.h"

#include <atomic>
#include <chrono>

#include <fmt/format.h>

namespace coastline {
namespace {
class TemporaryDisc {
public:
    explicit TemporaryDisc(std::filesystem::path path) : m_directory(std::move(path)) {
    }

    ~TemporaryDisc() {
        std::error_code error;
        std::filesystem::remove_all(m_directory, error);
    }

    TemporaryDisc(const TemporaryDisc &) = delete;
    TemporaryDisc &operator=(const TemporaryDisc &) = delete;

    const std::filesystem::path &Directory() const {
        return m_directory;
    }

private:
    std::filesystem::path m_directory;
};
} // namespace

std::expected<DecodedDisc, std::string> CreateDecodedDisc(bool raw, std::uint64_t bytes) {
    std::error_code error;
    const auto parent = std::filesystem::temp_directory_path(error);
    const auto space =
        error ? std::filesystem::space_info{} : std::filesystem::space(parent, error);
    if (error || space.available < bytes) {
        return std::unexpected(std::string("Insufficient temporary storage for decoded disc."));
    }
    static std::atomic<std::uint64_t> sequence{0};
    std::shared_ptr<TemporaryDisc> storage;
    for (unsigned attempt = 0; attempt < 128; ++attempt) {
        const auto ticks = std::chrono::steady_clock::now().time_since_epoch().count();
        const auto directory = parent / fmt::format("coastline-disc-{}-{}", ticks, sequence++);
        if (std::filesystem::create_directory(directory, error)) {
            storage = std::make_shared<TemporaryDisc>(directory);
            std::filesystem::permissions(directory, std::filesystem::perms::owner_all, error);
            break;
        }
        if (error) {
            break;
        }
    }
    if (!storage || error) {
        return std::unexpected(std::string("Cannot create temporary disc storage."));
    }
    return DecodedDisc{storage->Directory() / (raw ? "disc.bin" : "disc.iso"), storage, {}};
}

} // namespace coastline
