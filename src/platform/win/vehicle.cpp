#include "vehicle.h"

#include <cmath>

#include "camerachange.h"
#include "vehicleaccess.h"

Vehicle g_aVehicles[16];

void Vehicle::ClearCameraChanged(void) {
    ::ClearCameraChanged(this);
}

void UpdateVehicleSpeedAndPhysics(Vehicle *vehicle) {
    if (VEHICLE_GET(vehicle, WheelBodies)[1].contactFrames != 0) {
        const Vec3f &velocity = VEHICLE_GET(vehicle, Body).state.velocity;
        const float squared =
            ((long double)velocity.x * velocity.x + (long double)velocity.y * velocity.y) +
            (long double)velocity.z * velocity.z;
        VEHICLE_SET(vehicle, SpeedKph, std::sqrt((long double)squared) * 2.24f);
    }
    vehicle->UpdateVehiclePhysicsFrame();
}
