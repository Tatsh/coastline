#ifndef PR_ULTRATYPES_H
#define PR_ULTRATYPES_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * Integer and floating-point types of libultra.
 */

typedef unsigned char u8;       /*!< An unsigned 8-bit integer. */
typedef unsigned short u16;     /*!< An unsigned 16-bit integer. */
typedef unsigned int u32;       /*!< An unsigned 32-bit integer. */
typedef unsigned long long u64; /*!< An unsigned 64-bit integer. */

typedef signed char s8; /*!< A signed 8-bit integer. */
typedef short s16;      /*!< A signed 16-bit integer. */
typedef int s32;        /*!< A signed 32-bit integer. */
typedef long long s64;  /*!< A signed 64-bit integer. */

typedef volatile unsigned char vu8;       /*!< A volatile unsigned 8-bit integer. */
typedef volatile unsigned short vu16;     /*!< A volatile unsigned 16-bit integer. */
typedef volatile unsigned int vu32;       /*!< A volatile unsigned 32-bit integer. */
typedef volatile unsigned long long vu64; /*!< A volatile unsigned 64-bit integer. */

typedef volatile signed char vs8; /*!< A volatile signed 8-bit integer. */
typedef volatile short vs16;      /*!< A volatile signed 16-bit integer. */
typedef volatile int vs32;        /*!< A volatile signed 32-bit integer. */
typedef volatile long long vs64;  /*!< A volatile signed 64-bit integer. */

typedef float f32;  /*!< A 32-bit floating-point number. */
typedef double f64; /*!< A 64-bit floating-point number. */

#ifndef NULL
#define NULL 0 /*!< The null pointer. */
#endif

#ifndef TRUE
#define TRUE 1  /*!< The value of a true condition. */
#define FALSE 0 /*!< The value of a false condition. */
#endif

#ifdef __cplusplus
}
#endif

#endif /* PR_ULTRATYPES_H */
