#ifndef LIBULTRA_GU_GUINT_H
#define LIBULTRA_GU_GUINT_H

/**
 * @file
 * The tables and constants of the trigonometric routines.
 */

#include <ultra64.h>

#ifndef MIN
/** The lesser of a and b, each evaluated twice. */
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

/** Clamp a direction component to a signed byte, where 1.0 is 128. */
#define FTOFRAC8(x) ((int)MIN(((x) * (128.0)), 127.0) & 0xff)

/** Round d half away from zero to an int. */
#define ROUND(d) (int)(((d) >= 0.0) ? ((d) + 0.5) : ((d) - 0.5))

/**
 * The first quarter of a sine wave in s1.15 fixed point.
 *
 * @ghidraAddress USA: 0x802a4c10
 */
extern const s16 sintable[0x400];

/**
 * The coefficients of the sine polynomial of sinf().
 *
 * @ghidraAddress USA: 0x802ab8a0
 */
extern const double g_adSinfP[5];

/** @ghidraAddress USA: 0x802ab8c8 */
extern const double g_dSinfRpi;

/** @ghidraAddress USA: 0x802ab8d0 */
extern const double g_dSinfPio2Hi;

/** @ghidraAddress USA: 0x802ab8d8 */
extern const double g_dSinfPio2Lo;

/** @ghidraAddress USA: 0x802ab8e0 */
extern const float g_fSinfZero;

/**
 * The coefficients of the sine polynomial of cosf().
 *
 * @ghidraAddress USA: 0x802ab8f0
 */
extern const double g_adCosfP[5];

/** @ghidraAddress USA: 0x802ab918 */
extern const double g_dCosfRpi;

/** @ghidraAddress USA: 0x802ab920 */
extern const double g_dCosfPio2Hi;

/** @ghidraAddress USA: 0x802ab928 */
extern const double g_dCosfPio2Lo;

/** @ghidraAddress USA: 0x802ab930 */
extern const float g_fCosfZero;

/** @ghidraAddress USA: 0x802aba80 */
extern const float __libm_qnan_f;

#endif
