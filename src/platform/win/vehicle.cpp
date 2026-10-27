#include "vehicle.h"

#include "camerachange.h"

Vehicle g_aVehicles[16];

void Vehicle::ClearCameraChanged(void) {
    ::ClearCameraChanged(this);
}
