#include "vehiclestatus.h"

#include "frametime.h"
#include "game.h"
#include "laps.h"
#include "mathutil.h"
#include "racerslotaccess.h"
#include "track.h"
#include "vehicleaccess.h"
#include "vehicletrackpos.h"

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#include "win32/boot/strings.h"
#endif

#define GRID_CELL 32
#define GRID_CELLS 64
#define WRONG_WAY_FRAMES 32
#define WRONG_WAY_FLASH 0x10
#define WRONG_WAY_TIME 0.25f

#if TGR_PLATFORM == TGR_PLATFORM_WIN
typedef long double StatusReal;
#define STATUS_TIME_SUM(value) ((long double)g_fFrameDeltaTime + (value))
#define STATUS_NONZERO(value) ((value) < 0.0f || (value) > 0.0f)
#define STATUS_EXPIRED(value) (!((value) > 0.0L))
#define STATUS_NEGATIVE(value) (!((value) >= 0.0L))
#define STATUS_WRONG_DIRECTION(a, b) (!(Vec3fDot(a, b) >= 0.0L))
#define STATUS_GRID_COORD(value) FloatToInt((long double)(value) * 0.03125f)
#define STATUS_COMPARE_TIME(field) remaining
#else
typedef f32 StatusReal;
#define STATUS_TIME_SUM(value) ((value) + g_fFrameDeltaTime)
#define STATUS_NONZERO(value) ((value) != 0)
#define STATUS_EXPIRED(value) ((value) <= 0.0f)
#define STATUS_NEGATIVE(value) ((value) < 0.0f)
#define STATUS_WRONG_DIRECTION(a, b) ((f64)Vec3fDot(a, b) < 0.0)
#define STATUS_GRID_COORD(value) FloatToInt((value) / GRID_CELL)
#define STATUS_COMPARE_TIME(field) VEHICLE_GET(vehicle, field)
static const char sWrongWay[] = "%yyWRONG WAY";
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */

void UpdateRaceTimers(Vehicle *vehicle) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    StatusReal remaining;
#endif

    if (RACER_SLOT_GET(VEHICLE_GET(vehicle, Racer), Status) & RACER_CLOCK_STOPPED) {
        return;
    }
    if (g_dwGameMode == GAME_MODE_PRACTICE) {
        VEHICLE_SET(vehicle, LapTime, STATUS_TIME_SUM(VEHICLE_GET(vehicle, LapTime)));
        return;
    }
    VEHICLE_SET(vehicle, TotalTime, STATUS_TIME_SUM(VEHICLE_GET(vehicle, TotalTime)));
    VEHICLE_SET(vehicle, LapTime, STATUS_TIME_SUM(VEHICLE_GET(vehicle, LapTime)));
    if (g_dwGameMode == GAME_MODE_ARCADE
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        || g_dwGameMode == GAME_MODE_NETWORK
#endif
    ) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        remaining = (StatusReal)VEHICLE_GET(vehicle, TimeLeft) - g_fFrameDeltaTime;
        VEHICLE_SET(vehicle, TimeLeft, remaining);
#else
        VEHICLE_SET(vehicle, TimeLeft, VEHICLE_GET(vehicle, TimeLeft) - g_fFrameDeltaTime);
#endif
        if (STATUS_NEGATIVE(STATUS_COMPARE_TIME(TimeLeft))) {
            VEHICLE_SET(vehicle, TimeLeft, 0.0f);
        }
    }
}

void UpdateHudMessageTimers(Vehicle *vehicle) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    StatusReal remaining;
#endif

    if (STATUS_NONZERO(VEHICLE_GET(vehicle, MessageTime))) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        remaining = (StatusReal)VEHICLE_GET(vehicle, MessageTime) - g_fFrameDeltaTime;
        VEHICLE_SET(vehicle, MessageTime, remaining);
#else
        VEHICLE_GET(vehicle, MessageTime) -= g_fFrameDeltaTime;
