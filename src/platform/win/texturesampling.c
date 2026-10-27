#include "platform/win/texturesampling.h"

#include <math.h>
#include <stdint.h>
#include <string.h>

static unsigned SampleChannel(unsigned topLeft,
                              unsigned topRight,
                              unsigned bottomLeft,
                              unsigned bottomRight,
                              float x,
                              float y,
                              unsigned maximum) {
    long double top = topLeft + ((long double)topRight - topLeft) * x;
    long double bottom = bottomLeft + ((long double)bottomRight - bottomLeft) * x;
    long double value = top + (bottom - top) * y;
    double rounded = floor((double)(value + 0.5L));
    if (!(rounded >= 0.0)) {
        return 0;
    }
    if (rounded > maximum) {
        return maximum;
    }
    return (unsigned)rounded;
}

void SampleTexturePixelBilinear(unsigned char *pDest,
                                const unsigned char *pTopLeft,
                                const unsigned char *pTopRight,
                                const unsigned char *pBottomLeft,
                                const unsigned char *pBottomRight,
                                float x,
                                float y) {
    unsigned char result[4];
    static const unsigned channels[4] = {3, 0, 1, 2};
    unsigned i;
    for (i = 0; i < 4; i++) {
        unsigned channel = channels[i];
        result[channel] = (unsigned char)SampleChannel(pTopLeft[channel],
                                                       pTopRight[channel],
                                                       pBottomLeft[channel],
                                                       pBottomRight[channel],
                                                       x,
                                                       y,
                                                       255);
    }
    memcpy(pDest, result, sizeof(result));
}

static void SamplePackedPixel(unsigned char *destination,
                              const unsigned char *topLeft,
                              const unsigned char *topRight,
                              const unsigned char *bottomLeft,
                              const unsigned char *bottomRight,
                              float x,
                              float y,
                              unsigned colorBits,
                              unsigned alphaBits) {
    uint16_t pixels[4];
    uint16_t result = 0;
    unsigned channel;
    memcpy(&pixels[0], topLeft, sizeof(pixels[0]));
    memcpy(&pixels[1], topRight, sizeof(pixels[1]));
    memcpy(&pixels[2], bottomLeft, sizeof(pixels[2]));
    memcpy(&pixels[3], bottomRight, sizeof(pixels[3]));
    for (channel = 0; channel < 4; channel++) {
        unsigned shift = (3 - channel) * colorBits;
        unsigned mask = (1u << (channel == 0 ? alphaBits : colorBits)) - 1u;
        unsigned sample = SampleChannel((pixels[0] >> shift) & mask,
                                        (pixels[1] >> shift) & mask,
                                        (pixels[2] >> shift) & mask,
                                        (pixels[3] >> shift) & mask,
                                        x,
                                        y,
                                        mask);
        result = (uint16_t)((result << colorBits) | sample);
    }
    memcpy(destination, &result, sizeof(result));
}

void BilinearInterpolateArgb1555(unsigned char *destination,
                                 const unsigned char *topLeft,
                                 const unsigned char *topRight,
                                 const unsigned char *bottomLeft,
                                 const unsigned char *bottomRight,
                                 float x,
                                 float y) {
    SamplePackedPixel(destination, topLeft, topRight, bottomLeft, bottomRight, x, y, 5, 1);
}

void BilinearInterpolateArgb4444(unsigned char *destination,
                                 const unsigned char *topLeft,
                                 const unsigned char *topRight,
                                 const unsigned char *bottomLeft,
                                 const unsigned char *bottomRight,
                                 float x,
                                 float y) {
    SamplePackedPixel(destination, topLeft, topRight, bottomLeft, bottomRight, x, y, 4, 4);
}
