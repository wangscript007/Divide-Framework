#include "Headers/PhysXScene.h"

#include "Managers/Headers/SceneManager.h"
#include "Rendering/RenderPass/Headers/RenderQueue.h"
#include "Core/Headers/ParamHandler.h"
REGISTER_SCENE(PhysXScene);

//begin copy-paste
void PhysXScene::preRender(){
	getSkySGN(0)->getNode<Sky>()->setSunVector(_sunvector);
}
//<<end copy-paste

void PhysXScene::processTasks(U32 time){
	F32 FpsDisplay = 0.3f;
	if (time - _taskTimers[0] >= FpsDisplay){
		GUI::getInstance().modifyText("fpsDisplay", "FPS: %5.2f", Framerate::getInstance().getFps());
		GUI::getInstance().modifyText("RenderBinCount", "Number of items in Render Bin: %d", GFX_RENDER_BIN_SIZE);
		_taskTimers[0] += FpsDisplay;
	}
}

void PhysXScene::processInput(){

	if(state()->_angleLR) renderState()->getCamera()->RotateX(state()->_angleLR * FRAME_SPEED_FACTOR);
	if(state()->_angleUD) renderState()->getCamera()->RotateY(state()->_angleUD * FRAME_SPEED_FACTOR);
	if(state()->_moveFB)  renderState()->getCamera()->MoveForward(state()->_moveFB * (FRAME_SPEED_FACTOR/5));
	if(state()->_moveLR)  renderState()->getCamera()->MoveStrafe(state()->_moveLR * (FRAME_SPEED_FACTOR/5));
}

bool PhysXScene::load(const std::string& name){

	///Load scene resources
	SCENE_LOAD(name,true,true);
	///Add a light
	vec2<F32> sunAngle(0.0f, RADIANS(45.0f));
	_sunvector = vec3<F32>(-cosf(sunAngle.x) * sinf(sunAngle.y),-cosf(sunAngle.y),-sinf(sunAngle.x) * sinf(sunAngle.y));
	Light* light = addDefaultLight();
	light->setPosition(_sunvector);
	light->setLightProperties(LIGHT_PROPERTY_AMBIENT,vec4<F32>(1.0f,1.0f,1.0f,1.0f));
	light->setLightProperties(LIGHT_PROPERTY_DIFFUSE,vec4<F32>(1.0f,1.0f,1.0f,1.0f));
	light->setLightProperties(LIGHT_PROPERTY_SPECULAR,vec4<F32>(1.0f,1.0f,1.0f,1.0f));
	addDefaultSky();
	return loadState;
}

bool PhysXScene::loadResources(bool continueOnErrors){
	 _mousePressed = false;

	GUI::getInstance().addText("fpsDisplay",           //Unique ID
		                       vec2<U32>(60,20),          //Position
							    Font::DIVIDE_DEFAULT,    //Font
							   vec3<F32>(0.0f,0.2f, 1.0f),  //Color
							   "FPS: %s",0);    //Text and arguments
	GUI::getInstance().addText("RenderBinCount",
								vec2<U32>(60,30),
								 Font::DIVIDE_DEFAULT,
								vec3<F32>(0.6f,0.2f,0.2f),
								"Number of items in Render Bin: %d",0);

	
	_taskTimers.push_back(0.0f); //Fps
	renderState()->getCamera()->RotateX(RADIANS(-75));
	renderState()->getCamera()->RotateY(RADIANS(25));
	renderState()->getCamera()->setEye(vec3<F32>(0,30,-40));
    _addingActors = false;
    ParamHandler::getInstance().setParam("rendering.enableFog",false);
    ParamHandler::getInstance().setParam("postProcessing.bloomFactor",0.1f);
	return true;
}

bool PhysXScene::unload(){
	return Scene::unload();
}

