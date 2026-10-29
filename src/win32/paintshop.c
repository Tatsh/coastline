#include "win32/paintshop.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common/fileio.h"
#if TGR_PLATFORM == TGR_PLATFORM_SDL
#include "sdl/bitmap.h"
#endif

#define CAR_PAINT_SET(option, left, right, top, bottom, name)                                      \
    {{NULL, NULL, NULL, NULL}, option, left, right, top, bottom, name}
#define CAR_PAINT_SET_UNUSED CAR_PAINT_SET(-1, 0, 0, 0, 0, NULL)

CarPaintTextureSet g_CarPaintTextureSets[CAR_PAINT_GROUP_COUNT][CAR_PAINT_SET_COUNT] = {
    {
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET(-1, 0, 0, 0, 0, "ce-hood.bmp"),
        CAR_PAINT_SET(-1, 0, 0, 0, 0, "ce-roof.bmp"),
        CAR_PAINT_SET(-1, 0, 112, 0, 64, "ce-tail0.bmp"),
        CAR_PAINT_SET(-1, 0, 0, 0, 0, "ce-headl.bmp"),
        CAR_PAINT_SET(4, 0, 128, 28, 128, "ce-back.bmp"),
        CAR_PAINT_SET(3, 0, 128, 28, 128, "ce-lr.bmp"),
        CAR_PAINT_SET(5, 0, 128, 28, 128, "ce-rr.bmp"),
        CAR_PAINT_SET(7, 0, 128, 48, 128, "ce-rf.bmp"),
        CAR_PAINT_SET(1, 0, 128, 48, 128, "ce-lf.bmp"),
        CAR_PAINT_SET(6, 0, 160, 52, 128, "ce-rside.bmp"),
        CAR_PAINT_SET(2, 0, 160, 52, 128, "ce-lside.bmp"),
        CAR_PAINT_SET(0, 0, 0, 0, 0, "ce-front.bmp"),
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET(-1, 0, 112, 0, 64, "ce-tail0.bmp"),
        CAR_PAINT_SET(-1, 0, 112, 0, 64, "ce-tail1.bmp"),
        CAR_PAINT_SET(-1, 0, 112, 0, 64, "ce-tail2.bmp"),
        CAR_PAINT_SET(-1, 0, 112, 0, 64, "ce-tail3.bmp"),
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
    },
    {
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET(-1, 0, 0, 0, 0, "es-hood.bmp"),
        CAR_PAINT_SET(-1, 0, 0, 0, 0, "es-roof.bmp"),
        CAR_PAINT_SET(-1, 0, 96, 24, 64, "es-headl.bmp"),
        CAR_PAINT_SET(-1, 0, 64, 32, 64, "es-tail0.bmp"),
        CAR_PAINT_SET(2, 0, 160, 48, 128, "es-lside.bmp"),
        CAR_PAINT_SET(6, 0, 160, 48, 128, "es-rside.bmp"),
        CAR_PAINT_SET(4, 0, 128, 28, 128, "es-back.bmp"),
        CAR_PAINT_SET(5, 0, 128, 28, 128, "es-rr.bmp"),
        CAR_PAINT_SET(3, 0, 128, 28, 128, "es-lr.bmp"),
        CAR_PAINT_SET(7, 0, 128, 48, 128, "es-rf.bmp"),
        CAR_PAINT_SET(1, 0, 128, 48, 128, "es-lf.bmp"),
        CAR_PAINT_SET(0, 0, 0, 0, 0, "es-front.bmp"),
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET(-1, 0, 64, 32, 64, "es-tail0.bmp"),
        CAR_PAINT_SET(-1, 0, 64, 32, 64, "es-tail1.bmp"),
        CAR_PAINT_SET(-1, 0, 64, 32, 64, "es-tail2.bmp"),
        CAR_PAINT_SET(-1, 0, 64, 32, 64, "es-tail3.bmp"),
        CAR_PAINT_SET_UNUSED,
    },
    {
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET(-1, 0, 0, 0, 0, "ns-hood.bmp"),
        CAR_PAINT_SET(-1, 0, 0, 0, 0, "ns-roof.bmp"),
        CAR_PAINT_SET(-1, 0, 112, 4, 64, "ns-tail0.bmp"),
        CAR_PAINT_SET(-1, 0, 0, 0, 0, "ns-headl.bmp"),
        CAR_PAINT_SET(0, 0, 0, 0, 0, "ns-front.bmp"),
        CAR_PAINT_SET(7, 0, 128, 48, 128, "ns-rf.bmp"),
        CAR_PAINT_SET(1, 0, 128, 48, 128, "ns-lf.bmp"),
        CAR_PAINT_SET(3, 0, 128, 28, 128, "ns-lr.bmp"),
        CAR_PAINT_SET(5, 0, 128, 28, 128, "ns-rr.bmp"),
        CAR_PAINT_SET(4, 0, 128, 28, 128, "ns-back.bmp"),
        CAR_PAINT_SET(6, 0, 160, 52, 128, "ns-rside.bmp"),
        CAR_PAINT_SET(2, 0, 160, 52, 128, "ns-lside.bmp"),
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET(-1, 0, 112, 4, 64, "ns-tail0.bmp"),
        CAR_PAINT_SET(-1, 0, 112, 4, 64, "ns-tail1.bmp"),
        CAR_PAINT_SET(-1, 0, 112, 4, 64, "ns-tail2.bmp"),
        CAR_PAINT_SET(-1, 0, 112, 4, 64, "ns-tail3.bmp"),
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
    },
    {
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET(-1, 0, 0, 0, 0, "rs-hood.bmp"),
        CAR_PAINT_SET(-1, 0, 0, 0, 0, "rs-roof.bmp"),
        CAR_PAINT_SET(-1, 0, 0, 0, 0, "rs-headl.bmp"),
        CAR_PAINT_SET(-1, 0, 96, 8, 32, "rs-tail0.bmp"),
        CAR_PAINT_SET(0, 0, 0, 0, 0, "rs-front.bmp"),
        CAR_PAINT_SET(1, 0, 128, 52, 128, "rs-lf.bmp"),
        CAR_PAINT_SET(7, 0, 128, 52, 128, "rs-rf.bmp"),
        CAR_PAINT_SET(3, 0, 128, 28, 128, "rs-lr.bmp"),
        CAR_PAINT_SET(4, 0, 128, 28, 128, "rs-back.bmp"),
        CAR_PAINT_SET(5, 0, 128, 28, 128, "rs-rr.bmp"),
        CAR_PAINT_SET(2, 0, 160, 52, 128, "rs-lside.bmp"),
        CAR_PAINT_SET(6, 0, 160, 52, 128, "rs-rside.bmp"),
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET(-1, 0, 96, 8, 32, "rs-tail0.bmp"),
        CAR_PAINT_SET(-1, 0, 96, 8, 32, "rs-tail1.bmp"),
        CAR_PAINT_SET(-1, 0, 96, 8, 32, "rs-tail2.bmp"),
        CAR_PAINT_SET(-1, 0, 96, 8, 32, "rs-tail3.bmp"),
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
    },
    {
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET(-1, 0, 0, 0, 0, "sp-hood.bmp"),
        CAR_PAINT_SET(-1, 0, 0, 0, 0, "sp-roof.bmp"),
        CAR_PAINT_SET(-1, 0, 112, 20, 64, "sp-headl.bmp"),
        CAR_PAINT_SET(-1, 0, 128, 28, 64, "sp-tail0.bmp"),
        CAR_PAINT_SET(0, 0, 0, 0, 0, "sp-front.bmp"),
        CAR_PAINT_SET(4, 0, 128, 28, 128, "sp-back.bmp"),
        CAR_PAINT_SET(3, 0, 128, 28, 128, "sp-lr.bmp"),
        CAR_PAINT_SET(5, 0, 128, 28, 128, "sp-rr.bmp"),
        CAR_PAINT_SET(7, 0, 128, 52, 128, "sp-rf.bmp"),
        CAR_PAINT_SET(1, 0, 128, 52, 128, "sp-lf.bmp"),
        CAR_PAINT_SET(6, 0, 160, 52, 128, "sp-rside.bmp"),
        CAR_PAINT_SET(2, 0, 160, 52, 128, "sp-lside.bmp"),
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET(-1, 0, 128, 28, 64, "sp-tail0.bmp"),
        CAR_PAINT_SET(-1, 0, 128, 28, 64, "sp-tail1.bmp"),
        CAR_PAINT_SET(-1, 0, 128, 28, 64, "sp-tail2.bmp"),
        CAR_PAINT_SET(-1, 0, 128, 28, 64, "sp-tail3.bmp"),
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
    },
    {
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET(0, 0, 0, 0, 0, "ps-front.bmp"),
        CAR_PAINT_SET(-1, 0, 0, 0, 0, "ps-hood.bmp"),
        CAR_PAINT_SET(-1, 0, 0, 0, 0, "ps-roof.bmp"),
        CAR_PAINT_SET(-1, 0, 28, 20, 64, "ps-headl.bmp"),
        CAR_PAINT_SET(-1, 0, 64, 12, 32, "ps-tail0.bmp"),
        CAR_PAINT_SET(6, 0, 160, 52, 128, "ps-rside.bmp"),
        CAR_PAINT_SET(2, 0, 160, 52, 128, "ps-lside.bmp"),
        CAR_PAINT_SET(7, 0, 128, 48, 128, "ps-rf.bmp"),
        CAR_PAINT_SET(1, 0, 128, 48, 128, "ps-lf.bmp"),
        CAR_PAINT_SET(3, 0, 128, 28, 128, "ps-lr.bmp"),
        CAR_PAINT_SET(5, 0, 128, 28, 128, "ps-rr.bmp"),
        CAR_PAINT_SET(4, 0, 128, 28, 128, "ps-back.bmp"),
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET(-1, 0, 64, 12, 32, "ps-tail0.bmp"),
        CAR_PAINT_SET(-1, 0, 64, 12, 32, "ps-tail1.bmp"),
        CAR_PAINT_SET(-1, 0, 64, 12, 32, "ps-tail2.bmp"),
        CAR_PAINT_SET(-1, 0, 64, 12, 32, "ps-tail3.bmp"),
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
    },
    {
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET(-1, 0, 0, 0, 0, "m3-hood.bmp"),
        CAR_PAINT_SET(-1, 0, 0, 0, 0, "m3-roof.bmp"),
        CAR_PAINT_SET(-1, 0, 100, 16, 64, "m3-tail0.bmp"),
        CAR_PAINT_SET(-1, 0, 96, 28, 64, "m3-headl.bmp"),
        CAR_PAINT_SET(0, 0, 0, 0, 0, "m3-front.bmp"),
        CAR_PAINT_SET(3, 0, 128, 28, 128, "m3-lr.bmp"),
        CAR_PAINT_SET(4, 0, 128, 28, 128, "m3-back.bmp"),
        CAR_PAINT_SET(5, 0, 128, 28, 128, "m3-rr.bmp"),
        CAR_PAINT_SET(1, 0, 128, 52, 128, "m3-lf.bmp"),
        CAR_PAINT_SET(7, 0, 128, 52, 128, "m3-rf.bmp"),
        CAR_PAINT_SET(6, 0, 160, 52, 128, "m3-rside.bmp"),
        CAR_PAINT_SET(2, 0, 160, 52, 128, "m3-lside.bmp"),
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET(-1, 0, 100, 16, 64, "m3-tail0.bmp"),
        CAR_PAINT_SET(-1, 0, 100, 16, 64, "m3-tail1.bmp"),
        CAR_PAINT_SET(-1, 0, 100, 16, 64, "m3-tail2.bmp"),
        CAR_PAINT_SET(-1, 0, 100, 16, 64, "m3-tail3.bmp"),
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
    },
    {
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET(-1, 0, 0, 0, 0, "ip-hood.bmp"),
        CAR_PAINT_SET(-1, 0, 0, 0, 0, "ip-roof.bmp"),
        CAR_PAINT_SET(-1, 0, 0, 0, 0, "ip-tail0.bmp"),
        CAR_PAINT_SET(-1, 0, 128, 16, 64, "ip-headl.bmp"),
        CAR_PAINT_SET(0, 0, 0, 0, 0, "ip-front.bmp"),
        CAR_PAINT_SET(7, 0, 128, 52, 128, "ip-rf.bmp"),
        CAR_PAINT_SET(1, 0, 128, 52, 128, "ip-lf.bmp"),
        CAR_PAINT_SET(3, 0, 128, 28, 128, "ip-lr.bmp"),
        CAR_PAINT_SET(4, 0, 128, 28, 128, "ip-back.bmp"),
        CAR_PAINT_SET(5, 0, 128, 28, 128, "ip-rr.bmp"),
        CAR_PAINT_SET(6, 0, 160, 52, 128, "ip-rside.bmp"),
        CAR_PAINT_SET(2, 0, 160, 52, 128, "ip-lside.bmp"),
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET(-1, 0, 0, 0, 0, "ip-tail0.bmp"),
        CAR_PAINT_SET(-1, 0, 0, 0, 0, "ip-tail1.bmp"),
        CAR_PAINT_SET(-1, 0, 0, 0, 0, "ip-tail2.bmp"),
        CAR_PAINT_SET(-1, 0, 0, 0, 0, "ip-tail3.bmp"),
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
    },
    {
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET(-1, 0, 0, 0, 0, "ld-hood.bmp"),
        CAR_PAINT_SET(-1, 0, 0, 0, 0, "ld-roof.bmp"),
        CAR_PAINT_SET(-1, 0, 96, 20, 64, "ld-headl.bmp"),
        CAR_PAINT_SET(-1, 0, 64, 32, 64, "ld-tail0.bmp"),
        CAR_PAINT_SET(2, 0, 160, 48, 128, "ld-lside.bmp"),
        CAR_PAINT_SET(6, 0, 160, 48, 128, "ld-rside.bmp"),
        CAR_PAINT_SET(4, 0, 128, 28, 128, "ld-back.bmp"),
        CAR_PAINT_SET(5, 0, 128, 28, 128, "ld-rr.bmp"),
        CAR_PAINT_SET(3, 0, 128, 28, 128, "ld-lr.bmp"),
        CAR_PAINT_SET(7, 0, 128, 48, 128, "ld-rf.bmp"),
        CAR_PAINT_SET(1, 0, 128, 48, 128, "ld-lf.bmp"),
        CAR_PAINT_SET(0, 0, 0, 0, 0, "ld-front.bmp"),
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET(-1, 0, 64, 32, 64, "ld-tail0.bmp"),
        CAR_PAINT_SET(-1, 0, 64, 32, 64, "ld-tail1.bmp"),
        CAR_PAINT_SET(-1, 0, 64, 32, 64, "ld-tail2.bmp"),
        CAR_PAINT_SET(-1, 0, 64, 32, 64, "ld-tail3.bmp"),
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
    },
    {
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET(-1, 0, 128, 24, 128, "hm-sidet.bmp"),
        CAR_PAINT_SET(-1, 0, 0, 0, 0, "hm-step.bmp"),
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET(-1, 0, 0, 0, 0, "hm-sideb.bmp"),
        CAR_PAINT_SET(-1, 0, 0, 0, 0, "hm-roof.bmp"),
        CAR_PAINT_SET(-1, 0, 128, 12, 64, "hm-grill.bmp"),
        CAR_PAINT_SET(4, 0, 0, 0, 0, "hm-back.bmp"),
        CAR_PAINT_SET(-1, 0, 48, 12, 32, "hm-tail0.bmp"),
        CAR_PAINT_SET(0, 0, 128, 24, 64, "hm-front.bmp"),
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET(-1, 0, 48, 12, 32, "hm-tail0.bmp"),
        CAR_PAINT_SET(-1, 0, 48, 12, 32, "hm-tail1.bmp"),
        CAR_PAINT_SET(-1, 0, 48, 12, 32, "hm-tail2.bmp"),
        CAR_PAINT_SET(-1, 0, 48, 12, 32, "hm-tail3.bmp"),
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
    },
    {
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET(4, 0, 128, 28, 128, "mt-back.bmp"),
        CAR_PAINT_SET(5, 0, 128, 28, 128, "mt-rr.bmp"),
        CAR_PAINT_SET(3, 0, 128, 28, 128, "mt-lr.bmp"),
        CAR_PAINT_SET(-1, 0, 0, 0, 0, "mt-hood.bmp"),
        CAR_PAINT_SET(-1, 0, 0, 0, 0, "mt-roof.bmp"),
        CAR_PAINT_SET(7, 0, 128, 52, 128, "mt-rf.bmp"),
        CAR_PAINT_SET(1, 0, 128, 52, 128, "mt-lf.bmp"),
        CAR_PAINT_SET(-1, 0, 128, 28, 64, "mt-bumpr.bmp"),
        CAR_PAINT_SET(0, 0, 0, 0, 0, "mt-front.bmp"),
        CAR_PAINT_SET(-1, 0, 0, 0, 0, "mt-headl.bmp"),
        CAR_PAINT_SET(-1, 0, 64, 32, 64, "mt-tail0.bmp"),
        CAR_PAINT_SET(2, 0, 160, 48, 128, "mt-lside.bmp"),
        CAR_PAINT_SET(6, 0, 160, 48, 128, "mt-rside.bmp"),
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET(-1, 0, 64, 32, 64, "mt-tail0.bmp"),
        CAR_PAINT_SET(-1, 0, 64, 32, 64, "mt-tail1.bmp"),
        CAR_PAINT_SET(-1, 0, 64, 32, 64, "mt-tail2.bmp"),
        CAR_PAINT_SET(-1, 0, 64, 32, 64, "mt-tail3.bmp"),
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
    },
    {
        CAR_PAINT_SET_UNUSED, CAR_PAINT_SET_UNUSED, CAR_PAINT_SET_UNUSED, CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED, CAR_PAINT_SET_UNUSED, CAR_PAINT_SET_UNUSED, CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED, CAR_PAINT_SET_UNUSED, CAR_PAINT_SET_UNUSED, CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED, CAR_PAINT_SET_UNUSED, CAR_PAINT_SET_UNUSED, CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED, CAR_PAINT_SET_UNUSED, CAR_PAINT_SET_UNUSED, CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED, CAR_PAINT_SET_UNUSED, CAR_PAINT_SET_UNUSED, CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED, CAR_PAINT_SET_UNUSED, CAR_PAINT_SET_UNUSED, CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED, CAR_PAINT_SET_UNUSED,
    },
    {
        CAR_PAINT_SET_UNUSED, CAR_PAINT_SET_UNUSED, CAR_PAINT_SET_UNUSED, CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED, CAR_PAINT_SET_UNUSED, CAR_PAINT_SET_UNUSED, CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED, CAR_PAINT_SET_UNUSED, CAR_PAINT_SET_UNUSED, CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED, CAR_PAINT_SET_UNUSED, CAR_PAINT_SET_UNUSED, CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED, CAR_PAINT_SET_UNUSED, CAR_PAINT_SET_UNUSED, CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED, CAR_PAINT_SET_UNUSED, CAR_PAINT_SET_UNUSED, CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED, CAR_PAINT_SET_UNUSED, CAR_PAINT_SET_UNUSED, CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED, CAR_PAINT_SET_UNUSED,
    },
    {
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET(-1, 0, 0, 0, 0, "pj-hood.bmp"),
        CAR_PAINT_SET(-1, 0, 0, 0, 0, "pj-roof.bmp"),
        CAR_PAINT_SET(-1, 0, 96, 24, 64, "pj-headl.bmp"),
        CAR_PAINT_SET(-1, 0, 0, 0, 0, "pj-tail0.bmp"),
        CAR_PAINT_SET(2, 0, 160, 52, 128, "pj-lside.bmp"),
        CAR_PAINT_SET(6, 0, 160, 52, 128, "pj-rside.bmp"),
        CAR_PAINT_SET(4, 0, 128, 28, 128, "pj-back.bmp"),
        CAR_PAINT_SET(3, 0, 128, 28, 128, "pj-lr-rr.bmp"),
        CAR_PAINT_SET(7, 0, 128, 52, 128, "pj-rf.bmp"),
        CAR_PAINT_SET(1, 0, 128, 52, 128, "pj-lf.bmp"),
        CAR_PAINT_SET(0, 0, 0, 0, 0, "pj-front.bmp"),
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET(-1, 0, 0, 0, 0, "pj-tail0.bmp"),
        CAR_PAINT_SET(-1, 0, 0, 0, 0, "pj-tail1.bmp"),
        CAR_PAINT_SET(-1, 0, 0, 0, 0, "pj-tail2.bmp"),
        CAR_PAINT_SET(-1, 0, 0, 0, 0, "pj-tail3.bmp"),
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
    },
    {
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET(-1, 0, 0, 0, 0, "tr-hood.bmp"),
        CAR_PAINT_SET(-1, 0, 0, 0, 0, "tr-roof.bmp"),
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET(-1, 0, 64, 32, 64, "tr-tail0.bmp"),
        CAR_PAINT_SET(-1, 0, 0, 0, 0, "tr-headl.bmp"),
        CAR_PAINT_SET(0, 0, 0, 0, 0, "tr-front.bmp"),
        CAR_PAINT_SET(3, 0, 128, 28, 128, "tr-lr.bmp"),
        CAR_PAINT_SET(5, 0, 128, 28, 128, "tr-rr.bmp"),
        CAR_PAINT_SET(4, 0, 128, 28, 128, "tr-back.bmp"),
        CAR_PAINT_SET(7, 0, 128, 52, 128, "tr-rf.bmp"),
        CAR_PAINT_SET(1, 0, 128, 52, 128, "tr-lf.bmp"),
        CAR_PAINT_SET(6, 0, 160, 52, 128, "tr-rside.bmp"),
        CAR_PAINT_SET(2, 0, 160, 52, 128, "tr-lside.bmp"),
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET(-1, 0, 64, 32, 64, "tr-tail0.bmp"),
        CAR_PAINT_SET(-1, 0, 64, 32, 64, "tr-tail1.bmp"),
        CAR_PAINT_SET(-1, 0, 64, 32, 64, "tr-tail2.bmp"),
        CAR_PAINT_SET(-1, 0, 64, 32, 64, "tr-tail3.bmp"),
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
    },
    {
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET(-1, 0, 0, 0, 0, "mn-hood.bmp"),
        CAR_PAINT_SET(-1, 0, 0, 0, 0, "mn-roof.bmp"),
        CAR_PAINT_SET(-1, 0, 100, 20, 64, "mn-headl.bmp"),
        CAR_PAINT_SET(-1, 0, 32, 88, 128, "mn-tail0.bmp"),
        CAR_PAINT_SET(2, 0, 160, 48, 128, "mn-lside.bmp"),
        CAR_PAINT_SET(6, 0, 160, 48, 128, "mn-rside.bmp"),
        CAR_PAINT_SET(4, 0, 128, 28, 128, "mn-back.bmp"),
        CAR_PAINT_SET(5, 0, 128, 28, 128, "mn-rr.bmp"),
        CAR_PAINT_SET(3, 0, 128, 28, 128, "mn-lr.bmp"),
        CAR_PAINT_SET(7, 0, 128, 52, 128, "mn-rf.bmp"),
        CAR_PAINT_SET(1, 0, 128, 52, 128, "mn-lf.bmp"),
        CAR_PAINT_SET(0, 0, 0, 0, 0, "mn-front.bmp"),
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET(-1, 0, 32, 88, 128, "mn-tail0.bmp"),
        CAR_PAINT_SET(-1, 0, 32, 88, 128, "mn-tail1.bmp"),
        CAR_PAINT_SET(-1, 0, 32, 88, 128, "mn-tail2.bmp"),
        CAR_PAINT_SET(-1, 0, 32, 88, 128, "mn-tail3.bmp"),
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
        CAR_PAINT_SET_UNUSED,
    },
};
void *g_apPaintDamageBitmaps[PAINT_DAMAGE_BITMAP_COUNT];
int g_dwColorTintRed;
int g_dwColorTintGreen;
int g_dwColorTintBlue;
int g_dwConvertedBitmapWidth;
int g_dwConvertedBitmapHeight;
int g_nSelectedPaintOption10 = -1;
int g_nPaintTextureGroup;
int g_nPaintTextureSetIndex;
int g_bCarPaintTexturesEnabled = 1;

