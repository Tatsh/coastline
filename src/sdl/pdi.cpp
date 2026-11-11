#include "pdi.h"

#include <algorithm>
#include <array>
#include <fstream>
#include <limits>

namespace coastline {
namespace {
constexpr std::size_t HEADER = 304;
constexpr std::size_t RAW = 2352;
constexpr std::size_t DATA = 2048;
constexpr std::array<std::uint8_t, 16> SIGNATURE{
    'T', 'T', 'A', 'F', 'M', 'P', 'V', 'O', 'B', 'C', 'D', 'I', 'M', 'G', 1, 0};

constexpr auto ScrambleMask() {
    std::array<std::uint8_t, RAW - 12> result{};
    unsigned shift = 1;
    for (auto &byte : result) {
        for (unsigned bit = 0; bit < 8; ++bit) {
            byte |= static_cast<std::uint8_t>((shift & 1) << bit);
            shift = (shift >> 1) | (((shift ^ (shift >> 1)) & 1) << 14);
        }
    }
    return result;
}

constexpr auto SCRAMBLE = ScrambleMask();

bool Sync(std::span<const std::uint8_t> sector) {
    return sector[0] == 0 && sector[11] == 0 &&
           std::ranges::all_of(sector.subspan(1, 10), [](auto byte) { return byte == 0xff; });
}

void Normalize(std::span<std::uint8_t> sector) {
    if (Sync(sector) && (sector[15] == (1 ^ SCRAMBLE[3]) || sector[15] == (2 ^ SCRAMBLE[3]))) {
        for (std::size_t byte = 12; byte < RAW; ++byte) {
            sector[byte] ^= SCRAMBLE[byte - 12];
        }
    }
}

bool Descriptor(std::span<const std::uint8_t> data, std::uint64_t available) {
    constexpr std::array<std::uint8_t, 7> MAGIC{1, 'C', 'D', '0', '0', '1', 1};
    if (!std::ranges::equal(data.first(MAGIC.size()), MAGIC)) {
        return false;
    }
    std::uint32_t little = 0, big = 0;
    for (unsigned index = 0; index < 4; ++index) {
        little |= static_cast<std::uint32_t>(data[80 + index]) << (index * 8);
        big = (big << 8) | data[84 + index];
    }
    return little >= 17 && little == big && little <= available;
}

struct Layout {
    std::size_t stride;
    bool raw;
};

std::optional<Layout> Probe(std::ifstream &input, std::uint64_t bytes) {
    std::array<std::uint8_t, 2448> sector{};
    for (const auto stride : {RAW, RAW + 16, RAW + 96, DATA}) {
        if (bytes % stride != 0) {
            continue;
        }
        for (const std::uint64_t first : {0u, 150u}) {
            if (bytes / stride < first + 17) {
                continue;
            }
            input.clear();
            input.seekg(static_cast<std::streamoff>(HEADER + (first + 16) * stride));
            if (!input.read(reinterpret_cast<char *>(sector.data()), stride)) {
                continue;
            }
            if (stride != DATA) {
                Normalize(sector);
                if (!Sync(sector) || sector[15] != 1) {
                    continue;
                }
            }
            if (Descriptor(std::span(sector).subspan(stride == DATA ? 0 : 16),
                           bytes / stride - first)) {
                return Layout{stride, stride != DATA};
            }
        }
    }
    return std::nullopt;
}
} // namespace

std::expected<DecodedDisc, std::string> DecodePdi(const std::filesystem::path &image,
                                                  const ImportProgress &progress) {
    const auto invalid = std::unexpected(std::string("Invalid or unsupported PDI image."));
    std::error_code error;
    const auto size = std::filesystem::file_size(image, error);
    if (error || size < HEADER ||
        size > static_cast<std::uint64_t>(std::numeric_limits<std::streamoff>::max())) {
        return invalid;
    }
    std::ifstream input(image, std::ios::binary);
    std::array<std::uint8_t, HEADER> header{};
    if (!input.read(reinterpret_cast<char *>(header.data()), header.size()) ||
        !std::ranges::equal(std::span(header).first(SIGNATURE.size()), SIGNATURE)) {
        return invalid;
    }
    const auto layout = Probe(input, size - HEADER);
    if (!layout) {
        return invalid;
    }
    const auto count = (size - HEADER) / layout->stride;
    const auto stride = layout->raw ? RAW : DATA;
    auto decoded = CreateDecodedDisc(layout->raw, count * stride);
    if (!decoded) {
        return std::unexpected(decoded.error());
    }
    decoded->sources.push_back(image);
    std::ofstream output(decoded->file, std::ios::binary);
    if (!output) {
        return std::unexpected(std::string("Cannot create decoded disc image."));
    }
    constexpr std::size_t BATCH = 128;
    std::array<std::uint8_t, BATCH * 2448> buffer{};
    input.clear();
    input.seekg(HEADER);
    for (std::uint64_t first = 0; first < count;) {
        if (progress && !progress("Decoding PDI image...",
                                  static_cast<double>(first) / static_cast<double>(count))) {
            return std::unexpected(std::string("Import cancelled."));
        }
        const auto sectors =
            static_cast<std::size_t>(std::min<std::uint64_t>(BATCH, count - first));
        if (!input.read(reinterpret_cast<char *>(buffer.data()), sectors * layout->stride)) {
            return invalid;
        }
        for (std::size_t index = 0; index < sectors; ++index) {
            const auto sector = std::span(buffer).subspan(index * layout->stride, stride);
            if (layout->raw) {
                Normalize(sector);
            }
            if (!output.write(reinterpret_cast<const char *>(sector.data()), sector.size())) {
                return std::unexpected(std::string("Cannot write decoded disc image."));
            }
        }
        first += sectors;
    }
    output.close();
    if (!output) {
        return std::unexpected(std::string("Cannot finish decoded disc image."));
    }
    if (progress && !progress("Decoding PDI image...", 1.0)) {
        return std::unexpected(std::string("Import cancelled."));
    }
    return decoded;
}

} // namespace coastline
