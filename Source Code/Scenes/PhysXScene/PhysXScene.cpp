#include "Headers/PhysXScene.h"

#include "Managers/Headers/SceneManager.h"
#include "Rendering/RenderPass/Headers/RenderQueue.h"
#include "Core/Headers/ParamHandler.h"
REGISTER_SCENE(PhysXScene);

enum PhysXStateEnum{
    STATE_ADDING_ACTORS = 0,
    STATE_IDLE = 2,
    STATE_LOADING = 3
};

static boost::atomic<PhysXStateEnum > s_sceneState;

//begin copy-paste
void PhysXScene::preRender(){
    getSkySGN(0)->getNode<Sky>()->setSunVector(_sunvector);
}
//<<end copy-paste

void PhysXScene::processGUI(const U64 deltaTime){
    D32 FpsDisplay = getSecToMs(0.3);
    if (_guiTimers[0] >= FpsDisplay){
        _GUI->modifyText("fpsDisplay", "FPS: %3.0f. FrameTime: %3.1f", ApplicationTimer::getInstance().getFps(), ApplicationTimer::getInstance().getFrameTime());
        _GUI->modifyText("RenderBinCount", "Number of items in Render Bin: %d", GFX_RENDER_BIN_SIZE);
        _guiTimers[0] = 0.0;
    }
    Scene::processGUI(deltaTime);
}

void PhysXScene::processInput(const U64 deltaTime){

}

bool PhysXScene::load(const std::string& name, CameraManager* const cameraMgr, GUI* const gui){
    s_sceneState = STATE_LOADING;
    //Load scene resources
    bool loadState = SCENE_LOAD(name,cameraMgr,gui,true,true);
    //Add a light
    vec2<F32> sunAngle(0.0f, RADIANS(45.0f));
    _sunvector = vec3<F32>(-cosf(sunAngle.x) * sinf(sunAngle.y),-cosf(sunAngle.y),-sinf(sunAngle.x) * sinf(sunAngle.y));
    Light* light = addDefaultLight();
    light->setDirection(_sunvector);
    addDefaultSky();
    s_sceneState = STATE_IDLE;
    return loadState;
}

bool PhysXScene::loadResources(bool continueOnErrors){
    _GUI->addText("fpsDisplay",               //Unique ID
                  vec2<I32>(60,20),           //Position
                  Font::DIVIDE_DEFAULT,       //Font
                  vec3<F32>(0.0f,0.2f, 1.0f), //Color
                  "FPS: %s",0);               //Text and arguments
    _GUI->addText("RenderBinCount",
                  vec2<I32>(60,30),
                  Font::DIVIDE_DEFAULT,
                  vec3<F32>(0.6f,0.2f,0.2f),
                  "Number of items in Render Bin: %d",0);

    _guiTimers.push_back(0.0); //Fps
    renderState().getCamera().setFixedYawAxis(false);
    renderState().getCamera().setRotation(-45/*yaw*/,10/*pitch*/);
    renderState().getCamera().setEye(vec3<F32>(0,30,-40));
    renderState().getCamera().setFixedYawAxis(true);
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

    while(s_sceneState == STATE_ADDING_ACTORS);

    s_sceneState = STATE_ADDING_ACTORS;

    while(stackSize){
        for(U16 i=0;i<stackSize;i++){
            Pos.x = Offset + i * (CubeSize * 2.0f + Spacing);
            PHYSICS_DEVICE.createBox(Pos,CubeSize);
        }
        Offset += CubeSize;
        Pos.y += (CubeSize * 2.0f + Spacing);
        stackSize--;
    }

    s_sceneState = STATE_IDLE;
}

void PhysXScene::createTower(U32 size){
    while(s_sceneState == STATE_ADDING_ACTORS);
    s_sceneState = STATE_ADDING_ACTORS;

    for(U8 i = 0 ; i < size; i++)
        PHYSICS_DEVICE.createBox(vec3<F32>(0,5.0f+5*i,0),0.5f);
    
    s_sceneState = STATE_IDLE;
}


bool PhysXScene::onKeyUp(const OIS::KeyEvent& key){
    switch(key.key)	{
        default: break;
        case OIS::KC_5:{
            _paramHandler.setParam("simSpeed", IS_ZERO(_paramHandler.getParam<F32>("simSpeed")) ? 1.0f : 0.0f);
            PHYSICS_DEVICE.updateTimeStep();
            }break;
        case OIS::KC_1:{
            static bool hasGroundPlane = false;
            if(!hasGroundPlane){
                PHYSICS_DEVICE.createPlane(vec3<F32>(0,0,0),random(100.0f, 200.0f));
                hasGroundPlane = true;
            }
            }break;
        case OIS::KC_2:
            PHYSICS_DEVICE.createBox(vec3<F32>(0,random(10,30),0),random(0.5f,2.0f));
            break;
        case OIS::KC_3:{
            Kernel* kernel = Application::getInstance().getKernel();
            Task_ptr e(kernel->AddTask(0, true, true, DELEGATE_BIND(&PhysXScene::createTower, this, (U32)random(5, 20))));
            addTask(e);
            }break;
        case OIS::KC_4:{
            Kernel* kernel = Application::getInstance().getKernel();
            Task_ptr e(kernel->AddTask(0, true, true, DELEGATE_BIND(&PhysXScene::createStack, this, (U32)random(5, 10))));
            addTask(e);
        } break;
    }
    return Scene::onKeyUp(key);
}

bool PhysXScene::mouseMoved(const OIS::MouseEvent& key){
    if(_mousePressed[OIS::MB_Right]){
        if(_previousMousePos.x - key.state.X.abs > 1 )		 state()._angleLR = -1;
        else if(_previousMousePos.x - key.state.X.abs < -1 ) state()._angleLR =  1;
        else			                                     state()._angleLR =  0;

        if(_previousMousePos.y - key.state.Y.abs > 1 )		 state()._angleUD = -1;
        else if(_previousMousePos.y - key.state.Y.abs < -1 ) state()._angleUD =  1;
        else 			                                     state()._angleUD =  0;
    }

    return Scene::mouseMoved(key);
}

bool PhysXScene::mouseButtonReleased(const OIS::MouseEvent& key,OIS::MouseButtonID button){
    bool keyState = Scene::mouseButtonReleased(key,button);
    if(!_mousePressed[OIS::MB_Right]){
        state()._angleUD = 0;
        state()._angleLR = 0;
    }
    return keyState;
}