void LoadCarPaintTextures(int group, int damageVariants) {
    CarPaintTextureSet *set = g_CarPaintTextureSets[group];
    char path[0x400];

    for (int i = 0; i < CAR_PAINT_SET_COUNT; i++, set++) {
        if (set->apBitmaps[0] != NULL || set->pszBitmapName == NULL) {
            continue;
        }
        sprintf(path, "Paint\\%s", set->pszBitmapName);
        if (!CHK_FileExists(path)) {
            continue;
        }
        set->apBitmaps[0] = LoadBitmapFromFile(path);
        path[8] = 'd';
        if (CHK_FileExists(path)) {
            unsigned char *mask = LoadBitmapFromFile(path);
            if (set->apBitmaps[0] != NULL && mask != NULL) {
                ModulateRgbaByMask(
                    set->apBitmaps[0], g_dwConvertedBitmapWidth, g_dwConvertedBitmapHeight, mask);
                free(mask);
            }
        }
        if (set->apBitmaps[0] == NULL || set->nOption10 < 0 || !damageVariants ||
            !g_bCarPaintTexturesEnabled) {
            continue;
        }
        for (int damage = 1; damage < CAR_PAINT_BITMAP_COUNT; damage++) {
            unsigned allocationSize =
                (unsigned)g_dwConvertedBitmapHeight * (unsigned)g_dwConvertedBitmapWidth * 4;
            unsigned char *destination = malloc(allocationSize);
            set->apBitmaps[damage] = destination;
            if (destination != NULL) {
                const unsigned char *source = set->apBitmaps[0];
                unsigned bytes =
                    (unsigned)g_dwConvertedBitmapHeight * (unsigned)g_dwConvertedBitmapWidth * 4;
                while (bytes != 0) {
                    unsigned word;
                    memcpy(&word, source, 4);
                    memcpy(destination, &word, 4);
                    source += 4;
                    destination += 4;
                    bytes -= 4;
                }
                ApplyPaintDamageOverlay(damage,
                                        set->apBitmaps[damage],
                                        g_dwConvertedBitmapWidth,
                                        g_dwConvertedBitmapHeight);
            }
        }
    }
}

