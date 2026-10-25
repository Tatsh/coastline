#ifndef COASTLINE_TYPES_H
#define COASTLINE_TYPES_H

/**
 * @file
 * The integer and floating-point types of the console, for headers shared with Windows.
 */

#if TGR_PLATFORM == TGR_PLATFORM_N64
#include <PR/ultratypes.h>
#else
#include <stddef.h>
#include <stdint.h>

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
#endif

#endif
