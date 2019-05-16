--Fragment

#ifndef USE_COLOURED_WOIT
//#define USE_COLOURED_WOIT
#endif //USE_COLOURED_WOIT


#include "Utility.frag"

/* sum(rgb * a, a) */
layout(binding = TEXTURE_UNIT0) uniform sampler2D accumTexture;
/* prod(1 - a) */
layout(binding = TEXTURE_UNIT1) uniform sampler2D revealageTexture;

layout(location = TARGET_ALBEDO) out vec4 _colourOut;

void main() {
    ivec2 C = ivec2(gl_FragCoord.xy);

    float revealage = texelFetch(revealageTexture, C, 0).r;
    if (revealage == 1.0f) {
        // Save the blending and color texture fetch cost
        discard;
    }

    vec4 accum = texelFetch(accumTexture, C, 0);
    //vec4 normalData = texelFetch(normalAndVelocity, C, 0);
    // Suppress overflow
    if (isinf(maxComponent(abs(accum)))) {
        accum.rgb = vec3(accum.a);
    }

    // dst' =  (accum.rgb / accum.a) * (1 - revealage) + dst
    // [dst has already been modulated by the transmission colors and coverage and the blend mode
    // inverts revealage for us] 
    vec3 averageColor = accum.rgb / max(accum.a, 0.00001f);
#ifdef USE_COLOURED_WOIT
    _colourOut = vec4(averageColor, revealage);
#else
    _colourOut = vec4(averageColor, 1.0f - revealage);
#endif
}