void ApplyPaintDamageOverlay(int damage, unsigned char *pixels, int width, int height) {
    const unsigned char *mask = g_apPaintDamageBitmaps[damage - 1];
    unsigned rowCoordinate = 0;

    if (mask == NULL || height <= 0 || width <= 0) {
        return;
    }
    for (int y = 0; y < height; y++) {
        unsigned maskRow = (unsigned)((int)rowCoordinate / height) * 64;
        unsigned columnCoordinate = 0;
        for (int x = 0; x < width; x++) {
            unsigned maskColumn = (unsigned)((int)columnCoordinate / width);
            unsigned maskOffset = (maskRow + maskColumn) * 4;
            unsigned intensity = mask[(int)maskOffset];
            pixels[0] = pixels[0] * intensity / 255;
            pixels[1] = pixels[1] * intensity / 255;
            pixels[2] = pixels[2] * intensity / 255;
            pixels += 4;
            columnCoordinate += 64;
        }
        rowCoordinate += 64;
    }
}

void *GetPaintImage(int nGroup, int nSet, int nDamage) {
    CarPaintTextureSet *set = &g_CarPaintTextureSets[nGroup][nSet];
    g_nSelectedPaintOption10 = set->nOption10;
    return set->apBitmaps[nDamage];
}

void GetPaintRectangle(int nGroup, int nSet, int *pLeft, int *pRight, int *pTop, int *pBottom) {
    CarPaintTextureSet *set = &g_CarPaintTextureSets[nGroup][nSet];
    *pLeft = set->nLeft;
    *pRight = set->nRight;
    *pTop = set->nTop;
    *pBottom = set->nBottom;
}

