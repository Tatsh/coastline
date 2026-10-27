#ifndef COASTLINE_VEHICLECAMERA_H
#define COASTLINE_VEHICLECAMERA_H

/** The cameras of a vehicle. */
enum {
    VEHICLE_CAMERA_COCKPIT,   /*!< The view from the driver's seat. */
    VEHICLE_CAMERA_CHASE,     /*!< The camera that follows behind the vehicle. */
    VEHICLE_CAMERA_BUMPER,    /*!< The cockpit camera moved forward to the bumper. */
    VEHICLE_CAMERA_TRACKSIDE, /*!< A camera by the track that watches the vehicle and zooms with its
                                   distance. */
    VEHICLE_CAMERA_4,         /*!< A camera the routines reconstructed so far do not use. */
    VEHICLE_CAMERA_MIRROR,    /*!< The rear-view mirror. */
    VEHICLE_CAMERAS,          /*!< The number of cameras of a vehicle. */
};

#endif
