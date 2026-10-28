#include <camera.h>
#include <track.h>
#include <trackrender.h>
#include <vehiclemodel.h>

u8 g_abInstanceVisibilityMask[TRACK_MAX_INSTANCES];
u16 g_awCameraRayHitInstances[RAY_INSTANCES];
s32 g_adwVehicleModelSlotCars[MAX_VEHICLES];
s32 g_adwVehicleVisible[MAX_VEHICLES];
u16 g_awVisibleInstanceList[TRACK_MAX_INSTANCES];