int GetSelectedPaintOption(void) {
    return g_nSelectedPaintOption10;
}

int HasPaintTintPixels(const void *pImage, int nLeft, int nRight, int nTop, int nBottom) {
    int width = (int)((unsigned)nRight - (unsigned)nLeft);
    int height = (int)((unsigned)nBottom - (unsigned)nTop);
    if (pImage != NULL && width > 0 && height > 0) {
        const unsigned char *pixel = pImage;
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                if (pixel[0] == 0 && pixel[1] == pixel[2]) {
                    return 1;
                }
                pixel += 4;
            }
        }
    }
    return 0;
}

int BlitPaintImageToPixels(const void *pImage,
                           int nLeft,
                           int nRight,
                           int nTop,
                           int nBottom,
                           unsigned char *pDest,
                           int nWidth,
                           int nHeight) {
    int width = (int)((unsigned)nRight - (unsigned)nLeft);
    int height = (int)((unsigned)nBottom - (unsigned)nTop);
    if (pImage != NULL && width > 0 && height > 0) {
        const unsigned char *source = pImage;
        for (int y = 0; y < height; y++) {
            unsigned char *dest = pDest + ((nHeight - nTop - y - 1) * nWidth + nLeft) * 4;
            for (int x = 0; x < width; x++) {
                unsigned pixel;
                memcpy(&pixel, source, sizeof(pixel));
                memcpy(dest, &pixel, sizeof(pixel));
                if (source[0] == 0 && source[1] == source[2]) {
                    unsigned intensity = source[1];
                    dest[0] = (unsigned char)((int)(intensity * (unsigned)g_dwColorTintRed) / 255);
                    dest[1] =
                        (unsigned char)((int)(intensity * (unsigned)g_dwColorTintGreen) / 255);
                    dest[2] = (unsigned char)((int)(intensity * (unsigned)g_dwColorTintBlue) / 255);
                }
                source += 4;
                dest += 4;
            }
        }
    }
    return 1;
}

