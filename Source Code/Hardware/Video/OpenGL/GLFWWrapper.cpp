﻿#include "Headers/glRenderStateBlock.h"
#include "Headers/glImmediateModeEmulation.h"

#include "GUI/Headers/GUI.h"
#include "Core/Headers/Kernel.h"
#include "Core/Headers/ParamHandler.h"
#include "Rendering/Headers/Frustum.h"
#include "Managers/Headers/ShaderManager.h"
#include "Rendering/Lighting/Headers/Light.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Hardware/Video/OpenGL/glsw/Headers/glsw.h"
#include "Shaders/Headers/glUniformBufferObject.h"
#include <glsl/glsl_optimizer.h>
#include <gtc/type_ptr.hpp>

#define FTGL_LIBRARY_STATIC
#include <FTGL/ftgl.h>
#include <CEGUI/CEGUI.h>

#ifdef GLEW_MX
#	ifdef _DEBUG
#		pragma comment(lib, "CEGUIOpenGLRenderer_MX_d.lib")
#		pragma comment(lib, "ftgl_gl3_MX_d.lib")
#		pragma comment(lib, "glew32mxs_d.lib")
#	else //_DEBUG
#		pragma comment(lib, "CEGUIOpenGLRenderer_MX.lib")
#		pragma comment(lib, "ftgl_gl3_MX.lib")
#		pragma comment(lib, "glew32mxs.lib")
#	endif //_DEBUG
#else //GLEW_MX
#	ifdef _DEBUG
#		pragma comment(lib, "CEGUIOpenGLRenderer_d.lib")
#		pragma comment(lib, "ftgl_gl3_d.lib")
#		pragma comment(lib, "glew32s_d.lib")
#	else//_DEBUG
#		pragma comment(lib, "CEGUIOpenGLRenderer.lib")
#		pragma comment(lib, "ftgl_gl3.lib")
#		pragma comment(lib, "glew32s.lib")
#	endif //_DEBUG
#endif //GLEW_MX

glslopt_ctx* GL_API::_GLSLOptContex = NULL;

#ifdef GLEW_MX
///GLEW_MX requirement
static boost::thread_specific_ptr<GLEWContext> _GLEWContextPtr;
GLEWContext * glewGetContext(){ return _GLEWContextPtr.get(); }
#if defined( OS_WINDOWS )
static boost::thread_specific_ptr<WGLEWContext> _WGLEWContextPtr;
WGLEWContext* wglewGetContext(){ return _WGLEWContextPtr.get(); }
#else
static boost::thread_specific_ptr<GLXEWContext> _GLXEWContextPtr;
GLXEWContext* glxewGetContext(){ return _GLXEWContextPtr.get(); }
#endif
#endif//GLEW_MX

