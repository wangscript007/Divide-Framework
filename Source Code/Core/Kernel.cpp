#include "Headers/Kernel.h"

#include "GUI/Headers/GUI.h"
#include "GUI/Headers/GUIConsole.h"
#include "Utility/Headers/XMLParser.h"
#include "Core/Headers/ParamHandler.h"
#include "Managers/Headers/SceneManager.h"
#include "Managers/Headers/ShaderManager.h"
#include "Managers/Headers/CameraManager.h"
#include "Rendering/PostFX/Headers/PostFX.h"
#include "Hardware/Video/RenderStateBlock.h"
#include "Dynamics/Physics/Headers/PXDevice.h"
#include "Dynamics/Physics/Headers/PXDevice.h"
#include "Managers/Headers/FrameListenerManager.h"
#include "Rendering/Camera/Headers/FreeFlyCamera.h"
#include "Hardware/Input/Headers/InputInterface.h"

D32 Kernel::_currentTime = 0.0;
bool Kernel::_keepAlive = true;

Kernel::Kernel() :	_targetFrameRate(60),
					_GFX(GFX_DEVICE),               ///Video
				    _SFX(SFXDevice::getInstance()), ///Audio
					_PFX(PHYSICS_DEVICE),           ///Physics
					_GUI(GUI::getInstance()),       ///Graphical User Interface
				    _sceneMgr(SceneManager::getInstance()), ///Scene Manager 
					_camera(New FreeFlyCamera()),			///Default camera
					_cameraMgr(New CameraManager()),        ///Camera manager
					_inputInterface(InputInterface::getInstance())  ///Input interface
{

	 assert(_cameraMgr != NULL);
	 ///If camera has been updated, set a callback to inform the current scene
	 _cameraMgr->addCameraChangeListener(boost::bind(&SceneManager::updateCamera, ///update camera
													 boost::ref(SceneManager::getInstance()),
													 _cameraMgr->getActiveCamera()));
	 _camera->setEye(vec3<F32>(0,50,0));
	 _camera->addUpdateListener(boost::bind(&LightManager::update, ///< force all lights to update
										    boost::ref(LightManager::getInstance()),
											true));
	 ///As soon as a camera is added to the camera manager, the manager is responsible for cleaning it up
	 _cameraMgr->addNewCamera("defaultCamera",_camera);
}

Kernel::~Kernel(){
	SAFE_DELETE(_cameraMgr);
	_inputInterface.terminate();
	_inputInterface.DestroyInstance();
}

void Kernel::Idle(){
	SceneManager::getInstance().clean();
	PostFX::getInstance().idle();
	GFX_DEVICE.idle();
	PHYSICS_DEVICE.idle();
	FrameListenerManager::getInstance().idle();
}

void Kernel::MainLoopStatic(){
	/// Update time at every render loop
	Kernel::_currentTime += clock()/ D32( CLOCKS_PER_SEC);
	if(!_keepAlive) {
		///exiting the graphical rendering loop will return us to the current control point
		///if we return here, the next valid control point is in main() thus shutding down the application neatly
		GFX_DEVICE.exitRenderLoop(true);
		return;
	}

	GFX_DEVICE.clearBuffers(GFXDevice::COLOR_BUFFER | GFXDevice::DEPTH_BUFFER);
	SET_DEFAULT_STATE_BLOCK();
	Framerate::getInstance().SetSpeedFactor();
	FrameEvent evt;
	FrameListenerManager::getInstance().createEvent(FRAME_EVENT_STARTED,evt);
	_keepAlive = FrameListenerManager::getInstance().frameStarted(evt);

	PHYSICS_DEVICE.process();

	_keepAlive = Application::getInstance().getKernel()->MainLoop();

	FrameListenerManager::getInstance().createEvent(FRAME_EVENT_PROCESS,evt);
	_keepAlive = FrameListenerManager::getInstance().frameRenderingQueued(evt);

	GFX_DEVICE.swapBuffers();

	FrameListenerManager::getInstance().createEvent(FRAME_EVENT_ENDED,evt);
	_keepAlive = FrameListenerManager::getInstance().frameEnded(evt);
}

bool Kernel::MainLoop(){
	/// Cache resolution in case it changes in previous loop
	refreshAppData();

	_camera->RenderLookAt();	
	_sceneMgr.updateCamera(_camera);

	_sceneMgr.updateSceneState(_currentTime);

	_PFX.update();

	_sceneMgr.preRender();
	
	/// Inform listeners that we finished pre-rendering
	FrameEvent evt;
	FrameListenerManager::getInstance().createEvent(FRAME_PRERENDER_END,evt);

	if(!FrameListenerManager::getInstance().framePreRenderEnded(evt)){
		return false;
	}

	LightManager::getInstance().generateShadowMaps(_camera->getEye());

	PostFX::getInstance().render(_camera);

	ShaderManager::getInstance().unbind();//unbind all shaders

	///Preview depthmaps if needed
	LightManager::getInstance().previewDepthMaps();

	GUI::getInstance().draw();
	_inputInterface.tick();
	_sceneMgr.processInput();
	_sceneMgr.processEvents(abs(GETTIME()));
	return true;
}

