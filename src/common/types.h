#ifndef COASTLINE_TYPES_H
#define COASTLINE_TYPES_H

/**
 * @file
 * The integer and floating-point types of the console, for headers shared with Windows.
 */

#if TGR_PLATFORM == TGR_PLATFORM_N64
#include <PR/ultratypes.h>
#elif TGR_PLATFORM == TGR_PLATFORM_SDL
/*
 * The SDL game includes the headers of libultra beside this one, and both declare the types the
 * same way. uint64_t is unsigned long on 64-bit Unix, where libultra writes unsigned long long.
 */
#include <stddef.h>
#include <stdint.h>

#include <PR/ultratypes.h>

#ifndef TRUE
#define TRUE 1  /*!< The value of a true condition. */
#define FALSE 0 /*!< The value of a false condition. */
#endif

/* The integer types of Windows that the code of the PC release uses, at their 32-bit widths. */
typedef int BOOL;            /*!< A condition, TRUE or FALSE. */
typedef unsigned short WORD; /*!< An unsigned 16-bit integer. */
typedef uint32_t DWORD;      /*!< An unsigned 32-bit integer. */
typedef unsigned int UINT;   /*!< An unsigned integer. */
typedef int32_t LONG;        /*!< A signed 32-bit integer. */

/* The calling conventions of 32-bit x86 do not exist on other processors. */
#if !defined(_WIN32) || defined(_WIN64)
#define __cdecl
#define __stdcall
#define __thiscall
#define __fastcall
#endif
#else
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef TRUE
#define TRUE 1  /*!< The value of a true condition. */
#define FALSE 0 /*!< The value of a false condition. */
#endif

typedef uint8_t u8;   /*!< An unsigned 8-bit integer. */
typedef uint16_t u16; /*!< An unsigned 16-bit integer. */
typedef uint32_t u32; /*!< An unsigned 32-bit integer. */
typedef uint64_t u64; /*!< An unsigned 64-bit integer. */

typedef int8_t s8;   /*!< A signed 8-bit integer. */
typedef int16_t s16; /*!< A signed 16-bit integer. */
typedef int32_t s32; /*!< A signed 32-bit integer. */
typedef int64_t s64; /*!< A signed 64-bit integer. */

typedef float f32;  /*!< A 32-bit floating-point number. */
typedef double f64; /*!< A 64-bit floating-point number. */

#ifdef __cplusplus
}
#endif

#endif /* TGR_PLATFORM */

/*
 * Marks a shared helper the compiler of Boss Rally inlined into its caller. The Nintendo 64 build
 * compiles the helper as an ordinary function.
 */
#if TGR_BOSS_RALLY
#define BOSS_RALLY_INLINE inline
#else
#define BOSS_RALLY_INLINE
#endif

#endif /* COASTLINE_TYPES_H */
