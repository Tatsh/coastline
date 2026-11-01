#include "performance.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <mutex>
#include <string>
#include <thread>

#include <SDL3/SDL.h>
#include <fmt/format.h>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
// psapi.h needs the types of windows.h before it.
// clang-format off
#include <windows.h>
#include <psapi.h>
// clang-format on
#elif defined(__APPLE__)
#include <sys/resource.h>
#include <unistd.h>

#include <mach/mach.h>
#else
#include <sys/resource.h>
#include <unistd.h>
#endif

namespace coastline {

namespace {

constexpr double MB = 1024.0 * 1024.0;

// @p bytes in GB of 1024 MB past 1024 MB, and otherwise in MB of 1024 by 1024 bytes.
std::string MemorySize(std::uint64_t bytes) {
    double megabytes = static_cast<double>(bytes) / MB;
    if (megabytes > 1024.0) {
        return fmt::format("{:.1f} GB", megabytes / 1024.0);
    }
    return fmt::format("{:.0f} MB", megabytes);
}

// The seconds of the processor the process has used, in the user and the kernel modes.
std::optional<double> ProcessCpuSeconds() {
#if defined(_WIN32)
    FILETIME creation;
    FILETIME exit;
    FILETIME kernel;
    FILETIME user;
    if (!GetProcessTimes(GetCurrentProcess(), &creation, &exit, &kernel, &user)) {
        return std::nullopt;
    }
    auto seconds = [](const FILETIME &time) {
        ULARGE_INTEGER ticks;
        ticks.LowPart = time.dwLowDateTime;
        ticks.HighPart = time.dwHighDateTime;
        // A tick of FILETIME is 100 nanoseconds.
        return static_cast<double>(ticks.QuadPart) / 1e7;
    };
    return seconds(kernel) + seconds(user);
#else
    rusage usage{};
    if (getrusage(RUSAGE_SELF, &usage) != 0) {
        return std::nullopt;
    }
    auto seconds = [](const timeval &time) {
        return static_cast<double>(time.tv_sec) + static_cast<double>(time.tv_usec) / 1e6;
    };
    return seconds(usage.ru_utime) + seconds(usage.ru_stime);
#endif
}

// The bytes of memory of the game: on Linux the proportional set size of its anonymous memory
// and of the program, without the libraries and the GPU driver, the private bytes on Windows,
// and the physical footprint on macOS. The private bytes and the footprint include the private
// memory of the driver, which those systems do not tell apart.
std::optional<std::uint64_t> OwnMemoryBytes() {
#if defined(_WIN32)
    PROCESS_MEMORY_COUNTERS_EX counters{};
    if (!GetProcessMemoryInfo(GetCurrentProcess(),
                              reinterpret_cast<PROCESS_MEMORY_COUNTERS *>(&counters),
                              sizeof(counters))) {
        return std::nullopt;
    }
    return counters.PrivateUsage;
#elif defined(__APPLE__)
    task_vm_info_data_t info{};
    mach_msg_type_number_t count = TASK_VM_INFO_COUNT;
    kern_return_t result =
        task_info(mach_task_self(), TASK_VM_INFO, reinterpret_cast<task_info_t>(&info), &count);
    if (result != KERN_SUCCESS) {
        return std::nullopt;
    }
    return info.phys_footprint;
#elif defined(__linux__)
    {
        std::ifstream smaps("/proc/self/smaps");
        std::string text((std::istreambuf_iterator<char>(smaps)), std::istreambuf_iterator<char>());
        std::error_code error;
        auto executable = std::filesystem::read_symlink("/proc/self/exe", error);
        if (auto pss = SmapsGamePssBytes(text, error ? std::string() : executable.string())) {
            return pss;
        }
    }
    // Without smaps the resident memory stands in, the libraries included.
    std::ifstream file("/proc/self/statm");
    std::string line;
    std::getline(file, line);
    std::optional<std::uint64_t> pages = StatmResidentPages(line);
    if (!pages) {
        return std::nullopt;
    }
    return *pages * static_cast<std::uint64_t>(sysconf(_SC_PAGESIZE));
#else
    return std::nullopt;
#endif
}

// The share of an NVIDIA GPU each process uses, from NVML, loaded at run time.
class Nvml {
public:
    explicit Nvml(const std::string &gpuName) {
#if defined(_WIN32)
        m_library = SDL_LoadObject("nvml.dll");
        m_pid = GetCurrentProcessId();
#elif defined(__linux__)
        m_library = SDL_LoadObject("libnvidia-ml.so.1");
        m_pid = static_cast<unsigned>(getpid());
#endif
        if (m_library == nullptr) {
            return;
        }
        auto init = Function<Return (*)()>("nvmlInit_v2");
        auto count = Function<Return (*)(unsigned *)>("nvmlDeviceGetCount_v2");
        auto handle = Function<Return (*)(unsigned, Device *)>("nvmlDeviceGetHandleByIndex_v2");
        auto name = Function<Return (*)(Device, char *, unsigned)>("nvmlDeviceGetName");
        m_shutdown = Function<Return (*)()>("nvmlShutdown");
        m_utilization =
            Function<Return (*)(Device, ProcessSample *, unsigned *, unsigned long long)>(
                "nvmlDeviceGetProcessUtilization");
        if (init == nullptr || count == nullptr || handle == nullptr || name == nullptr ||
            m_shutdown == nullptr || m_utilization == nullptr || init() != SUCCESS) {
            m_shutdown = nullptr;
            return;
        }
        unsigned devices = 0;
        if (count(&devices) != SUCCESS) {
            devices = 0;
        }
        std::vector<Device> handles;
        std::vector<std::string> names;
        for (unsigned index = 0; index < devices; ++index) {
            Device device = nullptr;
            std::array<char, 96> deviceName{};
            if (handle(index, &device) == SUCCESS &&
                name(device, deviceName.data(), static_cast<unsigned>(deviceName.size())) ==
                    SUCCESS) {
                handles.push_back(device);
                names.emplace_back(deviceName.data());
            }
        }
        if (const auto match = UniqueGpuIndex(gpuName, names)) {
            m_devices.push_back(handles[*match]);
            SDL_Log("Sampling GPU %s utilization with NVML.", gpuName.c_str());
        } else {
            SDL_Log("Cannot uniquely match NVML GPU; disabling utilization sampling.");
        }
    }