namespace Divide{
    namespace GL{
        void initGlew(){
#ifdef GLEW_MX
            GLEWContext * currentGLEWContextsPtr =  _GLEWContextPtr.get();
            if (currentGLEWContextsPtr == NULL)	{
                currentGLEWContextsPtr = New GLEWContext;
                _GLEWContextPtr.reset(currentGLEWContextsPtr);
                ZeroMemory(currentGLEWContextsPtr, sizeof(GLEWContext));
            }
#endif //GLEW_MX
            glewExperimental=TRUE;
            //Everything is set up as needed, so initialize the OpenGL API
            GLuint err = glewInit();
            //Check for errors and print if any (They happen more than anyone thinks)
            //Bad drivers, old video card, corrupt GPU, anything can throw an error
            if (GLEW_OK != err){
                ERROR_FN(Locale::get("ERROR_GFX_DEVICE"),glewGetErrorString(err));
                PRINT_FN(Locale::get("WARN_SWITCH_D3D"));
                PRINT_FN(Locale::get("WARN_APPLICATION_CLOSE"));;
                //No need to continue, as switching to DX should be set before launching the application!
                exit(GLEW_INIT_ERROR);
            }
#ifdef GLEW_MX
    #if defined( OS_WINDOWS )

            WGLEWContext * currentWGLEWContextsPtr =  _WGLEWContextPtr.get();
            if (currentWGLEWContextsPtr == NULL)	{
                currentWGLEWContextsPtr = New WGLEWContext;
                _WGLEWContextPtr.reset(currentWGLEWContextsPtr);
                ZeroMemory(currentWGLEWContextsPtr, sizeof(WGLEWContext));
            }

            err = wglewInit();
            if (GLEW_OK != err){
                ERROR_FN(Locale::get("ERROR_GFX_DEVICE"),glewGetErrorString(err));
                PRINT_FN(Locale::get("WARN_SWITCH_D3D"));
                PRINT_FN(Locale::get("WARN_APPLICATION_CLOSE"));;
                exit(GLEW_INIT_ERROR);
            }
#	else //_WIN32

            GLXEWContext * currentGLXEWContextsPtr =  _GLXEWContextPtr.get();
            if (currentGLXEWContextsPtr == NULL)	{
                currentGLXEWContextsPtr = New GLXEWContext;
                _GLXEWContextPtr.reset(currentGLXEWContextsPtr);
                ZeroMemory(currentGLXEWContextsPtr, sizeof(GLXEWContext));
            }

            err = glxewInit();
            if (GLEW_OK != err){
                ERROR_FN(Locale::get("ERROR_GFX_DEVICE"),glewGetErrorString(err));
                PRINT_FN(Locale::get("WARN_SWITCH_D3D"));
                PRINT_FN(Locale::get("WARN_APPLICATION_CLOSE"));;
                exit(GLEW_INIT_ERROR);
            }
#	endif //_WIN32
#endif //GLEW_MX
        }
    }
}

