#include "platform/win/glidetexture.h"

#include <stdint.h>

int CalculateMipMapLevel(int *level, int width, int height) {
    int dimension = width > height ? width : height;
    int limit = 1;
    int selected;
    for (selected = 8; selected > 0; selected--) {
        if (dimension <= limit) {
            *level = selected;
            return 1;
        }
        limit *= 2;
    }
    *level = 0;
    return dimension <= 256;
}

int CalculateAspectRatioLevel(int *level, int width, int height) {
    int wide = width > height;
    int ratio =
        wide ? (int32_t)((uint32_t)width * 8u) / height : (int32_t)((uint32_t)height * 8u) / width;
    int selected;
    int exact;
    if (ratio == 64) {
        selected = 0;
        exact = 1;
    } else if (ratio == 32) {
        selected = 1;
        exact = 1;
    } else if (ratio == 16) {
        selected = 2;
        exact = 1;
    } else if (!wide && ratio == 8) {
        selected = 3;
        exact = 1;
    } else {
        selected = ratio > 64 ? 0 : ratio > 32 ? 1 : ratio > 16 ? 2 : 3;
        exact = 0;
    }
    *level = wide ? selected : 6 - selected;
    return exact;
}

int GetGlideTexFormatBytesPerPixel(int format) {
    return format >= 2 && format <= 4 ? 1 : 2;
}

unsigned ConvertN64Rgba5551ToArgb1555(unsigned color) {
    unsigned swapped = ((color & 255u) << 8) | ((color & 65535u) >> 8);
    return (swapped >> 1) | ((swapped & 1u) << 15);
}
