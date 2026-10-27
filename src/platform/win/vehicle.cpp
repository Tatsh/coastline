#include "vehicle.h"

#include <cmath>

#include "camerachange.h"
#include "vehicleaccess.h"
#include "vehicledeform.h"
#include "vehiclestatus.h"
#include "vehicletrackpos.h"

Vehicle g_aVehicles[16];

void Vehicle::DeformVehicleMeshByImpact(Vec3f *direction, s16 amount) {
    ::DeformVehicleMeshByImpact(this, direction, amount);
}

s32 Vehicle::UpdateVehicleTrackPosition() {
    return ::UpdateVehicleTrackPosition(this);
}

void Vehicle::UpdateRaceTimers() {
    ::UpdateRaceTimers(this);
}
void Vehicle::UpdateHudMessageTimers() {
    ::UpdateHudMessageTimers(this);
}
void Vehicle::CheckWrongWayWarning() {
    ::CheckWrongWayWarning(this);
}
void Vehicle::UpdateVehicleGridPosition() {
    ::UpdateVehicleGridPosition(this);
}

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
