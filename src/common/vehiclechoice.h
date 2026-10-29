#ifndef COASTLINE_VEHICLECHOICE_H
#define COASTLINE_VEHICLECHOICE_H

#include "common/vehicle.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Car selection may preserve the current car, select a car, or copy the first player's model. */
typedef enum { CAR_CHOICE_UNCHANGED, CAR_CHOICE_SELECTED, CAR_CHOICE_COPIED } VehicleCarChoice;

/** Select a car from the current game mode and championship season. */
VehicleCarChoice ChooseVehicleCarId(Vehicle *vehicle, s32 *carId);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_VEHICLECHOICE_H */
