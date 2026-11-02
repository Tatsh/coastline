#include <bit>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

#include "notesafety.h"

namespace {
using namespace coastline;
constexpr NoteEquipmentCounts COUNTS{2, 2, 3, 3, 5};

void Put(std::vector<std::uint8_t> &note, std::size_t at, std::uint32_t value) {
    for (std::size_t index = 0; index < 4; ++index) {
        note[at + index] = static_cast<std::uint8_t>(value >> ((3 - index) * 8));
    }
}

void CheckTimes() {
    assert(ValidSavedRaceTime(0.0f) && ValidSavedRaceTime(123.456f));
    assert(ValidSavedRaceTime(std::nextafter(21474836.0f, 0.0f)));
    assert(!ValidSavedRaceTime(21474836.0f));
    assert(!ValidSavedRaceTime(-1.0f));
    assert(!ValidSavedRaceTime(std::numeric_limits<float>::infinity()));
    assert(!ValidSavedRaceTime(std::numeric_limits<float>::quiet_NaN()));
    std::uint32_t random = 13;
    for (int index = 0; index < 100000; ++index) {
        random = random * 1664525u + 1013904223u;
        const auto value = std::bit_cast<float>(random);
        if (ValidSavedRaceTime(value)) {
            volatile auto hundredths = static_cast<std::int32_t>(value * 100.0f);
            assert(hundredths >= 0);
        }
    }
}

void CheckOptions() {
    std::vector<std::uint8_t> note(0x100);
    assert(ValidOptionsNote(note, COUNTS));
    for (const auto offset : {0, 4, 8}) {
        Put(note, offset, offset == 8 ? 2 : 11);
        assert(!ValidOptionsNote(note, COUNTS));
        Put(note, offset, 0);
    }
    for (std::size_t index = 0; index < COUNTS.size(); ++index) {
        Put(note, 0x80 + index * 4, static_cast<std::uint32_t>(COUNTS[index] - 1));
        assert(ValidOptionsNote(note, COUNTS));
        Put(note, 0x80 + index * 4, static_cast<std::uint32_t>(COUNTS[index]));
        assert(!ValidOptionsNote(note, COUNTS));
        Put(note, 0x80 + index * 4, 0xffffffffu);
        assert(!ValidOptionsNote(note, COUNTS));
        Put(note, 0x80 + index * 4, 0);
    }
    for (std::size_t size = 0; size < note.size(); ++size) {
        assert(!ValidOptionsNote(std::span(note).first(size), COUNTS));
    }
    note.push_back(0);
    assert(!ValidOptionsNote(note, COUNTS));
}

void CheckSeasons() {
    std::vector<std::uint8_t> note(0x200);
    assert(ValidSeasonNoteValues(note, COUNTS));
    for (const auto &range : {std::pair{0x2c, 24}, std::pair{0x8c, 10}, std::pair{0xe8, 10}}) {
        for (int index = 0; index < range.second; ++index) {
            const auto offset = static_cast<std::size_t>(range.first + index * 4);
            for (const auto bits :
                 {0x7fc00000u, 0x7f800000u, 0xff800000u, 0xbf800000u, 0x7f7fffffu}) {
                Put(note, offset, bits);
                assert(!ValidSeasonNoteValues(note, COUNTS));
            }
            Put(note, offset, 0);
        }
    }
    note[6] = 64;
    assert(!ValidSeasonNoteValues(note, COUNTS));
    note[6] = 255;
    assert(ValidSeasonNoteValues(note, COUNTS));
    Put(note, 0xd4, 0xffffffffu);
    assert(!ValidSeasonNoteValues(note, COUNTS));
    Put(note, 0xd4, 0);
    for (std::size_t size = 0; size < note.size(); ++size) {
        assert(!ValidSeasonNoteValues(std::span(note).first(size), COUNTS));
    }
}

void CheckGhosts() {
    std::vector<std::uint8_t> recording(10);
    assert(ValidGhostRecording(recording, 15, 20, COUNTS));
    for (std::size_t index = 0; index < COUNTS.size(); ++index) {
        recording[2 + index] = static_cast<std::uint8_t>(COUNTS[index]);
        assert(!ValidGhostRecording(recording, 15, 20, COUNTS));
        recording[2 + index] = 0;
    }
    recording[0] = 15;
    assert(!ValidGhostRecording(recording, 15, 20, COUNTS));
    recording[0] = 32;
    assert(!ValidGhostRecording(recording, 100, 100, COUNTS));
    recording[0] = 0;
    recording[1] = 32;
    assert(!ValidGhostRecording(recording, 100, 100, COUNTS));
    recording[1] = 0;
    recording.pop_back();
    assert(!ValidGhostRecording(recording, 15, 20, COUNTS));
    recording.resize(11);
    assert(!ValidGhostRecording(recording, 15, 20, COUNTS));
}
} // namespace

int main() {
    CheckTimes();
    CheckOptions();
    CheckSeasons();
    CheckGhosts();
}
