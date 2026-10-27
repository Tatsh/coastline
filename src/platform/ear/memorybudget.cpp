#include "memorybudget.h"

#include <windows.h>

#include "error.h"

namespace {
// @ghidraAddress EAR: 0x1003b7ec
std::uint32_t g_earSampleMemoryBudgetBytes;
// @ghidraAddress EAR: 0x1003b7f4
std::uint32_t g_dwSampleMemoryLimit;
// @ghidraAddress EAR: 0x1003b7f8
std::uint32_t g_dwSampleMemoryUsed;
} // namespace

int InitializeSampleMemoryBudget() {
    if (g_earSampleMemoryBudgetBytes > 0) {
        return 1;
    }
    MEMORYSTATUS status;
    GlobalMemoryStatus(&status);
    if (status.dwAvailPageFile < 512000) {
        SetEarLastError(0x10);
        return 0;
    }
    std::uint32_t blocks = status.dwAvailPageFile / 512000;
    int level = 3;
    bool complete = false;
    std::uint32_t threshold;
    do {
        threshold = 4 * level + 3;
        if (blocks <= threshold) {
            --level;
        } else {
            if (blocks - threshold < threshold) {
                --level;
            }
            complete = true;
        }
    } while (level > -1 && !complete);
    if (level == -1) {
        g_earSampleMemoryBudgetBytes = 512000;
        g_dwSampleMemoryLimit = 0;
    } else {
        g_earSampleMemoryBudgetBytes = threshold * 512000;
        g_dwSampleMemoryLimit = level * 1024000;
    }
    return 1;
}

std::uint32_t GetSampleMemoryLimit() {
    return g_dwSampleMemoryLimit;
}

std::uint32_t GetSampleMemoryUsed() {
    return g_dwSampleMemoryUsed;
}

int __stdcall ReserveSampleMemory(std::uint32_t bytes) {
    if (g_dwSampleMemoryUsed + bytes > g_dwSampleMemoryLimit) {
        return 0;
    }
    g_dwSampleMemoryUsed += bytes;
    return 1;
}

void __stdcall ReleaseSampleMemory(std::uint32_t bytes) {
    g_dwSampleMemoryUsed -= bytes;
}

int __stdcall SetSampleMemoryLimit(std::uint32_t bytes) {
    // The original accepts only limits no greater than current usage.
    if (bytes > g_dwSampleMemoryUsed) {
        return 0;
    }
    g_dwSampleMemoryLimit = bytes;
    return 1;
}
