#ifndef COASTLINE_VEHICLECHOICE_H
#define COASTLINE_VEHICLECHOICE_H

/**
 * @file
 * The choice of the car of a vehicle at the start of a race.
 */

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

/**
 * Select a car from the current game mode and championship season of the first player's profile.
 * A computer racer in arcade (or network in Boss Rally) copies the first player's car. Other
 * computer racers take a car of the season table. A local player in championship takes the first
 * car of the season, and a local player in another mode retains the current car.
 *
 * @param vehicle The vehicle whose car is chosen.
 * @param carId Receives the car, unwritten for CAR_CHOICE_UNCHANGED.
 * @return How the caller applies @p carId.
 */
VehicleCarChoice ChooseVehicleCarId(Vehicle *vehicle, s32 *carId);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_VEHICLECHOICE_H */