void PhysXScene::createStack(U32 size){
	F32 stackSize = size;
	F32 CubeSize = 1.0f;
	F32 Spacing = 0.0001f;
	vec3<F32> Pos(0, 10 + CubeSize,0);
	F32 Offset = -stackSize * (CubeSize * 2.0f + Spacing) * 0.5f + 0;
    while(_addingActors){}
    _addingActors = true;
	while(stackSize){
		for(U16 i=0;i<stackSize;i++){
			Pos.x = Offset + i * (CubeSize * 2.0f + Spacing);
			PHYSICS_DEVICE.createBox(Pos,CubeSize);
		}
		Offset += CubeSize;
		Pos.y += (CubeSize * 2.0f + Spacing);
		stackSize--;
	}
    _addingActors = false;
}

void PhysXScene::createTower(U32 size){
    while(_addingActors){}
    _addingActors = true;
	for(U8 i = 0 ; i < size; i++){
		PHYSICS_DEVICE.createBox(vec3<F32>(0,5.0f+5*i,0),0.5f);
	}
    _addingActors = false;
}

void PhysXScene::onKeyDown(const OIS::KeyEvent& key){
	Scene::onKeyDown(key);
	switch(key.key)	{
		case OIS::KC_W:
			state()->_moveFB = 0.25f;
			break;
		case OIS::KC_A:
			state()->_moveLR = 0.25f;
			break;
		case OIS::KC_S:
			state()->_moveFB = -0.25f;
			break;
		case OIS::KC_D:
			state()->_moveLR = -0.25f;
			break;
		default:
			break;
	}
}

void PhysXScene::onKeyUp(const OIS::KeyEvent& key){
	Scene::onKeyUp(key);
	switch(key.key)	{
		case OIS::KC_W:
		case OIS::KC_S:
			state()->_moveFB = 0;
			break;
		case OIS::KC_A:
		case OIS::KC_D:
			state()->_moveLR = 0;
			break;
		case OIS::KC_F1:
			_sceneGraph->print();
			break;
        case OIS::KC_1:
			PHYSICS_DEVICE.createPlane(vec3<F32>(0,0,0),random(0.5f,2.0f));
			break;
		case OIS::KC_2:
			PHYSICS_DEVICE.createBox(vec3<F32>(0,random(10,30),0),random(0.5f,2.0f));
			break;
        case OIS::KC_3:{
            Kernel* kernel = Application::getInstance().getKernel();
            Task_ptr e(New Task(kernel->getThreadPool(),0,true,true,boost::bind(&PhysXScene::createTower, boost::ref(*this),(U32)random(5,20))));
            addTask(e);
            }break;
		case OIS::KC_4:{
			Kernel* kernel = Application::getInstance().getKernel();
			Task_ptr e(New Task(kernel->getThreadPool(),0,true,true,boost::bind(&PhysXScene::createStack, boost::ref(*this),(U32)random(5,10))));
			addTask(e);
		} break;
		default:
			break;
	}

}


void PhysXScene::onMouseMove(const OIS::MouseEvent& key){

	if(_mousePressed){
		if(_prevMouse.x - key.state.X.abs > 1 )
			state()->_angleLR = -0.15f;
		else if(_prevMouse.x - key.state.X.abs < -1 )
			state()->_angleLR = 0.15f;
		else
			state()->_angleLR = 0;

		if(_prevMouse.y - key.state.Y.abs > 1 )
			state()->_angleUD = -0.1f;
		else if(_prevMouse.y - key.state.Y.abs < -1 )
			state()->_angleUD = 0.1f;
		else
			state()->_angleUD = 0;
	}
	
	_prevMouse.x = key.state.X.abs;
	_prevMouse.y = key.state.Y.abs;
}

void PhysXScene::onMouseClickDown(const OIS::MouseEvent& key,OIS::MouseButtonID button){
	Scene::onMouseClickDown(key,button);
	if(button == 0) 
		_mousePressed = true;
}

void PhysXScene::onMouseClickUp(const OIS::MouseEvent& key,OIS::MouseButtonID button){
	Scene::onMouseClickUp(key,button);
	if(button == 0)	{
		_mousePressed = false;
		state()->_angleUD = 0;
		state()->_angleLR = 0;
	}
}