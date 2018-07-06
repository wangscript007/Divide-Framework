#include "Headers/SpotLight.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "ShadowMapping/Headers/ShadowMap.h"

SpotLight::SpotLight(U8 slot, F32 range) : Light(slot,range,LIGHT_TYPE_SPOT) {
	_properties._attenuation.w = 35;
	_properties._position = vec4<F32>(0,0,5,2.0f);
	_properties._direction = vec4<F32>(1,1,0,0);
	_properties._direction.w = 1;
};

void SpotLight::setCameraToLightView(const vec3<F32>& eyePos){
	_eyePos = eyePos;
	///Trim the w parameter from the position
	vec3<F32> lightPosition = vec3<F32>(_properties._position);
	vec3<F32> spotTarget = vec3<F32>(_properties._direction);
	///A spot light has a target and a position
	_lightPos = vec3<F32>(lightPosition);
	///Tell our rendering API to move the camera
	GFX_DEVICE.lookAt(_lightPos,    //the light's  position
					  spotTarget);  //the light's target
}

void SpotLight::renderFromLightView(const U8 depthPass,const F32 sceneHalfExtent){
	GFX_DEVICE.lookAt(_lightPos,vec3<F32>(0.0f)/*target*/);
}