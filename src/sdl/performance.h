#pragma once

/**
 * @file
 * The statistics of the third state of F: the time of the frames, and the use of the processor,
 * the memory, the GPU, and the memory of the GPU by the game.
 */

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace coastline {

/** The states of F in g_bShowFpsCounter: 0 for off, 1 for the frame rate, and 2 for both. */
inline constexpr int FRAME_RATE_STATES = 3;

/** The state of F in g_bShowFpsCounter that shows the statistics beside the frame rate. */
inline constexpr int FRAME_RATE_STATISTICS = 2;

/**
 * @param text A line of /proc/self/statm.
 * @return The resident pages, the second number of the line, or nothing when the line has no
 * second number.
 */
inline std::optional<std::uint64_t> StatmResidentPages(std::string_view text) {
    std::size_t at = text.find_first_not_of(' ');
    if (at == std::string_view::npos) {
        return std::nullopt;
    }
    at = text.find(' ', at);
    if (at == std::string_view::npos) {
        return std::nullopt;
    }
    text.remove_prefix(at + 1);
    std::uint64_t pages = 0;
    auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), pages);
    if (error != std::errc{} || end == text.data()) {
        return std::nullopt;
    }
    return pages;
}

/**
 * @param text The contents of /proc/self/smaps.
 * @param executable The path of the program, as /proc/self/exe links to it.
 * @return The proportional set size in bytes of the memory of the game: the mappings without a
 * file (the heap, the stacks, and other anonymous memory) and those of the program. The shared
 * libraries, among them the GPU driver, and the mappings of devices are left out. The driver's own
 * anonymous memory cannot be told apart and is counted. Nothing when the text has no Pss line.
 */
inline std::optional<std::uint64_t> SmapsGamePssBytes(std::string_view text,
                                                      std::string_view executable) {
    std::uint64_t kilobytes = 0;
    bool counted = false;
    bool found = false;
    while (!text.empty()) {
        std::size_t end = text.find('\n');
        std::string_view line = text.substr(0, end);
        text.remove_prefix(end == std::string_view::npos ? text.size() : end + 1);
        if (line.starts_with("Pss:")) {
            line.remove_prefix(4);
            std::size_t digits = line.find_first_not_of(' ');
            std::uint64_t value = 0;
            if (digits == std::string_view::npos ||
                std::from_chars(line.data() + digits, line.data() + line.size(), value).ec !=
                    std::errc{}) {
                return std::nullopt;
            }
            found = true;
            if (counted) {
                kilobytes += value;
            }
            continue;
        }
        // The line of a mapping: its range, permissions, offset, device, inode, and path, if any.
        std::size_t dash = line.find('-');
        if (dash == 0 || dash == std::string_view::npos ||
            line.find_first_not_of("0123456789abcdef") != dash) {
            continue;
        }
        std::string_view rest = line;
        for (int field = 0; field < 5; ++field) {
            std::size_t space = rest.find(' ');
            rest.remove_prefix(space == std::string_view::npos ? rest.size() : space);
            rest.remove_prefix(std::min(rest.find_first_not_of(' '), rest.size()));
        }
        counted = rest.empty() || rest == "[heap]" || rest.starts_with("[stack") ||
                  rest.starts_with("[anon") || rest == executable;
    }
    if (!found) {
        return std::nullopt;
    }
    return kilobytes * 1024;
}

/**
 * The statistics of the frames, with the processor and the memory sampled once a second, and the
 * share of an NVIDIA GPU the game uses sampled once a second on a thread of its own.
 */
class PerformanceStats {
public:
    /** Start sampling, with NVML loaded when the system has it. */
    PerformanceStats();
    ~PerformanceStats();
    PerformanceStats(const PerformanceStats &) = delete;
    PerformanceStats &operator=(const PerformanceStats &) = delete;

    /** Record the time since the frame before, and sample once a second has passed. */
    void Frame();

    /**
     * @param gameGpuBytes The bytes of the textures and buffers the game has created.
     * @return The lines of the overlay in capitals, with PCT for a percent sign, each statistic
     * omitted until it is known and where the system does not report it.
     */
    std::vector<std::string> Lines(std::uint64_t gameGpuBytes) const;

private:
    class GpuSampler;

    std::unique_ptr<GpuSampler> m_gpu;
    std::uint64_t m_lastFrame = 0; // The nanoseconds of SDL of the frame before.
    double m_frameMs = 0.0;
    int m_framesNow = 0;                     // Drawn in the second so far.
    std::optional<double> m_framesPerSecond; // Of the second before.
    double m_worstMs = 0.0;                  // Of the second before.
    double m_worstNowMs = 0.0;               // Of the second so far.
    std::uint64_t m_secondStart = 0;
    std::optional<double> m_secondStartCpu; // The seconds of the processor at the second's start.
    std::optional<double> m_cpuPercent;     // Of one core, over the second before.
    std::optional<std::uint64_t> m_memoryBytes; // The memory of the game's own, as OwnMemoryBytes.
};

} // namespace coastline
