#ifndef _SHADOW_DIRECTIONAL_FRAG_
#define _SHADOW_DIRECTIONAL_FRAG_

#define LT(n) n, n, n, n, n, n, n, n, n, n, n, n, n, n, n, n

#if 0
// Reduces VSM light bleeding (remove the [0, amount] tail and linearly rescale (amount, 1])
float reduceLightBleeding(float pMax, float amount) {
    return linstep(amount, 1.0f, pMax);
}

float chebyshevUpperBound(vec2 moments, float depth, float minVariance) {
    if (depth <= moments.x + dvd_shadowingSettings.x) {
        return 1.0;
    }

    float variance = max(moments.y - (moments.x * moments.x), minVariance);
    float d = (depth - moments.x);
    float p_max = variance / (variance + d*d);
    //return max(1.0 - p_max, 0.0);
    return p_max;
}

float applyShadowDirectional(Shadow currentShadowSource, int splitCount, in float fragDepth) {

    // find the appropriate depth map to look up in based on the depth of this fragment
    g_shadowTempInt = 0;
    for (int i = 0; i < splitCount; ++i) {
        if (fragDepth > currentShadowSource._floatValues[i].x)      {
            g_shadowTempInt = i + 1;
        }
    }
    
    // GLOBAL
    const int SplitPowLookup[] = { 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
                                   LT(4), LT(5), LT(5), LT(6), LT(6), LT(6), LT(6),
                                   LT(7), LT(7), LT(7), LT(7), LT(7), LT(7), LT(7), LT(7)};
    // Ensure that every fragment in the quad choses the same split so that derivatives
    // will be meaningful for proper texture filtering and LOD selection.
    int SplitPow = 1 << g_shadowTempInt;
    int SplitX = int(abs(dFdx(SplitPow)));
    int SplitY = int(abs(dFdy(SplitPow)));
    int SplitXY = int(abs(dFdx(SplitY)));
    int SplitMax = max(SplitXY, max(SplitX, SplitY));
    g_shadowTempInt = SplitMax > 0 ? SplitPowLookup[SplitMax - 1] : g_shadowTempInt;

    if (g_shadowTempInt < 0 || g_shadowTempInt > splitCount) {
        return 1.0;
    }

    vec4 shadowCoords = currentShadowSource._lightVP[g_shadowTempInt] * VAR._vertexW;
    vec4 scPostW = shadowCoords / shadowCoords.w;
    if (!(sc.w <= 0 || (scPostW.x < 0 || scPostW.y < 0) || (scPostW.x >= 1 || scPostW.y >= 1))){
        float layer = float(g_shadowTempInt + currentShadowSource._arrayOffset.x);

        vec2 moments = texture(texDepthMapFromLightArray, vec3(scPostW.xy, layer)).rg;
       
        //float shadowBias = DEPTH_EXP_WARP * exp(DEPTH_EXP_WARP * dvd_shadowingSettings.y);
        //float shadowWarpedz1 = exp(scPostW.z * DEPTH_EXP_WARP);
        //return mix(chebyshevUpperBound(moments, shadowWarpedz1, dvd_shadowingSettings.y), 
        //             1.0, 
        //             clamp(((gl_FragCoord.z + dvd_shadowingSettings.z) - dvd_shadowingSettings.w) / dvd_shadowingSettings.z, 0.0, 1.0));
        return reduceLightBleeding(chebyshevUpperBound(moments, scPostW.z, dvd_shadowingSettings.y), 0.1);
    }
    
    return 1.0;
}
#else 
vec4 ShadowCoordPostW;

float chebyshevUpperBound(Shadow currentShadowSource, float distance)
{
    // We retrive the two moments previously stored (depth and depth*depth)
    float layer = float(g_shadowTempInt + currentShadowSource._arrayOffset.x);
    vec2 moments = texture(texDepthMapFromLightArray, vec3(ShadowCoordPostW.xy, layer)).rg;

    // Surface is fully lit. as the current fragment is before the light occluder
    if (distance <= moments.x)
        return 1.0;

    // The fragment is either in shadow or penumbra. We now use chebyshev's upperBound to check
    // How likely this pixel is to be lit (p_max)
    float variance = moments.y - (moments.x*moments.x);
    variance = max(variance, 0.00002);

    float d = distance - moments.x;
    float p_max = variance / (variance + d * d);

    return p_max;
}

float applyShadowDirectional(Shadow currentShadowSource, int splitCount, in float fragDepth) {
    g_shadowTempInt = 0;
    for (int i = 0; i < splitCount; ++i) {
        if (fragDepth > currentShadowSource._floatValues[i].x) {
            g_shadowTempInt = i + 1;
        }
    }
    // GLOBAL
    const int SplitPowLookup[] = { 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
        LT(4), LT(5), LT(5), LT(6), LT(6), LT(6), LT(6),
        LT(7), LT(7), LT(7), LT(7), LT(7), LT(7), LT(7), LT(7) };
    // Ensure that every fragment in the quad choses the same split so that derivatives
    // will be meaningful for proper texture filtering and LOD selection.
    int SplitPow = 1 << g_shadowTempInt;
    int SplitX = int(abs(dFdx(SplitPow)));
    int SplitY = int(abs(dFdy(SplitPow)));
    int SplitXY = int(abs(dFdx(SplitY)));
    int SplitMax = max(SplitXY, max(SplitX, SplitY));
    g_shadowTempInt = SplitMax > 0 ? SplitPowLookup[SplitMax - 1] : g_shadowTempInt;

    if (g_shadowTempInt < 0 || g_shadowTempInt > splitCount) {
        return 1.0;
    }

    vec4 shadowCoords = currentShadowSource._lightVP[g_shadowTempInt] * VAR._vertexW;

    ShadowCoordPostW = shadowCoords / shadowCoords.w;
    return chebyshevUpperBound(currentShadowSource, ShadowCoordPostW.z);
}

#endif
#endif //_SHADOW_DIRECTIONAL_FRAG_