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

/* The wave formats are packed to the byte in the headers of Windows, as in the files. */
#pragma pack(push, 1)
/** The format of a wave, as the multimedia API describes PCM. */
typedef struct tWAVEFORMATEX {
    WORD wFormatTag;       /*!< 1 for PCM. */
    WORD nChannels;        /*!< The channels. */
    DWORD nSamplesPerSec;  /*!< The frames a second. */
    DWORD nAvgBytesPerSec; /*!< The bytes a second. */
    WORD nBlockAlign;      /*!< The bytes of a frame. */
    WORD wBitsPerSample;   /*!< The bits of a sample. */
    WORD cbSize;           /*!< The bytes of extra format data. */
} WAVEFORMATEX;
#pragma pack(pop)

/** A chunk of a RIFF file, as the multimedia I/O API reads one. */
typedef struct _MMCKINFO {
    DWORD ckid;         /*!< The identifier of the chunk. */
    DWORD cksize;       /*!< The bytes of its data. */
    DWORD fccType;      /*!< The form or list type. */
    DWORD dwDataOffset; /*!< The offset of its data in the file. */
    DWORD dwFlags;      /*!< The flags. */
} MMCKINFO;

typedef uintptr_t UINT_PTR;  /*!< An unsigned integer the size of a pointer. */
typedef uintptr_t DWORD_PTR; /*!< An unsigned integer the size of a pointer. */
typedef char *HPSTR;         /*!< A string of the multimedia I/O API. */
typedef void *HMENU;         /*!< A handle of a menu. */
typedef void *HCURSOR;       /*!< A handle of a cursor. */
typedef void *HACCEL;        /*!< A handle of an accelerator table. */

/** A critical section of the Windows API, at its size on 64-bit Windows. */
typedef struct _CRITICAL_SECTION {
    void *DebugInfo;      /*!< The debugging information. */
    LONG LockCount;       /*!< The lock count. */
    LONG RecursionCount;  /*!< The recursion count. */
    HANDLE OwningThread;  /*!< The owning thread. */
    HANDLE LockSemaphore; /*!< The semaphore. */
    UINT_PTR SpinCount;   /*!< The spin count. */
} CRITICAL_SECTION;

typedef void *HMMIO;      /*!< A file of the multimedia I/O API. */
typedef UINT MCIDEVICEID; /*!< A device of the media control interface. */
typedef DWORD MCIERROR;   /*!< An error of the media control interface. */
typedef DWORD DPID;       /*!< A player of DirectPlay. */

/*
 * The DirectX interfaces and structures the declarations of the PC release name. The SDL game
 * does not use them, and they are incomplete.
 */
struct IDirectInputDeviceA;
struct IDirectInputDevice2A;
struct IDirectSound;
struct IDirectSoundBuffer;
struct IDirectPlay3A;
struct IDirectPlayLobby2A;
struct DIDEVICEINSTANCEA;
struct DIPROPDWORD;
struct DIPROPRANGE;
struct DIJOYSTATE2;
struct DPNAME;
struct DPSESSIONDESC2;
struct DPMSG_GENERIC;
struct DDSURFACEDESC;
typedef struct DDSURFACEDESC DDSURFACEDESC;
typedef struct IDirectInputDeviceA IDirectInputDeviceA;
typedef struct IDirectInputDevice2A IDirectInputDevice2A;
typedef struct IDirectSound IDirectSound;
typedef struct IDirectSoundBuffer IDirectSoundBuffer;
typedef struct IDirectPlay3A IDirectPlay3A;
typedef struct IDirectPlayLobby2A IDirectPlayLobby2A;
typedef struct DIDEVICEINSTANCEA DIDEVICEINSTANCEA;
typedef struct DIPROPDWORD DIPROPDWORD;
typedef struct DIPROPRANGE DIPROPRANGE;
typedef struct DIJOYSTATE2 DIJOYSTATE2;
typedef struct DPNAME DPNAME;
typedef struct DPSESSIONDESC2 DPSESSIONDESC2;
typedef struct DPMSG_GENERIC DPMSG_GENERIC;
typedef const DPNAME *LPCDPNAME;                 /*!< A constant DirectPlay name. */
typedef const DPSESSIONDESC2 *LPCDPSESSIONDESC2; /*!< A constant DirectPlay session. */
/** The callback of the enumeration of the players of a DirectPlay session. */
typedef BOOL (*LPDPENUMPLAYERSCALLBACK2)(
    DPID dpId, DWORD dwPlayerType, LPCDPNAME lpName, DWORD dwFlags, LPVOID lpContext);
#ifdef __cplusplus
typedef const GUID &REFGUID; /*!< A constant reference to a GUID. */
#else
typedef const GUID *REFGUID; /*!< A constant pointer to a GUID. */
#endif

#define WINAPI   /*!< The calling convention of the Windows API. */
#define CALLBACK /*!< The calling convention of a callback of the Windows API. */

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
