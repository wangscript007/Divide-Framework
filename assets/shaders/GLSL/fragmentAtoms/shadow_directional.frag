#ifndef _SHADOW_DIRECTIONAL_FRAG_
#define _SHADOW_DIRECTIONAL_FRAG_

#define LT(n) n, n, n, n, n, n, n, n, n, n, n, n, n, n, n, n

//Chebyshev Upper Bound
float VSM(vec2 moments, float distance) {
    const float E_x2 = moments.y;
    const float Ex_2 = moments.x * moments.x;
    const float variance = max(E_x2 - Ex_2, -(dvd_shadowingSettings.y));
    const float mD = (moments.x - distance);
    const float mD_2 = mD * mD;

    const float p = linstep(dvd_shadowingSettings.x, 1.0f, variance / (variance + mD_2));

    return saturate(max(p, distance <= moments.x ? 1.0f : 0.0f));
}

float getShadowFactorDirectional(in uint idx, in vec4 details) {
    const int Split = getCSMSlice(idx);

    const vec4 sc = dvd_shadowLightVP[Split + (idx * 6)] * VAR._vertexW;
    const vec3 shadowCoord = sc.xyz / sc.w;

    const bool inFrustum = shadowCoord.z <= 1.0f && 
                            all(bvec4(shadowCoord.x >= 0.0f,
                                      shadowCoord.x <= 1.0f,
                                      shadowCoord.y >= 0.0f,
                                      shadowCoord.y <= 1.0f));

    if (inFrustum)
    {
        const float layer = float(Split + details.y);

        vec2 moments = texture(texDepthMapFromLightArray, vec3(shadowCoord.xy, layer)).rg;
       
        //float shadowBias = DEPTH_EXP_WARP * exp(DEPTH_EXP_WARP * dvd_shadowingSettings.y);
        //float shadowWarpedz1 = exp(shadowCoord.z * DEPTH_EXP_WARP);
        //return mix(VSM(moments, shadowWarpedz1), 
        //             1.0, 
        //             clamp(((gl_FragCoord.z + dvd_shadowingSettings.z) - dvd_shadowingSettings.w) / dvd_shadowingSettings.z, 0.0, 1.0));

        //float bias = max(angleBias * (1.0 - dot(normal, lightDirection)), 0.0008);
        float bias = details.z;
        return max(VSM(moments, shadowCoord.z - bias), 0.2f);
    }
    
    return 1.0;
}
#endif //_SHADOW_DIRECTIONAL_FRAG_