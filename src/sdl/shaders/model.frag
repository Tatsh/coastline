#version 450

layout(location = 0) in vec2 inTexCoord;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inColor;
layout(location = 3) in vec3 inWorld;

layout(location = 0) out vec4 outColor;

layout(set = 2, binding = 0) uniform sampler2D textureMap;
layout(set = 2, binding = 1) uniform sampler2D textureMap1;
layout(set = 2, binding = 2) uniform sampler2D shadowMap;

// flags: x is the alpha a texel needs to be drawn, or -1 to draw every texel, y is 1 for a lit
// material, whose vertices have normals, z is 1 for a textured material, and w is 1 where the
// combiner of the RDP is given. combine holds its two words, and prim and env the primitive and
// the environment colour.
// lightDirection points toward the light in the space of the normals, and the light of a lit
// vertex is the ambient colour and the directional colour by the cosine of its angle to it.
layout(set = 3, binding = 0) uniform Material {
    vec4 flags;
    vec4 lightDirection;
    uvec4 combine;
    vec4 prim;
    vec4 env;
    vec4 ambient;
    vec4 directional;
    // The scale in xy and the offset in zw from the texture coordinate of the first texel to the
    // second, which a glyph samples its gradient by.
    vec4 texel1Transform;
    // The shadow of a car on the track, as RenderTrackInstance() lays it on the ground: the place
    // of the car in xy and the scales along and across it in zw, its heading in xy and in z 1
    // where the shadow falls on the fragment, and its colour, the fog by the fog of the car.
    vec4 shadowCenter;
    vec4 shadowForward;
    vec4 shadowColor;
    // The colour of the fog, and in w its range, or 0 where there is no fog.
    vec4 fogColor;
};

// The inputs of a cycle of the colour combiner, as the RDP numbers them. The second cycle reads the
// colour of the first as COMBINED.
vec3 ColorInput(uint code, int slot, vec4 t0, vec4 t1, vec4 shade, float lod, vec4 combined) {
    if (code == 0u) return combined.rgb;
    if (code == 7u && slot == 2) return vec3(combined.a);
    switch (code) {
    case 1u: return t0.rgb;
    case 2u: return t1.rgb;
    case 3u: return prim.rgb;
    case 4u: return shade.rgb;
    case 5u: return env.rgb;
    }
    if (slot == 0 && code == 6u) return vec3(1.0);
    if (slot == 2) {
        switch (code) {
        case 8u: return vec3(t0.a);
        case 9u: return vec3(t1.a);
        case 10u: return vec3(prim.a);
        case 11u: return vec3(shade.a);
        case 12u: return vec3(env.a);
        case 13u: return vec3(lod);
        }
    }
    if (slot == 3 && code == 6u) return vec3(1.0);
    return vec3(0.0);
}

float AlphaInput(uint code, int slot, vec4 t0, vec4 t1, vec4 shade, float lod, vec4 combined) {
    if (code == 0u && slot != 2) return combined.a;
    switch (code) {
    case 1u: return t0.a;
    case 2u: return t1.a;
    case 3u: return prim.a;
    case 4u: return shade.a;
    case 5u: return env.a;
    }
    if (slot == 2) return code == 0u ? lod : 0.0;
    return code == 6u ? 1.0 : 0.0;
}

