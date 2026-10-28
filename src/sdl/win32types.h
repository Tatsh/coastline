#ifndef COASTLINE_SDL_WIN32TYPES_H
#define COASTLINE_SDL_WIN32TYPES_H

/**
 * @file
 * The data types of the Windows API that the code of the PC release declares its data with, at
 * their sizes on Windows. The SDL game has none of the API itself. A handle is an opaque pointer.
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t DWORD;     /*!< An unsigned 32-bit integer. */
typedef uint16_t WORD;      /*!< An unsigned 16-bit integer. */
typedef uint8_t BYTE;       /*!< An unsigned 8-bit integer. */
typedef int32_t LONG;       /*!< A signed 32-bit integer, which long is not on 64-bit Unix. */
typedef uint32_t ULONG;     /*!< An unsigned 32-bit integer. */
typedef unsigned int UINT;  /*!< An unsigned integer. */
typedef int BOOL;           /*!< A Boolean value in an integer. */
typedef char CHAR;          /*!< A character. */
typedef char *LPSTR;        /*!< A string. */
typedef const char *LPCSTR; /*!< A constant string. */
typedef void *LPVOID;       /*!< A pointer to anything. */
typedef int32_t HRESULT;    /*!< A COM result code. */
typedef uintptr_t WPARAM;   /*!< The first parameter of a window message. */
typedef intptr_t LPARAM;    /*!< The second parameter of a window message. */
typedef intptr_t LRESULT;   /*!< The result of a window message. */
typedef void *HANDLE;       /*!< A handle of a kernel object. */
typedef void *HWND;         /*!< A handle of a window. */
typedef void *HINSTANCE;    /*!< A handle of a module instance. */
typedef void *HMODULE;      /*!< A handle of a module. */

/** A rectangle, from its left and top to its right and bottom edges. */
typedef struct tagRECT {
    LONG left;   /*!< The x of the left edge. */
    LONG top;    /*!< The y of the top edge. */
    LONG right;  /*!< The x past the right edge. */
    LONG bottom; /*!< The y past the bottom edge. */
} RECT;

/** A point. */
typedef struct tagPOINT {
    LONG x; /*!< The x coordinate. */
    LONG y; /*!< The y coordinate. */
} POINT;

/** A signed 64-bit integer in two halves. */
typedef union _LARGE_INTEGER {
    struct {
        DWORD LowPart; /*!< The low 32 bits. */
        LONG HighPart; /*!< The high 32 bits. */
    } u;               /*!< The two halves. */
    int64_t QuadPart;  /*!< The whole value. */
} LARGE_INTEGER;

/** A device-independent bitmap of GDI. */
typedef struct tagBITMAP {
    LONG bmType;       /*!< 0. */
    LONG bmWidth;      /*!< The width in pixels. */
    LONG bmHeight;     /*!< The height in pixels. */
    LONG bmWidthBytes; /*!< The bytes of a row. */
    WORD bmPlanes;     /*!< The colour planes, 1. */
    WORD bmBitsPixel;  /*!< The bits of a pixel. */
    LPVOID bmBits;     /*!< The pixels. */
} BITMAP;

/** A globally unique identifier. */
typedef struct _GUID {
    DWORD Data1;   /*!< The first 32 bits. */
    WORD Data2;    /*!< The next 16 bits. */
    WORD Data3;    /*!< The next 16 bits. */
    BYTE Data4[8]; /*!< The last 64 bits. */
} GUID;

#ifndef TRUE
#define TRUE 1  /*!< The value of a true condition. */
#define FALSE 0 /*!< The value of a false condition. */
#endif

#define MAX_PATH 260                         /*!< The longest path of the Windows API. */
#define SUCCEEDED(hr) (((HRESULT)(hr)) >= 0) /*!< Whether a COM result is a success. */
#define FAILED(hr) (((HRESULT)(hr)) < 0)     /*!< Whether a COM result is a failure. */
#define S_OK ((HRESULT)0)                    /*!< The COM result of a success. */
#define E_FAIL ((HRESULT)0x80004005)         /*!< The COM result of an unspecified failure. */

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_SDL_WIN32TYPES_H */
