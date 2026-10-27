#include "game.h"
#include "input.h"
#include "mathutil.h"
#include "physics.h"
#include "race.h"
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
        if (!(vehicle->speedKph > 50.0f)) {
            factor = 1.0f;
        } else if (vehicle->speedKph >= 100.0f) {
            factor = 0.95f;
        } else {
            factor = 1.0L - ((long double)vehicle->speedKph - 50.0f) * 0.001f;
        }
    }
    switch (vehicle->steeringMode) {
    case 0:
    case 1:
        target = (powl(11.0L, amount) - 1.0f) * sign * 0.1f * factor;
        target *= vehicle->steeringMode == 0 ? -0.0785398185253143310546875f :
                                               -0.087266467511653900146484375f;
        break;
    case 2:
    case 3:
        target = (powl(2.0L, amount) - 1.0f) * sign * factor;
        target *= vehicle->steeringMode == 2 ? -0.087266467511653900146484375f :
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
    vehicle->steerAngle = target;
}
#endif

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
#endif

#define ABS_OF(x) ((x) < 0.0f ? -(x) : (x))

/* N64 reads an unset gain for an unknown steering mode. */
#pragma GCC diagnostic push
#ifndef __clang__
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
#pragma GCC diagnostic ignored "-Wuninitialized"

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
    kph = vehicle->speedKph;
#if TGR_PLATFORM == TGR_PLATFORM_N64
    /* The original computes the speed of the body and discards it. */
    sqrtf(vehicle->body.state.velocity.z * vehicle->body.state.velocity.z +
          (vehicle->body.state.velocity.x * vehicle->body.state.velocity.x +
           vehicle->body.state.velocity.y * vehicle->body.state.velocity.y));
#endif
    Mtx4fInverseRotate(&local, vehicle->body.orientation, &vehicle->body.state.velocity);
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
    switch (vehicle->steeringMode) {
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
    g_fSteerLock = (u32)vehicle->steeringMode <= 6 ? 14.0f : 17.0f;
    g_fSteerLockLoss = (u32)vehicle->steeringMode <= 6 ? 10.0f : 14.0f;
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
    }
#endif
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
#endif
    if (most < target) {
        target = most;
    }
    if (ENGINE_LESS(target, least)) {
        target = least;
    }

    /* Back towards the middle, or across it, the wheels turn a radian a frame. */
    current = vehicle->steerAngle;
    returning = SIGN_OF(current) != SIGN_OF(target);
    if (!returning) {
        returning = (0.0f < current && ENGINE_LESS(target, current)) ||
                    (ENGINE_LESS(current, 0.0f) && current < target);
    }
    if (ENGINE_ZERO(current)) {
        returning = 0;
        vehicle->steerReturn = 0;
        current = vehicle->steerAngle;
    }
    down = ENGINE_LESS(target, current);
    up = current < target;
    if (down && vehicle->steerReturn < 0) {
        returning = 1;
    }
    if (up && vehicle->steerReturn > 0) {
        returning = 1;
    }
    if (returning) {
        vehicle->steerReturn = down ? -1 : 1;
        rate = 1.0f;
        current = vehicle->steerAngle;
        if (SIGN_OF(target) != SIGN_OF(current)) {
            target = 0.0f;
        }
    } else {
        vehicle->steerReturn = 0;
        current = vehicle->steerAngle;
    }
    if (ENGINE_LESS(ABS_OF(current - target), rate)) {
        vehicle->steerAngle = target;
    } else if (ENGINE_LESS(target, current)) {
        vehicle->steerAngle = current - rate;
    } else {
        vehicle->steerAngle = current + rate;
    }
}

