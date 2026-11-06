#include <PR/gu.h>

#include "common/camera.h"
#include "common/frametime.h"
#include "common/render.h"
#include "common/vehicle.h"
#include "common/weather.h"

void UpdatePrecipitationParticles(void) {
    f32 x;
    f32 y;
    f32 z;
    f32 dx;
    f32 dy;
    f32 dz;
    f32 jx;
    f32 jy;
    u32 cx;
    u32 cy;
    s32 i;
    s16(*pp)[3];
    f32 cur[3];
    s32 n;
    s16 sjx;
    s16 sjy;
    s16 sdx;
    s16 sdy;
    s16 sdz;
    /* This unused slot preserves the original IDO frame layout. */
    s32 u[3];
    f32 v[3];

    UpdateWindDirection();
    if (g_dwRainMode != 0) {
        UpdateLightningFlash();
    }
    g_dwRainParticleCount = 512 / g_dwNumViewports;
    for (n = 0; n < g_dwNumViewports; n++) {
        Vec3fMulAdd((Vec3f *)cur,
                    &g_aVehicles[n].activeCamera->position,
                    &g_aVehicles[n].activeCamera->forward,
                    3.0f);
        if (g_bRainParticlesInitialized == 0) {
            InitRandomRainParticles();
            x = cur[0];
            y = cur[1];
            z = cur[2];
            g_avRainPrevCameraPos[0].x = x;
            g_avRainPrevCameraPos[1].x = x;
            g_avRainPrevCameraPos[0].y = y;
            g_avRainPrevCameraPos[1].y = y;
            g_avRainPrevCameraPos[0].z = z;
            g_avRainPrevCameraPos[1].z = z;
            g_bRainParticlesInitialized = 1;
        }
        x = cur[0];
        y = cur[1];
        z = cur[2];
        if (g_dwSnowMode == 0 && g_dwRainMode == 0) {
            return;
        }
        dx = x - g_avRainPrevCameraPos[n].x;
        dy = y - g_avRainPrevCameraPos[n].y;
        dz = z - g_avRainPrevCameraPos[n].z;
        g_fRainCameraSpeed = sqrtf(dx * dx + dy * dy + dz * dz) / g_fFrameDeltaTime;
        if (g_fRainCameraSpeed > 0.27777777f) {
            g_fRainCameraSpeedScale =
                sqrtf(g_fRainCameraSpeed * 3.6000001f) * 0.27777777f / g_fRainCameraSpeed;
            g_fRainCameraSpeed = g_fRainCameraSpeed * g_fRainCameraSpeedScale;
            dx = dx * g_fRainCameraSpeedScale;
            dy = dy * g_fRainCameraSpeedScale;
            dz = dz * g_fRainCameraSpeedScale;
        } else {
            g_fRainCameraSpeedScale = 1.0f;
        }
        if (g_dwSnowMode != 0) {
            dz += g_fFrameDeltaTime * 0.5f;
        } else {
            g_avRainVelocity[n].x = cosf(g_fWindAngle) * (g_fFrameDeltaTime * g_fWindStrength);
            g_avRainVelocity[n].y = sinf(g_fWindAngle) * (g_fFrameDeltaTime * g_fWindStrength);
            g_avRainVelocity[n].z = g_fFrameDeltaTime + g_fFrameDeltaTime;
            v[0] = dx;
            v[1] = dy;
            v[2] = dz;
            dx += g_avRainVelocity[n].x;
            dy += g_avRainVelocity[n].y;
            dz += g_avRainVelocity[n].z;
            Vec3fScaleInPlace((Vec3f *)v, g_fRainCameraSpeedScale * 0.5f);
            Vec3fAddInPlace(&g_avRainVelocity[n], (Vec3f *)v);
        }
        if (g_dwSnowMode != 0) {
            jx = ((s32)(AdvanceRng() & 0xffff) * 3.05180437862873e-05f - 1.0f) * g_fFrameDeltaTime *
                 0.25f;
            jy = ((s32)(AdvanceRng() & 0xffff) * 3.05180437862873e-05f - 1.0f) * g_fFrameDeltaTime *
                 0.25f;
        } else {
            jx = jy = 0.0f;
        }
        x = cur[0];
        y = cur[1];
        z = cur[2];
        g_avRainPrevCameraPos[n].x = x;
        g_avRainPrevCameraPos[n].y = y;
        g_avRainPrevCameraPos[n].z = z;
        cx = AdvanceRng() & 0xf;
        cy = AdvanceRng() & 0xf;
        sjx = jx * 16383.5;
        sjy = jy * 16383.5;
        pp = g_awRainParticlePos[n];
        sdx = dx * 16383.5;
        sdy = dy * 16383.5;
        sdz = dz * 16383.5;
        for (i = 0; i < g_dwRainParticleCount; i++) {
            if (cx != 0) {
                cx--;
                pp[i][0] += sdx;
            } else {
                pp[i][0] = pp[i][0] + sdx + sjx;
                sjx = -sjx;
                cx = AdvanceRng() & 0xf;
            }
            if (cy != 0) {
                cy--;
                pp[i][1] += sdy;
            } else {
                pp[i][1] = pp[i][1] + sdy + sjy;
                sjy = -sjy;
                cy = AdvanceRng() & 0xf;
            }
            pp[i][2] += sdz;
        }
    }
}