//Let's try and create a  valid OpenGL context.
GLbyte GL_API::initHardware(const vec2<GLushort>& resolution, GLint argc, char **argv){
    ParamHandler& par = ParamHandler::getInstance();

    glfwSetErrorCallback(Divide::GL::glfw_error_callback);

    if( !glfwInit() )	return GLFW_INIT_ERROR;

#ifdef _DEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT,GL_TRUE);
#endif

    Divide::GL::_initStacks();

    //So, if someone selected High detail level, try to use pure 3.x API
    _msaaSamples = par.getParam<GLubyte>("rendering.FSAAsamples",2);
    _useMSAA = (par.getParam<GLubyte>("rendering.FSAAmethod",FS_MSAA) == FS_MSAA ||
                par.getParam<GLubyte>("rendering.FSAAmethod",FS_MSAA) == FS_MSwFXAA);

    if(_msaaSamples > 1 && _useMSAA)	glfwWindowHint(GLFW_SAMPLES, _msaaSamples);

    if(par.getParam<bool>("runtime.overrideRefreshRate",false)){
        //glfwWindowHint(GLFW_REFRESH_RATE,par.getParam<GLubyte>("runtime.targetRefreshRate",75));
    }

    glfwWindowHint(GLFW_RESIZABLE,par.getParam<bool>("runtime.allowWindowResize",false));
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    ///32Bit RGBA (R8G8B8A8), 24bit Depth, 8bit Stencil
    glfwWindowHint(GLFW_RED_BITS,8);
    glfwWindowHint(GLFW_GREEN_BITS,8);
    glfwWindowHint(GLFW_BLUE_BITS,8);
    glfwWindowHint(GLFW_ALPHA_BITS,8);
    glfwWindowHint(GLFW_DEPTH_BITS,24);
    glfwWindowHint(GLFW_STENCIL_BITS,8);
    glfwWindowHint(GLFW_OPENGL_PROFILE, par.getParam<bool>("runtime.useGLCompatProfile",true) ? GLFW_OPENGL_COMPAT_PROFILE : GLFW_OPENGL_CORE_PROFILE);

    //Store the main window ID for future reference
    // Open an OpenGL window; resolution is specified in the external XML files
    bool window = par.getParam<bool>("runtime.windowedMode",true);
    Divide::GL::_mainWindow = glfwCreateWindow( resolution.width,
                                                resolution.height,
                                                par.getParam<std::string>("appTitle").c_str(),
                                                window ? NULL : glfwGetPrimaryMonitor(),
                                                NULL);

    if(!Divide::GL::_mainWindow){
        glfwTerminate();
        return( GLFW_WINDOW_INIT_ERROR );
    }

    glfwMakeContextCurrent(Divide::GL::_mainWindow);
    //Init glew for main context
    Divide::GL::initGlew();

    //Geometry shaders became core in version 3.3
    if(!GLEW_VERSION_3_3){
        ERROR_FN(Locale::get("ERROR_GFX_DEVICE"),"The OpenGL version supported by the current GPU is too old!");
        return GLEW_OLD_HARDWARE;
    }

    glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
    Divide::GL::_loaderWindow = glfwCreateWindow(1,1,"divide-res-loader",NULL, Divide::GL::_mainWindow);
    if(!Divide::GL::_loaderWindow){
        glfwTerminate();
        return( GLFW_WINDOW_INIT_ERROR );
    }

#ifdef _DEBUG
    if(GLEW_ARB_debug_output) {
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
        glDebugMessageCallbackARB(&Divide::GL::DebugCallbackARB, NULL);
        ///Disable shader compiler errors (shader class handles that)
        glDebugMessageControlARB(GL_DEBUG_SOURCE_SHADER_COMPILER_ARB,
                                 GL_DEBUG_TYPE_ERROR_ARB,
                                 GL_DONT_CARE,
                                 0,
                                 NULL,
                                 GL_FALSE);
        Divide::GL::_useDebugOutputCallback = true;
    }else if(GLEW_AMD_debug_output){
        glDebugMessageCallbackAMD(&Divide::GL::DebugCallbackAMD, NULL);
        glDebugMessageEnableAMD(GL_DEBUG_CATEGORY_SHADER_COMPILER_AMD,
                                GL_DONT_CARE,
                                0,
                                NULL,
                                GL_FALSE);
        Divide::GL::_useDebugOutputCallback = true;
    }else{
        Divide::GL::_useDebugOutputCallback = false;
    }
#endif

    const GLFWvidmode* return_struct = glfwGetVideoMode(glfwGetPrimaryMonitor());

    GLint height = return_struct->height;
    GLint width = return_struct->width;
    glfwSetWindowPos(Divide::GL::_mainWindow, (width - resolution.width)*0.5f,(height - resolution.height)*0.5f);

#if defined( OS_WINDOWS )
    _hwnd = FindWindow(NULL,par.getParam<std::string>("appTitle").c_str());
    _hdc = GetDC(_hwnd);
#elif defined( OS_APPLE ) // Apple OS X
///??
#else //Linux
    _dpy = XOpenDisplay();
    _drawable = XCreateWindow( ...
#endif

    //If we got here, let's figure out what capabilities we have available
    GLint max_frag_uniform = 0, max_varying_floats = 0;
    GLint max_vertex_uniform = 0, max_vertex_attrib = 0,max_texture_units = 0;
    GLint buffers = 0,samplesEffective = 0;
    GLint major = 0, minor = 0, maxMinor = 0;
    GLint maxAnisotrophy = 0;
    GLint maxUBOBindings = 0, maxUBOBlockSize = 0;

    GLCheck(glGetIntegerv(GL_MAJOR_VERSION, &major)); // OpenGL major version
    GLCheck(glGetIntegerv(GL_MINOR_VERSION, &minor)); // OpenGL minor version
    GLCheck(glGetIntegerv(GL_SAMPLE_BUFFERS, &buffers));
    GLCheck(glGetIntegerv(GL_SAMPLES, &samplesEffective));
    GLCheck(glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, &max_frag_uniform)); //How many uniforms can we send to fragment shaders
    GLCheck(glGetIntegerv(GL_MAX_VARYING_FLOATS, &max_varying_floats)); //How many varying floats can we use inside a shader program
    GLCheck(glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &max_vertex_uniform)); //How many uniforms can we send to vertex shaders
    GLCheck(glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &max_vertex_attrib)); //How many attributes can we send to a vertex shader
    GLCheck(glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &max_texture_units)); //Maximum number of texture units we can address in shaders
    GLCheck(glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotrophy));//Maximum supporter anisotropic filtering level
    GLCheck(glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &maxUBOBindings));
    GLCheck(glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &maxUBOBlockSize));

    const GLubyte* glslVersionSupported;
    GLCheck(glslVersionSupported = glGetString(GL_SHADING_LANGUAGE_VERSION));
    std::string gpuVendorByte(reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
    setGPUVendor(GPU_VENDOR_OTHER);
    if(!gpuVendorByte.empty()){
             if(gpuVendorByte.compare(0,5,"Intel") == 0)  { setGPUVendor(GPU_VENDOR_INTEL);  }
        else if(gpuVendorByte.compare(0,6,"NVIDIA") == 0) { setGPUVendor(GPU_VENDOR_NVIDIA); }
        else if(gpuVendorByte.compare(0,3,"ATI") == 0)    { setGPUVendor(GPU_VENDOR_AMD);    }
        else if(gpuVendorByte.compare(0,3,"AMD") == 0)    { setGPUVendor(GPU_VENDOR_AMD);    }
    }else{
        gpuVendorByte = "Unknown GPU Vendor";
    }

    //Shader selection based on detail
    par.setParam("shaderDetailToken",par.getParam<GLubyte>("rendering.detailLevel"));
    par.setParam("GFX_DEVICE.maxTextureCombinedUnits",max_texture_units);
    //Cap max aniso to what the hardware supports
    if(maxAnisotrophy < par.getParam<GLubyte>("rendering.anisotropicFilteringLevel",1)){
        par.setParam("rendering.anisotropicFilteringLevel",maxAnisotrophy);
    }
    //Time to select our shaders.
    //We do not support OpenGL version lower than 3.0;
    if(major < 3){
        ERROR_FN(Locale::get("ERROR_OPENGL_NOT_SUPPORTED"));
        PRINT_FN(Locale::get("WARN_SWITCH_D3D"));
        PRINT_FN(Locale::get("WARN_APPLICATION_CLOSE"));
        return GLEW_OLD_HARDWARE;
    }else if(major == 3){
        setVersionId(OpenGL3x);
        maxMinor = 3;
    }else{
        setVersionId(OpenGL4x);
        /*   if(GLEW_VERSION_4_4) maxMinor = 4;
        else*/if(GLEW_VERSION_4_3) maxMinor = 3;
        else  if(GLEW_VERSION_4_2) maxMinor = 2;
        else  if(GLEW_VERSION_4_1) maxMinor = 1;
    }

    //Print all of the OpenGL functionality info to the console
    PRINT_FN(Locale::get("GL_MAX_UNIFORM"),max_frag_uniform);
    PRINT_FN(Locale::get("GL_MAX_FRAG_VARYING"),max_varying_floats);
    PRINT_FN(Locale::get("GL_MAX_VERT_UNIFORM"),max_vertex_uniform);
    PRINT_FN(Locale::get("GL_MAX_VERT_ATTRIB"),max_vertex_attrib);
    PRINT_FN(Locale::get("GL_MAX_TEX_UNITS"),max_texture_units);
    PRINT_FN(Locale::get("GL_MAX_VERSION"),major,maxMinor);
    PRINT_FN(Locale::get("GL_GLSL_SUPPORT"),glslVersionSupported);
    PRINT_FN(Locale::get("GL_VENDOR_STRING"),gpuVendorByte.c_str(), glGetString(GL_RENDERER));
    PRINT_FN(Locale::get("GL_MULTI_SAMPLE_INFO"),samplesEffective,buffers);
    PRINT_FN(Locale::get("GL_UBO_INFO"),maxUBOBindings, maxUBOBlockSize);

    GL_ENUM_TABLE::fill();

    //Set the clear color to a nice blue
    GL_API::clearColor(DefaultColors::DIVIDE_BLUE());
    if(glewIsSupported("GL_ARB_seamless_cube_map")){
        GLCheck(glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS));
    }
    //keep normals in the [0..1] range despite scaling geometry
    GLCheck(glEnable(GL_NORMALIZE));
    if(_msaaSamples > 1 && _useMSAA){
        GLCheck(glEnable(GL_MULTISAMPLE));
    }
    //Correct texcoord generation during rasterization despite Perspective changes
    GLCheck(glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST));

    //MipMap detail
    switch(par.getParam<GLubyte>("rendering.mipMapDetailLevel", 2)){
        case 0: glHint (GL_GENERATE_MIPMAP_HINT, GL_FASTEST); break;
        case 1: glHint (GL_GENERATE_MIPMAP_HINT, GL_DONT_CARE); break;
        default:
        case 2: glHint (GL_GENERATE_MIPMAP_HINT, GL_NICEST); break;
    }

    glfwSwapInterval(par.getParam<bool>("runtime.enableVSync",false) ? 1 : 0);
    GLint numberOfDisplayModes;
    const GLFWvidmode* modes = glfwGetVideoModes(glfwGetPrimaryMonitor(), &numberOfDisplayModes );
    if(modes) PRINT_FN(Locale::get("AVAILABLE_VIDEO_MODES"),numberOfDisplayModes);

    //start background loading thread
    _closeLoadingThread = false;
    _loaderThread = New boost::thread(&GL_API::loadInContextInternal, DELEGATE_REF(GL_API::getInstance()));

    //OpenGL is up and ready
    Divide::GL::_contextAvailable = true;

    _uniformBufferObjects.resize(UBO_PLACEHOLDER,NULL);
    _uniformBufferObjects[Matrices_UBO] = New glUniformBufferObject();
    _uniformBufferObjects[Matrices_UBO]->Create(Matrices_UBO, true,true);
    _uniformBufferObjects[Matrices_UBO]->ReserveBuffer(3 * 16, sizeof(GLfloat)); //View, Projection and ViewProjection 3 x 16 float values
    _uniformBufferObjects[Lights_UBO]  = New glUniformBufferObject();
    _uniformBufferObjects[Lights_UBO]->Create(Lights_UBO,true,false);
    _uniformBufferObjects[Lights_UBO]->ReserveBuffer(Config::MAX_LIGHTS_PER_SCENE_NODE, sizeof(LightProperties)); //Usually less or equal to 4

