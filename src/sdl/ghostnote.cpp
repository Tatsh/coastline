#include "ghostnote.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <fstream>
#include <iterator>
#include <system_error>

#include "common/rle.h"

namespace coastline {

namespace {

// The steering and the actions of each step, interleaved.
constexpr s32 GHOST_CHANNELS = 2;

// The signature of the file of the frames.
constexpr std::array<u8, 4> FRAMES_MAGIC{'C', 'G', 'F', 'r'};

u32 GetBig32(std::span<const u8> bytes, std::size_t at) {
    return static_cast<u32>(bytes[at]) << 24 | static_cast<u32>(bytes[at + 1]) << 16 |
           static_cast<u32>(bytes[at + 2]) << 8 | bytes[at + 3];
}

void PutBig32(std::span<u8> bytes, std::size_t at, u32 value) {
    bytes[at] = static_cast<u8>(value >> 24);
    bytes[at + 1] = static_cast<u8>(value >> 16);
    bytes[at + 2] = static_cast<u8>(value >> 8);
    bytes[at + 3] = static_cast<u8>(value);
}

void AppendLittle32(std::vector<u8> &bytes, std::size_t value) {
    for (int shift = 0; shift < 32; shift += 8) {
        bytes.push_back(static_cast<u8>(value >> shift));
    }
}

std::optional<std::size_t> GetLittle32(std::span<const u8> bytes, std::size_t at) {
    if (at + 4 > bytes.size()) {
        return std::nullopt;
    }
    return static_cast<std::size_t>(bytes[at]) | static_cast<std::size_t>(bytes[at + 1]) << 8 |
           static_cast<std::size_t>(bytes[at + 2]) << 16 |
           static_cast<std::size_t>(bytes[at + 3]) << 24;
}

// Each channel of a compressed ghost starts with its length in the byte order of the console, and
// the run-length routines read and write the length in the order of the host. A length past the
// end of the note fails.
bool SwapBlockLengths(std::span<u8> packed, bool fromConsole) {
    std::size_t at = 0;
    for (s32 channel = 0; channel < GHOST_CHANNELS; ++channel) {
        if (at + 4 > packed.size()) {
            return false;
        }
        u32 length = 0;
        if (fromConsole) {
            length = GetBig32(packed, at);
            std::memcpy(&packed[at], &length, sizeof(length));
        } else {
            std::memcpy(&length, &packed[at], sizeof(length));
            PutBig32(packed, at, length);
        }
        if (length > packed.size() - at - 4) {
            return false;
        }
        at += 4 + length;
    }
    return true;
}

} // namespace

std::optional<std::vector<u8>> PackGhostNote(std::span<const u8> recording) {
    // The scan reads a byte past the end of the recording.
    std::vector<u8> padded(recording.begin(), recording.end());
    padded.resize(padded.size() + GHOST_CHANNELS);
    std::vector<u8> note(GHOST_NOTE_SIZE, 0);
    s32 size = CompressRleChannels(note.data(),
                                   static_cast<s32>(note.size()),
                                   padded.data(),
                                   static_cast<s32>(recording.size()),
                                   GHOST_CHANNELS);
    if (size == -1 || !SwapBlockLengths(note, false)) {
        return std::nullopt;
    }
    return note;
}

std::optional<std::vector<u8>> UnpackGhostNote(std::span<const u8> note) {
    std::vector<u8> compressed(note.begin(), note.end());
    if (!SwapBlockLengths(compressed, true)) {
        return std::nullopt;
    }
    // A literal block may read past the end of the note, and two bytes of the note expand to at
    // most 130 bytes of each channel.
    compressed.resize(compressed.size() + 0x100);
    std::vector<u8> expanded(compressed.size() * 130 * GHOST_CHANNELS);
    s32 size = DecompressInterleavedRle(
        expanded.data(), static_cast<s32>(expanded.size()), compressed.data(), GHOST_CHANNELS);
    if (size < 2 || static_cast<std::size_t>(size) > GHOST_RECORDING_CAPACITY) {
        return std::nullopt;
    }
    expanded.resize(static_cast<std::size_t>(size));
    return expanded;
}

std::vector<u8> PackGhostFrames(std::span<const u8> recording, std::span<const u8> frames) {
    std::vector<u8> file(FRAMES_MAGIC.begin(), FRAMES_MAGIC.end());
    AppendLittle32(file, recording.size());
    file.insert(file.end(), recording.begin(), recording.end());
    AppendLittle32(file, frames.size());
    file.insert(file.end(), frames.begin(), frames.end());
    return file;
}

std::optional<std::vector<u8>> GhostFramesFor(std::span<const u8> file,
                                              std::span<const u8> recording) {
    if (file.size() < FRAMES_MAGIC.size() ||
        !std::equal(FRAMES_MAGIC.begin(), FRAMES_MAGIC.end(), file.begin())) {
        return std::nullopt;
    }
    std::size_t at = FRAMES_MAGIC.size();
    auto recordingSize = GetLittle32(file, at);
    at += 4;
    if (!recordingSize || *recordingSize > file.size() - at ||
        !std::ranges::equal(file.subspan(at, *recordingSize), recording)) {
        return std::nullopt;
    }
    at += *recordingSize;
    auto framesSize = GetLittle32(file, at);
    at += 4;
    if (!framesSize || *framesSize != file.size() - at) {
        return std::nullopt;
    }
    return std::vector<u8>(file.begin() + static_cast<std::ptrdiff_t>(at), file.end());
}

bool SaveGhostFrames(const std::filesystem::path &path,
                     std::span<const u8> recording,
                     std::span<const u8> frames) {
    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);
    auto file = PackGhostFrames(recording, frames);
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out.write(reinterpret_cast<const char *>(file.data()),
              static_cast<std::streamsize>(file.size()));
    return static_cast<bool>(out);
}

std::optional<std::vector<u8>> LoadGhostFrames(const std::filesystem::path &path,
                                               std::span<const u8> recording) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return std::nullopt;
    }
    std::vector<u8> file((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    return GhostFramesFor(file, recording);
}

} // namespace coastline
