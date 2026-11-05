#include <PR/gu.h>

#include "common/game.h"
#include "common/input.h"
#include "common/rigidbody.h"
#include "common/vehicle.h"

#if TGR_REGION == TGR_REGION_EU
#define STEER_RATE 0.008333333f
#else
#define STEER_RATE 0.01f
#endif

#define ENGINE_SIGN(x) ((x) == 0.0f ? 0.0f : ((x) > 0.0f ? 1.0f : -1.0f))

void UpdateVehicleEngineAndGearbox(Vehicle *car) {
    float rate;
    float s;
    float t;
    float a;
    float sp;
    float lim;
    Vec3f v;
    float x;
    float lo;
    float tgt;
    int ctlt;
    float old;
    unsigned int flags;
    short k;
    int g;

    s = car->input->steering;
    if (car->input->controllerLayout == 4) {
        a = s < 0.0f ? -s : s;
        car->steerAngle =
            -(ENGINE_SIGN(s) * sqrtf(sqrtf(a) * sqrtf(sqrtf(a) * a))) * 10.0f * 3.1415927f / 180.0f;
    } else {
        if (0.0f < s) {
            s = s - 0.07f;
            if (s < 0.0f) {
                s = 0;
            }
        } else {
            s = s + 0.07f;
            if (0.0f < s) {
                s = 0;
            }
        }
        sp = car->speedKph;
        sqrtf(car->body.state.velocity.z * car->body.state.velocity.z +
              (car->body.state.velocity.x * car->body.state.velocity.x +
               car->body.state.velocity.y * car->body.state.velocity.y));
        Mtx4fInverseRotate(&v, car->body.orientation, &car->body.state.velocity);
        t = v.x;
        if (t < 10.0f) {
            t = 10.0f;
        }
        if (70.0f < t) {
            t = 70.0f;
        }
        if (sp < 140.0f) {
            sp = 140.0f;
        }
        if (340.0f < sp) {
            sp = 340.0f;
        }
        sp -= 140.0f;
        k = 1;
        switch (car->steeringMode) {
        case 0:
            g_fSteerLock = 14.0f;
            g_fSteerRate = STEER_RATE;
            g_fSteerGain = 6.0f;
            g_fSteerLockLoss = 10.0f;
            g_wSteerGainSpeedShare = 0;
            k = g_wSteerGainSpeedShare;
            x = g_fSteerGain;
            break;
        case 1:
            g_fSteerLock = 14.0f;
            g_fSteerRate = STEER_RATE;
            g_fSteerGain = 6.0f;
            g_fSteerLockLoss = 10.0f;
            g_wSteerGainSpeedShare = 0;
            k = g_wSteerGainSpeedShare;
            x = 3.0f;
            break;
        case 2:
            g_fSteerLock = 17.0f;
            g_fSteerRate = 0.1f;
            g_fSteerGain = 16.0f;
            g_fSteerLockLoss = 14.0f;
            g_wSteerGainSpeedShare = 0;
            k = g_wSteerGainSpeedShare;
            x = g_fSteerGain;
            break;
        }
        rate = g_fSteerRate;
        lim = g_fSteerLock - t / 90.0f * g_fSteerLockLoss;
        if ((s < 0.0f ? -s : s) < 0.001f) {
            tgt = 0.0f;
            t = lim * 3.1415927f / 180.0f;
            lo = -lim * 3.1415927f / 180.0f;
        } else if (s < -0.75f) {
            t = lim * 3.1415927f / 180.0f;
            lo = -lim * 3.1415927f / 180.0f;
            tgt = t;
        } else if (0.75f < s) {
            lo = -lim * 3.1415927f / 180.0f;
            t = lim * 3.1415927f / 180.0f;
            tgt = lo;
        } else {
            x = x - k * (x / 2) * (sp / 200.0f);
            tgt = -s * (x * 3.1415927f / 180.0f);
            t = lim * 3.1415927f / 180.0f;
            lo = -lim * 3.1415927f / 180.0f;
        }
        if (t < tgt) {
            tgt = t;
        }
        if (tgt < lo) {
            tgt = lo;
        }
        k = ENGINE_SIGN(tgt) != ENGINE_SIGN(car->steerAngle) ||
            (0.0f < car->steerAngle && tgt < car->steerAngle) ||
            (car->steerAngle < 0.0f && car->steerAngle < tgt);
        if (car->steerAngle == 0.0f) {
            k = 0;
            car->steerReturn = 0;
        }
        g = tgt < car->steerAngle;
        ctlt = car->steerAngle < tgt;
        if (g && car->steerReturn < 0) {
            k = 1;
        }
        if (ctlt && car->steerReturn > 0) {
            k = 1;
        }
        if (k) {
            car->steerReturn = g ? -1 : 1;
            rate = 1.0f;
            if (ENGINE_SIGN(car->steerAngle) != ENGINE_SIGN(tgt)) {
                tgt = 0.0f;
            }
        } else {
            car->steerReturn = 0;
        }
        if ((car->steerAngle < tgt ? -(car->steerAngle - tgt) : car->steerAngle - tgt) < rate) {
            car->steerAngle = tgt;
        } else if (tgt < car->steerAngle) {
            car->steerAngle = car->steerAngle - rate;
        } else {
            car->steerAngle = car->steerAngle + rate;
        }
    }
    old = car->engineRpm;
    if (car->engineRpm < 800.0f) {
        car->engineRpm = 800.0f;
    }
    if (car->transmission != 0) {
        g = car->gear;
        if (g >= 2 && car->engineRpm < 4000.0f) {
            car->gear = g - 1;
        } else if (g < car->gearCount && 6000.0f < car->engineRpm &&
                   !(car->input->buttons & 0x20000)) {
            car->gear = g + 1;
        }
        flags = car->input->buttons;
    } else if (car->gear > 0 && (car->input->buttons & 0x200000)) {
        TransferFlags(car->input, 0x200000);
        car->gear--;
        flags = car->input->buttons;
    } else {
        flags = car->input->buttons;
        if (car->gear < car->gearCount && (flags & 0x100000) && !(flags & 0x20000)) {
            TransferFlags(car->input, 0x100000);
            car->gear++;
            flags = car->input->buttons;
        }
    }
    lo = 0.0f;
    if (g_dwGameMode == 1 && car->racePosition == 1) {
        a = (g_aVehicles[car->index ^ 1].distance < car->distance ?
                 -(g_aVehicles[car->index ^ 1].distance - car->distance) :
                 g_aVehicles[car->index ^ 1].distance - car->distance) *
            0.5f;
        lo = a - 18.0f;
        if (a < 18.0f) {
            lo = 0.0f;
        } else if (30.0f < lo) {
            lo = 30.0f;
        }
    }
    t = car->torqueCubic * car->engineRpm * car->engineRpm * car->engineRpm +
        car->torqueQuadratic * car->engineRpm * car->engineRpm +
        car->torqueLinear * car->engineRpm + car->torqueConstant + lo;
    if (car->transmission == 0) {
        t = t * 1.03;
    }
    if (!(flags & 0x10000)) {
        t = 0.0f;
    }
    g = 0;
    car->throttle = t * 7.0f;
    if (car->racer->status & 1) {
        car->gear = 0;
    } else {
        g = car->gear;
        if (g == 0) {
            car->gear = 1;
            g = 1;
        }
    }
    if (g != 0) {
        car->engineRpm = (-(car->wheelBodies[1].spinRate / 6.2831855f) * 60.0f * car->finalDrive) *
                         car->gearRatios[g];
    } else {
        if (car->input->buttons & 0x10000) {
            car->engineRpm += 300.0f;
        } else {
            car->engineRpm -= 200.0f;
        }
        car->throttle = 0.0f;
    }
    if (car->engineRpm < 0.0f) {
        car->engineRpm = -car->engineRpm;
    }
    if (car->engineRpm < 900.0f) {
        car->engineRpm = 900.0f;
    }
    if (8000.0f < car->engineRpm) {
        car->engineRpm = 8000.0f;
    }
    lim = car->engineRpm - old;
    if (400.0f < (lim < 0.0f ? -lim : lim)) {
        lim = ENGINE_SIGN(lim) * 400.0f;
    }
    car->engineRpm = old + lim;
    car->brake = 0;
    if (car->input->buttons & 0x40000) {
        car->brake = -140000.0f;
    }
}
