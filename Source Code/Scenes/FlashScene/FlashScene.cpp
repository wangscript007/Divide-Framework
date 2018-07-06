#include "Headers/FlashScene.h"
#include "GUI/Headers/GUI.h"
#include "Managers/Headers/CameraManager.h"

using namespace std;

void FlashScene::render(){
	
	_sceneGraph->render();
}


void FlashScene::preRender(){

} 

void FlashScene::processInput(){
	Camera* cam = CameraManager::getInstance().getActiveCamera();

	moveFB  = Application::getInstance().moveFB;
	moveLR  = Application::getInstance().moveLR;
	angleLR = Application::getInstance().angleLR;
	angleUD = Application::getInstance().angleUD;
	
	if(angleLR)	cam->RotateX(angleLR * Framerate::getInstance().getSpeedfactor());
	if(angleUD)	cam->RotateY(angleUD * Framerate::getInstance().getSpeedfactor());
	if(moveFB)	cam->PlayerMoveForward(moveFB * (Framerate::getInstance().getSpeedfactor()/5));
	if(moveLR)	cam->PlayerMoveStrafe(moveLR * (Framerate::getInstance().getSpeedfactor()/5));

}

void FlashScene::processEvents(F32 time){
	F32 FpsDisplay = 0.3f;
	if (time - _eventTimers[0] >= FpsDisplay)	{
		
		GUI::getInstance().modifyText("fpsDisplay", "FPS: %5.2f", Framerate::getInstance().getFps());
		_eventTimers[0] += FpsDisplay;
	}
}

bool FlashScene::load(const string& name){
	setInitialData();
	bool state = false;
	addDefaultLight();
	state = loadResources(true);	
	state = loadEvents(true);

	return state;
}

bool FlashScene::loadResources(bool continueOnErrors){
	angleLR=0.0f,angleUD=0.0f,moveFB=0.0f,moveLR=0.0f;
	_sunAngle = vec2(0.0f, RADIANS(45.0f));
	_sunVector = vec4(-cosf(_sunAngle.x) * sinf(_sunAngle.y),
							-cosf(_sunAngle.y),
							-sinf(_sunAngle.x) * sinf(_sunAngle.y),
							0.0f );
		GUI::getInstance().addText("fpsDisplay",           //Unique ID
		                       vec3(60,60,0),          //Position
							   BITMAP_8_BY_13,    //Font
							   vec3(0.0f,0.2f, 1.0f),  //Color
							   "FPS: %s",0);    //Text and arguments
		_eventTimers.push_back(0.0f);
    i = 0;
	return true;
}
