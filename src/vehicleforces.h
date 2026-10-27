#ifndef COASTLINE_VEHICLEFORCES_H
#define COASTLINE_VEHICLEFORCES_H

#ifdef __cplusplus
extern "C" {
#endif

/** The forces on the bodies of a vehicle. */
enum {
    VEHICLE_FORCE_SPRING_0, /*!< The spring of the rear wheel of the first side, first in the list
                                 of the body for its first pass. */
    VEHICLE_FORCE_SPRING_1, /*!< The spring of the front wheel of the first side. */
    VEHICLE_FORCE_SPRING_2, /*!< The spring of the rear wheel of the second side. */
    VEHICLE_FORCE_SPRING_3, /*!< The spring of the front wheel of the second side. */
    VEHICLE_FORCE_DAMPER_0, /*!< The damper of the rear wheel of the first side, first in the list
                                 of the body for its second pass. */
    VEHICLE_FORCE_DAMPER_1, /*!< The damper of the front wheel of the first side. */
    VEHICLE_FORCE_DAMPER_2, /*!< The damper of the rear wheel of the second side. */
    VEHICLE_FORCE_DAMPER_3, /*!< The damper of the front wheel of the second side. */
    VEHICLE_FORCE_GRAVITY,  /*!< The gravity of the body, last in the list of its first pass. */
    VEHICLE_FORCE_FRONT_WHEEL_GRAVITY, /*!< The gravity of the front wheels. */
    VEHICLE_FORCE_REAR_WHEEL_GRAVITY,  /*!< The gravity of the rear wheels. */
    VEHICLE_FORCE_DRAG,    /*!< The drag of the body, last in the list of its second pass. */
    VEHICLE_FORCE_WHEEL_0, /*!< The drive of wheelBodies[0], before the gravity of its wheel. */
    VEHICLE_FORCE_WHEEL_1, /*!< The drive of wheelBodies[1], before the gravity of its wheel. */
    VEHICLE_FORCE_WHEEL_2, /*!< The drive of wheelBodies[2], before the gravity of its wheel. */
    VEHICLE_FORCE_WHEEL_3, /*!< The drive of wheelBodies[3], before the gravity of its wheel. */
    VEHICLE_FORCES,        /*!< The number of forces of a vehicle. */
};

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_VEHICLEFORCES_H */
