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
    Asia, /*!< The Asian cartridge. */
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
 * The sources of the default pack imported with a conversion version different from the current
 * one. Source directories without version records also require a new import.
 *
 * @param defaults The directory of the default pack.
 * @return The incompatible sources, in the order of ImportSource.
 */
std::vector<ImportSource> StaleImports(const std::filesystem::path &defaults);

/**
 * Check whether import.ini records the disc and at least one cartridge at current conversion
 * versions, the regional layout is current, and every recorded file exists at its imported size.
 * Require a complete inventory with a matching record count and SHA-1 digest.
 * Required converted POD metadata and attract recordings must be available.
 *
 * @param defaults The directory of the default pack.
 * @return Whether the default pack is usable.
 */
bool DefaultPackUsable(const std::filesystem::path &defaults);

/**
 * Atomically record the relative path and size of every generated file after a successful import.
 * A versioned header records the file count and SHA-1 of the complete inventory body.
 *
 * @param defaults The directory of the default pack.
 * @return Whether the inventory was written.
 */
bool RecordImportFiles(const std::filesystem::path &defaults);

/**
 * @param sources The sources to import again, at least one.
 * @return A sentence that requests the sources be imported again, such as
 * "Import the disc and the USA cartridge again."
 */
std::string ReimportMessage(std::span<const ImportSource> sources);

} // namespace coastline