#endif
        if (STATUS_EXPIRED(STATUS_COMPARE_TIME(MessageTime))) {
            VEHICLE_SET(vehicle, MessageTime, 0.0f);
            VEHICLE_SET(vehicle, Message, NULL);
        }
    } else if (STATUS_NONZERO(VEHICLE_GET(vehicle, SubMessageTime))) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        remaining = (StatusReal)VEHICLE_GET(vehicle, SubMessageTime) - g_fFrameDeltaTime;
        VEHICLE_SET(vehicle, SubMessageTime, remaining);
#else
        VEHICLE_GET(vehicle, SubMessageTime) -= g_fFrameDeltaTime;
#endif
        if (STATUS_EXPIRED(STATUS_COMPARE_TIME(SubMessageTime))) {
            VEHICLE_SET(vehicle, SubMessageTime, 0.0f);
            VEHICLE_SET(vehicle, SubMessage, NULL);
        }
    }
}

void CheckWrongWayWarning(Vehicle *vehicle) {
    const char *warning;

    if (g_LoadedTrack.rootSection != NULL) {
        if (VEHICLE_GET(vehicle, Lap) < g_dwTotalLaps && !VEHICLE_GET(vehicle, FlyMode) &&
            STATUS_WRONG_DIRECTION(&VEHICLE_GET(vehicle, TrackDirection),
                                   &VEHICLE_GET(vehicle, Frame).forward)) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
            warning = GetStringResource(0xf3);
            VEHICLE_SET(
                vehicle, WrongWayFrames, (s32)((u32)VEHICLE_GET(vehicle, WrongWayFrames) + 1));
#else
            warning = sWrongWay;
            VEHICLE_SET(vehicle, WrongWayFrames, VEHICLE_GET(vehicle, WrongWayFrames) + 1);
#endif
            if (VEHICLE_GET(vehicle, WrongWayFrames) >= WRONG_WAY_FRAMES &&
                (VEHICLE_GET(vehicle, WrongWayFrames) & WRONG_WAY_FLASH) == WRONG_WAY_FLASH) {
                if (VEHICLE_GET(vehicle, Message) == NULL) {
                    VEHICLE_SET(vehicle, Message, warning);
                    VEHICLE_SET(vehicle, SubMessage, NULL);
                    VEHICLE_SET(vehicle, MessageTime, WRONG_WAY_TIME);
                }
            } else if (VEHICLE_GET(vehicle, Message) == warning) {
                VEHICLE_SET(vehicle, SubMessage, NULL);
                VEHICLE_SET(vehicle, Message, NULL);
            }
        } else {
            VEHICLE_SET(vehicle, WrongWayFrames, 0);
        }
    }
}

void UpdateVehicleGridPosition(Vehicle *vehicle) {
    if (g_LoadedTrack.rootSection == NULL) {
        return;
    }
    VEHICLE_SET(vehicle, GridX, STATUS_GRID_COORD(VEHICLE_GET(vehicle, Frame).position.x));
    VEHICLE_SET(vehicle, GridY, STATUS_GRID_COORD(VEHICLE_GET(vehicle, Frame).position.y));
#ifdef ENABLE_PATCHES
    /* The cells are unsigned, so only the upper bound applies. */
    if (VEHICLE_GET(vehicle, GridX) >= GRID_CELLS) {
        VEHICLE_SET(vehicle, GridX, GRID_CELLS - 1);
    }
    if (VEHICLE_GET(vehicle, GridY) >= GRID_CELLS) {
        VEHICLE_SET(vehicle, GridY, GRID_CELLS - 1);
    }
#else
    /* The original tests the unsigned cells against zero as well. */
    if (VEHICLE_GET(vehicle, GridX) < 0) {
        VEHICLE_SET(vehicle, GridX, 0);
    } else if (VEHICLE_GET(vehicle, GridX) >= GRID_CELLS) {
        VEHICLE_SET(vehicle, GridX, GRID_CELLS - 1);
    }
    if (VEHICLE_GET(vehicle, GridY) < 0) {
        VEHICLE_SET(vehicle, GridY, 0);
    } else if (VEHICLE_GET(vehicle, GridY) >= GRID_CELLS) {
        VEHICLE_SET(vehicle, GridY, GRID_CELLS - 1);
    }
#endif /* ENABLE_PATCHES */
    UpdateVehicleTrackPosition(vehicle);
}
