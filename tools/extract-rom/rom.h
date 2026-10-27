#pragma once

#include <cstdint>
#include <expected>
#include <filesystem>
#include <span>
#include <string_view>
#include <vector>

#include "error.h"

/** Extraction of the assets of a Top Gear Rally cartridge image. */
namespace Tools::ExtractRom {

/** Release a cartridge image belongs to. */
enum class Region {
    Usa, /*!< North America, game code NGRE. */
    Jpn, /*!< Japan, game code NTRJ. */
    Eur, /*!< Europe, game code NTRP. */
};

/**
 * Short name of a region, the value TGR_REGION takes in CMake.
 *
 * @param region The region.
 * @return `us`, `jp`, or `eu`.
 */
std::string_view regionName(Region region);

/** Cartridge image in big-endian byte order. */
class Rom {
private:
    std::vector<std::uint8_t> bytes; /*!< The image, `.z64` order whatever the input order was. */
    Region region;                   /*!< Release the image belongs to. */
    std::uint32_t assetStart;        /*!< Offset of the first asset. */

    Rom(std::vector<std::uint8_t> bytes, Region region, std::uint32_t assetStart);

public:
    [[nodiscard]] std::span<const std::uint8_t> getBytes() const {
        return bytes;
    }
    [[nodiscard]] Region getRegion() const {
        return region;
    }
    [[nodiscard]] std::uint32_t getAssetStart() const {
        return assetStart;
    }

    /**
     * Read a cartridge image and identify its release.
     *
     * The image may be in `.z64`, `.v64`, or `.n64` byte order. The release is read from the game
     * code of the header, and the SHA-1 of the big-endian image must be the release's. The assets
     * follow the loaded image of the program, whose length is that of the release's code and data.
     *
     * @param path File to read.
     * @return The image, or an error when the file is not an unmodified Top Gear Rally cartridge.
     */
    static std::expected<Rom, Common::Error> open(const std::filesystem::path &path);
};

} // namespace Tools::ExtractRom
