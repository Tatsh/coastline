#include <PR/gbi.h>

#include "common/vehicleaccess.h"
#include "common/vehicledeform.h"
#include "common/vehiclemodel.h"

void DeformVehicleMeshByImpact(Vehicle *vehicle, Vec3f *direction, s16 amount) {
    s32 a;
    s32 xmax;
    s32 xmin;
    s32 ymax;
    s32 ymin;
    s32 off;
    s32 ax;
    s32 ay;
    s32 az;
    s32 k;
    s32 j;
    u32 *dl;
    u32 w;
    Vtx_t *v;
    s32 x;
    s32 y;
    s32 z;

    a = (s32)(Atan2f(direction->x, direction->y) * 180.0f / 3.1415927f);
    if (a < 20 || a >= 340) {
        xmax = 0x3fff;
        xmin = 0xff;
        ymax = 0x3fff;
        ymin = -0x3fff;
        a = 0;
    } else if (a < 50) {
        xmax = 0x3fff;
        xmin = 0x80;
        ymax = 0x3fff;
        ymin = 0x40;
        a = 1;
    } else if (a < 130) {
        xmax = 0x3fff;
        xmin = -0x3fff;
        ymax = 0x3fff;
        ymin = 0x40;
        a = 2;
    } else if (a < 160) {
        xmax = -0x80;
        xmin = -0x3fff;
        ymax = 0x3fff;
        ymin = 0x40;
        a = 3;
    } else if (a < 200) {
        xmax = -0xff;
        xmin = -0x3fff;
        ymax = 0x3fff;
        ymin = -0x3fff;
        a = 4;
    } else if (a < 230) {
        xmax = -0x80;
        xmin = -0x3fff;
        ymax = -0x40;
        ymin = -0x3fff;
        a = 5;
    } else if (a < 310) {
        xmax = 0x3fff;
        xmin = -0x3fff;
        ymax = -0x40;
        ymin = -0x3fff;
        a = 6;
    } else {
        xmax = 0x3fff;
        xmin = 0x80;
        ymax = -0x40;
        ymin = -0x3fff;
        a = 7;
    }
    amount <<= 2;
    if (VEHICLE_GET(vehicle, Dents)[a] < 0x100) {
        VEHICLE_GET(vehicle, Dents)[a] += amount;
        VEHICLE_SET(vehicle, DentJitter, ((VEHICLE_GET(vehicle, DentJitter) + 5) & 7) - 4);
        off = VEHICLE_GET(vehicle, DentJitter);
        if (off < 0) {
            off++;
        }
        if (1.0f < direction->y) {
            ay = (amount) << 16 >> 16;
        } else if (0.0f < direction->y) {
            ay = (amount >> 1) << 16 >> 16;
        } else if (direction->y < -1.0f) {
            ay = (-amount) << 16 >> 16;
        } else {
            ay = (-(amount >> 1)) << 16 >> 16;
        }
        if (1.0f < direction->z) {
            az = (amount >> 2) << 16 >> 16;
        } else if (0.0f < direction->z) {
            az = (amount >> 3) << 16 >> 16;
        } else if (direction->z < -1.0f) {
            az = (-(amount >> 2)) << 16 >> 16;
        } else {
            az = (-(amount >> 3)) << 16 >> 16;
        }
        if (1.25f < direction->x) {
            ax = (amount) << 16 >> 16;
        } else if (0.0f < direction->x) {
            ax = (amount >> 1) << 16 >> 16;
        } else if (direction->x < -1.25f) {
            ax = (-amount) << 16 >> 16;
            az = (az << 1) << 16 >> 16;
        } else {
            ax = (s16)-amount;
        }
        for (k = 0; k < 3; k++) {
            for (j = 0; j < 10; j++) {
                if (j != 9 && (dl = (u32 *)VEHICLE_GET(vehicle, Model)->displayLists[k][j]) != 0) {
                    for (;;) {
                        switch (*dl >> 24) {
                        case 4:
                            w = (*dl++ >> 10) & 0x3f;
                            v = (Vtx_t *)*dl++;
                            while (w--) {
                                x = (s16)(v->ob[0] + off);
                                if (xmin < x && x < xmax) {
                                    y = (s16)(v->ob[1] + off);
                                    if (ymin < y && y < ymax) {
                                        z = (s16)(v->ob[2] + off);
                                        if (-0x30 < z && z < 0xe0) {
                                            if (y & 0x80) {
                                                v->ob[0] =
                                                    v->ob[0] + ((s32)(ax * (4 - (y & 0xfU))) >> 5);
                                            } else {
                                                v->ob[0] =
                                                    v->ob[0] + ((s32)(ax * ((y & 0xfU) - 12)) >> 5);
                                            }
                                            if (x & 0x80) {
                                                v->ob[1] =
                                                    v->ob[1] + ((s32)(ay * (4 - (x & 0xfU))) >> 5);
                                            } else {
                                                v->ob[1] =
                                                    v->ob[1] + ((s32)(ay * ((x & 0xfU) - 12)) >> 5);
                                            }
                                            x = (s16)(x + y);
                                            if (x & 0x80) {
                                                v->ob[2] =
                                                    v->ob[2] + ((s32)(az * (8 - (x & 0xfU))) >> 6);
                                            } else {
                                                v->ob[2] =
                                                    v->ob[2] + ((s32)(az * ((x & 0xfU) - 8)) >> 6);
                                            }
                                        }
                                    }
                                }
                                v = (Vtx_t *)((Vtx *)v + 1);
                            }
                            break;
                        case 0xb8:
                            goto next;
                        default:
                            dl++;
                            dl++;
                            break;
                        }
                    }
                next:;
                }
            }
        }
    }
}
