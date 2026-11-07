#include "common/vehiclereset-internal.h"
#include "common/vehiclereset.h"

#if TGR_PLATFORM == TGR_PLATFORM_N64
#include <PR/gu.h>
#include <PR/os_libc.h>
#else
#include <math.h>
#endif

#define HALF_PI 1.570796371f

void SetVehiclePosition(Vehicle *vehicle, f32 x, f32 y, f32 z) {
    VEHICLE_GET(vehicle, Matrix)[3][0] = x;
    VEHICLE_GET(vehicle, Matrix)[3][1] = y;
    VEHICLE_GET(vehicle, Matrix)[3][2] = z;
    VEHICLE_GET(vehicle, SprayOrigin).x = x;
    VEHICLE_GET(vehicle, SprayOrigin).y = y;
    VEHICLE_GET(vehicle, SprayOrigin).z = z;
    VEHICLE_GET(vehicle, Body).state.position.x = x;
    VEHICLE_GET(vehicle, Body).state.position.y = y;
    VEHICLE_GET(vehicle, Body).state.position.z = z;
    VEHICLE_GET(vehicle, Body).stepStates[1].position.x = x;
    VEHICLE_GET(vehicle, Body).stepStates[1].position.y = y;
    VEHICLE_GET(vehicle, Body).stepStates[1].position.z = z;
    VEHICLE_GET(vehicle, Body).stepStates[0].position.x = x;
    VEHICLE_GET(vehicle, Body).stepStates[0].position.y = y;
    VEHICLE_GET(vehicle, Body).stepStates[0].position.z = z;
    QuaternionToMatrix(VEHICLE_GET(vehicle, Body).orientation, &VEHICLE_GET(vehicle, Body).state);
}

void InitVehicleOrientationFromAngle(Vehicle *vehicle, f32 angle) {
    f32 forwardX = cosf(angle);
    f32 forwardY = sinf(angle);
    f32 sideX = cosf(angle + HALF_PI);
    f32 sideY = sinf(angle + HALF_PI);
    f32 half = angle * 0.5f;
#if TGR_PLATFORM == TGR_PLATFORM_N64
    /* The original reserves stack for 44 bytes of locals it does not use. */
    f32 unused[11];

    (void)unused;
#endif
    VEHICLE_GET(vehicle, Matrix)[0][0] = forwardX;
    VEHICLE_GET(vehicle, Matrix)[0][1] = forwardY;
    VEHICLE_GET(vehicle, Matrix)[0][2] = 0.0f;
    VEHICLE_GET(vehicle, Matrix)[1][0] = sideX;
    VEHICLE_GET(vehicle, Matrix)[1][1] = sideY;
    VEHICLE_GET(vehicle, Matrix)[1][2] = 0.0f;
    VEHICLE_GET(vehicle, Matrix)[2][1] = 0.0f;
    VEHICLE_GET(vehicle, Matrix)[2][0] = 0.0f;
    VEHICLE_GET(vehicle, Matrix)[2][2] = 1.0f;
    VEHICLE_GET(vehicle, Body).state.orientation.w = cosf(half);
    VEHICLE_GET(vehicle, Body).state.orientation.y = 0.0f;
    VEHICLE_GET(vehicle, Body).state.orientation.x = 0.0f;
    VEHICLE_GET(vehicle, Body).state.orientation.z = sinf(half);
    COPY_ORIENTATION_TO_STEPS(vehicle);
    QuaternionToMatrix(VEHICLE_GET(vehicle, Body).orientation, &VEHICLE_GET(vehicle, Body).state);
}
