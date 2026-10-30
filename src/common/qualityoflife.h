#ifndef COASTLINE_QUALITYOFLIFE_H
#define COASTLINE_QUALITYOFLIFE_H

/**
 * @file
 * The arithmetic and the parsing of the quality of life changes of the patched PC libraries.
 */

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Record a press and report whether it follows an earlier press within a window.
 *
 * A second press consumes both presses, and the next press starts a new pair.
 *
 * @param pLastPress The time of the unpaired earlier press, or 0 for none. Updated.
 * @param now The time of this press.
 * @param window The most time between the two presses, in the unit of the times.
 * @return Whether this press is the second of a pair.
 */
bool IsSecondPressWithin(unsigned *pLastPress, unsigned now, unsigned window);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_QUALITYOFLIFE_H */
