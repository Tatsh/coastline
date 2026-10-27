#include <PR/gu.h>

#include "camera.h"
#include "frametime.h"
#include "render.h"
#include "vehicle.h"
#include "weather.h"

#define RANDOM_TO_TWO (2.0f / 65535.0f)
#define EYE_AHEAD 3.0f
#if TGR_PLATFORM == TGR_PLATFORM_WIN
#define RAIN_MIN_CAMERA_SPEED 0.2777777910232544f
#define RAIN_CAMERA_SPEED_MUL 3.5999999046325684f
#else
#define RAIN_MIN_CAMERA_SPEED 0.2777777612f
#define RAIN_CAMERA_SPEED_MUL 3.600000143f
#endif
#define SNOW_FALL_RATE 0.5f
#define SNOW_DRIFT 0.25f
#define PARTICLE_UNITS 16383.5
#define DRIFT_HOLD_MASK 0xF

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#define PRECIPITATION_REAL long double
#define PRECIPITATION_PARTICLE_REAL long double
#define PRECIPITATION_INTEGER(value) ((s64)(value))
#define PRECIPITATION_CAMERA(view) g_aVehicles[view].GetActiveCamera()
#define NEGATE_DRIFT(value) ((s32)(0u - (u32)(value)))
#else
#define PRECIPITATION_REAL f32
#define PRECIPITATION_PARTICLE_REAL f64
#define PRECIPITATION_INTEGER(value) ((s32)(value))
#define PRECIPITATION_CAMERA(view) g_aVehicles[view].activeCamera
#define NEGATE_DRIFT(value) (-(value))
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */

