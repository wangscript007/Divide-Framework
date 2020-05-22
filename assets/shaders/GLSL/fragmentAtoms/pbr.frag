// AMAZING RESOURCE : http://www.frostbite.com/wp-content/uploads/2014/11/course_notes_moving_frostbite_to_pbr.pdf
// Reference: https://github.com/urho3d/Urho3D/blob/master/bin/CoreData/Shaders/GLSL/PBR.glsl
// Following BRDF methods are based upon research Frostbite EA
//[Lagrade et al. 2014, "Moving Frostbite to Physically Based Rendering"]

#ifndef M_EPSILON
#define M_EPSILON 0.0000001
#endif //M_EPSILON

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif //M_PI

#include "IBL.frag"

//Schlick Gaussian Fresnel
//specular  = the rgb specular color value of the pixel
//VdotH     = the dot product of the camera view direction and the half vector 
vec3 SchlickGaussianFresnel(in vec3 f0, in float VdotH)
{
    float sphericalGaussian = pow(2.0, (-5.55473 * VdotH - 6.98316) * VdotH);
    return mix(f0, vec3(1.0f), sphericalGaussian);
}

// Smith GGX corrected Visibility
// NdotL        = the dot product of the normal and direction to the light
// NdotV        = the dot product of the normal and the camera view direction
// roughness    = the roughness of the pixel
float SmithGGXSchlickVisibility(float NdotL, float NdotV, float roughness)
{
    float rough2 = roughness * roughness;
    float lambdaV = NdotL  * sqrt((-NdotV * rough2 + NdotV) * NdotV + rough2);
    float lambdaL = NdotV  * sqrt((-NdotL * rough2 + NdotL) * NdotL + rough2);

    return 0.5 / (lambdaV + lambdaL);
}

/// Schlick approximation of Smith GGX
///     nDotL: dot product of surface normal and light direction
///     nDotV: dot product of surface normal and view direction
///     roughness: surface roughness
float SchlickG1(in float factor, in float rough2)
{
    return 1.0 / (factor * (1.0 - rough2) + rough2);
}


float SchlickVisibility(float NdotL, float NdotV, float roughness)
{
    const float rough2 = roughness * roughness;
    return (SchlickG1(NdotL, rough2) * SchlickG1(NdotV, rough2)) * 0.25;
}

// Blinn Distribution
// NdotH        = the dot product of the normal and the half vector
// roughness    = the roughness of the pixel
float BlinnPhongDistribution(in float NdotH, in float roughness)
{
    float specPower = max((2.0 / (roughness * roughness)) - 2.0, 1e-4); // Calculate specular power from roughness
    return pow(clamp(NdotH, 0.0, 1.0), specPower);
}

// Beckmann Distribution
// NdotH        = the dot product of the normal and the half vector
// roughness    = the roughness of the pixel
float BeckmannDistribution(in float NdotH, in float roughness)
{
    float rough2 = roughness * roughness;
    float roughnessA = 1.0 / (4.0 * rough2 * pow(NdotH, 4.0));
    float roughnessB = NdotH * NdotH - 1.0;
    float roughnessC = rough2 * NdotH * NdotH;
    return roughnessA * exp(roughnessB / roughnessC);
}

// GGX Distribution
// NdotH        = the dot product of the normal and the half vector
// roughness    = the roughness of the pixel
float GGXDistribution(float NdotH, float roughness)
{
    float rough2 = roughness * roughness;
    float tmp = (NdotH * rough2 - NdotH) * NdotH + 1.0;
    return rough2 / (tmp * tmp);
}

// Lambertian Diffuse
// diffuseColor = the rgb color value of the pixel
// roughness    = the roughness of the pixel
// NdotV        = the normal dot with the camera view direction
// NdotL        = the normal dot with the light direction
// VdotH        = the camera view direction dot with the half vector
vec3 LambertianDiffuse(vec3 diffuseColor, float NdotL)
{
    return diffuseColor * NdotL;
}

// Burley Diffuse
// diffuseColor = the rgb color value of the pixel
// roughness    = the roughness of the pixel
// NdotV        = the normal dot with the camera view direction
// NdotL        = the normal dot with the light direction
// VdotH        = the camera view direction dot with the half vector
vec3 BurleyDiffuse(vec3 diffuseColor, float roughness, float NdotV, float NdotL, float VdotH)
{
    float energyBias = mix(roughness, 0.0, 0.5);
    float energyFactor = mix(roughness, 1.0, 1.0 / 1.51);
    float fd90 = energyBias + 2.0 * VdotH * VdotH * roughness;
    float f0 = 1.0;
    float lightScatter = f0 + (fd90 - f0) * pow(1.0f - NdotL, 5.0f);
    float viewScatter = f0 + (fd90 - f0) * pow(1.0f - NdotV, 5.0f);

    return diffuseColor * lightScatter * viewScatter * energyFactor;
}

