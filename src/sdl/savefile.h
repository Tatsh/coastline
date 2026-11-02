#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace coastline {

/**
 * Validate format-specific fields of an internal save's payload under its exclusive process lock.
 * Return false to reject the payload before it can be used by the game.
 */
using SavePayloadValidator = std::function<bool(std::span<const std::uint8_t>)>;

/**
 * Read a complete regular file without allocating beyond a caller-supplied limit.
 *
 * @param path File to read. Concurrent atomic replacement preserves the opened contents.
 * @param limit Maximum accepted length in bytes.
 * @return Complete contents, or std::nullopt for missing, oversized, or unreadable files.
 */
std::optional<std::vector<std::uint8_t>> ReadSaveFile(const std::filesystem::path &path,
                                                      std::size_t limit);

/** An exclusive lock shared by cooperating save writers, released on destruction or process exit.
 */
class SaveFile {
public:
    /**
     * Lock a save for a complete read-modify-write transaction, creating parent directories.
     *
     * @param path Destination file. A persistent sibling lock file coordinates other writers.
     * @return Locked save, or std::nullopt when its directory or lock cannot be opened.
     */
    static std::optional<SaveFile> Open(const std::filesystem::path &path);

    /** Release the process lock and its native handle. */
    ~SaveFile();
    /**
     * Transfer the source save's process lock without releasing it.
     */
    SaveFile(SaveFile &&) noexcept;
    /**
     * Release the destination lock and transfer the source save's process lock.
     * @return The destination save.
     */
    SaveFile &operator=(SaveFile &&) noexcept;

    /**
     * Read the locked save with bounded allocation.
     *
     * @param limit Maximum accepted length in bytes.
     * @return Complete contents, or std::nullopt for a missing, oversized, or unreadable save.
     */
    std::optional<std::vector<std::uint8_t>> Read(std::size_t limit) const;

    /**
     * Replace the save atomically after writing and synchronising a unique sibling file.
     *
     * @param bytes Complete new contents.
     * @return Whether replacement and synchronisation succeed. Failures before replacement
     * preserve the previous save.
     */
    bool Write(std::span<const std::uint8_t> bytes) const;

    /**
     * Delete the locked save while excluding cooperating writers.
     *
     * @return Whether the save is absent afterwards.
     */
    bool Remove() const;

private:
    struct State;
    explicit SaveFile(std::unique_ptr<State> state);
    std::unique_ptr<State> m_state;
};

/**
 * Lock and atomically replace a binary save.
 *
 * @param path Destination file.
 * @param bytes Complete new contents.
 * @return Whether replacement and synchronisation succeed. Failures before replacement preserve
 * the previous save.
 */
bool WriteSaveFile(const std::filesystem::path &path, std::span<const std::uint8_t> bytes);

/**
 * Lock and atomically replace a text save.
 *
 * @param path Destination file.
 * @param text Complete new contents, written without newline conversion.
 * @return Whether replacement and synchronisation succeed. Failures before replacement preserve
 * the previous save.
 */
bool WriteSaveFile(const std::filesystem::path &path, std::string_view text);

/**
 * Read an internal binary save with its format signature, payload length, and SHA-1 checksum.
 * Exclude cooperating writers while validating and optionally removing damaged or oversized saves.
 * Unreadable files are preserved when their size cannot prove corruption.
 *
 * @param path Internal save file; native interchange files and editable INI files are excluded.
 * @param limit Maximum payload size in bytes, excluding the integrity header.
 * @param resetInvalid Remove damaged saves; disable when scanning possibly unrelated files.
 * @param validate Optional format-specific validator, called after the checksum passes.
 * @return Verified payload, or std::nullopt for a missing, unreadable, or invalid save or size
 * limit.
 */
std::optional<std::vector<std::uint8_t>>
ReadVerifiedSaveFile(const std::filesystem::path &path,
                     std::size_t limit,
                     bool resetInvalid = true,
                     const SavePayloadValidator &validate = {});

/**
 * Atomically write an internal binary save with a format signature, length, and SHA-1 checksum.
 * The payload and integrity header are replaced together under the save's process lock.
 *
 * @param path Internal save file, with parent directories created as needed.
 * @param bytes Complete binary payload.
 * @return Whether replacement and synchronisation succeed.
 */
bool WriteVerifiedSaveFile(const std::filesystem::path &path, std::span<const std::uint8_t> bytes);

} // namespace coastline
