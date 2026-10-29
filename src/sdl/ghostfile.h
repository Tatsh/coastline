#pragma once

#include <cstdint>
#include <cstdio>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include <nlohmann/json_fwd.hpp>

namespace coastline {

/**
 * Describe a ghost recording of the PC release (RGho) as JSON: the recording, the settings, the
 * name of its slot, and each frame with every bit in a named field. The checksum and the sizes are
 * left out, as the file computes them from the rest.
 *
 * @param bytes The file.
 * @return The JSON, or nothing for a file that is not a recording with a valid checksum.
 */
std::optional<nlohmann::ordered_json> GhostToJson(std::span<const std::uint8_t> bytes);

/**
 * Build the file of a ghost recording from its JSON, the inverse of GhostToJson().
 *
 * @param json The JSON.
 * @return The file, or nothing for JSON that does not describe a recording.
 */
std::optional<std::vector<std::uint8_t>> GhostFromJson(const nlohmann::ordered_json &json);

/**
 * Write the JSON of a recording with each frame on one line.
 *
 * @param json The JSON of GhostToJson().
 * @return The text.
 */
std::string FormatGhostJson(const nlohmann::ordered_json &json);

} // namespace coastline

extern "C" {
/**
 * Open a ghost recording of the PC release: the file built from the JSON the import converted it
 * to, or the file itself, such as a saved recording.
 *
 * @param path The path of the recording in the PC release.
 * @return The file, open for reading, or null.
 */
FILE *OpenGhostRecording(const char *path);
}
