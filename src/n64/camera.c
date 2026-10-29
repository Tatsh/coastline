#include "common/camera.h"

#include <PR/R4300.h>
#include <PR/gu.h>
#include <PR/os_libc.h>

#include "common/game.h"
#include "common/system.h"
#include "common/tricache.h"
#include "common/util.h"
#include "common/visgrid.h"

/* The share of the way from the camera to each corner of the far rectangle the frustum keeps. */
#define FRUSTUM_CORNER_SHARE 0.75f

/* The perspective of a camera: its near plane, the shape of the screen, and degrees a radian. */
#define CAMERA_NEAR 2.4f
#define ASPECT_4_3 1.3333334f
#define DEGREES_PER_RADIAN 57.295776f

/* The point SetupProjectionMatrix() looks at, its distance, and the perspective, in degrees. */
#define SCREEN_CENTER_X 512.0f
#define SCREEN_CENTER_Y 384.0f
#define SCREEN_DISTANCE 1000.0f
#define SCREEN_FOV 45.0f
#define SCREEN_NEAR 10.0f
#define SCREEN_FAR 2000.0f

/* The point SetupMenuProjection() looks at, its distance, and its default clip planes. */
#define MENU_CENTER_X 160.0f
#define MENU_CENTER_Y 120.0f
#define MENU_DISTANCE 290.0f
#define MENU_NEAR 1.0
#define MENU_FAR 500.0

/* The RSP takes physical addresses, which are the KSEG0 addresses less its base. */
#define TO_RSP_ADDRESS(p) ((u32)((u8 *)(p) - K0BASE))
