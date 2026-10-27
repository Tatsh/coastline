#include "camerachange.h"

#include "vehicle.h"
#include "vehicleaccess.h"

void ClearCameraChanged(Vehicle *vehicle) {
    VEHICLE_SET(vehicle, CameraChanged, 0);
}
