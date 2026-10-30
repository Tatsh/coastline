/* Host check of the helpers of the quality of life changes in src/common/qualityoflife.c. */
#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "common/game.h"
#include "common/qualityoflife.h"

static void TestScene(void) {
    assert(ParseTestScene("arcade") == GAME_MODE_ARCADE);
    assert(ParseTestScene("demo") == GAME_MODE_DEMO);
    assert(ParseTestScene("practice") == GAME_MODE_PRACTICE);
    assert(ParseTestScene("seasonwinner") == GAME_MODE_AFTER_SEASON);
    assert(ParseTestScene("timeattack") == GAME_MODE_TIME_ATTACK);
    /* The value ends at white space, as the next key of the command line follows it. */
    assert(ParseTestScene("seasonwinner season=2") == GAME_MODE_AFTER_SEASON);
    assert(ParseTestScene("arcade\n") == GAME_MODE_ARCADE);
    /* A longer word, an unknown word, and the championship are not scenes. */
    assert(ParseTestScene("arcades") == -1);
    assert(ParseTestScene("championship") == -1);
    assert(ParseTestScene("") == -1);
}

static void TestSecondPress(void) {
    unsigned last = 0;

    /* A first press is not a second press, and a press within the window completes the pair. */
    assert(!IsSecondPressWithin(&last, 1000, 500));
    assert(IsSecondPressWithin(&last, 1500, 500));
    /* The pair is consumed, so a third press starts over. */
    assert(!IsSecondPressWithin(&last, 1600, 500));
    /* A press after the window starts a new pair in place of completing the old one. */
    assert(!IsSecondPressWithin(&last, 2101, 500));
    assert(IsSecondPressWithin(&last, 2200, 500));
    /* The counter of milliseconds wraps between the presses. */
    last = 0;
    assert(!IsSecondPressWithin(&last, 0xffffff00u, 500));
    assert(IsSecondPressWithin(&last, 0x00000010u, 500));
}

static int Near(float a, float b) {
    return fabsf(a - b) < 1e-4f;
}

static void TestHorPlusFov(void) {
    const float fov = 1.0f;
    const float horizontal43 = 2.0f * atanf(tanf(fov * 0.5f) * (4.0f / 3.0f));
    float fovY;

    /* 4:3 and every wider shape retain the vertical angle. */
    assert(GetHorPlusFovY(fov, 640.0f, 480.0f) == fov);
    assert(GetHorPlusFovY(fov, 1920.0f, 1080.0f) == fov);
    assert(GetHorPlusFovY(fov, 1680.0f, 1050.0f) == fov);
    assert(GetHorPlusFovY(fov, 3440.0f, 1440.0f) == fov);
    /* 5:4 retains the horizontal angle of 4:3, so its vertical angle grows. */
    fovY = GetHorPlusFovY(fov, 1280.0f, 1024.0f);
    assert(fovY > fov);
    assert(Near(2.0f * atanf(tanf(fovY * 0.5f) * 1280.0f / 1024.0f), horizontal43));
    /* The two branches meet at 4:3. */
    assert(Near(GetHorPlusFovY(fov, 639.999f, 480.0f), fov));
}

static void AssertCanvas(int width, int height, int canvasWidth, int canvasHeight) {
    int w, h;

    GetHudCanvasSize(width, height, &w, &h);
    if (w != canvasWidth || h != canvasHeight) {
        printf("%dx%d: canvas %dx%d, expected %dx%d\n",
               width,
               height,
               w,
               h,
               canvasWidth,
               canvasHeight);
    }
    assert(w == canvasWidth && h == canvasHeight);
}

static void TestHudCanvas(void) {
    /* 4:3 and wider shapes are 480 units tall, and the right edge moves out with the width. */
    AssertCanvas(640, 480, 640, 480);
    AssertCanvas(1024, 768, 640, 480);
    AssertCanvas(1920, 1080, 853, 480);
    AssertCanvas(1680, 1050, 768, 480);
    AssertCanvas(2560, 1080, 1138, 480);
    AssertCanvas(3440, 1440, 1147, 480);
    /* A frame buffer below 480 lines is drawn at its own size. */
    AssertCanvas(400, 300, 400, 300);
    /* Shapes narrower than 4:3 are 640 units wide, and the bottom edge moves down. */
    AssertCanvas(1280, 1024, 640, 512);
    AssertCanvas(600, 600, 600, 600);
    /* The width of a very wide canvas is limited to the twelve bits of a rectangle command. */
    AssertCanvas(7680, 1080, 2000, 281);
}

int main(void) {
    TestSecondPress();
    TestHorPlusFov();
    TestHudCanvas();
    TestScene();
    puts("qualityoflife: ok");
    return 0;
}
