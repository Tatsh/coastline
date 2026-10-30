#ifndef COASTLINE_X87_H
#define COASTLINE_X87_H

#if !(defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__)))
#include <math.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The x87 instructions the PC release computes with, which keep the extended result and return a
 * large angle unchanged. A compiler or processor without them takes the functions of the C
 * library of the same precision.
 */

/**
 * The cosine of FCOS.
 *
 * @param angle The angle, in radians.
 * @return The cosine.
 */
static inline long double X87Cosine(long double angle) {
#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
    long double result;
    __asm__("fcos" : "=t"(result) : "0"(angle));
    return result;
#else
    return cosl(angle);
#endif
}

/**
 * The sine of FSIN.
 *
 * @param angle The angle, in radians.
 * @return The sine.
 */
static inline long double X87Sine(long double angle) {
#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
    long double result;
    __asm__("fsin" : "=t"(result) : "0"(angle));
    return result;
#else
    return sinl(angle);
#endif
}

/**
 * The square root of FSQRT.
 *
 * @param value The value.
 * @return The square root.
 */
static inline long double X87SquareRoot(long double value) {
#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
    long double result;
    __asm__("fsqrt" : "=t"(result) : "0"(value));
    return result;
#else
    return sqrtl(value);
#endif
}

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_X87_H */
