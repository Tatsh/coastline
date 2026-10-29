#include "common/camerachange.h"

#include "common/vehicle.h"
#include "common/vehicleaccess.h"

void ClearCameraChanged(Vehicle *vehicle) {
    VEHICLE_SET(vehicle, CameraChanged, 0);
}
