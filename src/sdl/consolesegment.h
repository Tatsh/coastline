#pragma once

/**
 * @file
 * The main segment of a release of the Nintendo 64 in the import, read by the addresses of the
 * console, and the names the import gives the assets of its cartridge.
 */

#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "common/types.h"

namespace coastline {

/** The main segment of a release, as the console loads it. */
class ConsoleSegment {
public:
    /**
     * Read the main segment of a release and the manifest of its cartridge.
     *
     * @param release The directory of the import of the release, such as n64/usa.
     * @return The segment, or nothing with the reason logged.
     */
    static std::optional<ConsoleSegment> Load(const std::filesystem::path &release);

    /**
     * @param address An address of the console.
     * @param size The bytes from the address.
     * @return Whether the bytes are inside the segment.
     */
    bool Has(u32 address, u32 size) const;

    /**
     * @param address An address of the console.
     * @return The byte there, or 0 outside the segment.
     */
    u8 Byte(u32 address) const;

    /**
     * @param address An address of the console.
     * @return The big-endian word there.
     */
    u32 Word(u32 address) const;

    /**
     * @param address An address of the console.
     * @return The big-endian word there as a signed number.
     */
    s32 Signed(u32 address) const;

    /**
     * @param address An address of the console.
     * @return The big-endian float there.
     */
    f32 Float(u32 address) const;

    /**
     * @param address An address of the console.
     * @return The string that ends at the first zero from the address.
     */
    std::string Text(u32 address) const;

    /**
     * @param romStart Where an asset starts on the cartridge.
     * @return The name of the asset in the import, such as data/g_texLogo, or nothing.
     */
    std::optional<std::string> AssetName(u32 romStart) const;

private:
    std::vector<u8> m_bytes;
    std::map<u32, std::string> m_assets;
};

} // namespace coastline
