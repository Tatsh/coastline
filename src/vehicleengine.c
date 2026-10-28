#include "game.h"
#include "input.h"
#include "mathutil.h"
#include "physics.h"
#include "race.h"
#include "racerslotaccess.h"
#include "vehicleaccess.h"
#include "vehiclecount.h"
#include "vehiclephysics.h"

#if TGR_PLATFORM == TGR_PLATFORM_N64
#include <PR/gu.h>
#endif

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#include <math.h>
#define RACER_HELD 0x1

/** @ghidraAddress BRD3D: 0x100b3668 */
f32 g_fSteerLock = 14.0f;
/** @ghidraAddress BRD3D: 0x100b366c */
f32 g_fSteerRate = 0.01f;
/** @ghidraAddress BRD3D: 0x100b3670 */
f32 g_fSteerLockLoss = 10.0f;
/** @ghidraAddress BRD3D: 0x10aa346c */
s16 g_wSteerGainSpeedShare;

/* Axis bindings use the high bit of the control word. */
static void SteerVehicleByAnalogueInput(Vehicle *vehicle, f32 stick) {
    f32 factor = 1.0f;
    f32 amount = stick < 0.0f ? -stick : stick;
    long double target,
        sign = stick == 0.0f || stick != stick ? 0.0L : (stick > 0.0f ? 1.0L : -1.0L);
    if (IsVehicleSteeringSpeedSensitive()) {
        if (!(VEHICLE_GET(vehicle, SpeedKph) > 50.0f)) {
            factor = 1.0f;
        } else if (VEHICLE_GET(vehicle, SpeedKph) >= 100.0f) {
            factor = 0.95f;
        } else {
            factor = 1.0L - ((long double)VEHICLE_GET(vehicle, SpeedKph) - 50.0f) * 0.001f;
        }
    }
    switch (VEHICLE_GET(vehicle, SteeringMode)) {
    case 0:
    case 1:
        target = (powl(11.0L, amount) - 1.0f) * sign * 0.1f * factor;
        target *= VEHICLE_GET(vehicle, SteeringMode) == 0 ? -0.0785398185253143310546875f :
                                                            -0.087266467511653900146484375f;
        break;
    case 2:
    case 3:
        target = (powl(2.0L, amount) - 1.0f) * sign * factor;
        target *= VEHICLE_GET(vehicle, SteeringMode) == 2 ? -0.087266467511653900146484375f :
                                                            -0.095993109047412872314453125f;
        break;
    case 4:
        target = (long double)factor * stick * -0.087266467511653900146484375f;
        break;
    case 5:
        target = (long double)factor * stick * -0.10471975803375244140625f;
        break;
    case 6:
        target = (long double)factor * stick * -0.122173048555850982666015625f;
        break;
    default:
        target = (long double)stick * stick * sign * factor * -0.1308996975421905517578125f;
        break;
    }
    VEHICLE_SET(vehicle, SteerAngle, target);
}
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */

#define PI_F 3.141592741f
#define TWO_PI_F 6.283185482f

/* The button layout that steers straight with the stick. */
#define STICK_STEERING_LAYOUT 4

/* The middle of the stick that does not steer. */
#define STICK_DEAD_ZONE 0.07f

/* The revolutions of the engine: idle, the least and the most, the change of the automatic
 * gearbox, and the most they change a frame. */
#define IDLE_RPM 800.0f
#define MIN_RPM 900.0f
#define MAX_RPM 8000.0f
#define SHIFT_DOWN_RPM 4000.0f
#define SHIFT_UP_RPM 6000.0f
#define MAX_RPM_CHANGE 400.0f

/* The sign of a value: 0, 1 or -1. */
#if TGR_PLATFORM == TGR_PLATFORM_WIN
typedef long double EngineReal;
#define ENGINE_LESS(a, b) (!((a) >= (b)))
#define ENGINE_ZERO(x) (!((x) < 0.0f) && !((x) > 0.0f))
#define SIGN_OF(x) ((x) == 0.0L || (x) != (x) ? 0.0L : (0.0L < (x) ? 1.0L : -1.0L))
#else
typedef f32 EngineReal;
#define ENGINE_LESS(a, b) ((a) < (b))
#define ENGINE_ZERO(x) ((x) == 0.0f)
#define SIGN_OF(x) ((x) == 0.0f ? 0.0f : (0.0f < (x) ? 1.0f : -1.0f))
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */

#define ABS_OF(x) ((x) < 0.0f ? -(x) : (x))

