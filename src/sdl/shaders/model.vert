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

layout(set = 1, binding = 0) uniform Transform {
    mat4 viewProjection;
    mat4 model;
};

void main() {
    gl_Position = viewProjection * model * vec4(inPosition, 1.0);
    outTexCoord = inTexCoord;
    outNormal = mat3(model) * inNormal;
    outColor = inColor;
}
