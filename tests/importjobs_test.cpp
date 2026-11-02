#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <thread>
#include <vector>

#include "importjobs.h"

namespace {
using namespace coastline;
using namespace std::chrono_literals;

void CheckSuccess() {
    std::atomic<int> active{0};
    std::atomic<int> maximum{0};
    std::atomic<int> completed{0};
    std::atomic<bool> reporting{false};
    std::vector<ImportJob> jobs;
    for (int index = 0; index < 12; ++index) {
        jobs.emplace_back([&](const ImportProgress &progress) -> std::optional<std::string> {
            const auto running = active.fetch_add(1) + 1;
            int previous = maximum.load();
            while (previous < running && !maximum.compare_exchange_weak(previous, running)) {
            }
            assert(progress("working", 0.5));
            assert(progress("working", 0.25));
            assert(progress("working", std::numeric_limits<double>::quiet_NaN()));
            std::this_thread::sleep_for(5ms);
            assert(progress("working", 1.0));
            --active;
            ++completed;
            return std::nullopt;
        });
    }
    double previous = 0.0;
    auto failure = RunImportJobs(jobs, [&](const std::string &, double fraction) {
        assert(!reporting.exchange(true));
        assert(std::isfinite(fraction) && fraction >= previous && fraction <= 1.0);
        previous = fraction;
        std::this_thread::sleep_for(100us);
        reporting.store(false);
        return true;
    });
    const auto limit = static_cast<int>(std::clamp(std::thread::hardware_concurrency(), 1u, 4u));
    assert(!failure && active == 0 && completed == 12 && previous == 1.0);
    assert(maximum >= 1 && maximum <= limit);
    if (limit > 1) {
        assert(maximum > 1);
    }
}

void CheckCancellation() {
    std::atomic<int> started{0};
    std::atomic<int> active{0};
    std::vector<ImportJob> jobs(20, [&](const ImportProgress &progress) {
        ++started;
        ++active;
        for (int step = 0; step < 50; ++step) {
            if (!progress("working", static_cast<double>(step) / 50.0)) {
                --active;
                return std::optional<std::string>("Import cancelled.");
            }
            std::this_thread::sleep_for(1ms);
        }
        --active;
        return std::optional<std::string>{};
    });
    auto failure =
        RunImportJobs(jobs, [](const std::string &, double fraction) { return fraction < 0.01; });
    assert(failure == "Import cancelled." && active == 0 && started <= 4);
}

void CheckFailures() {
    std::atomic<int> active{0};
    std::atomic<bool> waiting{false};
    bool throwCancellation = false;
    std::vector<ImportJob> jobs;
    jobs.emplace_back([&](const ImportProgress &progress) -> std::optional<std::string> {
        ++active;
        waiting.store(true);
        while (progress("waiting", 0.0)) {
            std::this_thread::sleep_for(1ms);
        }
        --active;
        if (throwCancellation) {
            throw std::runtime_error("Cancelled.");
        }
        return "Cancelled.";
    });
    jobs.emplace_back([&](const ImportProgress &) -> std::optional<std::string> {
        while (!waiting.load()) {
            std::this_thread::yield();
        }
        throw std::runtime_error("Conversion failed.");
    });
    if (std::thread::hardware_concurrency() > 1) {
        assert(RunImportJobs(jobs) == "Conversion failed.");
        assert(active == 0);
        waiting.store(false);
        throwCancellation = true;
        assert(RunImportJobs(jobs) == "Conversion failed.");
        assert(active == 0);
    }
    const std::vector<ImportJob> one{[](const ImportProgress &progress) {
        progress("throwing callback", 0.1);
        return std::optional<std::string>{};
    }};
    assert(RunImportJobs(one, [](const std::string &, double) -> bool {
               throw std::runtime_error("Callback failed.");
           }) == "Callback failed.");
    assert(!RunImportJobs({}));
}
} // namespace

int main() {
    CheckSuccess();
    CheckCancellation();
    CheckFailures();
}
