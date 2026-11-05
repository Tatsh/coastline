#include "common/vehiclesound.h"

#include "common/audiomix.h"
#include "common/game.h"
#include "common/pause.h"
#include "common/rumble.h"
#include "common/soundchannel.h"
#include "common/soundeffect.h"
#include "common/soundpan.h"
#include "common/soundsource.h"
#include "common/track.h"
#include "common/vecmath.h"
#include "common/weather.h"

void UpdateVehicleSounds(Vehicle *car) {
    float u;
    float f;
    int k;
    Vehicle *view;
    CameraFrame *lst;
    int remote;
    float r;
    float l;
    int vol;
    int prev;
    int ground;
    int n;
    int m;
    Vec3f *pos;
    MixerChannel *ch;
    float *p;
    view = &g_aVehicles[g_aViewportRects[0].vehicleIndex];
    k = car->index << 1;
    if (g_dwRacePauseState != 0 || car->racer == 0) {
        g_aSoundEffectChannels[k].volume = 0;
        g_aSoundEffectChannels[k].step = 0;
        return;
    }
    remote = 0;
    if (g_dwNumViewports == 1) {
        lst = view->activeCamera;
        if (car->cameraChanged != 0 ||
            g_aVehicles[g_aViewportRects[0].vehicleIndex].cameraChanged != 0) {
            remote = 1;
        }
    } else {
        lst = car->activeCamera;
        if (car->cameraChanged != 0 ||
            g_aVehicles[g_aViewportRects[0].vehicleIndex].cameraChanged != 0 ||
            g_aVehicles[g_aViewportRects[1].vehicleIndex].cameraChanged != 0) {
            remote = 1;
        }
    }
    pos = &car->frame.position;
    car->dopplerFactor = ComputeDopplerFactor(
        pos, &car->previousPosition, &lst->position, &car->previousListenerPosition);
    ComputeStereoPan(pos, lst, &r, &l, &vol, 0);
    if (car->engineRpm > 0.0f) {
        u = car->engineRpm * 0.5f * 0.0007142857f * 11000.0f;
    } else {
        u = car->engineRpm * -0.5f * 0.0007142857f * 11000.0f;
    }
    f = car->dopplerFactor * u;
    if (f > 100000.0f) {
        f = 0;
    } else if (f < 0) {
        f = 0;
    }
    if (car->index == 0) {
        if (g_LoadedTrack.instances[car->groundInstances[0]].flags & 0x10) {
            g_dwSoundEffectEchoOffset = 11000;
        } else {
            g_dwSoundEffectEchoOffset = 0;
        }
    }
    if (remote == 0) {
        g_aSoundEffectChannels[k].step = f * 9.090909e-05f * 4294967296.0;
        f = vol;
        g_aSoundEffectChannels[k].volume = ((int)(f * r) << 16) + l * f;
    }
    ch = &g_aSoundEffectChannels[k];
    prev = car->effectSound;
    if (ch[1].step == 0 ||
        (car->body.skidding == 0 && car->effectSound >= 4 && car->effectSound < 8)) {
        car->effectSound = 0;
        car->effectVolume = 0;
        car->effectPriority = 0;
    }
    m = car->body.vehicleImpactVolume;
    if (car->effectVolume < car->body.vehicleImpactVolume ||
        (car->body.vehicleImpactVolume != 0 && car->effectPriority < 100)) {
        car->effectVolume = m;
        car->effectSound = 1;
        car->effectPriority = 100;
        if (car->index < (s32)g_dwNumLocalPlayers) {
            g_awRumbleCounter[car->index] = 0;
            g_awRumbleMotorOn[car->index] = 0;
            g_awRumbleOnFrames[car->index] = 5;
            g_awRumbleOffFrames[car->index] = 1;
            g_awRumbleFramesLeft[car->index] = 30;
            g_awRumbleActive[car->index] = 1;
        }
    }
    car->body.vehicleImpactVolume = 0;
    m = car->body.impactVolume;
    if (car->effectVolume < car->body.impactVolume ||
        (car->body.impactVolume != 0 && car->effectPriority < 90)) {
        car->effectVolume = m;
        car->effectSound = 2;
        car->effectPriority = 90;
        if (car->index < (s32)g_dwNumLocalPlayers) {
            g_awRumbleCounter[car->index] = 0;
            g_awRumbleMotorOn[car->index] = 0;
            g_awRumbleOnFrames[car->index] = 100;
            g_awRumbleOffFrames[car->index] = 1;
            g_awRumbleFramesLeft[car->index] = 30;
            g_awRumbleActive[car->index] = 1;
        }
    }
    car->body.impactVolume = 0;
    m = car->body.landed;
    if (car->effectVolume < car->body.landed ||
        (car->body.landed != 0 && car->effectPriority < 80)) {
        car->effectVolume = m;
        car->effectSound = 3;
        car->effectPriority = 80;
        if (car->index < (s32)g_dwNumLocalPlayers) {
            g_awRumbleCounter[car->index] = 0;
            g_awRumbleMotorOn[car->index] = 0;
            g_awRumbleOnFrames[car->index] = 3;
            g_awRumbleOffFrames[car->index] = 2;
            g_awRumbleFramesLeft[car->index] = 20;
            g_awRumbleActive[car->index] = 1;
        }
    }
    car->body.landed = 0;
    if (!(car->effectSound != 0 && (car->effectSound < 4 || car->effectSound > 7) &&
          (car->effectSound < 8 || car->effectSound > 12))) {
        car->body.unreconstructed202 = 0;
        ground = -1;
        if (car->wheelBodies[1].contactFrames != 0) {
            ground = car->wheelBodies[1].surface;
        }
        m = car->body.skidding;
        if (car->effectVolume < car->body.skidding ||
            (car->body.skidding != 0 && car->effectPriority < 40) ||
            (car->body.skidding != 0 && car->effectPriority == 40 && prev >= 4 && prev < 8)) {
            car->effectVolume = m;
            if (g_dwCurrentWeather == 3) {
                switch (ground) {
                case 0:
                case 1:
                case 2:
                case 3:
                    car->effectSound = 7;
                    car->effectVolume = m >> 1;
                    break;
                case 4:
                    car->effectSound = 7;
                    car->effectVolume = 0;
                    break;
                }
            } else {
                switch (ground) {
                case 0:
                case 3:
                    if (car->index < (s32)g_dwNumLocalPlayers &&
                        g_awRumbleActive[car->index] == 0) {
                        g_awRumbleCounter[car->index] = 0;
                        g_awRumbleMotorOn[car->index] = 0;
                        g_awRumbleOnFrames[car->index] = 1;
                        g_awRumbleOffFrames[car->index] = 9;
                        g_awRumbleFramesLeft[car->index] = 10;
                        g_awRumbleActive[car->index] = 1;
                    }
                    car->effectSound = 7;
                    m = car->body.skidding;
                    break;
                case 1:
                    if (car->index < (s32)g_dwNumLocalPlayers &&
                        g_awRumbleActive[car->index] == 0) {
                        g_awRumbleCounter[car->index] = 0;
                        g_awRumbleMotorOn[car->index] = 0;
                        g_awRumbleOnFrames[car->index] = 1;
                        g_awRumbleOffFrames[car->index] = 14;
                        g_awRumbleFramesLeft[car->index] = 15;
                        g_awRumbleActive[car->index] = 1;
                    }
                    car->effectSound = 5;
                    m = car->body.skidding;
                    break;
                case 2:
                    if (car->index < (s32)g_dwNumLocalPlayers &&
                        g_awRumbleActive[car->index] == 0) {
                        g_awRumbleCounter[car->index] = 0;
                        g_awRumbleMotorOn[car->index] = 0;
                        g_awRumbleOnFrames[car->index] = 1;
                        g_awRumbleOffFrames[car->index] = 12;
                        g_awRumbleFramesLeft[car->index] = 13;
                        g_awRumbleActive[car->index] = 1;
                    }
                    car->effectSound = 4;
                    m = car->body.skidding;
                    break;
                case 4:
                    car->effectSound = 12;
                    m = car->body.skidding;
                    break;
                }
                car->effectVolume = m >> 1;
            }
            car->effectPriority = 40;
            if (car->skidFrames++ > 16) {
                car->skidFrames = 16;
            }
            car->effectVolume *= car->skidFrames;
            car->effectVolume /= 12;
        } else {
            car->skidFrames = 0;
        }
        if (!(car->effectSound != 0 && (car->effectSound < 8 || car->effectSound > 12))) {
            if (g_dwCurrentWeather == 3) {
                switch (ground) {
                case 0:
                case 1:
                case 2:
                case 3:
                    car->effectSound = 11;
                    break;
                case 4:
                    car->effectSound = 10;
                    break;
                default:
                    car->effectSound = 0;
                    break;
                }
            } else {
                switch (ground) {
                case 0:
                case 3:
                    car->effectSound = 10;
                    break;
                case 1:
                    car->effectSound = 8;
                    break;
                case 2:
                    car->effectSound = 9;
                    break;
                case 4:
                    car->effectSound = 12;
                    break;
                default:
                    car->effectSound = 0;
                    break;
                }
            }
            car->effectPriority = 1;
            car->effectVolume = Vec3fLength(&car->placedVelocity) * 64.0f / 27.0f;
            if (car->effectVolume > 64) {
                car->effectVolume = 64;
            }
        }
    }
    if (car->effectSound == 0) {
        if (ch == &g_aSoundEffectChannels[0]) {
            g_aSoundEffectChannels[k + 1].volume = 0;
            g_aSoundEffectChannels[k + 1].step = 0;
        }
    } else {
        f = car->dopplerFactor * 11000.0f;
        if (f > 20000.0f) {
            f = 0;
        } else if (f < 0) {
            f = 0;
        }
        if (car->effectSound != prev && k == 0) {
            StartSoundChannelMuted(k + 1,
                                   g_aSoundEffects[car->effectSound].sample,
                                   g_aSoundEffects[car->effectSound].length,
                                   g_aSoundEffects[car->effectSound].loopLength);
        }
        if (remote == 0 && ch == &g_aSoundEffectChannels[0]) {
            n = (car->effectVolume * vol) >> 7;
            g_aSoundEffectChannels[k + 1].step = f * 9.090909e-05f * 4294967296.0;
            f = n;
            g_aSoundEffectChannels[k + 1].volume = ((int)(f * r) << 16) + l * f;
        }
    }
    if (g_dwNumViewports == 1) {
        p = &view->activeCamera->position.x;
        car->previousListenerPosition.x = p[0];
        car->previousListenerPosition.y = p[1];
        car->previousListenerPosition.z = p[2];
    } else {
        p = &car->activeCamera->position.x;
        car->previousListenerPosition.x = p[0];
        car->previousListenerPosition.y = p[1];
        car->previousListenerPosition.z = p[2];
    }
}
