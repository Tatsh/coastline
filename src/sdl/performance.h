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
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace coastline {

/** The states of F in g_bShowFpsCounter: 0 for off, 1 for the frame rate, and 2 for both. */
inline constexpr int FRAME_RATE_STATES = 3;

/** The state of F in g_bShowFpsCounter that shows the statistics beside the frame rate. */
inline constexpr int FRAME_RATE_STATISTICS = 2;

/**
 * Match the active adapter to one provider device, rejecting absent and ambiguous identities.
 *
 * @param active The active adapter identifier, compared exactly.
 * @param devices The provider identifiers in device-index order.
 * @return The unique matching index, or std::nullopt for an empty, absent, or duplicate identifier.
 */
inline std::optional<std::size_t> UniqueGpuIndex(std::string_view active,
                                                 std::span<const std::string> devices) {
    std::optional<std::size_t> result;
    if (active.empty()) {
        return result;
    }
    for (std::size_t index = 0; index < devices.size(); ++index) {
        if (devices[index] == active) {
            if (result) {
                return std::nullopt;
            }
            result = index;
        }
    }
    return result;
}

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

/** One-second timing averages and worst values since collection began. */
class FrameTimings {
public:
    /** Completed sampling interval averages and extrema since the first recorded frame. */
    struct Snapshot {
        /** Mean CPU work per frame in milliseconds, excluding presentation and pacing waits. */
        double workMs = 0.0;
        /** Mean interval between frame timestamps in milliseconds, including waits. */
        double intervalMs = 0.0;
        /** Frame intervals per second over the completed sampling interval. */
        double fps = 0.0;
        /** Greatest recorded CPU work time in milliseconds since collection began. */
        double worstWorkMs = 0.0;
        /** Greatest recorded interval in milliseconds since collection began. */
        double worstIntervalMs = 0.0;
        /** Lowest completed sampling interval frame rate since collection began. */
        double worstFps = 0.0;
    };

    /**
     * Accumulate a frame and publish averages after at least one second of timestamp progress.
     *
     * @param workNanoseconds CPU work duration in nanoseconds, excluding presentation and pacing.
     * @param now A monotonically increasing timestamp in nanoseconds from a consistent clock.
     * @return Whether a new snapshot became available during this call.
     */
    bool Record(std::uint64_t workNanoseconds, std::uint64_t now) {
        m_workTotal += static_cast<double>(workNanoseconds);
        ++m_workCount;
        m_worstWork = std::max(m_worstWork, workNanoseconds);
        if (!m_previous) {
            m_previous = now;
            m_start = now;
            return false;
        }
        const auto interval = now - *m_previous;
        m_previous = now;
        m_intervalTotal += static_cast<double>(interval);
        ++m_intervals;
        m_worstInterval = std::max(m_worstInterval, interval);
        if (now - m_start < 1000000000) {
            return false;
        }
        const double fps =
            static_cast<double>(m_intervals) * 1e9 / static_cast<double>(now - m_start);
        m_snapshot = {m_workTotal / static_cast<double>(m_workCount) / 1e6,
                      m_intervalTotal / static_cast<double>(m_intervals) / 1e6,
                      fps,
                      static_cast<double>(m_worstWork) / 1e6,
                      static_cast<double>(m_worstInterval) / 1e6,
                      m_snapshot ? std::min(m_snapshot->worstFps, fps) : fps};
        m_workTotal = 0.0;
        m_workCount = 0;
        m_intervalTotal = 0.0;
        m_intervals = 0;
        m_start = now;
        return true;
    }

    /**
     * @return The latest snapshot, absent until one second elapses. The reference remains valid
     * for this collector's lifetime; Record() can replace its value.
     */
    const std::optional<Snapshot> &Latest() const {
        return m_snapshot;
    }

private:
    std::optional<Snapshot> m_snapshot;
    std::optional<std::uint64_t> m_previous;
    std::uint64_t m_start = 0;
    double m_workTotal = 0.0;
    std::uint64_t m_workCount = 0;
    double m_intervalTotal = 0.0;
    std::uint64_t m_intervals = 0;
    std::uint64_t m_worstWork = 0;
    std::uint64_t m_worstInterval = 0;
};

/** Frame timings and system statistics, with GPU sampling on a separate thread. */
class PerformanceStats {
public:
    /**
     * Sample the active adapter only when its identity permits NVML.
     *
     * @param gpuName The active adapter identifier for an exact NVML device match.
     * @param allowNvml Whether the renderer permits NVML for the active adapter.
     */
    PerformanceStats(std::string gpuName, bool allowNvml);
    /** Stop GPU sampling and release provider resources before destruction completes. */
    ~PerformanceStats();
    /** Prevent copying the collector's GPU sampling thread. */
    PerformanceStats(const PerformanceStats &) = delete;
    /** Prevent assignment of the collector's GPU sampling thread. */
    PerformanceStats &operator=(const PerformanceStats &) = delete;

    /**
     * Record frame work and elapsed frame intervals, and sample once a second has passed.
     *
     * @param workNanoseconds CPU-side frame time excluding presentation waits and software pacing.
     */
    void Frame(std::uint64_t workNanoseconds);

    /**
     * @param gameGpuBytes The bytes of the textures and buffers the game has created.
     * @return The lines of the overlay in capitals, with PCT for a percent sign, each statistic
     * omitted until it is known and where the system does not report it.
     */
    std::vector<std::string> Lines(std::uint64_t gameGpuBytes) const;

private:
    class GpuSampler;

    std::unique_ptr<GpuSampler> m_gpu;
    FrameTimings m_frameTimings;
    std::uint64_t m_secondStart = 0;
    std::optional<double> m_secondStartCpu; // The seconds of the processor at the second's start.
    std::optional<double> m_cpuPercent;     // Of one core, over the second before.
    std::optional<std::uint64_t> m_memoryBytes; // The memory of the game's own, as OwnMemoryBytes.
};

} // namespace coastline
