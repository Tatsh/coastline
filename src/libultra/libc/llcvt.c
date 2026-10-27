#include <ultra64.h>

/* 2^63 and 2^64 as a double, and the top bit of a 64-bit integer. */
#define TWO_POW_63 9223372036854775808.0
#define TWO_POW_64 18446744073709551616.0
#define TOP_BIT ((unsigned long long)1 << 63)

/* 2^32, which the conversions from an unsigned integer add where 2^64 is due. */
#define TWO_POW_32 4294967296.0

long long __d_to_ll(double d) {
    return d;
}

long long __f_to_ll(float f) {
    return f;
}

/*
 * IDO compiles each conversion between a floating-point value and an unsigned 64-bit integer to
 * the inline sequence of the original. GCC converts correctly instead, and its form writes out the
 * results of that sequence.
 */
unsigned long long __d_to_ull(double d) {
#ifdef __GNUC__
    /* A value at or below -1, from 2^64, or a NaN makes the conversion invalid, and all ones is
     * returned. */
    if (d != d || d <= -1.0 || d >= TWO_POW_64) {
        return ~(unsigned long long)0;
    }
    if (d >= TWO_POW_63) {
        return (unsigned long long)(long long)(d - TWO_POW_63) | TOP_BIT;
    }
    return (long long)d;
#else
    return d;
#endif /* __GNUC__ */
}

unsigned long long __f_to_ull(float f) {
#ifdef __GNUC__
    /* A value at or below -1, from 2^64, or a NaN makes the conversion invalid, and all ones is
     * returned. */
    if (f != f || f <= -1.0f || f >= (float)TWO_POW_64) {
        return ~(unsigned long long)0;
    }
    if (f >= (float)TWO_POW_63) {
        return (unsigned long long)(long long)(f - (float)TWO_POW_63) | TOP_BIT;
    }
    return (long long)f;
#else
    return f;
#endif /* __GNUC__ */
}

double __ll_to_d(long long s) {
    return s;
}

float __ll_to_f(long long s) {
    return s;
}

double __ull_to_d(unsigned long long u) {
#ifdef __GNUC__
    double d = (long long)u;

    /* The correction for the top bit adds 2^32, not 2^64. */
    if ((long long)u < 0) {
        d += TWO_POW_32;
    }
    return d;
#else
    return u;
#endif /* __GNUC__ */
}

float __ull_to_f(unsigned long long u) {
#ifdef __GNUC__
    float f = (long long)u;

    /* The correction for the top bit adds 2^32, not 2^64. */
    if ((long long)u < 0) {
        f += (float)TWO_POW_32;
    }
    return f;
#else
    return u;
#endif /* __GNUC__ */
}
