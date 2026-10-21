#include "rom.h"

#include <algorithm>
#include <array>
#include <format>
#include <string>
#include <utility>

#include "byteorder.h"
#include "fileio.h"
#include "sha1.h"

namespace Tools::ExtractRom {

namespace {

using Common::Error;
using Common::ErrorCode;

constexpr std::uint32_t kBigEndianMagic = 0x80371240;
constexpr std::uint32_t kByteSwappedMagic = 0x37804012;
constexpr std::uint32_t kLittleEndianMagic = 0x40123780;
constexpr std::size_t kHeaderSize = 0x40;
constexpr std::size_t kNameOffset = 0x20;
constexpr std::string_view kName = "TOP GEAR RALLY";
constexpr std::size_t kGameCodeOffset = 0x3b;

struct Release {
    std::string_view gameCode;
    Region region;
    std::uint32_t assetStart;
    std::string_view sha1;
};

// The asset area begins where the loaded image ends. The loaded image ends at the start of the
// BSS segment, mapped from RAM to the cartridge (RAM 0x80200000 is cartridge 0x1000), and the
// first asset of every release, the ghost data, is read from exactly there. The digests are of the
// big-endian images.
constexpr std::array<Release, 3> kReleases{{
    {"NGRE", Region::Usa, 0xad400, "bfc51f086ebdb78904dc24ad0c3a2d946e26d022"},
    {"NTRJ", Region::Jpn, 0xadb30, "e0415b87c5a5b69ac581e79137406bdaa79b354a"},
    {"NTRP", Region::Eur, 0xaf0f0, "9fae5d98fe3464f00d59034690df90118646eb95"},
}};

std::unexpected<Error> invalid(std::string message) {
    return std::unexpected(Error{ErrorCode::InvalidInput, std::move(message)});
}

} // namespace

std::string_view regionName(Region region) {
    switch (region) {
    case Region::Usa:
        return "us";
    case Region::Jpn:
        return "jp";
    default:
        return "eu";
    }
}

std::expected<Rom, Error> Rom::open(const std::filesystem::path &path) {
    auto bytes = Common::readFile(path);
    if (!bytes) {
        return std::unexpected(bytes.error());
    }
    if (bytes->size() < kHeaderSize || bytes->size() % 4 != 0) {
        return invalid(std::format("{} is too short for a cartridge image.", path.string()));
    }
    switch (Common::readBig32(*bytes, 0)) {
    case kBigEndianMagic:
        break;
    case kByteSwappedMagic:
        for (std::size_t i = 0; i < bytes->size(); i += 2) {
            std::swap((*bytes)[i], (*bytes)[i + 1]);
        }
        break;
    case kLittleEndianMagic:
        for (std::size_t i = 0; i < bytes->size(); i += 4) {
            std::swap((*bytes)[i], (*bytes)[i + 3]);
            std::swap((*bytes)[i + 1], (*bytes)[i + 2]);
        }
        break;
    default:
        return invalid(std::format("{} is not a cartridge image.", path.string()));
    }
    const std::string_view name(reinterpret_cast<const char *>(bytes->data() + kNameOffset),
                                kName.size());
    if (name != kName) {
        return invalid(std::format("{} is not a Top Gear Rally cartridge.", path.string()));
    }
    const std::string_view gameCode(reinterpret_cast<const char *>(bytes->data() + kGameCodeOffset),
                                    4);
    const auto release = std::ranges::find(kReleases, gameCode, &Release::gameCode);
    if (release == kReleases.end()) {
        return invalid(std::format("{} has the unknown game code {}.", path.string(), gameCode));
    }
    if (const auto digest = Common::sha1Hex(*bytes); digest != release->sha1) {
        return invalid(std::format("{} has SHA-1 {}, and the {} release has {}.",
                                   path.string(),
                                   digest,
                                   regionName(release->region),
                                   release->sha1));
    }
    return Rom{std::move(*bytes), release->region, release->assetStart};
}

} // namespace Tools::ExtractRom
