#ifndef _TERRAIN_SPLATTING_FRAG_
#define _TERRAIN_SPLATTING_FRAG_

layout(binding = TEXTURE_SPLAT) uniform sampler2DArray texBlendMaps;

#if defined(PRE_PASS)
layout(binding = TEXTURE_NORMAL_TILE) uniform sampler2DArray texNormalMaps;
#else
layout(binding = TEXTURE_ALBEDO_TILE) uniform sampler2DArray texTileMaps;
#endif

layout(binding = TEXTURE_EXTRA_TILE) uniform sampler2DArray texExtraMaps;
layout(binding = TEXTURE_HELPER_TEXTURES) uniform sampler2DArray helperTextures;

#include "texturing.frag"

#if DETAIL_LEVEL > 0 && (defined(USE_PARALLAX_OCCLUSION_MAPPING) || defined(USE_PARALLAX_MAPPING))
#define HAS_PARALLAX
#endif

struct TerrainData {
#if !defined(PRE_PASS)
    vec4  albedo; // a = roughness
#endif
    vec3  normal;
    vec2  uv;
};


const int tiling[] = {
    int(TEXTURE_TILE_SIZE * 1.0f), //LoD 0
    int(TEXTURE_TILE_SIZE * 1.0f), //LoD 1
    int(TEXTURE_TILE_SIZE * 0.25f),
    int(TEXTURE_TILE_SIZE * 0.0625f),
    int(TEXTURE_TILE_SIZE * 0.015625f),
    int(TEXTURE_TILE_SIZE * 0.00390625f),
    int(TEXTURE_TILE_SIZE * 0.0009765625f),
};

vec4 _getTexture(in sampler2DArray tex, in vec3 uv) {
    if (DETAIL_LEVEL > 2 && LoD == 0) {
        return textureNoTile(tex, helperTextures, 3, uv);
    }

    if (DETAIL_LEVEL > 1) {
        return textureNoTile(tex, uv);
    }

    return texture(tex, uv);
}

void getBlendFactor(in vec2 uv, inout float blendAmount[TOTAL_LAYER_COUNT]) {
    uint offset = 0;
    for (uint i = 0; i < MAX_TEXTURE_LAYERS; ++i) {
        const vec4 blendColour = texture(texBlendMaps, vec3(uv, i));
        const uint layerCount = CURRENT_LAYER_COUNT[i];
        for (uint j = 0; j < layerCount; ++j) {
            blendAmount[offset + j] = blendColour[j];
        }
        offset += layerCount;
    }
}

#if defined(HAS_PARALLAX)

float getDisplacementValueFromCoords(vec2 sampleUV, in float[TOTAL_LAYER_COUNT] amnt) {
    float ret = 0.0f;
    for (uint i = 0; i < TOTAL_LAYER_COUNT; ++i) {
        ret = mix(ret, _getTexture(texExtraMaps, vec3(sampleUV, DISPLACEMENT_IDX[i])).r, amnt[i]);
    }
    return ret;
}

float getDisplacementValue(vec2 sampleUV) {
    vec2 blendUV = unscaledTextureCoords(sampleUV, tiling[LoD]);

    INSERT_BLEND_AMNT_ARRAY;
    getBlendFactor(blendUV, blendAmount);

    return getDisplacementValueFromCoords(sampleUV, blendAmount);
}
#endif

vec2 _getScaledCoords(in vec2 uv, in float[TOTAL_LAYER_COUNT] amnt) {
    vec2 scaledCoords = scaledTextureCoords(uv, tiling[LoD]);
#if defined(HAS_PARALLAX)
    if (LoD == 0) {
        const vec3 viewDir = normalize(-VAR._vertexWV.xyz);
        float currentHeight = getDisplacementValueFromCoords(scaledCoords, amnt);
#if defined(USE_PARALLAX_MAPPING)
        return ParallaxOffset(scaledCoords, viewDir, currentHeight);
#else
        return ParallaxOcclusionMapping(scaledCoords, viewDir, currentHeight);
#endif //USE_PARALLAX_OCCLUSION_MAPPING
    }
#endif //HAS_PARALLAX
    return scaledCoords;
}

