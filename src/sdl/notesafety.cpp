#include "notesafety.h"

#include <bit>
#include <cmath>

#include "ghostnote.h"

namespace coastline {
namespace {

std::uint32_t Big32(std::span<const std::uint8_t> bytes, std::size_t offset) {
    return static_cast<std::uint32_t>(bytes[offset]) << 24 |
           static_cast<std::uint32_t>(bytes[offset + 1]) << 16 |
           static_cast<std::uint32_t>(bytes[offset + 2]) << 8 | bytes[offset + 3];
}

bool ValidEquipment(std::span<const std::uint8_t> note,
                    std::size_t offset,
                    const NoteEquipmentCounts &counts) {
    if (offset > note.size() || note.size() - offset < counts.size() * 4) {
        return false;
    }
    for (std::size_t index = 0; index < counts.size(); ++index) {
        if (Big32(note, offset + index * 4) >= counts[index]) {
            return false;
        }
    }
    return true;
}

} // namespace

bool ValidSavedRaceTime(float seconds) {
    return std::isfinite(seconds) && seconds >= 0.0f && seconds < 21474836.0f;
}

bool ValidOptionsNote(std::span<const std::uint8_t> note, const NoteEquipmentCounts &counts) {
    return note.size() == 0x100 && Big32(note, 0) <= 10 && Big32(note, 4) <= 10 &&
           Big32(note, 8) <= 1 && ValidEquipment(note, 0x80, counts);
}

bool ValidSeasonNoteValues(std::span<const std::uint8_t> note, const NoteEquipmentCounts &counts) {
    if (note.size() != 0x200 || !ValidEquipment(note, 0xd4, counts)) {
        return false;
    }
    for (std::size_t index = 0; index < 24; ++index) {
        const auto place = note[6 + index];
        if ((place > 7 && place != 255) ||
            !ValidSavedRaceTime(std::bit_cast<float>(Big32(note, 0x2c + index * 4)))) {
            return false;
        }
    }
    for (std::size_t index = 0; index < 10; ++index) {
        if (!ValidSavedRaceTime(std::bit_cast<float>(Big32(note, 0x8c + index * 4))) ||
            !ValidSavedRaceTime(std::bit_cast<float>(Big32(note, 0xe8 + index * 4)))) {
            return false;
        }
    }
    return true;
}

bool ValidGhostRecording(std::span<const std::uint8_t> recording,
                         std::size_t tracks,
                         std::size_t cars,
                         const NoteEquipmentCounts &counts) {
    if (recording.size() < 10 || recording.size() > GHOST_RECORDING_CAPACITY ||
        (recording.size() - 8) % 2 != 0 || recording[0] >= tracks || recording[1] >= cars ||
        recording[0] >= 32 || recording[1] >= 32) {
        return false;
    }
    for (std::size_t index = 0; index < counts.size(); ++index) {
        if (recording[2 + index] >= counts[index]) {
            return false;
        }
    }
    return true;
}

} // namespace coastline
