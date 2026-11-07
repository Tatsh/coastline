#ifndef COASTLINE_VEHICLERESET_INTERNAL_H
#define COASTLINE_VEHICLERESET_INTERNAL_H

#include "common/vehicleaccess.h"

#define COPY_ORIENTATION_TO_STEPS(vehicle)                                                         \
    VEHICLE_GET((vehicle), Body).stepStates[1].orientation.w =                                     \
        VEHICLE_GET((vehicle), Body).state.orientation.w;                                          \
    VEHICLE_GET((vehicle), Body).stepStates[0].orientation.w =                                     \
        VEHICLE_GET((vehicle), Body).state.orientation.w;                                          \
    VEHICLE_GET((vehicle), Body).stepStates[1].orientation.x =                                     \
        VEHICLE_GET((vehicle), Body).state.orientation.x;                                          \
    VEHICLE_GET((vehicle), Body).stepStates[0].orientation.x =                                     \
        VEHICLE_GET((vehicle), Body).state.orientation.x;                                          \
    VEHICLE_GET((vehicle), Body).stepStates[1].orientation.y =                                     \
        VEHICLE_GET((vehicle), Body).state.orientation.y;                                          \
    VEHICLE_GET((vehicle), Body).stepStates[0].orientation.y =                                     \
        VEHICLE_GET((vehicle), Body).state.orientation.y;                                          \
    VEHICLE_GET((vehicle), Body).stepStates[1].orientation.z =                                     \
        VEHICLE_GET((vehicle), Body).state.orientation.z;                                          \
    VEHICLE_GET((vehicle), Body).stepStates[0].orientation.z =                                     \
        VEHICLE_GET((vehicle), Body).state.orientation.z

#endif