/*
    _uniformBufferObjects[Material_UBO]  = New glUniformBufferObject();
    _uniformBufferObjects[Material_UBO]->Create(Material_UBO,true,false);
    _uniformBufferObjects[Material_UBO]->ReserveBuffer(num materials here, sizeof(_mat->getShaderData()) );
*/
    for(GLuint index = 0; index < Config::MAX_CLIP_PLANES; ){
        _activeClipPlanes[index++] = false;
    }

    //That's it. Everything should be ready for draw calls
    PRINT_FN(Locale::get("START_OGL_API_OK"));

    RenderStateBlockDescriptor defaultGLStateDescriptor;
    GFX_DEVICE._defaultStateBlock = GFX_DEVICE.createStateBlock(defaultGLStateDescriptor);

    RenderStateBlockDescriptor defaultGLStateDescriptorNoDepth;
    defaultGLStateDescriptorNoDepth.setZReadWrite(false,false);
    _defaultStateNoDepth = GFX_DEVICE.createStateBlock(defaultGLStateDescriptorNoDepth);

    RenderStateBlockDescriptor state2DRenderingDesc;
    state2DRenderingDesc.setCullMode(CULL_MODE_NONE);
    state2DRenderingDesc.setZReadWrite(false,true);
    _state2DRendering = GFX_DEVICE.createStateBlock(state2DRenderingDesc);

    SET_DEFAULT_STATE_BLOCK(true);

    //Create an immediate mode shader
    ShaderManager::getInstance().init();
    ResourceDescriptor immediateModeShader("ImmediateModeEmulation");
    _imShader = CreateResource<ShaderProgram>(immediateModeShader);

    _prevPointString = New FTPoint();

    _glimInterfaces.reserve(4); //<For Bounding boxes, Skeletons, Debug Axis and Nav Meshes
    CEGUI::OpenGL3Renderer& GUIGLrenderer = CEGUI::OpenGL3Renderer::create();
    GUIGLrenderer.enableExtraStateSettings(par.getParam<bool>("GUI.CEGUI.ExtraStates"));
    GUI::getInstance().bindRenderer(GUIGLrenderer);

    GFX_DEVICE.changeResolution(resolution.width,resolution.height);
    return 0;
}

