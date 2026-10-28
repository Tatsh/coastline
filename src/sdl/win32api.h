#ifndef COASTLINE_SDL_WIN32API_H
#define COASTLINE_SDL_WIN32API_H

/**
 * @file
 * The few calls of the Windows API that the code of the PC release makes outside its backends, on
 * SDL. Each behaves as the call does for the game.
 */

#include <ctype.h>

#include <SDL3/SDL.h>

#include "sdl/win32types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @param frequency Receives the counts a second of the performance counter.
 * @return TRUE.
 */
static inline BOOL QueryPerformanceFrequency(LARGE_INTEGER *frequency) {
    frequency->QuadPart = (int64_t)SDL_GetPerformanceFrequency();
    return TRUE;
}

/**
 * @param count Receives the performance counter.
 * @return TRUE.
 */
static inline BOOL QueryPerformanceCounter(LARGE_INTEGER *count) {
    count->QuadPart = (int64_t)SDL_GetPerformanceCounter();
    return TRUE;
}

/** @return The milliseconds since SDL started, wrapping as the multimedia timer does. */
static inline DWORD timeGetTime(void) {
    return (DWORD)SDL_GetTicks();
}

/**
 * Set the resolution of the multimedia timer, which SDL does not need.
 *
 * @param period The milliseconds.
 * @return 0, TIMERR_NOERROR.
 */
static inline UINT timeBeginPeriod(UINT period) {
    return 0;
}

/**
 * @param period The milliseconds.
 * @return 0, TIMERR_NOERROR.
 */
static inline UINT timeEndPeriod(UINT period) {
    return 0;
}

/**
 * Create a mutex, which the game creates and closes but never waits on.
 *
 * @return A handle that is not null.
 */
static inline HANDLE CreateMutexA(void *attributes, BOOL initialOwner, const char *name) {
    static char mutex;
    return &mutex;
}

#define LOWORD(value) ((WORD)((DWORD_PTR)(value) & 0xFFFF))         /*!< The low half. */
#define HIWORD(value) ((WORD)(((DWORD_PTR)(value) >> 16) & 0xFFFF)) /*!< The high half. */
#define WA_INACTIVE 0                                               /*!< A window deactivated. */
#define MB_OK 0x00000000                                            /*!< An OK button. */
#define MB_ICONERROR 0x00000010                                     /*!< An error icon. */

/**
 * Show a message to the player in a box of SDL.
 *
 * @param window Unused.
 * @param text The message.
 * @param caption The title.
 * @param type MB_ICONERROR for an error.
 * @return 1, IDOK.
 */
static inline int MessageBoxA(HWND window, const char *text, const char *caption, UINT type) {
    SDL_ShowSimpleMessageBox((type & MB_ICONERROR) ? SDL_MESSAGEBOX_ERROR :
                                                     SDL_MESSAGEBOX_INFORMATION,
                             caption,
                             text,
                             nullptr);
    return 1;
}

#define INFINITE 0xFFFFFFFF /*!< A wait without a time limit. */
#define WAIT_OBJECT_0 0     /*!< The result of a wait that acquired the object. */

/**
 * Wait for the mutex of CreateMutexA(), which one thread uses.
 *
 * @return WAIT_OBJECT_0.
 */
static inline DWORD WaitForSingleObject(HANDLE handle, DWORD milliseconds) {
    return WAIT_OBJECT_0;
}

/**
 * Wait for mutexes of CreateMutexA(), which one thread uses.
 *
 * @return WAIT_OBJECT_0.
 */
static inline DWORD
WaitForMultipleObjects(DWORD count, const HANDLE *handles, BOOL all, DWORD milliseconds) {
    return WAIT_OBJECT_0;
}

/**
 * Release the mutex of CreateMutexA().
 *
 * @return TRUE.
 */
static inline BOOL ReleaseMutex(HANDLE handle) {
    return TRUE;
}

/**
 * Close a handle of CreateMutexA().
 *
 * @return TRUE.
 */
static inline BOOL CloseHandle(HANDLE handle) {
    return TRUE;
}

/**
 * Write a message for the debugger to the SDL log.
 *
 * @param message The message.
 */
static inline void OutputDebugStringA(const char *message) {
    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "%s", message);
}

/**
 * Turn a string to upper case in place, as the C library of Microsoft does.
 *
 * @param text The string.
 * @return The string.
 */
static inline char *_strupr(char *text) {
    for (char *c = text; *c != '\0'; ++c) {
        *c = (char)toupper((unsigned char)*c);
    }
    return text;
}

/** @param milliseconds The milliseconds to wait. */
static inline void Sleep(DWORD milliseconds) {
    SDL_Delay(milliseconds);
}

/**
 * Find a library the process loaded. The SDL game loads none.
 *
 * @param name The name of the library.
 * @return Null.
 */
static inline HMODULE GetModuleHandleA(const char *name) {
    return nullptr;
}

/**
 * Load a library. The SDL game has none of the libraries of the PC release.
 *
 * @param name The name of the library.
 * @return Null.
 */
static inline HMODULE LoadLibraryA(const char *name) {
    return nullptr;
}

/**
 * @param module A library of LoadLibraryA().
 * @param name The name of a routine.
 * @return Null.
 */
static inline void *GetProcAddress(HMODULE module, const char *name) {
    return nullptr;
}

/**
 * Post a message to the main window. The SDL game handles the messages of the window procedure
 * of the PC release it needs at the start of its next frame. It is safe on any thread.
 *
 * @param window Unused.
 * @param message The message.
 * @param wParam The first parameter.
 * @param lParam The second parameter.
 * @return TRUE.
 */
BOOL PostMessageA(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

/**
 * @param name The name of the message.
 * @return The first message number a window registers, 0xC000.
 */
static inline UINT RegisterWindowMessageA(const char *name) {
    return 0xC000;
}

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_SDL_WIN32API_H */
