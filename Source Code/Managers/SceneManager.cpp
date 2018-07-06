#include "Headers/SceneManager.h"
#include "Headers/AIManager.h"

#include "SceneList.h"
#include "Core/Headers/ParamHandler.h"
#include "Geometry/Importer/Headers/DVDConverter.h"
#include "Rendering/RenderPass/Headers/RenderPassCuller.h"

SceneManager::SceneManager() : FrameListener(),
                               _GUI(nullptr),
                               _activeScene(nullptr),
                               _renderPassCuller(nullptr),
                               _renderPassManager(nullptr),
                               _defaultMaterial(nullptr),
                               _init(false),
                               _previewDepthBuffer(false),
                               _frameCount(0)
{
    DVDConverter::createInstance();
    AIManager::createInstance();
}

SceneManager::~SceneManager(){
    //PRINT_FN(Locale::get("SCENE_MANAGER_DELETE"));
    PRINT_FN(Locale::get("SCENE_MANAGER_REMOVE_SCENES"));
    FOR_EACH(SceneMap::value_type& it, _sceneMap){
        SAFE_DELETE(it.second);
    }
    SAFE_DELETE(_renderPassCuller);
    _sceneMap.clear();
    //Destroy the model loader;
    DVDConverter::getInstance().destroyInstance();
}

bool SceneManager::init(GUI* const gui){
    //Load default material
    PRINT_FN(Locale::get("LOAD_DEFAULT_MATERIAL"));
    _defaultMaterial = XML::loadMaterialXML(ParamHandler::getInstance().getParam<std::string>("scriptLocation")+"/defaultMaterial");

    REGISTER_FRAME_LISTENER(&(this->getInstance()));
    _GUI = gui;
    _renderPassCuller = New RenderPassCuller();
    _renderPassManager = &RenderPassManager::getOrCreateInstance();
    _init = true;
    return true;
}

bool SceneManager::load(const std::string& sceneName, const vec2<U16>& resolution, CameraManager* const cameraMgr){
    assert(_init == true && _GUI != nullptr);
    PRINT_FN(Locale::get("SCENE_MANAGER_LOAD_SCENE_DATA"));
    //Initialize the model importer:
    if(!DVDConverter::getInstance().init()){
        return false;
    }
    XML::loadScene(sceneName, *this);
    if(!_activeScene){
        return false;
    }
    cacheResolution(resolution);
    return _activeScene->load(sceneName, cameraMgr, _GUI);
}

Scene* SceneManager::createScene(const std::string& name){
    Scene* scene = nullptr;

    if(!name.empty())
        scene = _sceneFactory[name]();

    if(scene != nullptr)
        _sceneMap.insert(std::make_pair(name, scene));

    return scene;
}

bool SceneManager::unloadCurrentScene()  {  
    AIManager::getInstance().pauseUpdate(true); 
    RemoveResource(_defaultMaterial);
    return _activeScene->unload();
}

bool SceneManager::deinitializeAI(bool continueOnErrors)  { 
    bool state = _activeScene->deinitializeAI(continueOnErrors);
    AIManager::getInstance().destroyInstance(); 
    return state;
}

bool SceneManager::framePreRenderStarted(const FrameEvent& evt){
    return true;
}

bool SceneManager::frameEnded(const FrameEvent& evt){
    _renderPassCuller->refresh();
    return true;
}

void SceneManager::update(const U64 deltaTime){
    _activeScene->getSceneGraph()->update();
}

void SceneManager::preRender() {
    _activeScene->preRender();
}

void SceneManager::updateVisibleNodes() {
    SceneGraph* sceneGraph = _activeScene->getSceneGraph();
    //sceneGraph->update();
    _renderPassCuller->cullSceneGraph(sceneGraph->getRoot(), _activeScene->state());
}

void SceneManager::renderVisibleNodes() {
    updateVisibleNodes();
    _renderPassManager->render(_activeScene->renderState(), _activeScene->getSceneGraph());
}

void SceneManager::render(const RenderStage& stage, const Kernel& kernel) {
    assert(_activeScene != nullptr);

    static DELEGATE_CBK renderFunction;
    if(renderFunction.empty()){
        if(_activeScene->renderCallback().empty()){
            renderFunction = DELEGATE_BIND(&SceneManager::renderVisibleNodes, DELEGATE_REF(SceneManager::getInstance()));
        }else{
            renderFunction = _activeScene->renderCallback();
        }
    }

    kernel.submitRenderCall(stage, _activeScene->renderState(), renderFunction);
}

void SceneManager::postRender(){
    _activeScene->postRender();
    // Preview depthmaps if needed
    LightManager::getInstance().previewShadowMaps();
    if(_previewDepthBuffer)
        GFX_DEVICE.previewDepthBuffer();

    _frameCount++;
}

///--------------------------Input Management-------------------------------------///

bool SceneManager::onKeyDown(const OIS::KeyEvent& key) {
    if(_GUI->keyCheck(key,true)){
        return _activeScene->onKeyDown(key);
    }
    return true;
}

bool SceneManager::onKeyUp(const OIS::KeyEvent& key) {
    if(_GUI->keyCheck(key,false)){
        return _activeScene->onKeyUp(key);
    }
    return true;
}

bool SceneManager::onMouseMove(const OIS::MouseEvent& arg) {
    if(_GUI->checkItem(arg)){
        return _activeScene->onMouseMove(arg);
    }
    return true;
}

bool SceneManager::onMouseClickDown(const OIS::MouseEvent& arg,OIS::MouseButtonID button) {
    if(_GUI->clickCheck(button,true)){
        return _activeScene->onMouseClickDown(arg,button);
    }
    return true;
}

bool SceneManager::onMouseClickUp(const OIS::MouseEvent& arg,OIS::MouseButtonID button) {
    if(_GUI->clickCheck(button,false)){
        return _activeScene->onMouseClickUp(arg,button);
    }
    return true;
}

bool SceneManager::onJoystickMoveAxis(const OIS::JoyStickEvent& arg,I8 axis,I32 deadZone) {
    return _activeScene->onJoystickMoveAxis(arg,axis,deadZone);
}

bool SceneManager::onJoystickMovePOV(const OIS::JoyStickEvent& arg,I8 pov){
    return _activeScene->onJoystickMovePOV(arg,pov);
}

bool SceneManager::onJoystickButtonDown(const OIS::JoyStickEvent& arg,I8 button){
    return _activeScene->onJoystickButtonDown(arg,button);
}

bool SceneManager::onJoystickButtonUp(const OIS::JoyStickEvent& arg, I8 button){
    return _activeScene->onJoystickButtonUp(arg,button);
}

bool SceneManager::sliderMoved( const OIS::JoyStickEvent &arg, I8 index){
    return _activeScene->sliderMoved(arg,index);
}

bool SceneManager::vector3Moved( const OIS::JoyStickEvent &arg, I8 index){
    return _activeScene->vector3Moved(arg,index);
}