void main() {
    vec4 t0 = flags.z > 0.5 ? texture(textureMap, inTexCoord) : vec4(1.0);
    vec4 t1 = texture(textureMap1, inTexCoord * texel1Transform.xy + texel1Transform.zw);
    vec4 shade = inColor;
    if (flags.y > 0.5) {
        float diffuse = max(dot(normalize(inNormal), lightDirection.xyz), 0.0);
        shade = vec4(min(ambient.rgb + directional.rgb * diffuse, vec3(1.0)), inColor.a);
    }
    vec4 color = t0 * shade;
    if (flags.w > 0.5) {
        // The fraction between two levels of detail, from the texels a pixel covers.
        vec2 footprint = max(abs(dFdx(inTexCoord)), abs(dFdy(inTexCoord))) *
                         vec2(textureSize(textureMap, 0));
        float lod = clamp(log2(max(max(footprint.x, footprint.y), 1.0)), 0.0, 1.0);
        uint high = combine.x;
        uint low = combine.y;
        vec4 none = vec4(0.0);
        vec3 rgb = (ColorInput(high >> 20 & 0xfu, 0, t0, t1, shade, lod, none) -
                    ColorInput(low >> 28 & 0xfu, 1, t0, t1, shade, lod, none)) *
                       ColorInput(high >> 15 & 0x1fu, 2, t0, t1, shade, lod, none) +
                   ColorInput(low >> 15 & 0x7u, 3, t0, t1, shade, lod, none);
        float alpha = (AlphaInput(high >> 12 & 0x7u, 0, t0, t1, shade, lod, none) -
                       AlphaInput(low >> 12 & 0x7u, 1, t0, t1, shade, lod, none)) *
                          AlphaInput(high >> 9 & 0x7u, 2, t0, t1, shade, lod, none) +
                      AlphaInput(low >> 9 & 0x7u, 3, t0, t1, shade, lod, none);
        color = clamp(vec4(rgb, alpha), 0.0, 1.0);
        // The second cycle, where combine.z is 1, takes the first as COMBINED. It reads the texels
        // the other way round.
        if (combine.z == 1u) {
            vec4 combined = color;
            rgb = (ColorInput(high >> 5 & 0xfu, 0, t1, t0, shade, lod, combined) -
                   ColorInput(low >> 24 & 0xfu, 1, t1, t0, shade, lod, combined)) *
                      ColorInput(high & 0x1fu, 2, t1, t0, shade, lod, combined) +
                  ColorInput(low >> 6 & 0x7u, 3, t1, t0, shade, lod, combined);
            alpha = (AlphaInput(low >> 21 & 0x7u, 0, t1, t0, shade, lod, combined) -
                     AlphaInput(low >> 3 & 0x7u, 1, t1, t0, shade, lod, combined)) *
                        AlphaInput(low >> 18 & 0x7u, 2, t1, t0, shade, lod, combined) +
                    AlphaInput(low & 0x7u, 3, t1, t0, shade, lod, combined);
            color = clamp(vec4(rgb, alpha), 0.0, 1.0);
        }
    }
    // Where lightDirection.w is 1, the pipeline covers the samples of the pixel by the alpha, and
    // the alpha a texel needs is the middle of an edge one pixel wide rather than a cut.
    float coverage = 1.0;
    if (lightDirection.w > 0.5) {
        coverage = clamp((color.a - flags.x) / max(fwidth(color.a), 0.0001) + 0.5, 0.0, 1.0);
        if (coverage <= 0.0) {
            discard;
        }
    } else if (color.a < flags.x) {
        discard;
    }
    if (shadowForward.z > 0.5) {
        // The texture lies along the car on the ground, centred under it, 1024 to its width, and
        // a texel of less than 8 draws nothing, as the threshold of the blend colour cuts it.
        vec2 offset = inWorld.xy - shadowCenter.xy;
        vec2 forward = shadowForward.xy;
        vec2 st = (vec2(shadowCenter.z * dot(offset, forward),
                        shadowCenter.w * dot(offset, vec2(-forward.y, forward.x))) +
                   512.0) /
                  1024.0;
        float shade = texture(shadowMap, clamp(st, 0.0, 1.0)).a;
        if (shade * 255.0 >= 8.0) {
            color.rgb = mix(color.rgb, shadowColor.rgb, shade);
        }
    }
    if (fogColor.w > 0.0) {
        // VertexFog() of the PC release: the range by the reciprocal of w, which runs straight
        // across the screen between the clipped vertices, and none of the colour below 8 of 255.
        float shown = trunc(fogColor.w * gl_FragCoord.w);
        color.rgb = mix(fogColor.rgb, color.rgb, (shown < 8.0 ? 0.0 : min(shown, 255.0)) / 255.0);
    }
    outColor = color;
    if (lightDirection.w > 0.5) {
        outColor.a = coverage;
    }
}