// The bits of a pixel of the bitmaps the paint shop reads.
#define PAINT_BITMAP_BITS 24
// The bytes of a pixel of RGBA 8888.
#define RGBA32_PIXEL_SIZE 4

void CopyBgr24ToRgba32Flipped(
    unsigned char *pDest, const unsigned char *pSrc, int nWidth, int nHeight, int nStride) {
    const unsigned char *pRow = pSrc + (nHeight - 1) * nStride;

    for (int y = nHeight; y != 0; y--) {
        const unsigned char *pPixel = pRow;

        for (int x = nWidth; x != 0; x--) {
            unsigned char bBlue = pPixel[0];
            unsigned char bGreen = pPixel[1];

            *pDest++ = pPixel[2];
            *pDest++ = bGreen;
            *pDest++ = bBlue;
            *pDest++ = 0xff;
            pPixel += 3;
        }
        pRow -= nStride;
    }
}

void *ConvertBitmap24ToRgba32(const BITMAP *pBitmap) {
    unsigned char *pPixels;

    if (pBitmap->bmBitsPixel != PAINT_BITMAP_BITS) {
        return nullptr;
    }
    pPixels = malloc(pBitmap->bmHeight * pBitmap->bmWidth * RGBA32_PIXEL_SIZE);
    if (pPixels != nullptr) {
        CopyBgr24ToRgba32Flipped(
            pPixels, pBitmap->bmBits, pBitmap->bmWidth, pBitmap->bmHeight, pBitmap->bmWidthBytes);
        g_dwConvertedBitmapWidth = pBitmap->bmWidth;
        g_dwConvertedBitmapHeight = pBitmap->bmHeight;
    }
    return pPixels;
}