    ~Nvml() {
        if (m_shutdown != nullptr) {
            m_shutdown();
        }
        if (m_library != nullptr) {
            SDL_UnloadObject(m_library);
        }
    }

    Nvml(const Nvml &) = delete;
    Nvml &operator=(const Nvml &) = delete;

    // Whether NVML reports a GPU.
    bool Loaded() const {
        return !m_devices.empty();
    }

    // The share of the streaming multiprocessors the process used in its newest sample of any GPU
    // after @p lastSeen, the time of the newest sample read before in microseconds, which advances
    // to the newest sample read now. Only the GPU the game draws on has samples of its process.
    std::optional<unsigned> ProcessPercent(unsigned long long &lastSeen) const {
        std::optional<unsigned> percent;
        unsigned long long newestOwn = 0;
        unsigned long long newest = lastSeen;
        for (Device device : m_devices) {
            unsigned count = 0;
            Return counted = m_utilization(device, nullptr, &count, lastSeen);
            if ((counted != SUCCESS && counted != INSUFFICIENT_SIZE) || count == 0) {
                continue;
            }
            std::vector<ProcessSample> samples(count);
            if (m_utilization(device, samples.data(), &count, lastSeen) != SUCCESS) {
                continue;
            }
            for (unsigned index = 0; index < count && index < samples.size(); ++index) {
                const ProcessSample &sample = samples[index];
                newest = std::max(newest, sample.timeStamp);
                if (sample.pid == m_pid && sample.timeStamp >= newestOwn) {
                    newestOwn = sample.timeStamp;
                    percent = sample.smUtil;
                }
            }
        }
        lastSeen = newest;
        return percent;
    }

private:
    // The types of nvml.h.
    using Return = int;
    using Device = struct NvmlDevice *;
    struct ProcessSample {
        unsigned pid;                 /*!< The process. */
        unsigned long long timeStamp; /*!< The time of the sample in microseconds. */
        unsigned smUtil;  /*!< The percentage of the streaming multiprocessors in use. */
        unsigned memUtil; /*!< The percentage of the time the memory was read or written. */
        unsigned encUtil; /*!< The percentage of the encoder in use. */
        unsigned decUtil; /*!< The percentage of the decoder in use. */
    };
    static constexpr Return SUCCESS = 0;
    static constexpr Return INSUFFICIENT_SIZE = 7;

    template <typename Pointer>
    Pointer Function(const char *name) const {
        return reinterpret_cast<Pointer>(SDL_LoadFunction(m_library, name));
    }

