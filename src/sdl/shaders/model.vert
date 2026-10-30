#version 450

// A vertex of a model of the console, in the units of its glTF node: the position, the texture
// coordinate, the normal of a lit vertex, and the colour of an unlit one.
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec4 inColor;

layout(location = 0) out vec2 outTexCoord;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec4 outColor;

// The morph targets of the keys of an animation of the vertices: for each key, the displacement of
// the position and then of the normal of every vertex.
layout(std430, set = 0, binding = 0) readonly buffer Targets {
    vec4 displacements[];
};

// morph: x and y are the two keys to blend, z is the vertices of a key, and w is 1 where the model
// is animated. blend.x is the share of the second key, and blend.y is 1 where the texture
// coordinates come from the normal, as G_TEXTURE_GEN of the RSP makes them, over a square whose
// height is blend.z of the texture.
layout(set = 1, binding = 0) uniform Transform {
    mat4 viewProjection;
    mat4 model;
    ivec4 morph;
    vec4 blend;
};

void main() {
    vec3 position = inPosition;
    vec3 normal = inNormal;
    if (morph.w == 1) {
        int first = (morph.x * morph.z + gl_VertexIndex) * 2;
        int second = (morph.y * morph.z + gl_VertexIndex) * 2;
        position += mix(displacements[first].xyz, displacements[second].xyz, blend.x);
        normal += mix(displacements[first + 1].xyz, displacements[second + 1].xyz, blend.x);
    }
    gl_Position = viewProjection * model * vec4(position, 1.0);
    outTexCoord = inTexCoord;
    outNormal = mat3(model) * normal;
    if (blend.y > 0.5) {
        // The reflection of the menus looks along z with x right and y up, so the turned normal
        // reads across and down the texture.
        vec3 turned = normalize(outNormal);
        outTexCoord = (turned.xy * 0.5 + 0.5) * vec2(1.0, blend.z);
    }
    outColor = inColor;
}
