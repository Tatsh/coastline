#include "vehicle.h"

#include <cmath>

#include "camerachange.h"

Vehicle g_aVehicles[16];

void Vehicle::ClearCameraChanged(void) {
    ::ClearCameraChanged(this);
}

void UpdateVehicleSpeedAndPhysics(Vehicle *vehicle) {
    if (vehicle->wheelBodies[1].contactFrames != 0) {
        const Vec3f &velocity = vehicle->body.state.velocity;
        const float squared =
            ((long double)velocity.x * velocity.x + (long double)velocity.y * velocity.y) +
            (long double)velocity.z * velocity.z;
        vehicle->speedKph = std::sqrt((long double)squared) * 2.24f;
    }
    vehicle->UpdateVehiclePhysicsFrame();
}
