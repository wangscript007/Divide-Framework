#include "Headers/LightManager.h"

#include "Hardware/Video/GFXDevice.h"
#include "Core/Headers/ParamHandler.h"
#include "Managers/Headers/ResourceManager.h"
#include "Rendering/Lighting/Headers/Light.h"
#include "Hardware/Video/FrameBufferObject.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"

LightManager::LightManager(){
}

LightManager::~LightManager(){
	clear();
}

void LightManager::setAmbientLight(const vec4& light){
	GFXDevice::getInstance().setAmbientLight(light);
}

bool LightManager::clear(){
	//Lights are removed by the sceneGraph
	for_each(LightMap::value_type& it, _lights){
		//in case we had some light hanging
		RemoveResource(it.second);
	}
	_lights.clear();
	if(_lights.empty()) return true;
	else return false;
}

bool LightManager::addLight(Light* const light){
	if(light->getId() == 0){
		light->setId(generateNewID());
	}
	LightMap::iterator& it = _lights.find(light->getId());
	if(it != _lights.end()){
		Console::getInstance().errorfn("LightManager: Light with ID: %d already exists in light map",light->getId());
		return false;
	}

	_lights.insert(std::make_pair(light->getId(),light));
	return true;
}

bool LightManager::removeLight(U32 lightId){
	LightMap::iterator it = _lights.find(lightId);

	if(it == _lights.end()){
		Console::getInstance().errorfn("LightManager: Could not remove light with ID: %d",lightId);
		return false;
	}

	_lights.erase(lightId); //remove it from the map
	return true;
}

U32 LightManager::generateNewID(){
	U32 tempId = _lights.size();
	while(!checkId(tempId)){
		tempId++;
	}
	return tempId;
}

bool LightManager::checkId(U32 value){
	for_each(LightMap::value_type& it, _lights){
		if(it.second->getId() == value){
			return false;
		}
	}
	return true;
}

void LightManager::update(){
	for_each(LightMap::value_type& light, _lights){
		light.second->onDraw();
	}
}

void LightManager::generateShadowMaps(){
	//Stop if we have shadows disabled
	if(!ParamHandler::getInstance().getParam<bool>("enableShadows")) return;
	//Tell the engine that we are drawing to depth maps
	//Remember the previous render stage type
	RENDER_STAGE prev = GFXDevice::getInstance().getRenderStage();
	//set the current render stage to SHADOW_STAGE
	GFXDevice::getInstance().setRenderStage(SHADOW_STAGE);
	//tell the rendering API to render geometry for depth storage
	GFXDevice::getInstance().toggleDepthMapRendering(true);
	//generate shadowmaps for each light
	for_each(LightMap::value_type& light, _lights){
		light.second->generateShadowMaps();
	}
	//Set normal rendering settings
	GFXDevice::getInstance().toggleDepthMapRendering(false);
	//Revert back to the previous stage
	GFXDevice::getInstance().setRenderStage(prev);
}

void LightManager::previewDepthMaps(){
	GFXDevice::getInstance().renderInViewport(vec4(0,0,256,256), boost::bind(&LightManager::drawDepthMap, boost::ref(LightManager::getInstance()), 0,0));
	GFXDevice::getInstance().renderInViewport(vec4(260,0,256,256), boost::bind(&LightManager::drawDepthMap, boost::ref(LightManager::getInstance()), 0,1));
	GFXDevice::getInstance().renderInViewport(vec4(520,0,256,256), boost::bind(&LightManager::drawDepthMap, boost::ref(LightManager::getInstance()), 0,2));
}

void LightManager::drawDepthMap(U8 light, U8 index){
	Quad3D renderQuad;
	std::stringstream ss;
	ss << "Light " << (U32)light << " viewport " << (U32)index;
	renderQuad.setName(ss.str());
	renderQuad.setDimensions(vec4(0,0,_lights[0]->getDepthMaps()[index]->getWidth(),_lights[0]->getDepthMaps()[index]->getHeight()));
	_lights[0]->getDepthMaps()[index]->Bind(0);
	GFXDevice::getInstance().toggle2D(true);
	GFXDevice::getInstance().renderModel(&renderQuad);
	GFXDevice::getInstance().toggle2D(false);
	_lights[0]->getDepthMaps()[index]->Unbind(0);
}

//If we have computed shadowmaps, bind them before rendering any geomtry;
//Always bind shadowmaps to slots 8, 9 and 8;
void LightManager::bindDepthMaps(){
	//Skip applying shadows if we are rendering to depth map, or we have shadows disabled
	//For deferred rendering, we need another approach
	if(!ParamHandler::getInstance().getParam<bool>("enableShadows")) return;
	U8 lightCount = 0;
	for_each(LightMap::value_type& light, _lights){
		std::vector<FrameBufferObject* >& depthMaps = light.second->getDepthMaps();
		for(U8 i = 0; i < depthMaps.size(); ++i){
			if(depthMaps[i]){
				depthMaps[i]->Bind(/*(depthMaps.size()*lightCount)+*/8+i);
			}
		}
		lightCount++;
	}
}


void LightManager::unbindDepthMaps(){
	if(!ParamHandler::getInstance().getParam<bool>("enableShadows")) return;
	U8 lightCount = 0;
	for_each(LightMap::value_type& light, _lights){
		std::vector<FrameBufferObject* >& depthMaps = light.second->getDepthMaps();
		for(I8 i = depthMaps.size() - 1; i >= 0; --i){
			if(depthMaps[i]){
				depthMaps[i]->Unbind(/*(depthMaps.size()*lightCount)+*/8+i);
			}
		}
		lightCount++;
	}
}

bool LightManager::shadowMappingEnabled(){
	if(!ParamHandler::getInstance().getParam<bool>("enableShadows")) return false;

	for_each(LightMap::value_type& light, _lights){
		std::vector<FrameBufferObject* >& depthMaps = light.second->getDepthMaps();
		if(!depthMaps.empty()) return true;
	}
	return false;
}


I8 LightManager::getDepthMapPerLightCount() {
	if(_lights.empty()) return 0; 
	else return _lights[0]->getDepthMaps().size();
}