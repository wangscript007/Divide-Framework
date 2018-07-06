#define MODE_BUMP		1
#define MODE_PARALLAX	2
#define MODE_RELIEF		3

#include "lightInput.cmn"

out vec4 _vertexWV;
out vec3 _normalWV;
out vec3 _viewDirection;
out vec3 _lightDirection[MAX_LIGHT_COUNT]; //<Light direction
smooth out float _attenuation[MAX_LIGHT_COUNT];
out vec4 _shadowCoord[MAX_SHADOW_CASTING_LIGHTS];

uniform bool dvd_enableShadowMapping = false;
uniform int  dvd_lightCount;
uniform mat4 dvd_lightProjectionMatrices[MAX_SHADOW_CASTING_LIGHTS];
uniform mat4 dvd_TestViewMatrix;

const float pidiv180 = 0.0174532777777778; //3.14159 / 180.0; // 36 degrees

void computeLightVectors(){
    _vertexWV = dvd_ViewMatrix * _vertexW; 	      //<ModelView Vertex  
    _normalWV = normalize(dvd_NormalMatrix * dvd_Normal); //<ModelView Normal 

#if defined(COMPUTE_TBN)
    vec3 T = normalize(dvd_NormalMatrix * dvd_Tangent);
    vec3 B = cross(_normalWV, T);

    if(length(dvd_Tangent) > 0){
        _normalWV = B;
    }

    _viewDirection  = vec3(dot(-_vertexWV.xyz, T), dot(-_vertexWV.xyz, B), dot(-_vertexWV.xyz, _normalWV));
#else
    _viewDirection = -_vertexWV.xyz;
#endif

    vec3 lightDirection; 
    vec3 lightPos;

    float distance = 1.0;
    float lightType = 0.0;
    float lightTypeInit = 0.0;
    Light currentLight;

    for(int i = 0; i < MAX_LIGHT_COUNT; i++){
        if(dvd_lightCount == i) break;

        currentLight = dvd_LightSource[dvd_lightIndex[i]];

        //Directional light => lightType == 0; Spot or Omni => lightType == 1
        lightTypeInit = currentLight._position.w;
        lightType = clamp(lightTypeInit, 0.0, 1.0);
        lightPos  = currentLight._position.xyz;

        //lightPosMV.w will be 0 for Directional Lights and 1 for Spot or Omni, so this avoids an "if/else"
        lightDirection = mix(-lightPos, normalize(_vertexWV.xyz - lightPos), lightType);

#if defined(COMPUTE_TBN)
        _lightDirection[i] = vec3(dot(lightDirection, T), dot(lightDirection, B), dot(lightDirection, _normalWV));
#else
        _lightDirection[i] = lightDirection;
#endif

        distance = length(lightDirection);
        //either _attenuation == 1 if light is directional or we compute the actual value for omni and spot
        _attenuation[i] = mix(1.0,max(1.0/(currentLight._attenuation.x + 
                                           currentLight._attenuation.y * distance + 
                                           currentLight._attenuation.z * distance * distance),
                                      0.0),//max
                              lightType);//mix

        // spotlight
        if (lightTypeInit > 1.5){
            float clampedCosine = max(0.0, dot(-lightDirection, normalize(currentLight._direction.rgb)));
            if (clampedCosine < cos(currentLight._attenuation.w * pidiv180)){ // outside of spotlight cone
                _attenuation[i] = 0.0;
            }else{
                _attenuation[i] = _attenuation[i] * pow(clampedCosine, currentLight._direction.w);
            }
        }
    }

    if(dvd_enableShadowMapping) {
        for(int i = 0; i < MAX_SHADOW_CASTING_LIGHTS; i++){
            if(dvd_lightCount == i) break;

            _shadowCoord[i] = dvd_lightProjectionMatrices[i] * _vertexW;
        }
    }	
}