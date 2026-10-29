#version 450

layout(location = 0) in vec2 inTexCoord;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inColor;

layout(location = 0) out vec4 outColor;

layout(set = 2, binding = 0) uniform sampler2D textureMap;

// x is the alpha a texel needs to be drawn, or -1 to draw every texel. y is 1 for a lit material,
// whose vertices have normals, and 0 for one whose vertices have colours. z is 1 for a textured
// material.
layout(set = 3, binding = 0) uniform Material {
    vec4 flags;
    vec4 lightDirection;
};

void main() {
    vec4 texel = flags.z > 0.5 ? texture(textureMap, inTexCoord) : vec4(1.0);
    vec4 shade = inColor;
    if (flags.y > 0.5) {
        float diffuse = max(dot(normalize(inNormal), -lightDirection.xyz), 0.0);
        shade = vec4(vec3(0.45 + 0.55 * diffuse), 1.0);
    }
    vec4 color = texel * shade;
    if (color.a < flags.x) {
        discard;
    }
    outColor = color;
}
