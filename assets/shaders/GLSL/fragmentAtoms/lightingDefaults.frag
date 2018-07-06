in vec2 _texCoord;
in vec4 _vertexW;
in vec4 _vertexWV;

#include "nodeBufferedInput.cmn"

uniform bool  dvd_enableFog = true;
uniform float fogDensity;
uniform vec3  fogColor;

vec3 applyFogColor(in vec3 color){
    const float LOG2 = 1.442695;
    float zDepth = gl_FragCoord.z / gl_FragCoord.w;
    return mix(fogColor, color, clamp(exp2(-fogDensity * fogDensity * zDepth * zDepth * LOG2), 0.0, 1.0));
}

vec4 applyFog(in vec4 color){
    return vec4(dvd_enableFog ? applyFogColor(color.rgb) : color.rgb, color.a);
}  