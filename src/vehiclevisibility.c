#include "camera.h"
#include "render.h"
#include "vehiclerender.h"
#include "visgrid.h"
#include "weather.h"

/* How far ahead of a vehicle its headlights are tested against the view, at night and in rain. */
#define HEADLIGHT_REACH 6.0f

void CheckVehicleVisibility(Vehicle *vehicle) {
    f32 distance;
    Vec3f ahead;

#if TGR_PLATFORM == TGR_PLATFORM_WIN
    if (vehicle->driver == NULL) {
        return;
    }
#endif

    /* The distance to the camera is computed and not used. */
    distance = Vec3fDistance(&vehicle->frame.position, &g_pActiveCamera->position);
    (void)distance;
    g_adwVehicleVisible[vehicle->index] = FALSE;
    g_adwVehicleInViewRange[vehicle->index] = FALSE;
    vehicle->fogFactor = ComputeFogFactorAtPoint(&vehicle->frame.position);
    if (vehicle != g_pActiveCameraVehicle) {
        if (g_dwNightMode || g_dwRainMode) {
            /* The beams of the headlights may be in view when the car is not. */
            Vec3fMulAdd(&ahead, &vehicle->frame.position, &vehicle->frame.forward, HEADLIGHT_REACH);
            if (!IsPointVisible(ahead.x, ahead.y) &&
                !IsPointVisible(vehicle->frame.position.x, vehicle->frame.position.y)) {
                return;
            }
        } else if (!IsPointVisible(vehicle->frame.position.x, vehicle->frame.position.y)) {
            return;
        }
    }
    if (vehicle == g_pActiveCameraVehicle &&
        (vehicle->activeCamera == &vehicle->cameras[VEHICLE_CAMERA_COCKPIT] ||
         vehicle->activeCamera == &vehicle->cameras[VEHICLE_CAMERA_MIRROR]) &&
        !g_dwVehicleCullingDisabled) {
        g_adwVehicleInViewRange[vehicle->index] = TRUE;
        return;
    }
    if (vehicle->ghostMode != VEHICLE_GHOST) {
        g_adwVehicleVisible[vehicle->index] = TRUE;
    }
    g_adwVehicleInViewRange[vehicle->index] = TRUE;
}
