#include "Headers/GFXDevice.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/Application.h"
#include "Core/Headers/ParamHandler.h"
#include "Core/Headers/ProfileTimer.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Platform/Video/Shaders/Headers/ShaderManager.h"
#include "Rendering/PostFX/Headers/PostFX.h"
#include "Rendering/Headers/ForwardPlusRenderer.h"
#include "Rendering/Headers/DeferredShadingRenderer.h"

namespace Divide {

/// Create a display context using the selected API and create all of the needed
/// primitives needed for frame rendering
ErrorCode GFXDevice::initRenderingAPI(I32 argc, char** argv) {
    ErrorCode hardwareState = createAPIInstance();
    if (hardwareState == ErrorCode::NO_ERR) {
        // Initialize the rendering API
        hardwareState = _api->initRenderingAPI(argc, argv);
    }

    if (hardwareState != ErrorCode::NO_ERR) {
        // Validate initialization
        return hardwareState;
    }

    stringImpl refreshRates;
    vectorAlg::vecSize displayCount = gpuState().getDisplayCount();
    for (vectorAlg::vecSize idx = 0; idx < displayCount; ++idx) {
        const vectorImpl<GPUState::GPUVideoMode>& registeredModes = gpuState().getDisplayModes(idx);
        Console::printfn(Locale::get("AVAILABLE_VIDEO_MODES"), idx, registeredModes.size());

        for (const GPUState::GPUVideoMode& mode : registeredModes) {
            // Optionally, output to console/file each display mode
            refreshRates = Util::StringFormat("%d", mode._refreshRate.front());
            vectorAlg::vecSize refreshRateCount = mode._refreshRate.size();
            for (vectorAlg::vecSize i = 1; i < refreshRateCount; ++i) {
                refreshRates += Util::StringFormat(", %d", mode._refreshRate[i]);
            }
            Console::printfn(Locale::get("CURRENT_DISPLAY_MODE"),
                mode._resolution.width,
                mode._resolution.height,
                mode._bitDepth,
                mode._formatName.c_str(),
                refreshRates.c_str());
        }
    }

    WindowManager& winManager = Application::getInstance().getWindowManager();
    // Initialize the shader manager
    ShaderManager::getInstance().init();
    // Create an immediate mode shader used for general purpose rendering (e.g.
    // to mimic the fixed function pipeline)
    _imShader = ShaderManager::getInstance().getDefaultShader();
    DIVIDE_ASSERT(_imShader != nullptr,
                  "GFXDevice error: No immediate mode emulation shader available!");
    PostFX::createInstance();
    // Create a shader buffer to store the following info:
    // ViewMatrix, ProjectionMatrix, ViewProjectionMatrix, CameraPositionVec, ViewportRec, zPlanesVec4 and ClipPlanes[MAX_CLIP_PLANES]
    // It should translate to (as seen by OpenGL) a uniform buffer without persistent mapping.
    // (Many small updates with BufferSubData are recommended with the target usage of the buffer)
    _gfxDataBuffer.reset(newSB("dvd_GPUBlock", 1, false, false, BufferUpdateFrequency::OFTEN));
    _gfxDataBuffer->create(1, sizeof(GPUBlock));
    // Every visible node will first update this buffer with required data (WorldMatrix, NormalMatrix, Material properties, Bone count, etc)
    // Due to it's potentially huge size, it translates to (as seen by OpenGL) a Shader Storage Buffer that's persistently and coherently mapped
    // We make sure the buffer is large enough to hold data for all of our rendering stages to minimize the number of writes per frame
    STUBBED("ToDo: Use different sizes for node buffer and command buffer, as commands for Z PrePas and Display are different -Ionut")
    STUBBED("ToDo: Increase buffer size to handle multiple shadow passes. Round robin with size factor 3 should suffice for shadows -Ionut")
    // Create a shader buffer to hold all of our indirect draw commands
    // Usefull if we need access to the buffer in GLSL/Compute programs
    for (U32 i = 0; i < _indirectCommandBuffers.size(); ++i) {
        _indirectCommandBuffers[i].reset(newSB(Util::StringFormat("dvd_GPUCmds%d", i), 1, true, false, BufferUpdateFrequency::OFTEN));
        _indirectCommandBuffers[i]->create(Config::MAX_VISIBLE_NODES + 1, sizeof(IndirectDrawCommand));
        _indirectCommandBuffers[i]->addAtomicCounter(3);
    }
    
    for (U32 i = 0; i < _nodeBuffers.size(); ++i) {
        _nodeBuffers[i].reset(newSB(Util::StringFormat("dvd_MatrixBlock%d", i), 1, true, true, BufferUpdateFrequency::OFTEN));
        _nodeBuffers[i]->create(Config::MAX_VISIBLE_NODES + 1, sizeof(NodeData));
    }

    // Resize our window to the target resolution
    const vec2<U16>& resolution = winManager.getResolution();
    changeResolution(resolution.width, resolution.height);
    setBaseViewport(vec4<I32>(0, 0, resolution.width, resolution.height));
    // Create general purpose render state blocks
    RenderStateBlock defaultState;
    _defaultStateBlockHash = defaultState.getHash();

    RenderStateBlock defaultStateNoDepth;
    defaultStateNoDepth.setZReadWrite(false, true);
    _defaultStateNoDepthHash = defaultStateNoDepth.getHash();

    RenderStateBlock state2DRendering;
    state2DRendering.setCullMode(CullMode::NONE);
    state2DRendering.setZReadWrite(false, true);
    _state2DRenderingHash = state2DRendering.getHash();

    RenderStateBlock stateDepthOnlyRendering;
    stateDepthOnlyRendering.setColorWrites(false, false, false, false);
    stateDepthOnlyRendering.setZFunc(ComparisonFunction::ALWAYS);
    _stateDepthOnlyRenderingHash = stateDepthOnlyRendering.getHash();

    // The general purpose render state blocks are both mandatory and must
    // differ from each other at a state hash level
    DIVIDE_ASSERT(_stateDepthOnlyRenderingHash != _state2DRenderingHash,
                  "GFXDevice error: Invalid default state hash detected!");
    DIVIDE_ASSERT(_state2DRenderingHash != _defaultStateNoDepthHash,
                  "GFXDevice error: Invalid default state hash detected!");
    DIVIDE_ASSERT(_defaultStateNoDepthHash != _defaultStateBlockHash,
                  "GFXDevice error: Invalid default state hash detected!");
    // Activate the default render states
    _previousStateBlockHash = _stateBlockMap[0].getHash();
    setStateBlock(_defaultStateBlockHash);
    // Our default render targets hold the screen buffer, depth buffer, and a
    // special, on demand,
    // down-sampled version of the depth buffer
    // Screen FB should use MSAA if available
    _renderTarget[to_uint(RenderTarget::SCREEN)] = newFB(true);
    // The depth buffer should probably be merged into the screen buffer
    _renderTarget[to_uint(RenderTarget::DEPTH)] = newFB(false);
    // We need to create all of our attachments for the default render targets
    // Start with the screen render target: Try a half float, multisampled
    // buffer (MSAA + HDR rendering if possible)
    TextureDescriptor screenDescriptor(TextureType::TEXTURE_2D_MS,
                                       GFXImageFormat::RGBA16F,
                                       GFXDataFormat::FLOAT_16);
    SamplerDescriptor screenSampler;
    screenSampler.setFilters(TextureFilter::NEAREST);
    screenSampler.setWrapMode(TextureWrap::CLAMP_TO_EDGE);
    screenSampler.toggleMipMaps(false);
    screenDescriptor.setSampler(screenSampler);
    // Next, create a depth attachment for the screen render target.
    // Must also multisampled. Use full float precision for long view distances
    SamplerDescriptor depthSampler;
    depthSampler.setFilters(TextureFilter::NEAREST);
    depthSampler.setWrapMode(TextureWrap::CLAMP_TO_EDGE);
    // Use greater or equal depth compare function, but depth comparison is
    // disabled, anyway.
    depthSampler._cmpFunc = ComparisonFunction::GEQUAL;
    depthSampler.toggleMipMaps(false);
    TextureDescriptor depthDescriptor(TextureType::TEXTURE_2D_MS,
                                      GFXImageFormat::DEPTH_COMPONENT32F,
                                      GFXDataFormat::FLOAT_32);
    depthDescriptor.setSampler(depthSampler);
    // Add the attachments to the render targets
    _renderTarget[to_uint(RenderTarget::SCREEN)]->addAttachment(screenDescriptor, TextureDescriptor::AttachmentType::Color0);
    _renderTarget[to_uint(RenderTarget::SCREEN)]->addAttachment(depthDescriptor, TextureDescriptor::AttachmentType::Depth);
    _renderTarget[to_uint(RenderTarget::SCREEN)]->create(resolution.width, resolution.height);
    _renderTarget[to_uint(RenderTarget::SCREEN)]->setClearColor(DefaultColors::DIVIDE_BLUE());
    // If we enabled anaglyph rendering, we need a second target, identical to
    // the screen target
    // used to render the scene at an offset
    if (_enableAnaglyph) {
        _renderTarget[to_uint(RenderTarget::ANAGLYPH)] = newFB(true);
        _renderTarget[to_uint(RenderTarget::ANAGLYPH)]->addAttachment(screenDescriptor, TextureDescriptor::AttachmentType::Color0);
        _renderTarget[to_uint(RenderTarget::ANAGLYPH)]->addAttachment(depthDescriptor, TextureDescriptor::AttachmentType::Depth);
        _renderTarget[to_uint(RenderTarget::ANAGLYPH)]->create(resolution.width, resolution.height);
        _renderTarget[to_uint(RenderTarget::ANAGLYPH)]->setClearColor(DefaultColors::DIVIDE_BLUE());
    }

    depthSampler.toggleMipMaps(true);
    depthSampler.setMinFilter(TextureFilter::NEAREST_MIPMAP_NEAREST);
    depthDescriptor.setSampler(depthSampler);
    _renderTarget[to_uint(RenderTarget::DEPTH)]->addAttachment(depthDescriptor, TextureDescriptor::AttachmentType::Depth);
    _renderTarget[to_uint(RenderTarget::DEPTH)]->toggleColorWrites(false);
    _renderTarget[to_uint(RenderTarget::DEPTH)]->create(resolution.width, resolution.height);
    _renderTarget[to_uint(RenderTarget::DEPTH)]->getAttachment(TextureDescriptor::AttachmentType::Depth)->lockAutomaticMipMapGeneration(true);
    // We also add a couple of useful cameras used by this class. One for
    // rendering in 2D and one for generating cube maps
    Application::getInstance().getKernel().getCameraMgr().addNewCamera(
        "2DRenderCamera", _2DCamera);
    Application::getInstance().getKernel().getCameraMgr().addNewCamera(
        "_gfxCubeCamera", _cubeCamera);
    // Initialized our HierarchicalZ construction shader (takes a depth
    // attachment and down-samples it for every mip level)
    _HIZConstructProgram =
        CreateResource<ShaderProgram>(ResourceDescriptor("HiZConstruct"));
    _HIZConstructProgram->Uniform("LastMip", ShaderProgram::TextureUsage::UNIT0);
    _HIZCullProgram = 
        CreateResource<ShaderProgram>(ResourceDescriptor("HiZOcclusionCull"));
    // Store our target z distances
    _gpuBlock._data._ZPlanesCombined.zw(vec2<F32>(
        ParamHandler::getInstance().getParam<F32>("rendering.zNear"),
        ParamHandler::getInstance().getParam<F32>("rendering.zFar")));
    // Create a separate loading thread that shares resources with the main
    // rendering context
    _state.startLoaderThread([&]() {
        threadedLoadCallback();
        // Use an atomic bool to check if the thread is still active
        _state.loadingThreadAvailable(true);
        // Run an infinite loop until we actually request otherwise
        while (!_state.closeLoadingThread()) {
            _state.consumeOneFromQueue();
        }
        // If we close the loading thread, update our atomic bool to make sure the
        // application isn't using it anymore
        _state.loadingThreadAvailable(false);
    });
    // Register a 2D function used for previewing the depth buffer.
#ifdef _DEBUG
    add2DRenderFunction(DELEGATE_BIND(&GFXDevice::previewDepthBuffer, this), 0);
#endif
    // We start of with a forward plus renderer
    setRenderer(RendererType::RENDERER_FORWARD_PLUS);
    ParamHandler::getInstance().setParam<bool>("rendering.previewDepthBuffer",
                                               false);
    // If render targets ready, we initialize our post processing system
    PostFX::getInstance().init(resolution);

    _commandBuildTimer = Time::ADD_TIMER("Command Generation Timer");

    _axisGizmo = getOrCreatePrimitive(false);
    _axisGizmo->name("GFXDeviceAxisGizmo");
    RenderStateBlock primitiveDescriptor(getRenderStateBlock(getDefaultStateBlock(true)));
    _axisGizmo->stateHash(primitiveDescriptor.getHash());
    // Everything is ready from the rendering point of view
    return ErrorCode::NO_ERR;
}

/// Revert everything that was set up in initRenderingAPI()
void GFXDevice::closeRenderingAPI() {
    DIVIDE_ASSERT(_api != nullptr,
                  "GFXDevice error: closeRenderingAPI called without init!");

    _axisGizmo->_canZombify = true;
    // Delete the internal shader
    RemoveResource(_HIZConstructProgram);
    RemoveResource(_HIZCullProgram);
    // Destroy our post processing system
    Console::printfn(Locale::get("STOP_POST_FX"));
    PostFX::destroyInstance();
    // Delete the renderer implementation
    Console::printfn(Locale::get("CLOSING_RENDERER"));
    // Delete our default render state blocks
    _stateBlockMap.clear();
    // Destroy all of the immediate mode emulation primitives created during
    // runtime
    MemoryManager::DELETE_VECTOR(_imInterfaces);
    _gfxDataBuffer->destroy();
    for (U32 i = 0; i < _indirectCommandBuffers.size(); ++i) {
        _indirectCommandBuffers[i]->destroy();
    }
    for (U32 i = 0; i < _nodeBuffers.size(); ++i) {
        _nodeBuffers[i]->destroy();
    }
    // Destroy all rendering passes and rendering bins
    RenderPassManager::destroyInstance();
    // Delete all of our rendering targets
    for (Framebuffer*& renderTarget : _renderTarget) {
        MemoryManager::DELETE(renderTarget);
    }
    _renderer.reset(nullptr);
    // Close the shader manager
    ShaderManager::getInstance().destroy();
    ShaderManager::getInstance().destroyInstance();
    // Close the rendering API
    _api->closeRenderingAPI();
    // Close the loading thread and wait for it to terminate
    _state.stopLoaderThread();
    Time::REMOVE_TIMER(_commandBuildTimer);
    switch (_API_ID) {
        case RenderAPI::OpenGL:
        case RenderAPI::OpenGLES: {
            GL_API::destroyInstance();
        } break;
        case RenderAPI::Direct3D: {
            DX_API::destroyInstance();
        } break;
        case RenderAPI::Vulkan: {
        } break;
        case RenderAPI::None: {
        } break;
        default: { 
        } break;
    };
}

/// After a swap buffer call, the CPU may be idle waiting for the GPU to draw to
/// the screen, so we try to do some processing
void GFXDevice::idle() {
    // Update the zPlanes if needed
    _gpuBlock._data._ZPlanesCombined.zw(vec2<F32>(
        ParamHandler::getInstance().getParam<F32>("rendering.zNear"),
        ParamHandler::getInstance().getParam<F32>("rendering.zFar")));
    // Pass the idle call to the post processing system
    PostFX::getInstance().idle();
    // And to the shader manager
    ShaderManager::getInstance().idle();
}

void GFXDevice::beginFrame() {
    _api->beginFrame();
    setStateBlock(_defaultStateBlockHash);
}

void GFXDevice::endFrame() {
    // Max number of frames before an unused primitive is recycled
    // (default: 180 - 3 seconds at 60 fps)
    static const I32 IN_MAX_FRAMES_RECYCLE_COUNT = 180;
    // Max number of frames before an unused primitive is deleted
    static const I32 IM_MAX_FRAMES_ZOMBIE_COUNT = 
        IN_MAX_FRAMES_RECYCLE_COUNT *
        IN_MAX_FRAMES_RECYCLE_COUNT;

    if (Application::getInstance().mainLoopActive()) {
        // Render all 2D debug info and call API specific flush function
        {
            GFX::Scoped2DRendering scoped2D(true);
            for (std::pair<U32, DELEGATE_CBK<> >& callbackFunction :
                _2dRenderQueue) {
                callbackFunction.second();
            }
        }

        // Remove dead primitives in 4 steps
        // 1) Partition the vector in 2 parts: valid objects first, zombie
        // objects second
        vectorImpl<IMPrimitive*>::iterator zombie = std::partition(
            std::begin(_imInterfaces), std::end(_imInterfaces),
            [](IMPrimitive* const priv) {
            return priv->zombieCounter() < IM_MAX_FRAMES_ZOMBIE_COUNT;
        });
        // 2) For every zombie object, free the memory it's using
        for (vectorImpl<IMPrimitive*>::iterator i = zombie;
             i != std::end(_imInterfaces); ++i) {
            MemoryManager::DELETE(*i);
        }
        // 3) Remove all the zombie objects once the memory is freed
        _imInterfaces.erase(zombie, std::end(_imInterfaces));
        // 4) Increment the zombie counter (if allowed) for the remaining primitives
        std::for_each(
            std::begin(_imInterfaces), std::end(_imInterfaces),
            [](IMPrimitive* primitive) -> void {
            if (primitive->_canZombify && primitive->inUse()) {
                // The zombie counter should always be reset on draw!
                primitive->zombieCounter(primitive->zombieCounter() + 1);
                // If the primitive wasn't used in a while, it may not be in use
                // so we should recycle it.
                if (primitive->zombieCounter() > IN_MAX_FRAMES_RECYCLE_COUNT) {
                    primitive->inUse(false);
                }
            }
        });

        FRAME_COUNT++;
        FRAME_DRAW_CALLS_PREV = FRAME_DRAW_CALLS;
        FRAME_DRAW_CALLS = 0;
    }
    // Activate the default render states
    setStateBlock(_defaultStateBlockHash);
    // Unbind shaders
    ShaderManager::getInstance().unbind();
    _api->endFrame();
}

void GFXDevice::handleWindowEvent(WindowEvent event, I32 data1, I32 data2) {
    Application& app = Application::getInstance();

    switch (event) {
        case WindowEvent::HIDDEN:{
        } break;
        case WindowEvent::SHOWN:{
        } break;
        case WindowEvent::MINIMIZED:{
            app.mainLoopPaused(true);
            app.getWindowManager().minimized(true);
        } break;
        case WindowEvent::MAXIMIZED:{
            app.getWindowManager().minimized(false);
        } break;
        case WindowEvent::RESTORED: {
            app.getWindowManager().minimized(false);
        } break;
        case WindowEvent::LOST_FOCUS:{
            app.getWindowManager().hasFocus(false);
        } break;
        case WindowEvent::GAINED_FOCUS:{
            app.getWindowManager().hasFocus(true);
        } break;
        case WindowEvent::RESIZED_INTERNAL:{
            setBaseViewport(vec4<I32>(0, 0, data1, data2));
            // Update the 2D camera so it matches our new rendering viewport
            _2DCamera->setProjection(vec4<F32>(0, data1, 0, data2),
                                     vec2<F32>(-1, 1));
            app.getKernel().onChangeWindowSize(to_ushort(data1), to_ushort(data2));
        } break;
        case WindowEvent::RESIZED_EXTERNAL:{
        } break;
    };
}

Renderer& GFXDevice::getRenderer() const {
    DIVIDE_ASSERT(_renderer != nullptr,
                  "GFXDevice error: Renderer requested but not created!");
    return *_renderer;
}

void GFXDevice::setRenderer(RendererType rendererType) {
    DIVIDE_ASSERT(rendererType != RendererType::COUNT,
                  "GFXDevice error: Tried to create an invalid renderer!");

    switch (rendererType) {
        case RendererType::RENDERER_FORWARD_PLUS: {
            _renderer.reset(new ForwardPlusRenderer());
        } break;
        case RendererType::RENDERER_DEFERRED_SHADING: {
            _renderer.reset(new DeferredShadingRenderer());
        } break;
    }
}

ErrorCode GFXDevice::createAPIInstance() {
    DIVIDE_ASSERT(_api == nullptr,
                  "GFXDevice error: initRenderingAPI called twice!");
    switch (_API_ID) {
        case RenderAPI::OpenGL:
        case RenderAPI::OpenGLES: {
            _api = &GL_API::getInstance();
        } break;
        case RenderAPI::Direct3D: {
            _api = &DX_API::getInstance();
            Console::errorfn(Locale::get("ERROR_GFX_DEVICE_API"));
            return ErrorCode::GFX_NOT_SUPPORTED;
        } break;
        case RenderAPI::Vulkan: {
            Console::errorfn(Locale::get("ERROR_GFX_DEVICE_API"));
            return ErrorCode::GFX_NOT_SUPPORTED;
        } break;
        case RenderAPI::None: {
            Console::errorfn(Locale::get("ERROR_GFX_DEVICE_API"));
            return ErrorCode::GFX_NOT_SUPPORTED;
        } break;
        default: {
            Console::errorfn(Locale::get("ERROR_GFX_DEVICE_API"));
            return ErrorCode::GFX_NON_SPECIFIED;
        } break;
    };

    return ErrorCode::NO_ERR;
}
};