//Get Fresnel
//specular  = the rgb specular color value of the pixel
//VdotH     = the dot product of the camera view direction and the half vector 
vec3 Fresnel(vec3 f0, float VdotH) {
    //Schlick Fresnel
    return mix(f0, vec3(1.0f), pow(1.01f - VdotH, 5.0f));
}

// Get Distribution
// NdotH        = the dot product of the normal and the half vector
// roughness    = the roughness of the pixel
// BlinnPhongDistribution(...);
// GGXDistribution(...);
// BeckmannDistribution(...);

// Get Visibility
// NdotL        = the dot product of the normal and direction to the light
// NdotV        = the dot product of the normal and the camera view direction
// roughness    = the roughness of the pixel
// SmithGGXSchlickVisibility(...);
// SchlickVisibility(...);

// Get Diffuse
// diffuseColor = the rgb color value of the pixel
// roughness    = the roughness of the pixel
// NdotV        = the normal dot with the camera view direction
// NdotL        = the normal dot with the light direction
// VdotH        = the camera view direction dot with the half vector

// BurleyDiffuse(...) => PBR
// LambertianDiffuse(...) => diffuse * NdotL

vec4 PBR(in vec3 lightDirectionWV,
         in vec4 lightColourAndAtt,
         in vec3 occlusionMetallicRoughness,
         in vec4 albedoAndShadow,
         in vec3 normalWV,
         in float ndl)
{
    const float metallic = occlusionMetallicRoughness.g;
    const float roughness = occlusionMetallicRoughness.b;
    const vec3 specular = mix(vec3(0.04f), albedoAndShadow.rgb, metallic);
    
    const vec3 Hn = normalize(VAR._viewDirectionWV + lightDirectionWV);
    const float vdh = clamp((dot(VAR._viewDirectionWV, Hn)), M_EPSILON, 1.0);
    const float ndh = clamp((dot(normalWV, Hn)), M_EPSILON, 1.0);
    const float ndv = clamp((dot(normalWV, VAR._viewDirectionWV)), M_EPSILON, 1.0);

    const vec3 diffuseFactor = BurleyDiffuse(albedoAndShadow.rgb, roughness, ndv, ndl, vdh) * albedoAndShadow.a;

    const vec3 fresnelTerm = Fresnel(specular, vdh);

    const float distTerm = GGXDistribution(ndh, roughness);
    const float visTerm = SchlickVisibility(ndl, ndv, roughness);

    const vec3 specularFactor = ((fresnelTerm * distTerm * visTerm) / M_PI) *albedoAndShadow.a;

    return vec4((lightColourAndAtt.rgb * (diffuseFactor + specularFactor) * lightColourAndAtt.a) / M_PI, length(specularFactor));
}


vec4 Phong(in vec3 lightDirectionWV,
           in vec4 lightColourAndAtt,
           in vec3 occlusionMetallicRoughness,
           in vec4 albedoAndShadow,
           in vec3 normalWV,
           in float ndl)
{
    const float metallic = occlusionMetallicRoughness.g;
    const float roughness = occlusionMetallicRoughness.b;
    const vec3 specular = mix(vec3(0.04f), albedoAndShadow.rgb, metallic);

    const vec3 Hn = normalize(VAR._viewDirectionWV + lightDirectionWV);
    const float vdh = clamp((dot(VAR._viewDirectionWV, Hn)), M_EPSILON, 1.0);
    const float ndh = clamp((dot(normalWV, Hn)), M_EPSILON, 1.0);
    const float ndv = clamp((dot(normalWV, VAR._viewDirectionWV)), M_EPSILON, 1.0);

    const vec3 diffuseFactor = LambertianDiffuse(albedoAndShadow.rgb, ndl) * albedoAndShadow.a;

    const vec3 fresnelTerm = Fresnel(specular, vdh);
    const float distTerm = BlinnPhongDistribution(ndh, roughness);
    const float visTerm = SchlickVisibility(ndl, ndv, roughness);

    const vec3 specularFactor = ((fresnelTerm * distTerm * visTerm) / M_PI) * albedoAndShadow.a;

    return vec4((lightColourAndAtt.rgb * (diffuseFactor + specularFactor) * lightColourAndAtt.a) / M_PI, length(specularFactor));
}