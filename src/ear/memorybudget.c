#include "memorybudget.h"

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#include <windows.h>
#else
#include <SDL3/SDL.h>
#endif

#include "error.h"

// @ghidraAddress EAR: 0x1003b7ec
static uint32_t g_earSampleMemoryBudgetBytes;
// @ghidraAddress EAR: 0x1003b7f4
static uint32_t g_dwSampleMemoryLimit;
// @ghidraAddress EAR: 0x1003b7f8
static uint32_t g_dwSampleMemoryUsed;

int InitializeSampleMemoryBudget(void) {
    if (g_earSampleMemoryBudgetBytes > 0) {
        return 1;
    }
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    MEMORYSTATUS status;
    GlobalMemoryStatus(&status);
    if (status.dwAvailPageFile < 512000) {
        SetEarLastError(0x10);
        return 0;
    }
    uint32_t blocks = status.dwAvailPageFile / 512000;
#else
    // The memory of the computer stands for the free page file, counted in 32 bits as Windows
    // counts it.
    uint64_t memory = (uint64_t)SDL_GetSystemRAM() * 1048576;
    uint32_t available = memory > UINT32_MAX ? UINT32_MAX : (uint32_t)memory;
    if (available < 512000) {
        SetEarLastError(0x10);
        return 0;
    }
    uint32_t blocks = available / 512000;
#endif
    int level = 3;
    bool complete = false;
    uint32_t threshold;
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

uint32_t GetSampleMemoryLimit(void) {
    return g_dwSampleMemoryLimit;
}

uint32_t GetSampleMemoryUsed(void) {
    return g_dwSampleMemoryUsed;
}

int __stdcall ReserveSampleMemory(uint32_t bytes) {
    if (g_dwSampleMemoryUsed + bytes > g_dwSampleMemoryLimit) {
        return 0;
    }
    g_dwSampleMemoryUsed += bytes;
    return 1;
}

void __stdcall ReleaseSampleMemory(uint32_t bytes) {
    g_dwSampleMemoryUsed -= bytes;
}

int __stdcall SetSampleMemoryLimit(uint32_t bytes) {
    // The original accepts only limits no greater than current usage.
    if (bytes > g_dwSampleMemoryUsed) {
        return 0;
    }
    g_dwSampleMemoryLimit = bytes;
    return 1;
}
