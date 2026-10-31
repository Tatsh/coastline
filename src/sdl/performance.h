#pragma once

/**
 * @file
 * The statistics of the third state of F: the time of the frames, and the use of the processor,
 * the memory, the GPU, and the memory of the GPU by the game.
 */

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
    double m_worstMs = 0.0;    // Of the second before.
    double m_worstNowMs = 0.0; // Of the second so far.
    std::uint64_t m_secondStart = 0;
    std::optional<double> m_secondStartCpu; // The seconds of the processor at the second's start.
    std::optional<double> m_cpuPercent;     // Of one core, over the second before.
    std::optional<std::uint64_t> m_residentBytes;
};

} // namespace coastline
