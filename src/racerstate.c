#include "frametime.h"
#include "pause.h"
#include "racers.h"
#include "racerslot.h"
#include "racerslotaccess.h"
#include "vehicleaccess.h"
#include "vehicledeform.h"
#include "vehiclestatus.h"

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#include "platform/win/input.h"
#define VEHICLE_RACE_CALL(vehicle, method) ((vehicle)->method())
#define RACER_RECIPROCAL(value) ((f32)(1.0L / (value)))

void RacerSlot::UpdateRacerSlotRaceState(void) {
    RacerSlot *slot = this;
    s32 impactDegrees;
#else
#define VEHICLE_RACE_CALL(vehicle, method) method(vehicle)
#define RACER_RECIPROCAL(value) (1.0f / (value))

void UpdateRacerSlotRaceState(RacerSlot *slot) {
#endif
    if (g_dwRacePauseState || RACER_SLOT_GET(slot, Vehicle) == NULL) {
        return;
    }
    if (VEHICLE_GET(RACER_SLOT_GET(slot, Vehicle), Body).impactSpeed) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        impactDegrees =
            (s32)(s64)(Atan2f(VEHICLE_GET(RACER_SLOT_GET(slot, Vehicle), Body).impactPoint.x,
                              VEHICLE_GET(RACER_SLOT_GET(slot, Vehicle), Body).impactPoint.y) *
                       57.2957763671875f);
        SetForceFeedbackDirection(impactDegrees > 180 ? 1 : -1);
        if (VEHICLE_GET(RACER_SLOT_GET(slot, Vehicle), Body).impactSpeed >= 10) {
            RACER_SLOT_GET(slot, Vehicle)
                ->DeformVehicleMeshByImpact(
                    &VEHICLE_GET(RACER_SLOT_GET(slot, Vehicle), Body).impactPoint,
                    VEHICLE_GET(RACER_SLOT_GET(slot, Vehicle), Body).impactSpeed);
        }
#else
        DeformVehicleMeshByImpact(
            RACER_SLOT_GET(slot, Vehicle),
            &VEHICLE_GET(RACER_SLOT_GET(slot, Vehicle), Body).impactPoint,
            (s16)VEHICLE_GET(RACER_SLOT_GET(slot, Vehicle), Body).impactSpeed);
#endif
        VEHICLE_GET(RACER_SLOT_GET(slot, Vehicle), Body).impactSpeed = 0;
    }
    VEHICLE_RACE_CALL(RACER_SLOT_GET(slot, Vehicle), UpdateRaceTimers);
    VEHICLE_RACE_CALL(RACER_SLOT_GET(slot, Vehicle), UpdateHudMessageTimers);
    VEHICLE_RACE_CALL(RACER_SLOT_GET(slot, Vehicle), CheckWrongWayWarning);
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    slot->UpdateRaceProgress();
#else
    UpdateRaceProgress(slot);
#endif
    VEHICLE_RACE_CALL(RACER_SLOT_GET(slot, Vehicle), UpdateVehicleGridPosition);
    Vec3fSubtract(&VEHICLE_GET(RACER_SLOT_GET(slot, Vehicle), PlacedVelocity),
                  &VEHICLE_GET(RACER_SLOT_GET(slot, Vehicle), Frame).position,
                  &VEHICLE_GET(RACER_SLOT_GET(slot, Vehicle), PreviousPosition));
    Vec3fScaleInPlace(&VEHICLE_GET(RACER_SLOT_GET(slot, Vehicle), PlacedVelocity),
                      RACER_RECIPROCAL(g_fFrameDeltaTime));
    VEHICLE_SET(RACER_SLOT_GET(slot, Vehicle),
                CameraHeading,
                Atan2f(VEHICLE_GET(RACER_SLOT_GET(slot, Vehicle), ActiveCamera)->forward.x,
                       VEHICLE_GET(RACER_SLOT_GET(slot, Vehicle), ActiveCamera)->forward.y));
    if (VEHICLE_GET(RACER_SLOT_GET(slot, Vehicle), SlotCooldown)) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        VEHICLE_SET(RACER_SLOT_GET(slot, Vehicle),
                    SlotCooldown,
                    (s32)((u32)VEHICLE_GET(RACER_SLOT_GET(slot, Vehicle), SlotCooldown) - 1));
#else
        VEHICLE_ADD(RACER_SLOT_GET(slot, Vehicle), SlotCooldown, -1);
#endif
    }
}
