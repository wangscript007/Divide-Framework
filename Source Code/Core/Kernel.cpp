#include "Headers/Kernel.h"

#include "GUI/Headers/GUI.h"
#include "GUI/Headers/GUISplash.h"
#include "GUI/Headers/GUIConsole.h"
#include "Utility/Headers/XMLParser.h"
#include "Core/Headers/ParamHandler.h"
#include "Managers/Headers/AIManager.h"
#include "Managers/Headers/LightManager.h"
#include "Managers/Headers/SceneManager.h"
#include "Managers/Headers/ShaderManager.h"
#include "Managers/Headers/CameraManager.h"
#include "Rendering/PostFX/Headers/PostFX.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Dynamics/Physics/Headers/PXDevice.h"
#include "Hardware/Input/Headers/InputInterface.h"
#include "Managers/Headers/FrameListenerManager.h"
#include "Hardware/Video/Headers/RenderStateBlock.h"

#include "Rendering/Headers/ForwardRenderer.h"
#include "Rendering/Headers/DeferredShadingRenderer.h"
#include "Rendering/Headers/DeferredLightingRenderer.h"

D32 Kernel::_lastFrameTime = 0.0;
D32 Kernel::_currentTime = 0.0;
D32 Kernel::_currentTimeDelta = 0.0;
bool Kernel::_keepAlive = true;
bool Kernel::_applicationReady = false;
bool Kernel::_renderingPaused = false;

boost::function0<void> Kernel::_mainLoopCallback;
ProfileTimer* s_appLoopTimer = NULL;

Kernel::Kernel(I32 argc, char **argv, Application& parentApp) :
                    _argc(argc),
                    _argv(argv),
                    _APP(parentApp),
                    _GFX(GFXDevice::getOrCreateInstance()), //Video
                    _SFX(SFXDevice::getOrCreateInstance()), //Audio
                    _PFX(PXDevice::getOrCreateInstance()),  //Physics
                    _GUI(GUI::getOrCreateInstance()),       //Graphical User Interface
                    _sceneMgr(SceneManager::getOrCreateInstance()),  //Scene Manager
                    _shaderMgr(ShaderManager::getOrCreateInstance()),//Shader Manager
                    _cameraMgr(New CameraManager()),         //Camera manager
                    _frameMgr(FrameListenerManager::getOrCreateInstance()),
                    _lightPool(LightManager::getOrCreateInstance()),//Light pool
                    _inputInterface(InputInterface::getOrCreateInstance()),//Input interface
                    _activeScene(NULL)
{
    ResourceCache::createInstance();
    PostFX::createInstance();
     assert(_cameraMgr != NULL);
     //If camera has been updated, set a callback to inform the current scene
     _cameraMgr->addCameraChangeListener(DELEGATE_BIND(&SceneManager::updateCameras, //update camera
                                                       DELEGATE_REF(_sceneMgr)));
     //We have an A.I. thread, a networking thread, a PhysX thread, the main update/rendering thread
     //so how many threads do we allocate for tasks? That's up to the programmer to decide for each app
     //we add the A.I. thread in the same pool as it's a task. ReCast should also use this ...
     _mainTaskPool = New boost::threadpool::pool(Config::THREAD_LIMIT + 1 /*A.I.*/);
     s_appLoopTimer = ADD_TIMER("MainLoopTimer");
}

Kernel::~Kernel(){
    SAFE_DELETE(_cameraMgr);
    _mainTaskPool->wait();
    SAFE_DELETE(_mainTaskPool);
    REMOVE_TIMER(s_appLoopTimer);
}

void Kernel::idle(){
    GFX_DEVICE.idle();
    PHYSICS_DEVICE.idle();
    PostFX::getInstance().idle();
    SceneManager::getInstance().idle();
    LightManager::getInstance().idle();
    ShaderManager::getInstance().idle();
    FrameListenerManager::getInstance().idle();

    std::string pendingLanguage = ParamHandler::getInstance().getParam<std::string>("language");
    if(pendingLanguage.compare(Locale::currentLanguage()) != 0){
        Locale::changeLanguage(pendingLanguage);
    }
}