I8 Kernel::Initialize(const std::string& entryPoint) {
	I8 windowId = -1;
	ParamHandler& par = ParamHandler::getInstance();
	///Using OpenGL for rendering as default
	_GFX.setApi(OpenGL);
	///Target FPS is usually 60. So all movement is capped around that value
	Framerate::getInstance().Init(_targetFrameRate);
	///Load info from XML files
	XML::loadScripts(entryPoint);
	///Create mem log file
	std::string mem = par.getParam<std::string>("memFile");
	if(mem.compare("none") == 0) mem = "mem.log";
	Application::getInstance().setMemoryLogFile(mem);
	PRINT_FN("Initializing the rendering interface");
	vec2<U16> resolution(par.getParam<I32>("resolutionWidth"),par.getParam<I32>("resolutionHeight"));
	windowId = _GFX.initHardware(resolution);
	_GFX.registerKernel(this);
	_GFX.enableFog(0.01f,vec4<F32>(0.2f, 0.2f, 0.4f, 1.0f));
	PRINT_FN("Initializing the sound interface");
	_SFX.initHardware();
	PRINT_FN("Initializing the physics interface");
	_PFX.initPhysics();
	PRINT_FN("Initializing the post-processing interface");
	PostFX::getInstance().init(resolution);
	///Bind the kernel with the input interface
	_inputInterface.initialize(this,par.getParam<std::string>("appTitle"));
	PRINT_FN("Loading default material from XML");
	///Load default material
	XML::loadMaterialXML(par.getParam<std::string>("scriptLocation")+"/defaultMaterial");
	PRINT_FN("Loading scene data ...");
	_sceneMgr.updateCamera(_camera);
	_sceneMgr.load(std::string(""));
	PRINT_FN("Initial data loaded ...\nCreating AI entities ...");
	_sceneMgr.initializeAI(true);
	PRINT_FN("AI Entities created ...\n Adding default camera ...");
	PRINT_FN("Creating console gui element ...");
	_GUI.createConsole();

	PRINT_FN("Entering main rendering loop ...");


	return windowId;
}

void Kernel::beginLogicLoop(){
	//Target FPS is 60. So all movement is capped around that value
	GFX_DEVICE.initDevice(_targetFrameRate);
}

void Kernel::Shutdown(){
	_keepAlive = false;
	PRINT_FN("Shutting down graphical user interface ...");
	_GUI.DestroyInstance(); ///Deactivate GUI
	PRINT_FN("Closing physics interface!");
	_PFX.exitPhysics();
	PRINT_FN("Closing post-processing interface!");
	PostFX::getInstance().DestroyInstance();
	PRINT_FN("Unloading scenes and shutting down the scene manager!");
	_sceneMgr.deinitializeAI(true);
	_sceneMgr.DestroyInstance();
	PRINT_FN("Emptying the resource cache ...");
	ResourceCache::getInstance().DestroyInstance();
	PRINT_FN("Engine resources unloaded successfully!");

	PRINT_FN("Closing hardware interface(GFX,SFX,PhysX, input,network) engine ...");
	_SFX.closeAudioApi();
	_SFX.DestroyInstance();
	_GFX.closeRenderingApi();
	_GFX.DestroyInstance();
}

void Kernel::refreshAppData(){
	_sceneMgr.cacheResolution(Application::getInstance().getResolution());
}

///--------------------------Input Management-------------------------------------///
///The console is a global GUI element, scene agnostic, so it's handled in the Kernel


bool Kernel::onKeyDown(const OIS::KeyEvent& key) {
	if(GUIConsole::getInstance().isConsoleOpen()){
		GUIConsole::getInstance().onKeyDown(key);
	}else{
		GET_ACTIVE_SCENE()->onKeyDown(key);
	}
	return true;
}

bool Kernel::onKeyUp(const OIS::KeyEvent& key) {
	if(GUIConsole::getInstance().isConsoleOpen()){
		GUIConsole::getInstance().onKeyUp(key);
	}else{
		GET_ACTIVE_SCENE()->onKeyUp(key);
	}
	return true;
}

bool Kernel::onMouseMove(const OIS::MouseEvent& arg) {
	if(GUIConsole::getInstance().isConsoleOpen()){
		GUIConsole::getInstance().onMouseMove(arg);
	}else{
		GET_ACTIVE_SCENE()->onMouseMove(arg);
	}
	return true;
}

bool Kernel::onMouseClickDown(const OIS::MouseEvent& arg,OIS::MouseButtonID button) {
	if(GUIConsole::getInstance().isConsoleOpen()){
		GUIConsole::getInstance().onMouseClickDown(arg,button);
	}else{
		GET_ACTIVE_SCENE()->onMouseClickDown(arg,button);
	}
	return true;
}

bool Kernel::onMouseClickUp(const OIS::MouseEvent& arg,OIS::MouseButtonID button) {
	if(GUIConsole::getInstance().isConsoleOpen()){
		GUIConsole::getInstance().onMouseClickUp(arg,button);
	}else{
		GET_ACTIVE_SCENE()->onMouseClickUp(arg,button);
	}
	return true;
}

bool Kernel::OnJoystickMoveAxis(const OIS::JoyStickEvent& arg,I8 axis) {
	GET_ACTIVE_SCENE()->OnJoystickMoveAxis(arg,axis);
	return true;
}

bool Kernel::OnJoystickMovePOV(const OIS::JoyStickEvent& arg,I8 pov){
	GET_ACTIVE_SCENE()->OnJoystickMovePOV(arg,pov);
	return true;
}

bool Kernel::OnJoystickButtonDown(const OIS::JoyStickEvent& arg,I8 button){
	GET_ACTIVE_SCENE()->OnJoystickButtonDown(arg,button);
	return true;
}

bool Kernel::OnJoystickButtonUp(const OIS::JoyStickEvent& arg, I8 button){
	GET_ACTIVE_SCENE()->OnJoystickButtonUp(arg,button);
	return true;
}