#if TGR_PLATFORM == TGR_PLATFORM_SDL
void *LoadBitmapFromFile(const char *pszPath) {
    BITMAP bm;
    void *pPixels;

    if (!LoadGameBitmap(pszPath, &bm)) {
        return nullptr;
    }
    if (bm.bmBitsPixel != PAINT_BITMAP_BITS) {
        free(bm.bmBits);
        return nullptr;
    }
    pPixels = ConvertBitmap24ToRgba32(&bm);
    free(bm.bmBits);
    return pPixels;
}
#else
void *LoadBitmapFromFile(const char *pszPath) {
    HBITMAP hBitmap;
    BITMAP bm;
    void *pPixels;

    hBitmap =
        LoadImageA(nullptr, pszPath, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION);
    if (hBitmap == nullptr) {
        return nullptr;
    }
    GetObjectA(hBitmap, sizeof(bm), &bm);
    if (bm.bmBitsPixel != PAINT_BITMAP_BITS) {
        // The bitmap is not deleted here, as in the original.
        return nullptr;
    }
    pPixels = ConvertBitmap24ToRgba32(&bm);
    DeleteObject(hBitmap);
    return pPixels;
}
#endif /* TGR_PLATFORM == TGR_PLATFORM_SDL */

void SetColorTintMultipliers(int nRed, int nGreen, int nBlue) {
    g_dwColorTintRed = nRed;
    g_dwColorTintGreen = nGreen;
    g_dwColorTintBlue = nBlue;
}

