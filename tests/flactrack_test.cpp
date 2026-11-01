#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <filesystem>
#include <vector>

#include <FLAC/stream_encoder.h>

#include "flactrack.h"

int main() {
    const auto path =
        std::filesystem::temp_directory_path() /
        ("coastline-flac-" +
         std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".flac");
    constexpr std::size_t FRAMES = 51203;
    std::vector<FLAC__int32> input(FRAMES * 2);
    std::vector<std::int16_t> expected(input.size());
    for (std::size_t index = 0; index < input.size(); ++index) {
        input[index] = static_cast<int>((index * 31) % 65536) - 32768;
        expected[index] = static_cast<std::int16_t>(input[index]);
    }
    auto *encoder = FLAC__stream_encoder_new();
    assert(encoder);
    assert(FLAC__stream_encoder_set_channels(encoder, 2));
    assert(FLAC__stream_encoder_set_bits_per_sample(encoder, 16));
    assert(FLAC__stream_encoder_set_sample_rate(encoder, 44100));
    assert(FLAC__stream_encoder_set_blocksize(encoder, 4096));
    assert(FLAC__stream_encoder_init_file(encoder, path.string().c_str(), nullptr, nullptr) ==
           FLAC__STREAM_ENCODER_INIT_STATUS_OK);
    assert(FLAC__stream_encoder_process_interleaved(encoder, input.data(), FRAMES));
    assert(FLAC__stream_encoder_finish(encoder));
    FLAC__stream_encoder_delete(encoder);

    coastline::FlacTrack track;
    for (std::size_t chunk : {1, 17, 127, 512, 1000, 4095, 4096, 8191}) {
        assert(track.Open(path));
        std::vector<std::int16_t> decoded;
        for (std::size_t at = 0; at < FRAMES; at += chunk) {
            const auto count = std::min(chunk, FRAMES - at);
            assert(track.Decode(decoded, count));
            assert(decoded.size() == (at + count) * 2);
        }
        assert(decoded == expected);
        assert(!track.Decode(decoded, 1));
        assert(decoded == expected);
    }
    assert(track.Open(path));
    std::vector<std::int16_t> first;
    assert(track.Decode(first, 17));
    assert(track.Open(path));
    std::vector<std::int16_t> restarted{123, -321};
    assert(track.Decode(restarted, 44100));
    assert(restarted.size() == 2 + 44100 * 2);
    assert(restarted[0] == 123 && restarted[1] == -321);
    assert(std::equal(restarted.begin() + 2, restarted.end(), expected.begin()));
    assert(!track.Open(path.parent_path() / "coastline-missing-flac-file"));
    std::vector<std::int16_t> missing;
    assert(!track.Decode(missing, 10) && missing.empty());
    std::filesystem::remove(path);
}
