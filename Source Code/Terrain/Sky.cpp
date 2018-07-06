#include "Sky.h"

#include "Utility/Headers/ParamHandler.h"
#include "Managers/ResourceManager.h"
#include "Managers/SceneManager.h"
#include "Managers/CameraManager.h"
#include "Hardware/Video/GFXDevice.h"
#include "Hardware/Video/ShaderHandler.h"
#include "Geometry/Predefined/Sphere3D.h"
using namespace std;

Sky::Sky() : _skyShader(NULL), _skybox(NULL), _init(false){}

Sky::~Sky(){
	RemoveResource(_skyShader);
	RemoveResource(_skybox);
}

bool Sky::load() {
	if(_init) return false;
   	string location = ParamHandler::getInstance().getParam<string>("assetsLocation")+"/misc_images/";
	ResourceDescriptor skybox("SkyBox");
	skybox.setFlag(true); //no default material;
	_sky = ResourceManager::getInstance().loadResource<Sphere3D>(skybox);
	_sky->getResolution() = 4;
	_skyNode = SceneManager::getInstance().getActiveScene()->getSceneGraph()->getRoot()->addNode(_sky);
	_sky->setRenderState(false);
	ResourceDescriptor sun("Sun");
	sun.setFlag(true);
	_sun = ResourceManager::getInstance().loadResource<Sphere3D>(sun);
	_sun->getResolution() = 16;
	_sun->getSize() = 0.1f;
	_sunNode = SceneManager::getInstance().getActiveScene()->getSceneGraph()->getRoot()->addNode(_sun);
	_sun->setRenderState(false);
	ResourceDescriptor skyboxTextures("SkyboxTextures");
	skyboxTextures.setResourceLocation(location+"skybox_2.jpg "+ location+"skybox_1.jpg "+
								       location+"skybox_5.jpg "+ location+"skybox_6.jpg "+ 
									   location+"skybox_3.jpg "+ location+"skybox_4.jpg");

	_skybox =  ResourceManager::getInstance().loadResource<TextureCubemap>(skyboxTextures);
	ResourceDescriptor skyShaderDescriptor("sky");
	_skyShader = ResourceManager::getInstance().loadResource<Shader>(skyShaderDescriptor);
	assert(_skyShader);
	Console::getInstance().printfn("Generated sky cubemap and sun OK!");
	_init = true;
	return true;
}

void Sky::draw() const{
	if(!_init) return;
	if (_drawSky && _drawSun) drawSkyAndSun();
	if (_drawSky && !_drawSun) drawSky();
	if (!_drawSky && _drawSun) drawSun();
	GFXDevice::getInstance().clearBuffers(GFXDevice::DEPTH_BUFFER);
}

void Sky::setParams(const vec3& eyePos, const vec3& sunVect, bool invert, bool drawSun, bool drawSky) {
	if(!_init) load();
	_eyePos = eyePos;	_sunVect = sunVect;
	_invert = invert;	_drawSun = drawSun;
	_drawSky = drawSky;
}

void Sky::drawSkyAndSun() const {
	_skyNode->getTransform()->setPosition(vec3(_eyePos.x,_eyePos.y,_eyePos.z));
	_skyNode->getTransform()->scale(vec3(1.0f, _invert ? -1.0f : 1.0f, 1.0f));

	GFXDevice::getInstance().ignoreStateChanges(true);

	RenderState s(false,false,false,true);
	GFXDevice::getInstance().setRenderState(s);

	_skybox->Bind(0);
	_skyShader->bind();
	
		_skyShader->UniformTexture("texSky", 0);
		_skyShader->Uniform("enable_sun", true);
		_skyShader->Uniform("sun_vector", _sunVect);

		GFXDevice::getInstance().drawSphere3D(_skyNode);
		
	
	_skyShader->unbind();
	_skybox->Unbind(0);
	GFXDevice::getInstance().ignoreStateChanges(false);
	
}

void Sky::drawSky() const {
	_skyNode->getTransform()->setPosition(vec3(_eyePos.x,_eyePos.y,_eyePos.z));
	_skyNode->getTransform()->scale(vec3(1.0f, _invert ? -1.0f : 1.0f, 1.0f));
	GFXDevice::getInstance().ignoreStateChanges(true);

	RenderState s(false,false,false,true);
	GFXDevice::getInstance().setRenderState(s);

	_skybox->Bind(0);
	_skyShader->bind();
	
		_skyShader->UniformTexture("texSky", 0);
		_skyShader->Uniform("enable_sun", false);

		GFXDevice::getInstance().drawSphere3D(_skyNode);
	
	_skyShader->unbind();
	_skybox->Unbind(0);

	GFXDevice::getInstance().ignoreStateChanges(false);

}

void Sky::drawSun() const {
	_sun->getMaterial()->setDiffuse(SceneManager::getInstance().getActiveScene()->getLights()[0]->getDiffuseColor());
	_sunNode->getTransform()->setPosition(vec3(_eyePos.x-_sunVect.x,_eyePos.y-_sunVect.y,_eyePos.z-_sunVect.z));
	
	GFXDevice::getInstance().ignoreStateChanges(true);
	RenderState s(false,false,false,true);
	GFXDevice::getInstance().setRenderState(s);

	GFXDevice::getInstance().drawSphere3D(_sunNode);

	GFXDevice::getInstance().ignoreStateChanges(false);
}


