-- Fragment.BloomApply

layout(binding = TEXTURE_UNIT0) uniform sampler2D texScreen;
layout(binding = TEXTURE_UNIT1) uniform sampler2D texBloom;
uniform float bloomFactor;

out vec4 _colourOut;

void main() {
    _colourOut = texture(texScreen, VAR._texCoord);
    vec3 bloom = texture(texBloom, VAR._texCoord).rgb;
    _colourOut.rgb = 1.0f - ((1.0f - bloom * bloomFactor) * (1.0f - _colourOut.rgb));
}

-- Fragment.BloomCalc

#include "utility.frag"
out vec4 _bloomOut;

layout(binding = TEXTURE_UNIT0) uniform sampler2D texScreen;

void main() {    
    vec4 screenColour = texture(texScreen, VAR._texCoord);
    if (luminance(screenColour.rgb) > 0.75f) {
        _bloomOut = screenColour;
    }
}
