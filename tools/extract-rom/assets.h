#pragma once

#include <cstdint>
#include <expected>
#include <filesystem>
#include <span>
#include <vector>

#include "error.h"
#include "rom.h"

namespace Tools::ExtractRom {

/**
 * One asset of the cartridge. The game addresses every asset by its cartridge offset, and the
 * assets cover the whole area after the loaded image.
 *
 * A compressed asset is self-describing. An 8-byte header gives the total length of the asset
 * and the length of its decompressed data, and zlib chunks follow, each prefixed with its
 * length and padded to an even length. The game inflates each chunk into a 16,000-byte buffer.
 *
 * A raw asset is a stretch between compressed assets. The track geometry addresses its streamed
 * textures inside such a stretch, and the 32-byte track section headers end a stretch.
 */
struct Asset {
    std::uint32_t offset;           /*!< Cartridge offset. */
    std::uint32_t size;             /*!< Length in the cartridge, padding included. */
    bool compressed;                /*!< Whether the asset is a chain of zlib chunks. */
    std::uint32_t decompressedSize; /*!< Length of the decompressed data, 0 for a raw asset. */
    std::uint32_t chunks;           /*!< Number of zlib chunks, 0 for a raw asset. */
};

/**
 * Inflate a compressed asset.
 *
 * @param blob The asset, header included.
 * @return The decompressed data, or an error when the chain is not well formed.
 */
std::expected<std::vector<std::uint8_t>, Common::Error>
decompress(std::span<const std::uint8_t> blob);

/**
 * Find every asset of the cartridge.
 *
 * Every even offset after the loaded image is tried as the start of a compressed asset, and a
 * candidate is accepted only when its whole chain inflates to the length its header gives. The
 * stretches between accepted assets are the raw assets.
 *
 * @param rom The cartridge.
 * @return The assets in cartridge order, covering the whole asset area.
 */
std::vector<Asset> scan(const Rom &rom);

/**
 * Write every asset to a directory, with a manifest.
 *
 * A raw asset is written as `OFFSET.bin`. A compressed asset is written as `OFFSET.z`, the bytes
 * of the cartridge, and `OFFSET.bin`, the decompressed data. `manifest.txt` lists every asset as
 * `OFFSET KIND SIZE DECOMPRESSED CHUNKS`, in cartridge order. `boot.bin` is the first 0x1000 bytes
 * of the cartridge, the header and the IPL3, which the ROM build copies as they are.
 *
 * @param rom The cartridge.
 * @param assets The assets scan() found.
 * @param directory Directory to write. It is created when missing.
 * @return Nothing, or the first error.
 */
std::expected<void, Common::Error>
extract(const Rom &rom, const std::vector<Asset> &assets, const std::filesystem::path &directory);

} // namespace Tools::ExtractRom
