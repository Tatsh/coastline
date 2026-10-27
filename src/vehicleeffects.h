#ifndef COASTLINE_VEHICLEEFFECTS_H
#define COASTLINE_VEHICLEEFFECTS_H

/** The segments of the spray a wheel throws. */
#define SPRAY_SEGMENTS 9
/** The vertices of each segment of the spray, of which the first two are drawn. */
#define SPRAY_SEGMENT_VERTICES 4

#ifdef __cplusplus
extern "C" {
#endif

struct Vehicle;
/** Reset wheel spray and smoke timers. */
void ResetVehicleEffects(struct Vehicle *vehicle);

#ifdef __cplusplus
}
#endif

#endif
