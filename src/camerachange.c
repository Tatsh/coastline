#include "camerachange.h"

#include "vehicle.h"

void ClearCameraChanged(Vehicle *vehicle) {
    vehicle->cameraChanged = 0;
}
