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
 * @ghidraAddress EUR: 0x802a73a0
 * @ghidraAddress JPN: 0x802a6e40
 */
extern const s16 sintable[0x400];

/**
 * The coefficients of the sine polynomial of sinf().
 *
 * @ghidraAddress USA: 0x802ab8a0
 * @ghidraAddress EUR: 0x802ad590
 * @ghidraAddress JPN: 0x802abfd0
 */
extern const double g_adSinfP[5];

/**
 * @ghidraAddress USA: 0x802ab8c8
 * @ghidraAddress EUR: 0x802ad5b8
 * @ghidraAddress JPN: 0x802abff8
 */
extern const double g_dSinfRpi;

/**
 * @ghidraAddress USA: 0x802ab8d0
 * @ghidraAddress EUR: 0x802ad5c0
 * @ghidraAddress JPN: 0x802ac000
 */
extern const double g_dSinfPio2Hi;

/**
 * @ghidraAddress USA: 0x802ab8d8
 * @ghidraAddress EUR: 0x802ad5c8
 * @ghidraAddress JPN: 0x802ac008
 */
extern const double g_dSinfPio2Lo;

/**
 * @ghidraAddress USA: 0x802ab8e0
 * @ghidraAddress EUR: 0x802ad5d0
 * @ghidraAddress JPN: 0x802ac010
 */
extern const float g_fSinfZero;

/**
 * The coefficients of the sine polynomial of cosf().
 *
 * @ghidraAddress USA: 0x802ab8f0
 * @ghidraAddress EUR: 0x802ad5e0
 * @ghidraAddress JPN: 0x802ac020
 */
extern const double g_adCosfP[5];

/**
 * @ghidraAddress USA: 0x802ab918
 * @ghidraAddress EUR: 0x802ad608
 * @ghidraAddress JPN: 0x802ac048
 */
extern const double g_dCosfRpi;

/**
 * @ghidraAddress USA: 0x802ab920
 * @ghidraAddress EUR: 0x802ad610
 * @ghidraAddress JPN: 0x802ac050
 */
extern const double g_dCosfPio2Hi;

/**
 * @ghidraAddress USA: 0x802ab928
 * @ghidraAddress EUR: 0x802ad618
 * @ghidraAddress JPN: 0x802ac058
 */
extern const double g_dCosfPio2Lo;

/**
 * @ghidraAddress USA: 0x802ab930
 * @ghidraAddress EUR: 0x802ad620
 * @ghidraAddress JPN: 0x802ac060
 */
extern const float g_fCosfZero;

/**
 * @ghidraAddress USA: 0x802aba80
 * @ghidraAddress EUR: 0x802ad770
 * @ghidraAddress JPN: 0x802ac1b0
 */
extern const float __libm_qnan_f;

#endif
