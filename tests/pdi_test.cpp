#include <algorithm>
#include <array>
#include <cassert>
#include <fstream>
#include <vector>

#include "pdi.h"

namespace {
using Bytes = std::vector<std::uint8_t>;

void Write(const std::filesystem::path &path, const Bytes &bytes) {
    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char *>(bytes.data()), bytes.size());
    assert(file.good());
}

Bytes Read(const std::filesystem::path &path) {
    std::ifstream file(path, std::ios::binary);
    return Bytes(std::istreambuf_iterator<char>(file), {});
}

auto Mask() {
    std::array<bool, 2352 * 8> bits{};
    bits[0] = true;
    for (std::size_t bit = 15; bit < bits.size(); ++bit) {
        bits[bit] = bits[bit - 15] != bits[bit - 14];
    }
    std::array<std::uint8_t, 2340> result{};
    for (std::size_t byte = 0; byte < result.size(); ++byte) {
        for (unsigned bit = 0; bit < 8; ++bit) {
            result[byte] |= static_cast<std::uint8_t>(bits[byte * 8 + bit] << bit);
        }
    }
    assert(result[0] == 1 && result[1] == 0x80 && result[2] == 0 && result[3] == 0x60);
    return result;
}

struct Fixture {
    Bytes pdi;
    Bytes wanted;
};

Fixture Image(std::size_t stride, bool scramble, bool pregap) {
    const bool raw = stride != 2048;
    const std::size_t size = raw ? 2352 : 2048;
    const std::size_t first = pregap ? 150 : 0;
    constexpr std::size_t VOLUME = 22;
    const auto count = first + VOLUME + (raw ? 320 : 0);
    Fixture fixture{Bytes(304), Bytes(count * size)};
    std::copy_n("TTAFMPVOBCDIMG", 14, fixture.pdi.begin());
    fixture.pdi[14] = 1;
    const auto mask = Mask();
    for (std::size_t index = 0; index < count; ++index) {
        Bytes sector(stride);
        for (std::size_t byte = 0; byte < size; ++byte) {
            sector[byte] = static_cast<std::uint8_t>(index * 37 + byte * 13);
        }
        if (raw && index >= first && index < first + VOLUME) {
            sector[0] = sector[11] = sector[12] = sector[14] = 0;
            std::fill_n(sector.begin() + 1, 10, 0xff);
            sector[13] = 2;
            sector[15] = 1;
        }
        if (index == first + 16) {
            const auto offset = raw ? 16 : 0;
            std::copy_n("\1CD001\1", 7, sector.begin() + offset);
            std::fill_n(sector.begin() + offset + 80, 8, 0);
            sector[offset + 80] = VOLUME;
            sector[offset + 87] = VOLUME;
        }
        std::copy_n(sector.begin(), size, fixture.wanted.begin() + index * size);
        if (raw && scramble && index >= first && index < first + VOLUME) {
            for (std::size_t byte = 12; byte < size; ++byte) {
                sector[byte] ^= mask[byte - 12];
            }
        }
        if (stride > size) {
            std::fill(sector.begin() + size, sector.end(), 0xad);
        }
        fixture.pdi.insert(fixture.pdi.end(), sector.begin(), sector.end());
    }
    return fixture;
}

void Check(const std::filesystem::path &file, std::size_t stride, bool scramble, bool pregap) {
    const auto fixture = Image(stride, scramble, pregap);
    Write(file, fixture.pdi);
    double previous = 0;
    auto decoded = coastline::DecodePdi(file, [&](const std::string &, double fraction) {
        assert(fraction >= previous && fraction <= 1.0);
        previous = fraction;
        return true;
    });
    assert(decoded && previous == 1.0);
    assert((decoded->sources == std::vector{file}));
    assert(decoded->file.extension() == (stride == 2048 ? ".iso" : ".bin"));
    assert(Read(decoded->file) == fixture.wanted);
    const auto directory = decoded->file.parent_path();
    decoded->storage.reset();
    assert(!std::filesystem::exists(directory));
    for (const auto cancel : {0.0, 0.3, 1.0}) {
        const auto stopped = coastline::DecodePdi(
            file, [&](const std::string &, double fraction) { return fraction < cancel; });
        assert(!stopped && stopped.error() == "Import cancelled.");
    }
}
} // namespace

int main() {
    auto temporary = coastline::CreateDecodedDisc(false, 0);
    assert(temporary);
    const auto file = temporary->file.parent_path() / "source.pdi";
    for (const auto stride : {2048u, 2352u, 2368u, 2448u}) {
        for (const auto pregap : {false, true}) {
            Check(file, stride, false, pregap);
            if (stride != 2048) {
                Check(file, stride, true, pregap);
            }
        }
    }
    const auto fixture = Image(2352, true, false);
    for (std::size_t size = 0; size <= 304; ++size) {
        Write(file, Bytes(fixture.pdi.begin(), fixture.pdi.begin() + size));
        assert(!coastline::DecodePdi(file));
    }
    for (std::size_t index = 0; index < 16; ++index) {
        auto bad = fixture.pdi;
        bad[index] ^= 1;
        Write(file, bad);
        assert(!coastline::DecodePdi(file));
    }
    for (std::size_t bytes = 1; bytes < 96; ++bytes) {
        Write(file, Bytes(fixture.pdi.begin(), fixture.pdi.end() - bytes));
        assert(!coastline::DecodePdi(file));
    }
    auto bad = fixture.pdi;
    bad[304 + 16 * 2352 + 16] ^= 1;
    Write(file, bad);
    assert(!coastline::DecodePdi(file));
    bad = fixture.pdi;
    bad[304 + 16 * 2352 + 16 + 84] ^= 1;
    Write(file, bad);
    assert(!coastline::DecodePdi(file));
    bad = fixture.pdi;
    std::fill_n(bad.begin() + 304 + 16 * 2352 + 16 + 80, 8, 0xff);
    Write(file, bad);
    assert(!coastline::DecodePdi(file));
    assert(!coastline::DecodePdi(file.parent_path() / "missing.pdi"));
}
