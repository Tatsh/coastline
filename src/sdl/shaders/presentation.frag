#version 450

layout(location = 0) in vec2 inTexCoord;
layout(location = 0) out vec4 outColor;
layout(set = 2, binding = 0) uniform sampler2D frame;
layout(set = 3, binding = 0) uniform Presentation {
    vec4 sizes;
    vec4 effects;
};

float Luma(vec3 color) {
    return dot(color, vec3(0.299, 0.587, 0.114));
}

vec3 ToLinear(vec3 color) {
    return mix(color / 12.92, pow((color + 0.055) / 1.055, vec3(2.4)),
               step(vec3(0.04045), color));
}

vec3 ToSRGB(vec3 color) {
    return mix(color * 12.92, 1.055 * pow(color, vec3(1.0 / 2.4)) - 0.055,
               step(vec3(0.0031308), color));
}

vec3 Read(ivec2 at) {
    return texelFetch(frame, clamp(at, ivec2(0), ivec2(sizes.xy) - 1), 0).rgb;
}

vec3 Reconstruct(vec2 uv) {
    vec2 position = uv * sizes.xy - 0.5;
    ivec2 origin = ivec2(floor(position));
    vec2 fraction = fract(position);
    vec3 a = Read(origin);
    vec3 b = Read(origin + ivec2(1, 0));
    vec3 c = Read(origin + ivec2(0, 1));
    vec3 d = Read(origin + ivec2(1, 1));
    vec3 minimum = min(min(a, b), min(c, d));
    vec3 maximum = max(max(a, b), max(c, d));
    vec2 gradient = vec2(Luma(b + d - a - c), Luma(c + d - a - b));
    float strength = length(gradient);
    vec2 normal = strength > 0.0001 ? gradient / strength : vec2(1.0, 0.0);
    vec2 tangent = vec2(-normal.y, normal.x);
    float stretch = 1.0 + clamp(strength * 2.0, 0.0, 1.0);
    vec3 result = vec3(0.0);
    float total = 0.0;
    for (int y = -1; y <= 2; ++y) {
        for (int x = -1; x <= 2; ++x) {
            vec2 delta = vec2(x, y) - fraction;
            vec2 directed = vec2(dot(delta, normal) * stretch, dot(delta, tangent) / stretch);
            float distance = min(length(directed), 2.0);
            // A windowed sinc reconstructs detail along edges without mixing across them.
            float weight = distance < 0.0001 ? 1.0 :
                sin(3.14159265 * distance) * sin(1.57079633 * distance) /
                (4.9348022 * distance * distance);
            result += Read(origin + ivec2(x, y)) * weight;
            total += weight;
        }
    }
    vec3 linear = mix(mix(a, b, fraction.x), mix(c, d, fraction.x), fraction.y);
    result = abs(total) > 0.0001 ? result / total : linear;
    // Limit ringing to the four adjacent samples and restore contrast lost in enlargement.
    return clamp(result + (result - linear) * 0.2, minimum, maximum);
}

void main() {
    vec2 uv = inTexCoord;
    bool consumer = effects.y > 0.5 && effects.y < 1.5;
    bool pvm = effects.y > 1.5 && effects.y < 2.5;
    bool pc = effects.y > 3.5;
    if (effects.y > 0.5) {
        vec2 centered = uv * 2.0 - 1.0;
        float curvature = consumer ? 0.035 : pvm ? 0.012 : pc ? 0.006 : 0.0;
        centered *= 1.0 + curvature * dot(centered, centered);
        uv = centered * 0.5 + 0.5;
        if (any(lessThan(uv, vec2(0.0))) || any(greaterThan(uv, vec2(1.0)))) {
            outColor = vec4(0.0, 0.0, 0.0, 1.0);
            return;
        }
    }
    vec3 color;
    if (effects.x > 1.5) {
        color = Reconstruct(uv);
    } else if (effects.x > 0.5) {
        color = Read(ivec2(uv * sizes.xy));
    } else {
        color = texture(frame, uv).rgb;
    }
    if (effects.y > 0.5) {
        color = ToLinear(clamp(color, 0.0, 1.0));
        if (consumer) {
            vec2 spread = vec2(0.5 / sizes.x, 0.0);
            vec3 soft = 0.5 * (ToLinear(texture(frame, uv - spread).rgb) +
                               ToLinear(texture(frame, uv + spread).rgb));
            color = mix(color, soft, 0.35);
        }
        // Fade scanlines near native resolution to avoid a moire pattern when shrinking.
        float scanStrength = (consumer ? 0.24 : pvm ? 0.42 : pc ? 0.08 : 0.32) *
            smoothstep(1.0, 2.0, sizes.w / sizes.y);
        float scan = 1.0 - scanStrength * (0.5 + 0.5 * cos(6.28318531 * uv.y * sizes.y));
        int rowOffset = consumer ? (int(gl_FragCoord.y) / 2) % 2 :
            pc ? int(gl_FragCoord.y) % 2 : 0;
        int phosphor = (int(gl_FragCoord.x) + rowOffset) % 3;
        vec3 mask = vec3(consumer ? 0.82 : pvm ? 0.94 : pc ? 0.94 : 0.76);
        mask[phosphor] = 1.0;
        if (consumer && int(gl_FragCoord.y) % 3 == 0) {
            mask *= 0.9;
        }
        vec2 edge = min(uv, 1.0 - uv);
        float border = smoothstep(0.0, pc ? 0.004 : 0.012, min(edge.x, edge.y));
        color *= scan * mask * border;
        float maskAverage = (1.0 + 2.0 * (consumer ? 0.82 : pvm ? 0.94 : pc ? 0.94 : 0.76)) / 3.0;
        if (consumer) {
            maskAverage *= 0.96666667;
        }
        float gain = 1.0 / ((1.0 - scanStrength * 0.5) * maskAverage);
        color = color * gain / (1.0 + color * (gain - 1.0));
        color = ToSRGB(color);
    }
    outColor = vec4(clamp(color, 0.0, 1.0), 1.0);
}