void Kernel::mainLoopApp(){
    START_TIMER(s_appLoopTimer);
    // Update time at every render loop
    Kernel::_lastFrameTime    = Kernel::_currentTime;
    Kernel::_currentTime      = GETMSTIME();
    Kernel::_currentTimeDelta = Kernel::_currentTime - Kernel::_lastFrameTime;

    FrameListenerManager& frameMgr = FrameListenerManager::getInstance();
    Application& APP = Application::getInstance();

    if(!_keepAlive || APP.ShutdownRequested()) {
        //exiting the graphical rendering loop will return us to the current control point
        //if we return here, the next valid control point is in main() thus shutding down the application neatly
        GFX_DEVICE.exitRenderLoop(true);
        return;
    }

    Kernel::idle();
    //Restore GPU to default state: clear buffers and set default render state
    GFX_DEVICE.beginFrame();
    //Get current frame-to-application speed factor
    Framerate::getInstance().update();
    //Launch the FRAME_STARTED event
    FrameEvent evt;
    frameMgr.createEvent(FRAME_EVENT_STARTED,evt);
    _keepAlive = frameMgr.frameStarted(evt);
    //Process physics
    PHYSICS_DEVICE.process(_currentTimeDelta);
    //Process the current frame
    _keepAlive = APP.getKernel()->mainLoopScene(evt);
    //Launch the FRAME_PROCESS event (a.k.a. the frame processing has ended event)
    frameMgr.createEvent(FRAME_EVENT_PROCESS,evt);
    _keepAlive = frameMgr.frameRenderingQueued(evt);

    GFX_DEVICE.endFrame();
    //Launch the FRAME_ENDED event (buffers have been swapped)
    frameMgr.createEvent(FRAME_EVENT_ENDED,evt);
    _keepAlive = frameMgr.frameEnded(evt);

    STOP_TIMER(s_appLoopTimer);
}

void Kernel::firstLoop(){
    static bool first = true;
    
    ParamHandler& par = ParamHandler::getInstance();
    static bool shadowMappingEnabled = par.getParam<bool>("rendering.enableShadows");
    //Skip one frame so all resources can be built while the splash screen is displayed
    if(first){
        par.setParam("rendering.enableShadows",false);
        mainLoopApp();
        Kernel::_applicationReady = true;
        //Hide splash screen
        GFX_DEVICE.changeResolution(par.getParam<U16>("runtime.resolutionWidth"),
                                    par.getParam<U16>("runtime.resolutionHeight"));
        GFX_DEVICE.setWindowPos(10,50);
        first = false;
    }else{// skip another frame so all buffers and shaders are refreshed
        par.setParam("rendering.enableShadows", shadowMappingEnabled);
        mainLoopApp();
        _mainLoopCallback = DELEGATE_REF(Kernel::mainLoopApp);
#if defined(_DEBUG) || defined(_PROFILE)
        Framerate::getInstance().benchmark(true);
#endif
    }
    _currentTime =  GETMSTIME();
}

static const I32 SKIP_TICKS = 1000 / Config::TICKS_PER_SECOND;
bool Kernel::mainLoopScene(FrameEvent& evt){
    if(_renderingPaused) {
        idle();
        return true;
    }
    _activeScene = GET_ACTIVE_SCENE();
    _cameraMgr->update(_currentTimeDelta);

    _loops = 0;
    //while(_currentTime > _nextGameTick && _loops < Config::MAX_FRAMESKIP) 
    {
        // Update scene based on input
        _sceneMgr.processInput(_currentTimeDelta);
 
        // process all scene events
        _sceneMgr.processTasks(_currentTimeDelta);

        //Update the scene state based on current time
        _sceneMgr.updateSceneState(_currentTimeDelta);

        //Update physics
        _PFX.update(_currentTimeDelta);

        _nextGameTick += SKIP_TICKS;
        _loops++;
    }

    bool sceneRenderState = presentToScreen(evt);
    // Get input events
    _inputInterface.update(_currentTimeDelta);
    // Draw the GUI
    _GUI.draw(_currentTimeDelta);
    return sceneRenderState;
}

bool Kernel::presentToScreen(FrameEvent& evt){
    _frameMgr.createEvent(FRAME_PRERENDER_START,evt);
    if(!_frameMgr.framePreRenderStarted(evt)) return false;

    //Prepare scene for rendering
    _sceneMgr.preRender();

    //perform time-sensitive shader tasks
    _shaderMgr.update(_currentTimeDelta);

    _frameMgr.createEvent(FRAME_PRERENDER_END,evt);
    if(!_frameMgr.framePreRenderEnded(evt)) return false;

    // Render the scene adding any post-processing effects that we have active
    PostFX::getInstance().render();

    _sceneMgr.postRender();

    //render debug primitives and cleanup after us
    _GFX.flush();
    return true;
}