void GL_API::exitRenderLoop(bool killCommand) {
    Divide::GL::_applicationClosing = true;
    glfwSetWindowShouldClose(Divide::GL::_mainWindow,true);
    glfwSetWindowShouldClose(Divide::GL::_loaderWindow,true);
}

///clear up stuff ...
void GL_API::closeRenderingApi(){
    _closeLoadingThread = true;
    _loaderThread->join();

    Divide::GL::_applicationClosing = true;
    glfwSetWindowShouldClose(Divide::GL::_mainWindow,true);
    glfwSetWindowShouldClose(Divide::GL::_loaderWindow,true);

    try{
        CEGUI::System::destroy();
    }
    catch( ... ){
        D_ERROR_FN(Locale::get("ERROR_CEGUI_DESTROY"));
    }
    //Clean font cache
    for_each(FontCache::value_type it, _fonts){
        SAFE_DELETE(it.second);
    }
    for_each(glIMPrimitive* priv, _glimInterfaces){
        SAFE_DELETE(priv);
    }
    for(GLubyte i = 0; i < UBO_PLACEHOLDER; i++){
        SAFE_DELETE(_uniformBufferObjects[i]);
    }

    _fonts.clear();
    _glimInterfaces.clear(); //<Should call all destructors
    SAFE_DELETE(_prevPointString);
    SAFE_DELETE(_state2DRendering);

    glfwDestroyWindow(Divide::GL::_mainWindow);
    glfwDestroyWindow(Divide::GL::_loaderWindow);
    glfwTerminate();

    SAFE_DELETE(_loaderThread);
}

