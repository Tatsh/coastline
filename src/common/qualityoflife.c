#include "common/qualityoflife.h"

#include <ctype.h>
#include <math.h>
#include <string.h>

#include "common/game.h"

/* The layout of the HUD, in units of the canvas. */
#define HUD_LAYOUT_WIDTH 640
#define HUD_LAYOUT_HEIGHT 480
/* Integer rectangle commands have twelve bits per coordinate. */
#define HUD_CANVAS_MAX_WIDTH 2000

bool IsSecondPressWithin(unsigned *pLastPress, unsigned now, unsigned window) {
    bool second = *pLastPress != 0 && now - *pLastPress <= window;

    *pLastPress = second ? 0 : now;
    return second;
}

float GetHorPlusFovY(float fov, float width, float height) {
    if (width * 3.0f >= height * 4.0f) {
        return fov;
    }
    return 2.0f * atanf(tanf(fov * 0.5f) * (4.0f / 3.0f) * height / width);
}

void GetHudCanvasSize(int width, int height, int *pCanvasWidth, int *pCanvasHeight) {
    if (width * 3 >= height * 4) {
        *pCanvasHeight = height < HUD_LAYOUT_HEIGHT ? height : HUD_LAYOUT_HEIGHT;
        *pCanvasWidth = (int)((double)width * *pCanvasHeight / height + 0.5);
        if (*pCanvasWidth > HUD_CANVAS_MAX_WIDTH) {
            *pCanvasHeight = (int)((double)*pCanvasHeight * HUD_CANVAS_MAX_WIDTH / *pCanvasWidth);
            *pCanvasWidth = HUD_CANVAS_MAX_WIDTH;
        }
    } else {
        *pCanvasWidth = width < HUD_LAYOUT_WIDTH ? width : HUD_LAYOUT_WIDTH;
        *pCanvasHeight = (int)((double)height * *pCanvasWidth / width + 0.5);
    }
}

int ParseTestScene(const char *pszValue) {
    static const struct {
        const char *pszName;
        int nGameMode;
    } scenes[] = {
        {"arcade", GAME_MODE_ARCADE},
        {"demo", GAME_MODE_DEMO},
        {"practice", GAME_MODE_PRACTICE},
        {"seasonwinner", GAME_MODE_AFTER_SEASON},
        {"timeattack", GAME_MODE_TIME_ATTACK},
    };

    for (size_t i = 0; i < sizeof(scenes) / sizeof(scenes[0]); ++i) {
        size_t length = strlen(scenes[i].pszName);
        if (strncmp(pszValue, scenes[i].pszName, length) == 0 &&
            (pszValue[length] == '\0' || isspace((unsigned char)pszValue[length]))) {
            return scenes[i].nGameMode;
        }
    }
    return -1;
}
