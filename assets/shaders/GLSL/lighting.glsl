//-- Geometry
//#include "inOut.geom"

-- Vertex
#include "vboInputData.vert"
#include "lightingDefaults.vert"

#if defined(ADD_FOLIAGE)
#include "foliage.vert"
#endif
void main(void){

    computeData();

#if defined(ADD_FOLIAGE) && defined(IS_TREE)
    computeFoliageMovementTree(dvd_Vertex);
#endif
    
    computeLightVectors();
    //Compute the final vert position
    gl_Position = dvd_WorldViewProjectionMatrix * dvd_Vertex;
}

-- Fragment

#define SKIP_TEXTURES

#include "phong_lighting.frag"
#include "fog.frag"

out vec4 _colorOut;

void main (void){
    gl_FragDepth = gl_FragCoord.z;

    vec4 color = Phong(_texCoord, _normalWV);

    applyFog(color);

    _colorOut = color;
}

-- Fragment.Texture

#include "phong_lighting.frag"
#include "fog.frag"
out vec4 _colorOut;

void main (void){
    gl_FragDepth = gl_FragCoord.z;

    vec4 color = Phong(_texCoord, _normalWV);

    applyFog(color);

    _colorOut = color;
}
    
-- Fragment.Bump

#include "phong_lighting.frag"
#include "bumpMapping.frag"
#include "fog.frag"

out vec4 _colorOut;

void main (void){

    gl_FragDepth = gl_FragCoord.z;

#if defined(USE_PARALLAX_MAPPING)
    vec4 color = ParallaxMapping(_texCoord, _lightDirection[bumpMapLightId]);
#elif defined(USE_RELIEF_MAPPING)
    vec4 color = ReliefMapping(bumpMapLightId,_texCoord);
#else
    vec4 color = NormalMapping(_texCoord);
#endif

    if(color.a < ALPHA_DISCARD_THRESHOLD) discard;	
    applyFog(color);

    _colorOut = color;
}