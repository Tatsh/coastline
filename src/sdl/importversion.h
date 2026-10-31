#pragma once

#include <filesystem>
#include <span>
#include <string>
#include <vector>

namespace coastline {

/**
 * The version of the conversion of the Boss Rally disc: the cars, the tracks, the images, and the
 * other files ImportDisc() converts, n64model.cpp included. Increase it with every change to the
 * output of the conversion, so that the game warns about data imported before the change.
 */
inline constexpr int DISC_CONVERTER_VERSION = 2;

/**
 * The version of the conversion of a Top Gear Rally cartridge: the models, the images, the music,
 * and the other files ImportRom() converts, n64model.cpp included. Increase it with every change
 * to the output of the conversion, so that the game warns about data imported before the change.
 */
inline constexpr int CARTRIDGE_CONVERTER_VERSION = 1;

/** A source of the data of the default pack. */
enum class ImportSource {
    Disc, /*!< The Boss Rally disc. */
    Usa,  /*!< The USA cartridge. */
    Jpn,  /*!< The Japanese cartridge. */
    Eur,  /*!< The European cartridge. */
};

/**
 * Record in import.ini of the default pack that @p source was imported with the current version
 * of its conversion.
 *
 * @param defaults The directory of the default pack.
 * @param source The source imported.
 * @return Whether the file was written.
 */
bool RecordImport(const std::filesystem::path &defaults, ImportSource source);

/**
 * The sources of the default pack imported with an older version of their conversion than the
 * current one. A source whose files are in the pack (cars for the disc, n64/usa, n64/jpn, or
 * n64/eur for a cartridge) and that import.ini does not list counts as old.
 *
 * @param defaults The directory of the default pack.
 * @return The old sources, in the order of ImportSource.
 */
std::vector<ImportSource> StaleImports(const std::filesystem::path &defaults);

/**
 * @param sources The sources to import again, at least one.
 * @return A sentence that requests the sources be imported again, such as
 * "Import the disc and the USA cartridge again."
 */
std::string ReimportMessage(std::span<const ImportSource> sources);

} // namespace coastline
