#ifndef COASTLINE_VEHICLECHOICE_H
#define COASTLINE_VEHICLECHOICE_H

#include "common/vehicle.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Car selection may preserve the current car, select a car, or copy the first player's model. */
typedef enum {
    CAR_CHOICE_UNCHANGED, /*!< Preserve the car already selected. */
    CAR_CHOICE_SELECTED,  /*!< Use the newly selected car. */
    CAR_CHOICE_COPIED     /*!< Copy the first player's car. */
} VehicleCarChoice;

/** Select a car from the current game mode and championship season. */
VehicleCarChoice ChooseVehicleCarId(Vehicle *vehicle, s32 *carId);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_VEHICLECHOICE_H */