void UpdatePrecipitationParticles(void) {
    CameraFrame *camera;
    Vec3f eye, move;
    Vec3f *previous, *velocity;
    s16 *particle;
    s16 driftX, driftY, moveX, moveY, moveZ;
    s32 view, i, holdX, holdY;
    f32 dx, dy, dz, driftStepX, driftStepY;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    long double rawX, rawY, rawZ, speed, rawScale, cosine, rawSine;
    volatile long double snowFall;
#else
    f32 scale, sine;
#endif

    UpdateWindDirection();
    if (g_dwRainMode) {
        UpdateLightningFlash();
    }
    g_dwRainParticleCount = RAIN_PARTICLES / g_dwNumViewports;
    for (view = 0; view < g_dwNumViewports; view++) {
        camera = PRECIPITATION_CAMERA(view);
        Vec3fMulAdd(&eye, &camera->position, &camera->forward, EYE_AHEAD);
        if (!g_bRainParticlesInitialized) {
            InitRandomRainParticles();
            g_avRainPrevCameraPos[0] = eye;
            g_avRainPrevCameraPos[1] = eye;
            g_bRainParticlesInitialized = TRUE;
        }
        if (!g_dwSnowMode && !g_dwRainMode) {
            return;
        }

        /* The rain moves with the camera, by less than the camera when it is fast. */
        previous = &g_avRainPrevCameraPos[view];
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        rawX = (long double)eye.x - previous->x;
        rawY = (long double)eye.y - previous->y;
        rawZ = (long double)eye.z - previous->z;
        dx = rawX;
        dy = rawY;
        dz = rawZ;
        speed = (rawX * dx + rawY * dy) + rawZ * dz;
        __asm__("fsqrt" : "=t"(speed) : "0"(speed));
        speed /= g_fFrameDeltaTime;
        g_fRainCameraSpeed = speed;
        if (speed > RAIN_MIN_CAMERA_SPEED) {
            rawScale = (long double)g_fRainCameraSpeed * RAIN_CAMERA_SPEED_MUL;
            __asm__("fsqrt" : "=t"(rawScale) : "0"(rawScale));
            rawScale = rawScale * RAIN_MIN_CAMERA_SPEED / g_fRainCameraSpeed;
            g_fRainCameraSpeedScale = rawScale;
            g_fRainCameraSpeed = rawScale * g_fRainCameraSpeed;
            dx = (long double)dx * g_fRainCameraSpeedScale;
            dy = (long double)dy * g_fRainCameraSpeedScale;
            dz = (long double)dz * g_fRainCameraSpeedScale;
        } else {
            g_fRainCameraSpeedScale = 1.0f;
        }
#else
        dx = eye.x - previous->x;
        dy = eye.y - previous->y;
        dz = eye.z - previous->z;
        /* The speed is read from its global at each use, and again after the square root. */
        g_fRainCameraSpeed = sqrtf(dx * dx + dy * dy + dz * dz) / g_fFrameDeltaTime;
        if (g_fRainCameraSpeed > RAIN_MIN_CAMERA_SPEED) {
            scale = sqrtf(g_fRainCameraSpeed * RAIN_CAMERA_SPEED_MUL) * RAIN_MIN_CAMERA_SPEED /
                    g_fRainCameraSpeed;
            g_fRainCameraSpeedScale = scale;
            dx *= scale;
            dy *= scale;
            dz *= scale;
            g_fRainCameraSpeed = g_fRainCameraSpeed * scale;
        } else {
            g_fRainCameraSpeedScale = 1.0f;
        }

#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */
        /* Snow falls slowly. Rain falls faster and blows with the wind. */
        if (g_dwSnowMode) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
            snowFall = (long double)g_fFrameDeltaTime * -SNOW_FALL_RATE;
            dz = (long double)dz - snowFall;
#else
            dz += g_fFrameDeltaTime * SNOW_FALL_RATE;
#endif
        } else {
            velocity = &g_avRainVelocity[view];
#if TGR_PLATFORM == TGR_PLATFORM_WIN
            __asm__("fcos" : "=t"(cosine) : "0"((long double)g_fWindAngle));
            __asm__("fsin" : "=t"(rawSine) : "0"((long double)g_fWindAngle));
            velocity->x = cosine * g_fWindStrength * g_fFrameDeltaTime;
#else
            velocity->x = cosf(g_fWindAngle) * (g_fFrameDeltaTime * g_fWindStrength);
            sine = sinf(g_fWindAngle);
#endif
            velocity->z = g_fFrameDeltaTime + g_fFrameDeltaTime;
            move.x = dx;
            move.y = dy;
            move.z = dz;
            dz = (PRECIPITATION_REAL)dz + velocity->z;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
            velocity->y = rawSine * g_fWindStrength * g_fFrameDeltaTime;
#else
            velocity->y = sine * (g_fFrameDeltaTime * g_fWindStrength);
#endif
            dx = (PRECIPITATION_REAL)dx + velocity->x;
            dy = (PRECIPITATION_REAL)dy + velocity->y;
            Vec3fScaleInPlace(&move, g_fRainCameraSpeedScale * 0.5f);
            Vec3fAddInPlace(velocity, &move);
        }

        /* Snow drifts from side to side. */
        if (g_dwSnowMode) {
            driftStepX = ((PRECIPITATION_REAL)(s32)(AdvanceRng() & 0xFFFF) * RANDOM_TO_TWO - 1.0f) *
                         g_fFrameDeltaTime * SNOW_DRIFT;
            driftStepY = ((PRECIPITATION_REAL)(s32)(AdvanceRng() & 0xFFFF) * RANDOM_TO_TWO - 1.0f) *
                         g_fFrameDeltaTime * SNOW_DRIFT;
        } else {
            driftStepX = 0.0f;
            driftStepY = 0.0f;
        }

        /*
         * Every particle moves the same, and a drift goes to a run of a random length of
         * particles at a time, its direction alternating.
         */
        *previous = eye;
        holdX = AdvanceRng() & DRIFT_HOLD_MASK;
        holdY = AdvanceRng() & DRIFT_HOLD_MASK;
        driftX = PRECIPITATION_INTEGER((PRECIPITATION_PARTICLE_REAL)driftStepX * PARTICLE_UNITS);
        driftY = PRECIPITATION_INTEGER((PRECIPITATION_PARTICLE_REAL)driftStepY * PARTICLE_UNITS);
        moveX = PRECIPITATION_INTEGER((PRECIPITATION_PARTICLE_REAL)dx * PARTICLE_UNITS);
        moveY = PRECIPITATION_INTEGER((PRECIPITATION_PARTICLE_REAL)dy * PARTICLE_UNITS);
        moveZ = PRECIPITATION_INTEGER((PRECIPITATION_PARTICLE_REAL)dz * PARTICLE_UNITS);
        particle = g_awRainParticlePos[view][0];
        for (i = 0; i < g_dwRainParticleCount; i++, particle += 3) {
            if (holdX != 0) {
                holdX--;
                particle[0] += moveX;
            } else {
                particle[0] = particle[0] + moveX + driftX;
                driftX = NEGATE_DRIFT(driftX);
                holdX = AdvanceRng() & DRIFT_HOLD_MASK;
            }
            if (holdY != 0) {
                holdY--;
                particle[1] += moveY;
            } else {
                particle[1] = particle[1] + moveY + driftY;
                driftY = NEGATE_DRIFT(driftY);
                holdY = AdvanceRng() & DRIFT_HOLD_MASK;
            }
            particle[2] += moveZ;
        }
    }
}
