#include "platform/win/diagnostics.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

// DbgHelp requires Windows declarations first.
// clang-format off
#include <windows.h>
#include <dbghelp.h>
#include <tlhelp32.h>
// clang-format on

#ifdef ENABLE_PATCHES
static LPTOP_LEVEL_EXCEPTION_FILTER previousFilter;
static PVOID exceptionObserver;
static LONG reportingCrash;
static bool initialized;
static HMODULE debugLibrary;
static char logPath[MAX_PATH];
static char dumpPath[MAX_PATH];

typedef BOOL(WINAPI *WriteDumpProc)(HANDLE,
                                    DWORD,
                                    HANDLE,
                                    MINIDUMP_TYPE,
                                    PMINIDUMP_EXCEPTION_INFORMATION,
                                    PMINIDUMP_USER_STREAM_INFORMATION,
                                    PMINIDUMP_CALLBACK_INFORMATION);
static WriteDumpProc writeDump;

static void SetDiagnosticPath(char *path, const char *file) {
    DWORD length = GetModuleFileNameA(nullptr, path, MAX_PATH);
    if (!length || length >= MAX_PATH) {
        path[0] = '\0';
        return;
    }
    char *separator = strrchr(path, '\\');
    if (!separator || (size_t)(separator + 1 - path) + strlen(file) >= MAX_PATH) {
        path[0] = '\0';
        return;
    }
    strcpy(separator + 1, file);
}

static void WriteCrashStack(const CONTEXT *context) {
    char line[256];
    DWORD frame = context->Ebp;
    DWORD words[64];
    SIZE_T count = 0;
    for (unsigned index = 0; frame && index < 64; ++index) {
        DWORD record[2];
        if (!ReadProcessMemory(GetCurrentProcess(),
                               (const void *)(uintptr_t)frame,
                               record,
                               sizeof(record),
                               &count) ||
            count != sizeof(record)) {
            break;
        }
        snprintf(
            line, sizeof(line), "frame %u return=0x%08lx frame=0x%08lx\n", index, record[1], frame);
        WriteApplicationDiagnostic(line);
        if (record[0] <= frame || record[0] - frame > 0x100000) {
            break;
        }
        frame = record[0];
    }
    count = 0;
    ReadProcessMemory(
        GetCurrentProcess(), (const void *)(uintptr_t)context->Esp, words, sizeof(words), &count);
    for (unsigned index = 0; index < count / sizeof(words[0]); ++index) {
        snprintf(line,
                 sizeof(line),
                 "stack 0x%08lx = 0x%08lx\n",
                 context->Esp + index * 4,
                 words[index]);
        WriteApplicationDiagnostic(line);
    }
}

static void WriteExceptionDetails(EXCEPTION_POINTERS *exception) {
    char line[1024];
    EXCEPTION_RECORD *record = exception->ExceptionRecord;
    CONTEXT *context = exception->ContextRecord;
    snprintf(line,
             sizeof(line),
             "code=0x%08lx address=%p thread=%lu build=%s %s %s\n",
             record->ExceptionCode,
             record->ExceptionAddress,
             GetCurrentThreadId(),
             COASTLINE_GIT_SHA,
             __DATE__,
             __TIME__);
    WriteApplicationDiagnostic(line);
    for (DWORD index = 0; index < record->NumberParameters; ++index) {
        snprintf(line,
                 sizeof(line),
                 "exception parameter %lu = 0x%08lx\n",
                 index,
                 (unsigned long)record->ExceptionInformation[index]);
        WriteApplicationDiagnostic(line);
    }
    snprintf(line,
             sizeof(line),
             "EIP=%08lx ESP=%08lx EBP=%08lx EFLAGS=%08lx\n"
             "EAX=%08lx EBX=%08lx ECX=%08lx EDX=%08lx ESI=%08lx EDI=%08lx\n",
             context->Eip,
             context->Esp,
             context->Ebp,
             context->EFlags,
             context->Eax,
             context->Ebx,
             context->Ecx,
             context->Edx,
             context->Esi,
             context->Edi);
    WriteApplicationDiagnostic(line);
    WriteCrashStack(context);
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
    if (snapshot != INVALID_HANDLE_VALUE) {
        MODULEENTRY32 entry = {0};
        entry.dwSize = sizeof(entry);
        if (Module32First(snapshot, &entry)) {
            do {
                snprintf(line,
                         sizeof(line),
                         "module %p size=0x%08lx %s\n",
                         entry.modBaseAddr,
                         entry.modBaseSize,
                         entry.szExePath);
                WriteApplicationDiagnostic(line);
            } while (Module32Next(snapshot, &entry));
        }
        CloseHandle(snapshot);
    }
}

