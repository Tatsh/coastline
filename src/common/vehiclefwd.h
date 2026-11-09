#ifndef COASTLINE_VEHICLE_FORWARD_H
#define COASTLINE_VEHICLE_FORWARD_H

#if TGR_BOSS_RALLY && defined(__cplusplus)
/** A vehicle in the race simulation. */
class Vehicle;
#else
/** A vehicle in the race simulation. */
typedef struct Vehicle Vehicle;
#endif

#endif /* COASTLINE_VEHICLE_FORWARD_H */