I8 Kernel::initialize(const std::string& entryPoint) {
    I8 windowId = -1;
    I8 returnCode = 0;
    ParamHandler& par = ParamHandler::getInstance();

    Console::getInstance().bindConsoleOutput(DELEGATE_BIND(&GUIConsole::printText,
                                                            GUI::getInstance().getConsole(),
                                                            _1,_2));
    //Using OpenGL for rendering as default
    _GFX.setApi(OpenGL);

    //Target FPS is usually 60. So all movement is capped around that value
    Framerate::getInstance().init(Config::TARGET_FRAME_RATE);

    //Load info from XML files
    std::string startupScene = XML::loadScripts(entryPoint);

    //Create mem log file
    std::string mem = par.getParam<std::string>("memFile");
    _APP.setMemoryLogFile(mem.compare("none") == 0 ? "mem.log" : mem);

    PRINT_FN(Locale::get("START_RENDER_INTERFACE"));
    vec2<U16> resolution(par.getParam<U16>("runtime.resolutionWidth"),
                         par.getParam<U16>("runtime.resolutionHeight"));
    windowId = _GFX.initHardware(resolution/2,_argc,_argv);

    //If we could not initialize the graphics device, exit
    if(windowId < 0)
        return windowId;

    //Load and render the splash screen
    _GFX.setRenderStage(FINAL_STAGE);
    _GFX.beginFrame();
    GUISplash("divideLogo.jpg",vec2<U16>(400,300)).render();
    _GFX.endFrame();

    //We start of with a forward renderer. Change it to something else on scene load ... or ... whenever
    _GFX.setRenderer(New ForwardRenderer());
    _GFX.registerKernel(this);

    _lightPool.init();

    PRINT_FN(Locale::get("START_SOUND_INTERFACE"));
    if((returnCode = _SFX.initHardware()) < 0)
        return returnCode;

    PRINT_FN(Locale::get("START_PHYSICS_INTERFACE"));
    if((returnCode =_PFX.initPhysics(Config::TARGET_FRAME_RATE)) < 0)
        return returnCode;

    PostFX::getInstance().init(resolution);

    size_t windowHandle = (size_t)_GFX.getHWND();
    std::string windowTitle = par.getParam<std::string>("appTitle");
    //Bind the kernel with the input interface
    _inputInterface.initialize(this,windowTitle,windowHandle);

    //Load default material
    PRINT_FN(Locale::get("LOAD_DEFAULT_MATERIAL"));
    XML::loadMaterialXML(par.getParam<std::string>("scriptLocation")+"/defaultMaterial");

    //Initialize GUI with our current resolution
    GUI::getInstance().init();

    _sceneMgr.init();

    if(!_sceneMgr.load(startupScene, resolution, _cameraMgr)){       //< Load the scene
        ERROR_FN(Locale::get("ERROR_SCENE_LOAD"),startupScene.c_str());
        return MISSING_SCENE_DATA;
    }

    if(!_sceneMgr.checkLoadFlag()){
        ERROR_FN(Locale::get("ERROR_SCENE_LOAD_NOT_CALLED"),startupScene.c_str());
        return MISSING_SCENE_LOAD_CALL;
    }

    PRINT_FN(Locale::get("INITIAL_DATA_LOADED"));

    PRINT_FN(Locale::get("CREATE_AI_ENTITIES_START"));
    //Initialize and start the AI
    _sceneMgr.initializeAI(true);
    PRINT_FN(Locale::get("CREATE_AI_ENTITIES_END"));
    _activeScene = GET_ACTIVE_SCENE();
    _GUI.cacheResolution(resolution);
    _nextGameTick = GETMSTIME();
    return windowId;
}

void Kernel::beginLogicLoop(){
    PRINT_FN(Locale::get("START_RENDER_LOOP"));
    //lock the scene
    GET_ACTIVE_SCENE()->state().toggleRunningState(true);
    //The first loop compiles all the textures, so, do not render the first frame
    _mainLoopCallback = DELEGATE_REF(Kernel::firstLoop);
    //Target FPS is define in "config.h". So all movement is capped around that value
    //start rendering
    GFX_DEVICE.initDevice(Config::TARGET_FRAME_RATE);
}

