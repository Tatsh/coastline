#include "importjobs.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <exception>
#include <mutex>
#include <numeric>
#include <thread>
#include <vector>

namespace coastline {

std::optional<std::string> RunImportJobs(std::span<const ImportJob> jobs,
                                         const ImportProgress &progress) {
    if (jobs.empty()) {
        return std::nullopt;
    }
    constexpr const char *CANCELLED = "Import cancelled.";
    std::atomic<bool> stopped{false};
    std::atomic<std::size_t> next{0};
    std::mutex progressMutex;
    std::vector<double> fractions(jobs.size());
    std::vector<unsigned char> cancelled(jobs.size());
    std::vector<std::optional<std::string>> failures(jobs.size());
    auto worker = [&] {
        while (!stopped.load()) {
            const auto index = next.fetch_add(1);
            if (index >= jobs.size() || stopped.load()) {
                return;
            }
            std::string lastStep = "Converting assets...";
            const ImportProgress report = [&](const std::string &step, double fraction) {
                std::lock_guard lock(progressMutex);
                if (stopped.load()) {
                    cancelled[index] = 1;
                    return false;
                }
                if (std::isfinite(fraction)) {
                    fractions[index] = std::max(fractions[index], std::clamp(fraction, 0.0, 1.0));
                }
                lastStep = step;
                const double combined = std::accumulate(fractions.begin(), fractions.end(), 0.0) /
                                        static_cast<double>(jobs.size());
                if (progress && !progress(step, combined)) {
                    cancelled[index] = 1;
                    stopped.store(true);
                    return false;
                }
                return true;
            };
            try {
                failures[index] = jobs[index](report);
                if (!failures[index] && !report(lastStep, 1.0)) {
                    failures[index] = CANCELLED;
                }
            } catch (const std::exception &error) {
                failures[index] = error.what();
            } catch (...) {
                failures[index] = "Import task failed with an unknown exception.";
            }
            if (cancelled[index]) {
                failures[index] = CANCELLED;
            }
            if (failures[index]) {
                stopped.store(true);
            }
        }
    };
    const auto count =
        std::min(jobs.size(),
                 static_cast<std::size_t>(std::clamp(std::thread::hardware_concurrency(), 1u, 4u)));
    std::vector<std::thread> workers;
    std::optional<std::string> launchFailure;
    try {
        workers.reserve(count);
        for (std::size_t index = 0; index < count; ++index) {
            workers.emplace_back(worker);
        }
    } catch (const std::exception &error) {
        stopped.store(true);
        launchFailure = error.what();
    }
    for (auto &thread : workers) {
        thread.join();
    }
    if (launchFailure) {
        return launchFailure;
    }
    for (const auto &failure : failures) {
        if (failure && *failure != CANCELLED) {
            return failure;
        }
    }
    return stopped.load() ? std::optional<std::string>(CANCELLED) : std::nullopt;
}

} // namespace coastline
