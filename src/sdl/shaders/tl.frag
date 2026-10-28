#version 450

layout(location = 0) in vec4 inDiffuse;
layout(location = 1) in vec4 inSpecular;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec4 outColor;

layout(set = 2, binding = 0) uniform sampler2D textureMap;

// The render states of Direct3D 5 that change a pixel. The components of flags enable the
// texture, the alpha of the texture, the colour key, and the specular colour. The components of
// modes enable the fog and select the alpha function (a D3DCMPFUNC, or 0 for no test).
layout(set = 3, binding = 0) uniform PixelState {
    vec4 fogColor;
    ivec4 flags;
    ivec4 modes;
    vec4 alphaReference;
};

bool PassesAlphaTest(float alpha, int function, float reference) {
    switch (function) {
    case 1: return false;
    case 2: return alpha < reference;
    case 3: return alpha == reference;
    case 4: return alpha <= reference;
    case 5: return alpha > reference;
    case 6: return alpha != reference;
    case 7: return alpha >= reference;
    default: return true;
    }
}

void main() {
    vec4 color = inDiffuse;
    if (flags.x != 0) {
        vec4 texel = texture(textureMap, inTexCoord);
        // A colour-keyed texel has alpha 0 from the upload, and the key discards it.
        if (flags.z != 0 && texel.a == 0.0) {
            discard;
        }
        // Under D3DTBLEND_MODULATE the colours multiply, and the alpha is the alpha of the
        // texture when it has one and of the vertex otherwise.
        color.rgb *= texel.rgb;
        if (flags.y != 0) {
            color.a = texel.a;
        }
    }
    if (flags.w != 0) {
        color.rgb = min(color.rgb + inSpecular.rgb, vec3(1.0));
    }
    // With vertex fog, the alpha of the specular colour is the share of the pixel colour.
    if (modes.x != 0) {
        color.rgb = mix(fogColor.rgb, color.rgb, inSpecular.a);
    }
    if (modes.y != 0 && !PassesAlphaTest(color.a, modes.y, alphaReference.x)) {
        discard;
    }
    outColor = color;
}
