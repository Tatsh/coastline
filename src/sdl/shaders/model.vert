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

// The most poses a draw blends.
const int MAX_POSES = 64;

// morph: z is the vertices of a key, and w is the number of poses. blend.y is 1 where the texture
// coordinates come from the normal across the whole texture, as G_TEXTURE_GEN of the RSP makes
// them. Each pose is the first and the second key to blend and the share of the second, and the
// vertices take the sum of the poses, as each animation moves its own vertices.
// blend.z is 1 where the generated coordinates reflect the sky on a car: they follow the axes of
// the look-at of the reflection, across a window of 31 of the 32 columns of the sky from the
// column in reflectAcross.w and down its 128 rows.
layout(set = 1, binding = 0) uniform Transform {
    mat4 viewProjection;
    mat4 model;
    ivec4 morph;
    vec4 blend;
    vec4 reflectAcross;
    vec4 reflectUp;
    vec4 poses[MAX_POSES];
};

void main() {
    vec3 position = inPosition;
    vec3 normal = inNormal;
    for (int i = 0; i < min(morph.w, MAX_POSES); ++i) {
        int first = (int(poses[i].x) * morph.z + gl_VertexIndex) * 2;
        int second = (int(poses[i].y) * morph.z + gl_VertexIndex) * 2;
        position += mix(displacements[first].xyz, displacements[second].xyz, poses[i].z);
        normal += mix(displacements[first + 1].xyz, displacements[second + 1].xyz, poses[i].z);
    }
    gl_Position = viewProjection * model * vec4(position, 1.0);
    outTexCoord = inTexCoord;
    outNormal = mat3(model) * normal;
    if (blend.z > 0.5) {
        // G_TEXTURE_GEN of the RSP with the look-at of guLookAtReflectF(), in the tile the
        // reflection of the race sets.
        vec3 turned = normalize(outNormal);
        vec2 generated = vec2(dot(turned, reflectAcross.xyz), dot(turned, reflectUp.xyz)) * 0.5 +
                         0.5;
        outTexCoord = vec2((reflectAcross.w + generated.x * 31.0) / 32.0,
                           (0.5 + generated.y * 127.0) / 128.0);
    } else if (blend.y > 0.5) {
        // The reflection of the menus looks along z with x right and y up, so the turned normal
        // reads across and down the texture.
        vec3 turned = normalize(outNormal);
        outTexCoord = turned.xy * 0.5 + 0.5;
    }
    outColor = inColor;
}
