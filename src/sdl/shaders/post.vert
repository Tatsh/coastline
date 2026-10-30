#version 450

// A pass over the whole frame: one triangle from the index of its vertex, which covers the frame,
// and the coordinate of the frame at each corner, from its top left.
layout(location = 0) out vec2 outTexCoord;

void main() {
    vec2 corner = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    outTexCoord = corner;
    gl_Position = vec4(corner.x * 2.0 - 1.0, 1.0 - corner.y * 2.0, 0.0, 1.0);
}