void GL_API::initDevice(GLuint targetFrameRate){
    assert(_imShader != NULL);
    while(!glfwWindowShouldClose(Divide::GL::_mainWindow)) {
        Kernel::mainLoopStatic();
    }
}

bool GL_API::initShaders(){
    //Init glsw library
    GLint glswState = glswInit();
    glswAddDirectiveToken("","#version 130\n/*“Copyright 2009-2013 DIVIDE-Studio”*/");
    glswAddDirectiveToken("","#extension GL_ARB_uniform_buffer_object : require");
    glswAddDirectiveToken("","#extension GL_EXT_gpu_shader4 : enable");

    if(getGPUVendor() == GPU_VENDOR_NVIDIA){ //nVidia specific
        //glswAddDirectiveToken("","#pragma optionNV(fastmath on)");
        //glswAddDirectiveToken("","#pragma optionNV(fastprecision on)");
        //glswAddDirectiveToken("","#pragma optionNV(inline all)");
        //glswAddDirectiveToken("","#pragma optionNV(strict on)");
        //glswAddDirectiveToken("","#pragma optionNV(unroll all)");
    }

    if(getGPUVendor() == GPU_VENDOR_AMD){
        glswAddDirectiveToken("","#define SKIP_HARDWARE_CLIPPING");
    }
    std::string maxInstances("#define MAX_INSTANCES " + Util::toString(Config::MAX_INSTANCE_COUNT));
    std::string clipPlanes("#define MAX_CLIP_PLANES " + Util::toString(Config::MAX_CLIP_PLANES));
    std::string lightCount("#define MAX_LIGHT_COUNT " + Util::toString(Config::MAX_LIGHTS_PER_SCENE_NODE));
    std::string shadowCount("#define MAX_SHADOW_CASTING_LIGHTS " + Util::toString(Config::MAX_SHADOW_CASTING_LIGHTS_PER_NODE));
    glswAddDirectiveToken("", maxInstances.c_str());
    glswAddDirectiveToken("", clipPlanes.c_str());
    glswAddDirectiveToken("", lightCount.c_str());
    glswAddDirectiveToken("", shadowCount.c_str());
    glswAddDirectiveToken("","#define Z_TEST_SIGMA 0.0001");
    glswAddDirectiveToken("","#define ALPHA_DISCARD_THRESHOLD 0.2");
    glswAddDirectiveToken("","//__CUSTOM_DEFINES__");
    glswAddDirectiveToken("Fragment","//__CUSTOM_FRAGMENT_UNIFORMS__");
    glswAddDirectiveToken("Vertex","//__CUSTOM_VERTEX_UNIFORMS__");

    GL_API::_GLSLOptContex = glslopt_initialize(GFX_DEVICE.getApi() == OpenGLES);
    if(glswState == 1 && GL_API::_GLSLOptContex != NULL){
        return true;
    }
    return false;
}

