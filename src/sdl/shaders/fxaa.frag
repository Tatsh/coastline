#version 450

// Fast approximate anti-aliasing of the frame, as FXAA of Timothy Lottes finds an edge: the
// luminance of the four diagonal neighbours gives the direction along the edge, and the frame is
// blended along it, unless the blend leaves the range of the neighbourhood.
layout(location = 0) in vec2 inTexCoord;

layout(location = 0) out vec4 outColor;

layout(set = 2, binding = 0) uniform sampler2D frame;

// The size of a pixel of the frame in xy.
layout(set = 3, binding = 0) uniform Frame {
    vec4 pixel;
};

const float SPAN_MAX = 8.0;
const float REDUCE_MULTIPLIER = 1.0 / 8.0;
const float REDUCE_MINIMUM = 1.0 / 128.0;

float Luma(vec3 color) {
    return dot(color, vec3(0.299, 0.587, 0.114));
}

vec3 Sample(vec2 offset) {
    return texture(frame, inTexCoord + offset).rgb;
}

void main() {
    vec2 size = pixel.xy;
    float northWest = Luma(Sample(vec2(-1.0, -1.0) * size));
    float northEast = Luma(Sample(vec2(1.0, -1.0) * size));
    float southWest = Luma(Sample(vec2(-1.0, 1.0) * size));
    float southEast = Luma(Sample(vec2(1.0, 1.0) * size));
    vec3 middle = Sample(vec2(0.0));
    float lumaMiddle = Luma(middle);
    float lumaMinimum =
        min(lumaMiddle, min(min(northWest, northEast), min(southWest, southEast)));
    float lumaMaximum =
        max(lumaMiddle, max(max(northWest, northEast), max(southWest, southEast)));
    vec2 direction = vec2(-((northWest + northEast) - (southWest + southEast)),
                          (northWest + southWest) - (northEast + southEast));
    float reduce = max((northWest + northEast + southWest + southEast) * 0.25 * REDUCE_MULTIPLIER,
                       REDUCE_MINIMUM);
    float scale = 1.0 / (min(abs(direction.x), abs(direction.y)) + reduce);
    direction = clamp(direction * scale, vec2(-SPAN_MAX), vec2(SPAN_MAX)) * size;
    vec3 near = 0.5 * (Sample(direction * (1.0 / 3.0 - 0.5)) + Sample(direction * (2.0 / 3.0 - 0.5)));
    vec3 far = near * 0.5 + 0.25 * (Sample(direction * -0.5) + Sample(direction * 0.5));
    float lumaFar = Luma(far);
    outColor = vec4(lumaFar < lumaMinimum || lumaFar > lumaMaximum ? near : far, 1.0);
}
