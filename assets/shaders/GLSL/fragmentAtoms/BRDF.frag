#ifndef _BRDF_FRAG_
#define _BRDF_FRAG_

#include "lightInput.cmn"
#include "lightData.frag"
#include "utility.frag"
#include "materialData.frag"
#include "shadowMapping.frag"
#include "phong_lighting.frag"

void getBRDFFactors(const in LightPropertiesFrag lightProp, inout MaterialProperties materialProp) {
#if defined(USE_SHADING_PHONG)
    Phong(lightProp, materialProp);
#elif defined(USE_SHADING_BLINN_PHONG)
    Phong(lightProp, materialProp);
#elif defined(USE_SHADING_TOON)
#elif defined(USE_SHADING_OREN_NAYAR)
#else //if defined(USE_SHADING_COOK_TORRANCE)
#endif
}

vec4 getPixelColor(const in vec2 texCoord, in vec3 normal, in vec4 textureColor) {
    
#if defined(HAS_TRANSPARENCY)
#   if defined(USE_OPACITY_DIFFUSE_MAP)
        float alpha = textureColor.a;
#   endif
#   if defined(USE_OPACITY_DIFFUSE)
        float alpha = dvd_MatDiffuse.a;
#   endif
#   if defined(USE_OPACITY_MAP)
        vec4 opacityMap = texture(texOpacityMap, texCoord);
        float alpha = max(min(opacityMap.r, opacityMap.g), min(opacityMap.b, opacityMap.a));
#   endif
        if (dvd_useAlphaTest && alpha < ALPHA_DISCARD_THRESHOLD){
            discard;
        }
#   else
        const float alpha = 1.0;
#   endif

#if defined (USE_DOUBLE_SIDED)
        normal = normalize(gl_FrontFacing ? normal : -normal);
#else
        normal = normalize(normal);
#endif

        MaterialProperties materialProp;

#if defined(USE_SPECULAR_MAP)
        materialProp.specularColor = texture(texSpecularMap, texCoord).rgb;
#else
        materialProp.specularColor = dvd_MatSpecular;
#endif

#if defined(USE_SHADING_FLAT)
        materialProp.ambient = dvd_MatAmbient;
        materialProp.diffuse = dvd_MatDiffuse.rgb;
        materialProp.specular = vec3(0.0);
#else
    // Apply all lighting contributions
    for (uint i = 0; i < _lightCount; i++){
        getBRDFFactors(getLightProperties(i, normal), materialProp);
    }
#endif
    vec3 color = (materialProp.ambient + dvd_lightAmbient * dvd_MatAmbient) + 
                 (materialProp.diffuse * textureColor.rgb) + 
                  materialProp.specular;
    
    color *= mix(mix(1.0, 2.0, dvd_isHighlighted), 3.0, dvd_isSelected);
    // Apply shadowing
    color *= mix(1.0, shadow_loop(), dvd_shadowMapping);

    //return vec4(dvd_LightSource[0]._position.xyz, 1.0);
#if defined(_DEBUG) && defined(DEBUG_SHADOWMAPPING)
    if (dvd_showShadowDebugInfo) {
        switch (_shadowTempInt){
            case -1: color    = vec3(1.0); break;
            case  0: color.r += 0.15; break;
            case  1: color.g += 0.25; break;
            case  2: color.b += 0.40; break;
            case  3: color   += vec3(0.15, 0.25, 0.40); break;
        };
    }
#endif
    return vec4(color, alpha);
}

vec4 getPixelColor(const in vec2 texCoord, const in vec3 normal){
#if defined(SKIP_TEXTURES)
    return getPixelColor(texCoord, normal, vec4(1.0));
#else
    return getPixelColor(texCoord, normal, getTextureColor(texCoord));
#endif
}

#endif