static LONG WINAPI ObserveApplicationException(EXCEPTION_POINTERS *exception) {
    DWORD code = exception->ExceptionRecord->ExceptionCode;
    if (code != EXCEPTION_ACCESS_VIOLATION && code != EXCEPTION_ILLEGAL_INSTRUCTION &&
        code != EXCEPTION_INT_DIVIDE_BY_ZERO) {
        return EXCEPTION_CONTINUE_SEARCH;
    }
    if (!InterlockedCompareExchange(&reportingCrash, 1, 0)) {
        DWORD savedError = GetLastError();
        WriteApplicationDiagnostic("First-chance fault; another handler may handle it.\n");
        WriteExceptionDetails(exception);
        InterlockedExchange(&reportingCrash, 0);
        SetLastError(savedError);
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

static LONG WINAPI ReportApplicationCrash(EXCEPTION_POINTERS *exception) {
    if (InterlockedCompareExchange(&reportingCrash, 1, 0)) {
        return EXCEPTION_CONTINUE_SEARCH;
    }
    WriteApplicationDiagnostic("Unhandled exception.\n");
    WriteExceptionDetails(exception);
    if (writeDump && dumpPath[0]) {
        HANDLE dump = CreateFileA(dumpPath,
                                  GENERIC_WRITE,
                                  FILE_SHARE_READ,
                                  nullptr,
                                  CREATE_ALWAYS,
                                  FILE_ATTRIBUTE_NORMAL,
                                  nullptr);
        if (dump != INVALID_HANDLE_VALUE) {
            MINIDUMP_EXCEPTION_INFORMATION info = {GetCurrentThreadId(), exception, FALSE};
            BOOL result = writeDump(GetCurrentProcess(),
                                    GetCurrentProcessId(),
                                    dump,
                                    MiniDumpNormal,
                                    &info,
                                    nullptr,
                                    nullptr);
            CloseHandle(dump);
            WriteApplicationDiagnostic(result ? "Crash minidump written.\n" :
                                                "Crash minidump failed.\n");
        }
    }
    WriteApplicationDiagnostic("Crash report complete; passing exception to the system.\n");
    InterlockedExchange(&reportingCrash, 0);
    return previousFilter ? previousFilter(exception) : EXCEPTION_CONTINUE_SEARCH;
}

void WriteApplicationDiagnostic(const char *message) {
    DWORD savedError = GetLastError();
    if (!logPath[0]) {
        SetDiagnosticPath(logPath, "coastline.log");
    }
    HANDLE log = CreateFileA(logPath,
                             FILE_APPEND_DATA,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             nullptr,
                             OPEN_ALWAYS,
                             FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,
                             nullptr);
    if (log != INVALID_HANDLE_VALUE) {
        char prefix[96];
        SYSTEMTIME time;
        DWORD written;
        GetSystemTime(&time);
        int length = snprintf(prefix,
                              sizeof(prefix),
                              "%04u-%02u-%02u %02u:%02u:%02u.%03u UTC [%lu:%lu] ",
                              time.wYear,
                              time.wMonth,
                              time.wDay,
                              time.wHour,
                              time.wMinute,
                              time.wSecond,
                              time.wMilliseconds,
                              GetCurrentProcessId(),
                              GetCurrentThreadId());
        WriteFile(log, prefix, length, &written, nullptr);
        WriteFile(log, message, (DWORD)strlen(message), &written, nullptr);
        CloseHandle(log);
    }
    SetLastError(savedError);
}

void InitializeApplicationDiagnostics(void) {
    if (initialized) {
        return;
    }
    initialized = true;
    SetDiagnosticPath(logPath, "coastline.log");
    SetDiagnosticPath(dumpPath, "coastline-crash.dmp");
    HANDLE log = CreateFileA(logPath,
                             GENERIC_WRITE,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             nullptr,
                             CREATE_ALWAYS,
                             FILE_ATTRIBUTE_NORMAL,
                             nullptr);
    if (log != INVALID_HANDLE_VALUE) {
        CloseHandle(log);
    }
    previousFilter = SetUnhandledExceptionFilter(ReportApplicationCrash);
    exceptionObserver = AddVectoredExceptionHandler(1, ObserveApplicationException);
    WriteApplicationDiagnostic("Application diagnostics initialized.\n");
    WriteApplicationDiagnostic("github.com/Tatsh/coastline " COASTLINE_GIT_SHA " " __DATE__
                               " " __TIME__ "\n");
    debugLibrary = LoadLibraryA("dbghelp.dll");
    if (debugLibrary) {
        writeDump = (WriteDumpProc)GetProcAddress(debugLibrary, "MiniDumpWriteDump");
    }
    WriteApplicationDiagnostic(writeDump ? "Crash minidump support ready.\n" :
                                           "Crash minidump support unavailable.\n");
}

void ShutdownApplicationDiagnostics(void) {
    if (!initialized) {
        return;
    }
    SetUnhandledExceptionFilter(previousFilter);
    if (exceptionObserver) {
        RemoveVectoredExceptionHandler(exceptionObserver);
        exceptionObserver = nullptr;
    }
    if (debugLibrary) {
        FreeLibrary(debugLibrary);
        debugLibrary = nullptr;
    }
    writeDump = nullptr;
    initialized = false;
}
#endif /* ENABLE_PATCHES */
