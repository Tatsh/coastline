#ifndef COASTLINE_RACE_SPECIALS_H
#define COASTLINE_RACE_SPECIALS_H

#ifdef __cplusplus
extern "C" {
#endif
/** Find the airplane, its path and trigger, and the waterfalls of the track. */
void FindTrackSpecials(void);
/** Animate the airplane, waterfalls, and rotating track instances. */
void UpdateTrackSpecials(void);

#ifdef __cplusplus
}
#endif
#endif
