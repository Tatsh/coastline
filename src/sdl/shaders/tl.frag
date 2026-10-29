#version 450

layout(location = 0) in vec4 inDiffuse;
layout(location = 1) in vec4 inSpecular;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec4 outColor;

layout(set = 2, binding = 0) uniform sampler2D textureMap;
layout(set = 2, binding = 1) uniform sampler2D textureMap1;

// The render states of Direct3D 5 that change a pixel. The components of flags enable the
// texture, the alpha of the texture, the colour key, and the specular colour. The components of
// modes enable the fog and select the alpha function (a D3DCMPFUNC, or 0 for no test).
layout(set = 3, binding = 0) uniform PixelState {
    vec4 fogColor;
    ivec4 flags;
    ivec4 modes;
    vec4 alphaReference;
    // The colour combiner of the RDP of the Nintendo 64. The selectors of the a, b, c, and d of
    // each cycle, whether it combines and has two cycles, the primitive and environment colours,
    // the level of detail fraction and the alpha threshold, and the scale and offset of the
    // coordinates of the second texture.
    ivec4 combineColor[2];
    ivec4 combineAlpha[2];
    ivec4 combineFlags;
    vec4 primitive;
    vec4 environment;
    vec4 combineValues;
    vec4 texture1Transform;
};

// The inputs of the a, b, c, and d of the colour of a cycle, by the selectors of the RDP.
vec3 ColorInput(int slot, int selector, vec4 combined, vec4 texel0, vec4 texel1, vec4 shade) {
    switch (selector) {
    case 0: return combined.rgb;
    case 1: return texel0.rgb;
    case 2: return texel1.rgb;
    case 3: return primitive.rgb;
    case 4: return shade.rgb;
    case 5: return environment.rgb;
    default: break;
    }
    if (slot == 0 || slot == 3) {
        return selector == 6 ? vec3(1.0) : vec3(0.0);
    }
    if (slot == 2) {
        switch (selector) {
        case 7: return vec3(combined.a);
        case 8: return vec3(texel0.a);
        case 9: return vec3(texel1.a);
        case 10: return vec3(primitive.a);
        case 11: return vec3(shade.a);
        case 12: return vec3(environment.a);
        case 14: return vec3(combineValues.x);
        default: return vec3(0.0);
        }
    }
    return vec3(0.0);
}

float AlphaInput(int slot, int selector, vec4 combined, vec4 texel0, vec4 texel1, vec4 shade) {
    switch (selector) {
    case 0: return slot == 2 ? 0.0 : combined.a;
    case 1: return texel0.a;
    case 2: return texel1.a;
    case 3: return primitive.a;
    case 4: return shade.a;
    case 5: return environment.a;
    case 6: return slot == 2 ? combineValues.x : 1.0;
    default: return 0.0;
    }
}

vec4 CombineCycle(int cycle, vec4 combined, vec4 texel0, vec4 texel1, vec4 shade) {
    // The second cycle reads the texels the other way round.
    vec4 first = cycle == 1 ? texel1 : texel0;
    vec4 second = cycle == 1 ? texel0 : texel1;
    ivec4 c = combineColor[cycle];
    ivec4 a = combineAlpha[cycle];
    vec3 rgb = (ColorInput(0, c.x, combined, first, second, shade) -
                ColorInput(1, c.y, combined, first, second, shade)) *
                   ColorInput(2, c.z, combined, first, second, shade) +
               ColorInput(3, c.w, combined, first, second, shade);
    float alpha = (AlphaInput(0, a.x, combined, first, second, shade) -
                   AlphaInput(1, a.y, combined, first, second, shade)) *
                      AlphaInput(2, a.z, combined, first, second, shade) +
                  AlphaInput(3, a.w, combined, first, second, shade);
    return clamp(vec4(rgb, alpha), 0.0, 1.0);
}

vec4 Combine(vec4 shade) {
    vec4 texel0 = texture(textureMap, inTexCoord);
    vec4 texel1 = texture(textureMap1, inTexCoord * texture1Transform.xy + texture1Transform.zw);
    vec4 combined = CombineCycle(0, vec4(0.0), texel0, texel1, shade);
    if (combineFlags.y != 0) {
        combined = CombineCycle(1, combined, texel0, texel1, shade);
    }
    return combined;
}

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
    if (combineFlags.x != 0) {
        vec4 combined = Combine(inDiffuse);
        if (combined.a < combineValues.y) {
            discard;
        }
        outColor = combined;
        return;
    }
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
