// Host check of the TOP GEAR GHOST note of the SDL game: the run-length channels of the console in
// its byte order, and the file of the frames of the race engine stored beside the pak.

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <vector>

#include "ghostnote.h"

namespace {

int g_failures = 0;

void Check(bool condition, const char *description) {
    if (!condition) {
        std::cerr << "FAIL: " << description << '\n';
        ++g_failures;
    }
}

// A header and five steps of full steering to the right with the accelerator held.
std::vector<u8> ShortRecording() {
    std::vector<u8> recording{2, 1, 0, 3, 2, 2, 0, 0};
    for (int step = 0; step < 5; ++step) {
        recording.push_back(0x46);
        recording.push_back(0x01);
    }
    return recording;
}

// A lap of the console in its form: the steering sweeps and the actions change in runs.
std::vector<u8> LapRecording() {
    std::vector<u8> recording{4, 3, 1, 0, 2, 1, 0, 0};
    for (int step = 0; step < 3000; ++step) {
        auto steering = static_cast<std::int8_t>((step / 7) % 141 - 70);
        recording.push_back(static_cast<u8>(steering));
        recording.push_back(step % 400 < 350 ? 0x01 : 0x04);
    }
    return recording;
}

void TestShortNote() {
    auto recording = ShortRecording();
    auto note = coastline::PackGhostNote(recording);
    Check(note.has_value(), "a short recording packs");
    if (!note) {
        return;
    }
    Check(note->size() == coastline::GHOST_NOTE_SIZE, "the note takes 58 pages");
    // The even bytes: literals 2, 0, 2, 0, and a run of five 0x46. The odd bytes: literals 1, 3,
    // 2, 0, and a run of five 1. Each channel starts with its length, most significant byte first.
    const std::vector<u8> expected{0, 0, 0, 7, 0xFC, 2, 0, 2, 0, 2, 0x46,
                                   0, 0, 0, 7, 0xFC, 1, 3, 2, 0, 2, 0x01};
    Check(std::equal(expected.begin(), expected.end(), note->begin()),
          "the channels are written as the console writes them");
    Check(std::all_of(note->begin() + static_cast<std::ptrdiff_t>(expected.size()),
                      note->end(),
                      [](u8 byte) { return byte == 0; }),
          "the rest of the note is zero");
    auto unpacked = coastline::UnpackGhostNote(*note);
    Check(unpacked && *unpacked == recording, "the short note unpacks to its recording");
}

void TestLapNote() {
    auto recording = LapRecording();
    auto note = coastline::PackGhostNote(recording);
    Check(note.has_value(), "a lap packs");
    if (!note) {
        return;
    }
    auto unpacked = coastline::UnpackGhostNote(*note);
    Check(unpacked && *unpacked == recording, "the lap unpacks to its recording");
}

void TestLargeAndBrokenNotes() {
    // Steering that never repeats fills the note with literals.
    std::vector<u8> noise(coastline::GHOST_RECORDING_CAPACITY);
    std::uint32_t seed = 1;
    for (u8 &byte : noise) {
        seed = seed * 1103515245u + 12345u;
        byte = static_cast<u8>(seed >> 16);
    }
    Check(!coastline::PackGhostNote(noise), "a recording too large for the note does not pack");
    std::vector<u8> broken(coastline::GHOST_NOTE_SIZE, 0);
    broken[0] = 0x7F;
    Check(!coastline::UnpackGhostNote(broken), "a length past the end does not unpack");
    std::vector<u8> empty(coastline::GHOST_NOTE_SIZE, 0);
    Check(!coastline::UnpackGhostNote(empty), "a note with empty channels does not unpack");
}

void TestFrames() {
    auto recording = ShortRecording();
    std::vector<u8> frames(24 * 3);
    for (std::size_t i = 0; i < frames.size(); ++i) {
        frames[i] = static_cast<u8>(i);
    }
    auto file = coastline::PackGhostFrames(recording, frames);
    auto read = coastline::GhostFramesFor(file, recording);
    Check(read && *read == frames, "the frames read back with their recording");
    auto other = recording;
    other.back() ^= 1;
    Check(!coastline::GhostFramesFor(file, other), "the frames of another recording are refused");
    file.pop_back();
    Check(!coastline::GhostFramesFor(file, recording), "a truncated file is refused");
    Check(!coastline::GhostFramesFor(std::vector<u8>{'C', 'G'}, recording),
          "a file shorter than its signature is refused");
}

} // namespace

int main() {
    TestShortNote();
    TestLapNote();
    TestLargeAndBrokenNotes();
    TestFrames();
    if (g_failures != 0) {
        std::cerr << g_failures << " checks failed.\n";
        return 1;
    }
    return 0;
}