void Kernel::shutdown(){
    _keepAlive = false;
    //release the scene
    GET_ACTIVE_SCENE()->state().toggleRunningState(false);
    PRINT_FN(Locale::get("STOP_GUI"));
    Console::getInstance().bindConsoleOutput(boost::function2<void, const char*, bool>());
    _GUI.destroyInstance(); ///Deactivate GUI
    PRINT_FN(Locale::get("STOP_POST_FX"));
    PostFX::getInstance().destroyInstance();
    PRINT_FN(Locale::get("STOP_SCENE_MANAGER"));
    AIManager::getInstance().pauseUpdate(true);
    _sceneMgr.unloadCurrentScene();
    _sceneMgr.deinitializeAI(true);
    AIManager::getInstance().Destroy();
    AIManager::getInstance().destroyInstance();
    _sceneMgr.destroyInstance();
    _shaderMgr.Destroy();
    _lightPool.destroyInstance();
    _GFX.closeRenderer();
    PRINT_FN(Locale::get("STOP_RESOURCE_CACHE"));
    ResourceCache::getInstance().destroyInstance();
    PRINT_FN(Locale::get("STOP_ENGINE_OK"));
    _frameMgr.destroyInstance();
    _shaderMgr.destroyInstance();
    PRINT_FN(Locale::get("STOP_PHYSICS_INTERFACE"));
    _PFX.exitPhysics();
    PRINT_FN(Locale::get("STOP_HARDWARE"));
    _inputInterface.terminate();
    _inputInterface.destroyInstance();
    _SFX.closeAudioApi();
    _SFX.destroyInstance();
    _GFX.closeRenderingApi();
    _GFX.destroyInstance();
    Locale::clear();
}

void Kernel::updateResolutionCallback(I32 w, I32 h){
    if(!_applicationReady) return;
    ShaderManager::getInstance().refresh();
    Application& app = Application::getInstance();
    //minimized
    _renderingPaused = (w == 0 || h == 0);
    app.setResolutionWidth(w);
    app.setResolutionHeight(h);
    //Update post-processing render targets and buffers
    PostFX::getInstance().reshapeFBO(w, h);
    vec2<U16> newResolution(w,h);
    //Update the graphical user interface
    GUI::getInstance().onResize(newResolution);
    // Cache resolution for faster access
    SceneManager::getInstance().cacheResolution(newResolution);
}

///--------------------------Input Management-------------------------------------///

bool Kernel::onKeyDown(const OIS::KeyEvent& key) {
    if(GUI::getInstance().keyCheck(key,true)){
        _activeScene->onKeyDown(key);
    }
    return true;
}

bool Kernel::onKeyUp(const OIS::KeyEvent& key) {
    if(GUI::getInstance().keyCheck(key,false)){
        _activeScene->onKeyUp(key);
    }
    return true;
}

bool Kernel::onMouseMove(const OIS::MouseEvent& arg) {
    if(GUI::getInstance().checkItem(arg)){
        _activeScene->onMouseMove(arg);
    }
    return true;
}

bool Kernel::onMouseClickDown(const OIS::MouseEvent& arg,OIS::MouseButtonID button) {
    if(GUI::getInstance().clickCheck(button,true)){
        _activeScene->onMouseClickDown(arg,button);
    }
    return true;
}

bool Kernel::onMouseClickUp(const OIS::MouseEvent& arg,OIS::MouseButtonID button) {
    if(GUI::getInstance().clickCheck(button,false)){
        _activeScene->onMouseClickUp(arg,button);
    }
    return true;
}

bool Kernel::onJoystickMoveAxis(const OIS::JoyStickEvent& arg,I8 axis,I32 deadZone) {
    _activeScene->onJoystickMoveAxis(arg,axis,deadZone);
    return true;
}

bool Kernel::onJoystickMovePOV(const OIS::JoyStickEvent& arg,I8 pov){
    _activeScene->onJoystickMovePOV(arg,pov);
    return true;
}

bool Kernel::onJoystickButtonDown(const OIS::JoyStickEvent& arg,I8 button){
    _activeScene->onJoystickButtonDown(arg,button);
    return true;
}

bool Kernel::onJoystickButtonUp(const OIS::JoyStickEvent& arg, I8 button){
    _activeScene->onJoystickButtonUp(arg,button);
    return true;
}

bool Kernel::sliderMoved( const OIS::JoyStickEvent &arg, I8 index){
    _activeScene->sliderMoved(arg,index);
    return true;
}

bool Kernel::vector3Moved( const OIS::JoyStickEvent &arg, I8 index){
    _activeScene->vector3Moved(arg,index);
    return true;
}