/* Steer the front wheels towards the stick, faster back towards the middle. */
static inline void SteerByStick(Vehicle *vehicle, f32 stick) {
    Vec3f local;
    f32 forward, kph, rate, lock, most, least, current;
    EngineReal gain, target;
    s32 down, up;
    s16 returning;
#if TGR_PLATFORM == TGR_PLATFORM_N64
    s16 share = 1;
#endif

    /* The middle of the stick does not steer. */
    if (0.0f < stick) {
        stick = stick - STICK_DEAD_ZONE;
        if (stick < 0.0f) {
            stick = 0.0f;
        }
    } else {
        stick = stick + STICK_DEAD_ZONE;
        if (0.0f < stick) {
            stick = 0.0f;
        }
    }
    kph = VEHICLE_GET(vehicle, SpeedKph);
#if TGR_PLATFORM == TGR_PLATFORM_N64
    /* The original computes the speed of the body and discards it. */
    sqrtf(
        VEHICLE_GET(vehicle, Body).state.velocity.z * VEHICLE_GET(vehicle, Body).state.velocity.z +
        (VEHICLE_GET(vehicle, Body).state.velocity.x * VEHICLE_GET(vehicle, Body).state.velocity.x +
         VEHICLE_GET(vehicle, Body).state.velocity.y *
             VEHICLE_GET(vehicle, Body).state.velocity.y));
#endif
    Mtx4fInverseRotate(
        &local, VEHICLE_GET(vehicle, Body).orientation, &VEHICLE_GET(vehicle, Body).state.velocity);
    forward = local.x;
    if (ENGINE_LESS(forward, 10.0f)) {
        forward = 10.0f;
    }
    if (70.0f < forward) {
        forward = 70.0f;
    }
    if (ENGINE_LESS(kph, 140.0f)) {
        kph = 140.0f;
    }
    if (340.0f < kph) {
        kph = 340.0f;
    }
    kph = kph - 140.0f;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    switch (VEHICLE_GET(vehicle, SteeringMode)) {
    case 0:
        g_fSteerRate = 0.008f;
        gain = 6.0L;
        break;
    case 1:
        g_fSteerRate = 0.01f;
        gain = 5.0L;
        break;
    case 2:
        g_fSteerRate = 0.013f;
        gain = 4.0L;
        break;
    case 3:
        g_fSteerRate = 0.017f;
        gain = 3.0L;
        break;
    case 4:
        g_fSteerRate = 0.022f;
        gain = 3.0L;
        break;
    case 5:
        g_fSteerRate = 0.03f;
        gain = 3.0L;
        break;
    case 6:
        g_fSteerRate = 0.05f;
        gain = 3.0L;
        break;
    default:
        g_fSteerRate = 0.07f;
        gain = 16.0L;
        break;
    }
    g_fSteerLock = (u32)VEHICLE_GET(vehicle, SteeringMode) <= 6 ? 14.0f : 17.0f;
    g_fSteerLockLoss = (u32)VEHICLE_GET(vehicle, SteeringMode) <= 6 ? 10.0f : 14.0f;
    g_wSteerGainSpeedShare = 0;
#else
    switch (vehicle->steeringMode) {
    case 0:
        g_fSteerLock = 14.0f;
        g_fSteerRate = 0.01f;
        g_fSteerGain = 6.0f;
        g_fSteerLockLoss = 10.0f;
        gain = g_fSteerGain;
        g_wSteerGainSpeedShare = 0;
        share = g_wSteerGainSpeedShare;
        break;
    case 1:
        g_fSteerLock = 14.0f;
        g_fSteerRate = 0.01f;
        g_fSteerGain = 6.0f;
        g_fSteerLockLoss = 10.0f;
        gain = 3.0f;
        g_wSteerGainSpeedShare = 0;
        share = g_wSteerGainSpeedShare;
        break;
    case 2:
        g_fSteerLock = 17.0f;
        g_fSteerRate = 0.1f;
        g_fSteerGain = 16.0f;
        g_fSteerLockLoss = 14.0f;
        gain = g_fSteerGain;
        g_wSteerGainSpeedShare = 0;
        share = g_wSteerGainSpeedShare;
        break;
#ifdef ENABLE_PATCHES
    default:
        /* An unknown mode steers with the settings and the gain set last. */
        gain = g_fSteerGain;
        break;
#else
        /* An unknown mode reads an unset gain. */
#endif /* ENABLE_PATCHES */
    }
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */
    rate = g_fSteerRate;

    /* The lock narrows with the speed; a hard push takes all of it, a soft one the gain. */
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    lock =
        g_fSteerLock - (long double)forward * 0.011111111380159854888916015625f * g_fSteerLockLoss;
#define ENGINE_RADIANS 0.01745329238474369049072265625f
    most = (long double)lock * ENGINE_RADIANS;
    least = (long double)lock * -ENGINE_RADIANS;
    if (ENGINE_LESS(ABS_OF(stick), 0.001f)) {
        target = 0.0L;
    } else if (ENGINE_LESS(stick, -0.75f)) {
        target = (long double)lock * ENGINE_RADIANS;
    } else if (0.75f < stick) {
        target = (long double)lock * -ENGINE_RADIANS;
    } else {
        target = -(gain * ENGINE_RADIANS * stick);
    }
#else
    lock = g_fSteerLock - forward / 90.0f * g_fSteerLockLoss;
    if (ABS_OF(stick) < 0.001f) {
        most = lock * PI_F / 180.0f;
        least = -lock * PI_F / 180.0f;
        target = 0.0f;
    } else if (stick < -0.75f) {
        most = lock * PI_F / 180.0f;
        least = -lock * PI_F / 180.0f;
        target = most;
    } else if (0.75f < stick) {
        least = -lock * PI_F / 180.0f;
        most = lock * PI_F / 180.0f;
        target = least;
    } else {
        gain = gain - (f32)share * (gain / 2.0f) * (kph / 200.0f);
        target = -stick * (gain * PI_F / 180.0f);
        most = lock * PI_F / 180.0f;
        least = -lock * PI_F / 180.0f;
    }
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */
    if (most < target) {
        target = most;
    }
    if (ENGINE_LESS(target, least)) {
        target = least;
    }

    /* Back towards the middle, or across it, the wheels turn a radian a frame. */
    current = VEHICLE_GET(vehicle, SteerAngle);
    returning = SIGN_OF(current) != SIGN_OF(target);
    if (!returning) {
        returning = (0.0f < current && ENGINE_LESS(target, current)) ||
                    (ENGINE_LESS(current, 0.0f) && current < target);
    }
    if (ENGINE_ZERO(current)) {
        returning = 0;
        VEHICLE_SET(vehicle, SteerReturn, 0);
        current = VEHICLE_GET(vehicle, SteerAngle);
    }
    down = ENGINE_LESS(target, current);
    up = current < target;
    if (down && VEHICLE_GET(vehicle, SteerReturn) < 0) {
        returning = 1;
    }
    if (up && VEHICLE_GET(vehicle, SteerReturn) > 0) {
        returning = 1;
    }
    if (returning) {
        VEHICLE_SET(vehicle, SteerReturn, down ? -1 : 1);
        rate = 1.0f;
        current = VEHICLE_GET(vehicle, SteerAngle);
        if (SIGN_OF(target) != SIGN_OF(current)) {
            target = 0.0f;
        }
    } else {
        VEHICLE_SET(vehicle, SteerReturn, 0);
        current = VEHICLE_GET(vehicle, SteerAngle);
    }
    if (ENGINE_LESS(ABS_OF(current - target), rate)) {
        VEHICLE_SET(vehicle, SteerAngle, target);
    } else if (ENGINE_LESS(target, current)) {
        VEHICLE_SET(vehicle, SteerAngle, current - rate);
    } else {
        VEHICLE_SET(vehicle, SteerAngle, current + rate);
    }
}

