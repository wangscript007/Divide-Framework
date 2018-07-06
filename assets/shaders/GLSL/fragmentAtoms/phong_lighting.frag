
in vec2 _texCoord;
in vec3 _normalWV;
in vec4 _vertexWV;

uniform mat4  material;
uniform float opacity;
uniform int   isSelected;

#ifndef SKIP_TEXTURES
#include "texturing.frag"
#endif

#include "phong_light_loop.frag"

vec4 Phong(in vec2 texCoord, in vec3 normal){
    
#if defined(USE_OPACITY_MAP)
    // discard material if it is bellow opacity threshold
    if(texture(texOpacityMap, texCoord).a < ALPHA_DISCARD_THRESHOLD) discard;
#else
    if(opacity< ALPHA_DISCARD_THRESHOLD) discard;
#endif


#ifndef SKIP_TEXTURES
    //this shader is generated only for nodes with at least 1 texture
    vec4 tBase;

    //Get the texture color. use Replace for the first texture
    applyTexture(texDiffuse0, textureOperation0, 0, texCoord, tBase);
    
    //If we have a second diffuse texture
    if(textureCount > 1){
        //Apply the second texture over the first
        applyTexture(texDiffuse1, textureOperation1, 0, texCoord, tBase);
    }
    //If the texture's alpha channel is less than 1/3, discard current fragment
    if(tBase.a < ALPHA_DISCARD_THRESHOLD) discard;
#endif

    MaterialProperties materialProp;
    materialProp.shadowFactor = 1.0;
    phong_loop(texCoord, normalize(normal), materialProp);
    //Add global ambient value
    materialProp.ambient += dvd_lightAmbient * material[0];
    //Add selection ambient value
    materialProp.ambient = mix(materialProp.ambient, vec4(1.0), isSelected);
#ifndef SKIP_TEXTURES
    materialProp.ambient *= tBase;
    materialProp.diffuse *= tBase;
#endif

    //Add material color terms to the final color
    vec4 linearColor = materialProp.ambient + materialProp.diffuse + materialProp.specular;
    //Apply shadowing
    linearColor *= (0.4 + 0.6 * materialProp.shadowFactor);

    //vec3 gamma = vec3(1.0/2.2);
//	return vec4(pow(linearColor, gamma), linearColor.a);
    return linearColor;

}