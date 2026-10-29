#include <common/camera.h>
#include <common/track.h>
#include <common/trackrender.h>
#include <common/vehiclemodel.h>

u8 g_abInstanceVisibilityMask[TRACK_MAX_INSTANCES];
u16 g_awCameraRayHitInstances[RAY_INSTANCES];
s32 g_adwVehicleModelSlotCars[MAX_VEHICLES];
s32 g_adwVehicleVisible[MAX_VEHICLES];
u16 g_awVisibleInstanceList[TRACK_MAX_INSTANCES];