// The size of the path of a bitmap of the damage of the paint.
#define PAINT_DAMAGE_PATH_SIZE 0x400

void LoadPaintDamageBitmaps(void) {
    char szPath[PAINT_DAMAGE_PATH_SIZE];

    for (int i = 0; i < PAINT_DAMAGE_BITMAP_COUNT; i++) {
        sprintf(szPath, "Paint\\damage%d.bmp", i + 1);
        g_apPaintDamageBitmaps[i] = LoadBitmapFromFile(szPath);
    }
}

// A bitmap from malloc() is freed and its pointer cleared.
static inline void FreeBitmap(void **ppBitmap) {
    if (*ppBitmap != nullptr) {
        free(*ppBitmap);
        *ppBitmap = nullptr;
    }
}

void FreeCarPaintTextures(void) {
    for (int nGroup = 0; nGroup < CAR_PAINT_GROUP_COUNT; nGroup++) {
        for (int nSet = 0; nSet < CAR_PAINT_SET_COUNT; nSet++) {
            for (int nBitmap = 0; nBitmap < CAR_PAINT_BITMAP_COUNT; nBitmap++) {
                FreeBitmap(&g_CarPaintTextureSets[nGroup][nSet].apBitmaps[nBitmap]);
            }
        }
    }
}

void FreeAllPaintBitmaps(void) {
    FreeCarPaintTextures();
    for (int i = 0; i < PAINT_DAMAGE_BITMAP_COUNT; i++) {
        FreeBitmap(&g_apPaintDamageBitmaps[i]);
    }
}
