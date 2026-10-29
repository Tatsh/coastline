#include "common/camera.h"
#include "common/render.h"
#include "common/vehicleaccess.h"
#include "common/vehiclerender.h"
#include "common/visgrid.h"
#include "common/weather.h"

/* How far ahead of a vehicle its headlights are tested against the view, at night and in rain. */
#define HEADLIGHT_REACH 6.0f

void CheckVehicleVisibility(Vehicle *vehicle) {
    f32 distance;
    Vec3f ahead;

#if TGR_BOSS_RALLY
    if (VEHICLE_GET(vehicle, Driver) == NULL) {
        return;
    }
#endif

    /* The distance to the camera is computed and not used. */
    distance = Vec3fDistance(&VEHICLE_GET(vehicle, Frame).position, &g_pActiveCamera->position);
    (void)distance;
    g_adwVehicleVisible[VEHICLE_GET(vehicle, Index)] = FALSE;
    g_adwVehicleInViewRange[VEHICLE_GET(vehicle, Index)] = FALSE;
    VEHICLE_SET(vehicle, FogFactor, ComputeFogFactorAtPoint(&VEHICLE_GET(vehicle, Frame).position));
    if (vehicle != g_pActiveCameraVehicle) {
        if (g_dwNightMode || g_dwRainMode) {
            /* The beams of the headlights may be in view when the car is not. */
            Vec3fMulAdd(&ahead,
                        &VEHICLE_GET(vehicle, Frame).position,
                        &VEHICLE_GET(vehicle, Frame).forward,
                        HEADLIGHT_REACH);
            if (!IsPointVisible(ahead.x, ahead.y) &&
                !IsPointVisible(VEHICLE_GET(vehicle, Frame).position.x,
                                VEHICLE_GET(vehicle, Frame).position.y)) {
                return;
            }
        } else if (!IsPointVisible(VEHICLE_GET(vehicle, Frame).position.x,
                                   VEHICLE_GET(vehicle, Frame).position.y)) {
            return;
        }
    }
    if (vehicle == g_pActiveCameraVehicle &&
        (VEHICLE_GET(vehicle, ActiveCamera) ==
             &VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_COCKPIT] ||
         VEHICLE_GET(vehicle, ActiveCamera) ==
             &VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_MIRROR]) &&
        !g_dwVehicleCullingDisabled) {
        g_adwVehicleInViewRange[VEHICLE_GET(vehicle, Index)] = TRUE;
        return;
    }
    if (VEHICLE_GET(vehicle, GhostMode) != VEHICLE_GHOST) {
        g_adwVehicleVisible[VEHICLE_GET(vehicle, Index)] = TRUE;
    }
    g_adwVehicleInViewRange[VEHICLE_GET(vehicle, Index)] = TRUE;
}