void UpdateVehicleEngineAndGearbox(Vehicle *vehicle) {
    PlayerInputState *input = vehicle->input;
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

#endif
    previous = vehicle->engineRpm;
    if (ENGINE_LESS(vehicle->engineRpm, IDLE_RPM)) {
        vehicle->engineRpm = IDLE_RPM;
    }
    if (vehicle->transmission) {
        /* The automatic gearbox changes down and up by the revolutions, not up in reverse. */
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        if (analogueThrottle && input->stickY > 0.4f) {
            shiftDown = 4000.0L - (long double)input->stickY * -2000.0f;
            shiftUp = shiftDown;
            shiftDown -= 2000.0f;
        }
#endif
        if (vehicle->gear >= 2 && vehicle->engineRpm < shiftDown) {
            vehicle->gear--;
        } else if (vehicle->gear < vehicle->gearCount && shiftUp < vehicle->engineRpm &&
                   !(vehicle->input->buttons & INPUT_REVERSE)) {
            vehicle->gear++;
        }
    } else if (vehicle->gear > 0 && (vehicle->input->buttons & INPUT_GEAR_DOWN)) {
        TransferFlags(vehicle->input, INPUT_GEAR_DOWN);
        vehicle->gear--;
    } else if (vehicle->gear < vehicle->gearCount && (vehicle->input->buttons & INPUT_GEAR_UP) &&
               !(vehicle->input->buttons & INPUT_REVERSE)) {
        TransferFlags(vehicle->input, INPUT_GEAR_UP);
        vehicle->gear++;
    }
    buttons = vehicle->input->buttons;

    /* Arcade catch-up torque depends on distance behind the first racer. */
    extra = 0.0f;
    rpm = vehicle->engineRpm;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    if (g_dwGameMode == GAME_MODE_ARCADE && vehicle->racePosition != 0) {
        leader = 0.0f;
        for (i = 0; i < g_dwNumVehicles; i++) {
            distance = g_aRacerSlots[i].vehicle ? g_aRacerSlots[i].vehicle->distance :
                                                  g_aRacerSlots[i].distance;
            if (ENGINE_LESS(leader, distance)) {
                leader = distance;
            }
        }
        extra = (long double)leader - vehicle->distance;
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
        extra = vehicle->racePosition == 0 ? 0.0L : (vehicle->racePosition == 1 ? 20.0L : 30.0L);
    }
    torque = (((long double)vehicle->torqueCurve[0] * rpm + vehicle->torqueCurve[1]) * rpm +
              vehicle->torqueCurve[2]) *
                 rpm +
             vehicle->torqueCurve[3] + extra;
    if (!vehicle->transmission) {
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
    torque = vehicle->torqueCurve[0] * rpm * rpm * rpm + vehicle->torqueCurve[1] * rpm * rpm +
             vehicle->torqueCurve[2] * rpm + vehicle->torqueCurve[3] + extra;
    if (!vehicle->transmission) {
        torque = (f32)((f64)torque * 1.03);
    }
#endif
    if (!(buttons & INPUT_ACCELERATE)) {
        torque = 0.0f;
    }
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    if (analogueThrottle && input->stickY > 0.0f) {
        multiplier = (long double)input->stickY * 7.0f;
    }
    vehicle->throttle = torque * multiplier;
#else
    vehicle->throttle = torque * 7.0f;
#endif
    if (vehicle->racer->status & RACER_HELD) {
        vehicle->gear = 0;
        gear = 0;
    } else {
        gear = vehicle->gear;
        if (gear == 0) {
            vehicle->gear = 1;
            gear = 1;
        }
    }
    if (gear) {
        /* In gear the engine turns with the first front wheel. */
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        if (buttons & INPUT_REVERSE) {
            gear = 1;
        }
        vehicle->engineRpm =
            ((long double)vehicle->gearRatios[gear] * vehicle->finalDrive) *
            ((long double)vehicle->wheelBodies[1].spinRate * 0.15915493667125701904296875f) *
            -60.0f;
#else
        vehicle->engineRpm =
            vehicle->gearRatios[gear] *
            (-(vehicle->wheelBodies[1].spinRate / TWO_PI_F) * 60.0f * vehicle->finalDrive);
#endif
    } else {
        if (vehicle->input->buttons & INPUT_ACCELERATE) {
            vehicle->engineRpm = vehicle->engineRpm + 300.0f;
        } else {
            vehicle->engineRpm = vehicle->engineRpm - 200.0f;
        }
        vehicle->throttle = 0.0f;
    }

    /* Limit the revolutions and their change per frame. */
    if (ENGINE_LESS(vehicle->engineRpm, 0.0f)) {
        vehicle->engineRpm = -vehicle->engineRpm;
    }
    if (ENGINE_LESS(vehicle->engineRpm, MIN_RPM)) {
        vehicle->engineRpm = MIN_RPM;
    }
    if (MAX_RPM < vehicle->engineRpm) {
        vehicle->engineRpm = MAX_RPM;
    }
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    if (analogueThrottle && vehicle->gear == 0) {
        limit = (long double)input->stickY * MAX_RPM;
        if (ENGINE_LESS(limit, MIN_RPM)) {
            limit = MIN_RPM;
        }
        if (ENGINE_LESS(limit, vehicle->engineRpm)) {
            vehicle->engineRpm = limit;
        }
    }
#endif
    change = (EngineReal)vehicle->engineRpm - previous;
    if (MAX_RPM_CHANGE < ABS_OF(change)) {
        change = SIGN_OF(change) * MAX_RPM_CHANGE;
    }
    vehicle->brake = 0.0f;
    vehicle->engineRpm = previous + change;
    if (vehicle->input->buttons & INPUT_BRAKE) {
        vehicle->brake = -140000.0f;
    }
}
#pragma GCC diagnostic pop