#if defined(PRE_PASS)
vec3 _getUnderwaterNormal(in vec2 uv) {
    return (2.0f * texture(helperTextures, vec3(uv, 2)).rgb - 1.0f);
}

vec3 _getTerrainNormal(in vec2 uv, in float blendAmount[TOTAL_LAYER_COUNT]) {
    vec3 normal = vec3(0.0f);

    for (uint i = 0; i < TOTAL_LAYER_COUNT; ++i) {
        normal = mix(normal, _getTexture(texNormalMaps, vec3(uv, NORMAL_IDX[i])).rgb, blendAmount[i]);
    }

    return (2.0f * normal - 1.0f);
}

vec3 _getTerrainNormal(in vec2 uv) {
    INSERT_BLEND_AMNT_ARRAY;
    getBlendFactor(uv, blendAmount);

    return _getTerrainNormal(_getScaledCoords(uv, blendAmount), blendAmount);
}
#else //PRE_PASS

vec4 _getUnderwaterAlbedo(in vec2 uv, in float waterDepth) {
#if defined(LOW_QUALITY)
    return vec4(texture(helperTextures, vec3(uv, 1)).rgb, 0.3f);
#else //LOW_QUALITY
    float time2 = float(dvd_time) * 0.0001f;
    vec4 scrollingUV = vec4(uv, uv + time2);
    scrollingUV.s -= time2;

    return vec4(mix((texture(helperTextures, vec3(scrollingUV.st, 0)).rgb + texture(helperTextures, vec3(scrollingUV.pq, 0)).rgb) * 0.5f,
                    texture(helperTextures, vec3(uv, 1)).rgb,
                    waterDepth),
                0.3f);
#endif //LOW_QUALITY
}

vec4 _getTerrainAlbedo(in vec2 uv, in float blendAmount[TOTAL_LAYER_COUNT]) {
    vec4 ret = vec4(0.0f, 0.0f, 0.0f, 0.0f);
    for (uint i = 0; i < TOTAL_LAYER_COUNT; ++i) {
        // Albedo & Roughness
        ret = mix(ret, _getTexture(texTileMaps, vec3(uv, ALBEDO_IDX[i])), blendAmount[i]);
        // ToDo: AO
    }

    return ret;
}

vec4 _getTerrainAlbedo(in vec2 uv) {
    INSERT_BLEND_AMNT_ARRAY;
    getBlendFactor(uv, blendAmount);

    return _getTerrainAlbedo(_getScaledCoords(uv, blendAmount), blendAmount);
}
#endif // PRE_PASS

TerrainData BuildTerrainData(in vec2 waterDetails) {
    TerrainData ret;
    ret.uv = getTexCoord();
#if !defined(PRE_PASS)
    ret.albedo = vec4(0.0f, 0.0f, 0.0f, 0.0f);
    ret.normal = getNormal(ret.uv);
#else //PRE_PASS
#if defined(LOW_QUALITY)
    ret.normal = VAR._normalWV;
#else //LOW_QUALITY
    ret.normal = vec3(1.0f);
#endif //LOW_QUALITY
#endif //PRE_PASS

#if defined(PRE_PASS)
#   if !defined(LOW_QUALITY)
    ret.normal = VAR._tbn * mix(_getTerrainNormal(ret.uv),
                                _getUnderwaterNormal(ret.uv * UNDERWATER_TILE_SCALE),
                                waterDetails.x);
#   endif //LOW_QUALITY

    ret.normal = normalize(ret.normal);
#else // PRE_PASS

    ret.albedo = mix(_getTerrainAlbedo(ret.uv),
                     _getUnderwaterAlbedo(ret.uv * UNDERWATER_TILE_SCALE, waterDetails.y),
                     waterDetails.x);
#endif //PRE_PASS

    return ret;
}


#endif //_TERRAIN_SPLATTING_FRAG_