    SDL_SharedObject *m_library = nullptr;
    unsigned m_pid = 0;
    std::vector<Device> m_devices;
    Return (*m_shutdown)() = nullptr;
    Return (*m_utilization)(Device, ProcessSample *, unsigned *, unsigned long long) = nullptr;
};

} // namespace

// NVML on a thread of its own, away from the frames.
class PerformanceStats::GpuSampler {
public:
    explicit GpuSampler(std::string gpuName)
        : m_thread([this, name = std::move(gpuName)] { Run(name); }) {
    }

    ~GpuSampler() {
        {
            std::lock_guard lock(m_mutex);
            m_stop = true;
        }
        m_wake.notify_one();
        m_thread.join();
    }

    GpuSampler(const GpuSampler &) = delete;
    GpuSampler &operator=(const GpuSampler &) = delete;

    std::optional<unsigned> Percent() {
        std::lock_guard lock(m_mutex);
        return m_percent;
    }

private:
    void Run(const std::string &gpuName) {
        Nvml nvml(gpuName);
        if (!nvml.Loaded()) {
            return;
        }
        unsigned long long lastSeen = 0;
        std::unique_lock lock(m_mutex);
        while (!m_wake.wait_for(lock, std::chrono::seconds(1), [this] { return m_stop; })) {
            lock.unlock();
            std::optional<unsigned> percent = nvml.ProcessPercent(lastSeen);
            lock.lock();
            m_percent = percent;
        }
    }

    std::mutex m_mutex;
    std::condition_variable m_wake;
    bool m_stop = false;
    std::optional<unsigned> m_percent;
    std::thread m_thread;
};

PerformanceStats::PerformanceStats(std::string gpuName, bool allowNvml) {
    if (allowNvml && !gpuName.empty()) {
        m_gpu = std::make_unique<GpuSampler>(std::move(gpuName));
    }
}

PerformanceStats::~PerformanceStats() = default;

void PerformanceStats::Frame(std::uint64_t workNanoseconds) {
    std::uint64_t now = SDL_GetTicksNS();
    const bool sampled = m_frameTimings.Record(workNanoseconds, now);
    if (m_secondStart == 0) {
        m_secondStart = now;
        m_secondStartCpu = ProcessCpuSeconds();
        m_memoryBytes = OwnMemoryBytes();
    }
    if (sampled) {
        std::optional<double> cpu = ProcessCpuSeconds();
        if (cpu && m_secondStartCpu) {
            m_cpuPercent = (*cpu - *m_secondStartCpu) /
                           (static_cast<double>(now - m_secondStart) / 1e9) * 100.0;
        }
        m_memoryBytes = OwnMemoryBytes();
        m_secondStart = now;
        m_secondStartCpu = cpu;
    }
}

std::vector<std::string> PerformanceStats::Lines(std::uint64_t gameGpuBytes) const {
    std::vector<std::string> lines;
    if (const auto &timing = m_frameTimings.Latest()) {
        lines.push_back(fmt::format("FPS: {:.2f}, WORST: {:.2f}", timing->fps, timing->worstFps));
        lines.push_back(fmt::format(
            "CPU WORK: {:.2f} MS, WORST: {:.2f} MS", timing->workMs, timing->worstWorkMs));
        lines.push_back(fmt::format(
            "INTERVAL: {:.4f} MS, WORST: {:.4f} MS", timing->intervalMs, timing->worstIntervalMs));
    }
    // The menu fonts draw a percent sign as a full stop, and PCT stands for it. The use of one
    // core comes first, and then of every core together.
    if (m_cpuPercent) {
        int cores = std::max(SDL_GetNumLogicalCPUCores(), 1);
        lines.push_back(
            fmt::format("CPU: {:.0f} PCT, {:.0f} PCT", *m_cpuPercent, *m_cpuPercent / cores));
    }
    if (m_memoryBytes) {
        lines.push_back("GAME RAM: " + MemorySize(*m_memoryBytes));
    }
    lines.push_back("GAME VRAM: " + MemorySize(gameGpuBytes));
    // The share of the GPU comes and goes with the samples of NVML, so it is the last line.
    std::optional<unsigned> gpu = m_gpu ? m_gpu->Percent() : std::nullopt;
    if (gpu) {
        lines.push_back(fmt::format("GPU: {} PCT", *gpu));
    }
    return lines;
}

} // namespace coastline
