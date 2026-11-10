#version 450

layout(location = 0) in vec2 inTexCoord;
layout(location = 0) out vec4 outColor;
layout(set = 2, binding = 0) uniform sampler2D frame;
layout(set = 3, binding = 0) uniform HdrOptions {
    vec4 values;
};

void main() {
    ivec2 size = textureSize(frame, 0);
    ivec2 pixel = clamp(ivec2(inTexCoord * vec2(size)), ivec2(0), size - 1);
    vec3 srgb = values.w > 0.5 ? texelFetch(frame, pixel, 0).rgb :
                                texture(frame, inTexCoord).rgb;
    vec3 linear = mix(srgb / 12.92, pow((srgb + 0.055) / 1.055, vec3(2.4)),
                      step(vec3(0.04045), srgb));
    float highlight = smoothstep(0.5, 1.0, max(max(linear.r, linear.g), linear.b));
    float boost = 1.0 + values.z * highlight * min(values.y - 1.0, 1.0) * 0.5;
    outColor = vec4(linear * values.x * boost, 1.0);
}
