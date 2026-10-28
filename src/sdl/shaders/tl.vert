#version 450

// A vertex transformed and lit by the game, as Direct3D 5 takes one: the screen position, the
// depth, the reciprocal of w, the diffuse and specular colours, and the texture coordinates.
layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec4 inDiffuse;
layout(location = 2) in vec4 inSpecular;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec4 outDiffuse;
layout(location = 1) out vec4 outSpecular;
layout(location = 2) out vec2 outTexCoord;

// x and y scale screen pixels to the range 0 to 2. Direct3D 5 centres pixels on integer
// coordinates, a half pixel from where Vulkan, Metal, and Direct3D 12 centre them, and z and w
// store the offset in pixels.
layout(set = 1, binding = 0) uniform Viewport {
    vec4 scale;
};

void main() {
    // A reciprocal of zero puts the vertex at w 1, as the fixed pipeline treats it.
    float w = inPosition.w != 0.0 ? 1.0 / inPosition.w : 1.0;
    vec2 ndc = vec2((inPosition.x + scale.z) * scale.x - 1.0,
                    1.0 - (inPosition.y + scale.w) * scale.y);
    gl_Position = vec4(ndc * w, inPosition.z * w, w);
    // The colours are D3DCOLOR words, in the byte order B, G, R, A.
    outDiffuse = inDiffuse.bgra;
    outSpecular = inSpecular.bgra;
    outTexCoord = inTexCoord;
}