void UpdateVehicleEngineAndGearbox(Vehicle *vehicle) {
    PlayerInputState *input = VEHICLE_GET(vehicle, Input);
    f32 stick = input->steering;
    f32 previous, rpm;
#if TGR_PLATFORM == TGR_PLATFORM_N64
    f32 amount, root, gap;
#endif
    EngineReal extra, torque, change, shiftDown = SHIFT_DOWN_RPM;
    f32 shiftUp = SHIFT_UP_RPM;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    s32 analogueThrottle = IsVehicleControlAnalogue(2);
    EngineReal multiplier = 7.0L, limit;
    f32 leader, distance;
    s32 i;
#endif
    u32 buttons;
    s32 gear;

#if TGR_PLATFORM == TGR_PLATFORM_WIN
    if (IsVehicleControlAnalogue(0) || IsVehicleControlAnalogue(1)) {
        SteerVehicleByAnalogueInput(vehicle, stick);
    } else {
        SteerByStick(vehicle, stick);
    }
#else
    if (input->controllerLayout == STICK_STEERING_LAYOUT) {
        /* Steer straight from the stick, by a curve of it. */
        amount = ABS_OF(stick);
        root = sqrtf(amount);
        vehicle->steerAngle =
            -(sqrtf(sqrtf(sqrtf(amount) * amount) * root) * SIGN_OF(stick)) * 10.0f * PI_F / 180.0f;
    } else {
        SteerByStick(vehicle, stick);
    }

#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */
    previous = VEHICLE_GET(vehicle, EngineRpm);
    if (ENGINE_LESS(VEHICLE_GET(vehicle, EngineRpm), IDLE_RPM)) {
        VEHICLE_SET(vehicle, EngineRpm, IDLE_RPM);
    }
    if (VEHICLE_GET(vehicle, Transmission)) {
        /* The automatic gearbox changes down and up by the revolutions, not up in reverse. */
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        if (analogueThrottle && input->stickY > 0.4f) {
            shiftDown = 4000.0L - (long double)input->stickY * -2000.0f;
            shiftUp = shiftDown;
            shiftDown -= 2000.0f;
        }
#endif
        if (VEHICLE_GET(vehicle, Gear) >= 2 && VEHICLE_GET(vehicle, EngineRpm) < shiftDown) {
            VEHICLE_SET(vehicle, Gear, VEHICLE_GET(vehicle, Gear) - 1);
        } else if (VEHICLE_GET(vehicle, Gear) < VEHICLE_GET(vehicle, GearCount) &&
                   shiftUp < VEHICLE_GET(vehicle, EngineRpm) &&
                   !(VEHICLE_GET(vehicle, Input)->buttons & INPUT_REVERSE)) {
            VEHICLE_SET(vehicle, Gear, VEHICLE_GET(vehicle, Gear) + 1);
        }
    } else if (VEHICLE_GET(vehicle, Gear) > 0 &&
               (VEHICLE_GET(vehicle, Input)->buttons & INPUT_GEAR_DOWN)) {
        TransferFlags(VEHICLE_GET(vehicle, Input), INPUT_GEAR_DOWN);
        VEHICLE_SET(vehicle, Gear, VEHICLE_GET(vehicle, Gear) - 1);
    } else if (VEHICLE_GET(vehicle, Gear) < VEHICLE_GET(vehicle, GearCount) &&
               (VEHICLE_GET(vehicle, Input)->buttons & INPUT_GEAR_UP) &&
               !(VEHICLE_GET(vehicle, Input)->buttons & INPUT_REVERSE)) {
        TransferFlags(VEHICLE_GET(vehicle, Input), INPUT_GEAR_UP);
        VEHICLE_SET(vehicle, Gear, VEHICLE_GET(vehicle, Gear) + 1);
    }
    buttons = VEHICLE_GET(vehicle, Input)->buttons;

    /* Arcade catch-up torque depends on distance behind the first racer. */
    extra = 0.0f;
    rpm = VEHICLE_GET(vehicle, EngineRpm);
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    if (g_dwGameMode == GAME_MODE_ARCADE && VEHICLE_GET(vehicle, RacePosition) != 0) {
        leader = 0.0f;
        for (i = 0; i < g_dwNumVehicles; i++) {
            distance = RACER_SLOT_GET(&(g_aRacerSlots[i]), Vehicle) ?
                           VEHICLE_GET(RACER_SLOT_GET(&(g_aRacerSlots[i]), Vehicle), Distance) :
                           RACER_SLOT_GET(&(g_aRacerSlots[i]), Distance);
            if (ENGINE_LESS(leader, distance)) {
                leader = distance;
            }
        }
        extra = (long double)leader - VEHICLE_GET(vehicle, Distance);
        if (ENGINE_LESS(extra, 0.0L)) {
            extra = -extra;
        }
        extra = extra * 0.5f - 18.0f;
        if (ENGINE_LESS(extra, 0.0L)) {
            extra = 0.0L;
        } else if (extra > 30.0f) {
            extra = 30.0L;
        }
    } else if (g_dwGameMode == GAME_MODE_NETWORK) {
        extra = VEHICLE_GET(vehicle, RacePosition) == 0 ?
                    0.0L :
                    (VEHICLE_GET(vehicle, RacePosition) == 1 ? 20.0L : 30.0L);
    }
    torque = (((long double)VEHICLE_GET(vehicle, TorqueCurve)[0] * rpm +
               VEHICLE_GET(vehicle, TorqueCurve)[1]) *
                  rpm +
              VEHICLE_GET(vehicle, TorqueCurve)[2]) *
                 rpm +
             VEHICLE_GET(vehicle, TorqueCurve)[3] + extra;
    if (!VEHICLE_GET(vehicle, Transmission)) {
        torque *= 1.03f;
    }
#else
    if (g_dwGameMode == GAME_MODE_ARCADE && vehicle->racePosition == 1) {
        gap = ABS_OF(g_aVehicles[vehicle->index ^ 1].distance - vehicle->distance) * 0.5f;
        if (gap < 18.0f) {
            extra = 0.0f;
        } else {
            extra = gap - 18.0f;
            if (30.0f < extra) {
                extra = 30.0f;
            }
        }
    }
    torque = VEHICLE_GET(vehicle, TorqueCurve)[0] * rpm * rpm * rpm +
             VEHICLE_GET(vehicle, TorqueCurve)[1] * rpm * rpm +
             VEHICLE_GET(vehicle, TorqueCurve)[2] * rpm + VEHICLE_GET(vehicle, TorqueCurve)[3] +
             extra;
    if (!vehicle->transmission) {
        torque = (f32)((f64)torque * 1.03);
    }
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */
    if (!(buttons & INPUT_ACCELERATE)) {
        torque = 0.0f;
    }
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    if (analogueThrottle && input->stickY > 0.0f) {
        multiplier = (long double)input->stickY * 7.0f;
    }
    VEHICLE_SET(vehicle, Throttle, torque * multiplier);
#else
    vehicle->throttle = torque * 7.0f;
#endif
    if (RACER_SLOT_GET(VEHICLE_GET(vehicle, Racer), Status) & RACER_HELD) {
        VEHICLE_SET(vehicle, Gear, 0);
        gear = 0;
    } else {
        gear = VEHICLE_GET(vehicle, Gear);
        if (gear == 0) {
            VEHICLE_SET(vehicle, Gear, 1);
            gear = 1;
        }
    }
    if (gear) {
        /* In gear the engine turns with the first front wheel. */
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        if (buttons & INPUT_REVERSE) {
            gear = 1;
        }
        VEHICLE_SET(vehicle,
                    EngineRpm,
                    ((long double)VEHICLE_GET(vehicle, GearRatios)[gear] *
                     VEHICLE_GET(vehicle, FinalDrive)) *
                        ((long double)VEHICLE_GET(vehicle, WheelBodies)[1].spinRate *
                         0.15915493667125701904296875f) *
                        -60.0f);
#else
        vehicle->engineRpm = VEHICLE_GET(vehicle, GearRatios)[gear] *
                             (-(VEHICLE_GET(vehicle, WheelBodies)[1].spinRate / TWO_PI_F) * 60.0f *
                              vehicle->finalDrive);
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */
    } else {
        if (VEHICLE_GET(vehicle, Input)->buttons & INPUT_ACCELERATE) {
            VEHICLE_SET(vehicle, EngineRpm, VEHICLE_GET(vehicle, EngineRpm) + 300.0f);
        } else {
            VEHICLE_SET(vehicle, EngineRpm, VEHICLE_GET(vehicle, EngineRpm) - 200.0f);
        }
        VEHICLE_SET(vehicle, Throttle, 0.0f);
    }

    /* Limit the revolutions and their change per frame. */
    if (ENGINE_LESS(VEHICLE_GET(vehicle, EngineRpm), 0.0f)) {
        VEHICLE_SET(vehicle, EngineRpm, -VEHICLE_GET(vehicle, EngineRpm));
    }
    if (ENGINE_LESS(VEHICLE_GET(vehicle, EngineRpm), MIN_RPM)) {
        VEHICLE_SET(vehicle, EngineRpm, MIN_RPM);
    }
    if (MAX_RPM < VEHICLE_GET(vehicle, EngineRpm)) {
        VEHICLE_SET(vehicle, EngineRpm, MAX_RPM);
    }
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    if (analogueThrottle && VEHICLE_GET(vehicle, Gear) == 0) {
        limit = (long double)input->stickY * MAX_RPM;
        if (ENGINE_LESS(limit, MIN_RPM)) {
            limit = MIN_RPM;
        }
        if (ENGINE_LESS(limit, VEHICLE_GET(vehicle, EngineRpm))) {
            VEHICLE_SET(vehicle, EngineRpm, limit);
        }
    }
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */
    change = (EngineReal)VEHICLE_GET(vehicle, EngineRpm) - previous;
    if (MAX_RPM_CHANGE < ABS_OF(change)) {
        change = SIGN_OF(change) * MAX_RPM_CHANGE;
    }
    VEHICLE_SET(vehicle, Brake, 0.0f);
    VEHICLE_SET(vehicle, EngineRpm, previous + change);
    if (VEHICLE_GET(vehicle, Input)->buttons & INPUT_BRAKE) {
        VEHICLE_SET(vehicle, Brake, -140000.0f);
    }
}