bool GL_API::deInitShaders(){
    glslopt_cleanup(_GLSLOptContex);
    //Shut down glsw and clean memory
    return (glswShutdown() == 1);
}

void GL_API::changeResolutionInternal(GLushort w, GLushort h){
    glfwSetWindowSize(Divide::GL::_mainWindow,w,h);

    ParamHandler& par = ParamHandler::getInstance();
    GLfloat zNear  = par.getParam<GLfloat>("runtime.zNear");
    GLfloat zFar   = par.getParam<GLfloat>("runtime.zFar");
    GLfloat fov    = par.getParam<GLfloat>("runtime.verticalFOV");
    GLfloat ratio  = par.getParam<GLfloat>("runtime.aspectRatio");
    par.setParam("runtime.horizontalFOV", Util::yfov_to_xfov((GLfloat)fov, ratio));
    // Reset the coordinate system before modifying
    Divide::GL::_matrixMode(PROJECTION_MATRIX);
    Divide::GL::_loadIdentity();
    // Set the viewport to be the entire window
    GL_API::setViewport(vec4<GLuint>(0,0,w,h),true);
    // Set the clipping volume
    Divide::GL::_perspective(fov,ratio,zNear,zFar);

    Divide::GL::_matrixMode(VIEW_MATRIX);
    Divide::GL::_loadIdentity();

    _cachedResolution.width = w;
    _cachedResolution.height = h;
    //Update view frustum
    Frustum::getInstance().setZPlanes(vec2<GLfloat>(zNear, zFar));
    //Inform the Kernel
    Kernel::updateResolutionCallback(w,h);
}

void GL_API::setWindowPos(GLushort w, GLushort h) const {
    glfwSetWindowPos(Divide::GL::_mainWindow,w,h);
}

void GL_API::setMousePosition(GLdouble x, GLdouble y) const {
    glfwSetCursorPos(Divide::GL::_mainWindow,x,y);
}

vec3<GLfloat> GL_API::unproject(const vec3<GLfloat>& windowCoord) const {
    vec3<GLfloat> coords;
    Divide::GL::_unproject(windowCoord, coords);
    return coords;
}

void GL_API::idle(){
    glfwPollEvents();
}

bool GL_API::loadInContext(const CurrentContext& context, boost::function0<GLvoid> callback) {
    if(callback.empty())
        return false;

    if(context == GFX_LOADING_CONTEXT){
        while(!_loadQueue.push(callback));
    }else{
        callback();
    }
    return true;
}

void GL_API::loadInContextInternal(){
    glfwMakeContextCurrent(Divide::GL::_loaderWindow);
#ifdef GLEW_MX
    Divide::GL::initGlew();
#endif
    while(!_closeLoadingThread){
        if(_loadQueue.empty()){
            boost::this_thread::sleep(boost::posix_time::milliseconds(10));//<Avoid burning the CPU - Ionut
            continue;
        }
        boost::function0<GLvoid> callback;
        while(_loadQueue.pop(callback)){
            callback();
            glFlush();
        }
    }
    glfwMakeContextCurrent(NULL);
}