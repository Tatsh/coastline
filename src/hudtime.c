#include <stdio.h>

#include "hud.h"
#include "text.h"

#if TGR_PC
#define TIME_HUNDREDTHS(seconds) ((s32)(s64)((long double)(seconds) * 100.0f))
#define TIME_TEXT_FORMAT "%s%d:%02d.%02d"
#define TIME_NEXT_LINE(y) ((s32)((u32)(y) + 15))
#else
#define TIME_HUNDREDTHS(seconds) ((s32)((seconds) * 100.0f))
#define TIME_TEXT_FORMAT "%s%d'%02d\"%02d"
#define TIME_NEXT_LINE(y) ((y) + 15)
#endif

void DrawLabelWithTime(const char *label, const char *color, f32 seconds, s32 x, s32 y) {
    s32 hundredths = TIME_HUNDREDTHS(seconds);
    s32 whole = hundredths / 100;
    s32 minutes = whole / 60;
    char text[32];

    sprintf(text, TIME_TEXT_FORMAT, color, minutes, whole - minutes * 60, hundredths - whole * 100);
    DrawText(text, x, TIME_NEXT_LINE(y));
    DrawText(label, x, y);
}

#if TGR_PC
void DrawFormattedLapTimeText(const char *color, s32 lap, f32 seconds, s32 x, s32 y) {
    s32 hundredths = TIME_HUNDREDTHS(seconds);
    s32 whole = hundredths / 100;
    s32 minutes = whole / 60;
    char text[32];

    sprintf(text, "%s%d. %d:%02d.%02d", color, lap, minutes, whole % 60, hundredths % 100);
    DrawText(text, x, y);
}
#endif /* TGR_PC */
