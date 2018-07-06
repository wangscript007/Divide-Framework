#include "Headers/GFXDevice.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/ParamHandler.h"
#include "Managers/Headers/ShaderManager.h"
#include "Rendering/PostFX/Headers/PostFX.h"
#include "Core/Resources/Headers/ResourceCache.h"

//Max number of frames before an unused primitive is deleted (default: 180 - 3 seconds at 60 fps)
const I32 IM_MAX_FRAMES_ZOMBIE_COUNT = 180;

I8 GFXDevice::initHardware(const vec2<U16>& resolution, I32 argc, char **argv) {
    I8 hardwareState = _api.initHardware(resolution,argc,argv);

    if(hardwareState != NO_ERR)
        return hardwareState;

    //Create an immediate mode shader
    ShaderManager::getInstance().init(_kernel);
    _imShader = ShaderManager::getInstance().getDefaultShader();
    DIVIDE_ASSERT(_imShader != nullptr, "GFXDevice error: No immediate mode emulation shader available!");
    _imShader->Uniform("tex",0);

    //View, Projection, ViewProjection, Camera Position, Viewport, 
    //zPlanes and ClipPlanes[MAX_CLIP_PLANES] 3 x 16 + 4 + 4 + 4 + 4 * MAX_CLIP_PLANES float values
    _gfxDataBuffer = newSB(false, false);
    _gfxDataBuffer->Create((3 * 16) + 4 + 4 + 4 + (4 * Config::MAX_CLIP_PLANES), sizeof(F32)); 
    _gfxDataBuffer->Bind(Divide::SHADER_BUFFER_GPU_BLOCK);

    _nodeMatricesBuffer = newSB(true);
    _nodeMatricesBuffer->Create(Config::MAX_VISIBLE_NODES, sizeof(NodeData2));
    _nodeMatricesBuffer->Bind(Divide::SHADER_BUFFER_NODE_TRANSFORMS);

    _nodeMaterialsBuffer = newSB(true);
    _nodeMaterialsBuffer->Create(Config::MAX_VISIBLE_NODES, sizeof(NodeData4)); 
    _nodeMaterialsBuffer->BindRange(Divide::SHADER_BUFFER_NODE_MATERIAL, 0, Config::MAX_VISIBLE_NODES);

    changeResolution(resolution);

    RenderStateBlockDescriptor defaultStateDescriptor;
    _defaultStateBlockHash = getOrCreateStateBlock(defaultStateDescriptor);
    RenderStateBlockDescriptor defaultStateDescriptorNoDepth;
    defaultStateDescriptorNoDepth.setZReadWrite(false, true);
    _defaultStateNoDepthHash = getOrCreateStateBlock(defaultStateDescriptorNoDepth);
    RenderStateBlockDescriptor state2DRenderingDesc;
    state2DRenderingDesc.setCullMode(CULL_MODE_NONE);
    state2DRenderingDesc.setZReadWrite(false, true);
    _state2DRenderingHash = getOrCreateStateBlock(state2DRenderingDesc);
    RenderStateBlockDescriptor stateDepthOnlyRendering;
    stateDepthOnlyRendering.setColorWrites(false, false, false, false);
    stateDepthOnlyRendering.setZFunc(CMP_FUNC_ALWAYS);
    _stateDepthOnlyRenderingHash = getOrCreateStateBlock(stateDepthOnlyRendering);
    _stateBlockMap[0] = nullptr;

    DIVIDE_ASSERT(_stateDepthOnlyRenderingHash != _state2DRenderingHash,    "GFXDevice error: Invalid default state hash detected!");
    DIVIDE_ASSERT(_state2DRenderingHash        != _defaultStateNoDepthHash, "GFXDevice error: Invalid default state hash detected!");
    DIVIDE_ASSERT(_defaultStateNoDepthHash     != _defaultStateBlockHash,   "GFXDevice error: Invalid default state hash detected!");

    setStateBlock(_defaultStateBlockHash);

    //Screen FB should use MSAA if available, else fallback to normal color FB (no AA or FXAA)
    _renderTarget[RENDER_TARGET_SCREEN]       = newFB(true);
    _renderTarget[RENDER_TARGET_DEPTH]        = newFB(false);
    _renderTarget[RENDER_TARGET_DEPTH_RANGES] = newFB(false);

    TextureDescriptor depthRangesDescriptor(TEXTURE_2D, RGBA32F, FLOAT_32);
    SamplerDescriptor depthRangesSampler;
    depthRangesSampler.setFilters(TEXTURE_FILTER_NEAREST);
    depthRangesSampler.setWrapMode(TEXTURE_CLAMP_TO_EDGE);
    depthRangesSampler.toggleMipMaps(false);
    depthRangesDescriptor.setSampler(depthRangesSampler);
    vec2<U16> tileSize(Config::Lighting::LIGHT_GRID_TILE_DIM_X, Config::Lighting::LIGHT_GRID_TILE_DIM_Y);
    vec2<U16> resTemp(resolution + tileSize);

    TextureDescriptor screenDescriptor(TEXTURE_2D_MS, RGBA16F, FLOAT_16);
    SamplerDescriptor screenSampler;
    screenSampler.setFilters(TEXTURE_FILTER_NEAREST, TEXTURE_FILTER_NEAREST);
    screenSampler.setWrapMode(TEXTURE_CLAMP_TO_EDGE);
    screenSampler.toggleMipMaps(false);
    screenDescriptor.setSampler(screenSampler);

    TextureDescriptor depthDescriptorHiZ(TEXTURE_2D_MS, DEPTH_COMPONENT32F, FLOAT_32);
    SamplerDescriptor depthSamplerHiZ;
    depthSamplerHiZ.setFilters(TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST, TEXTURE_FILTER_NEAREST);
    depthSamplerHiZ.setWrapMode(TEXTURE_CLAMP_TO_EDGE);
    depthSamplerHiZ.toggleMipMaps(true);
    depthSamplerHiZ._useRefCompare = true; //< Use compare function
    depthSamplerHiZ._cmpFunc = CMP_FUNC_GEQUAL; //< Use greater or equal
    depthDescriptorHiZ.setSampler(depthSamplerHiZ);

    TextureDescriptor depthDescriptor(TEXTURE_2D_MS, DEPTH_COMPONENT32F, FLOAT_32);
    SamplerDescriptor depthSampler;
    depthSampler.setFilters(TEXTURE_FILTER_NEAREST);
    depthSampler.setWrapMode(TEXTURE_CLAMP_TO_EDGE);
    depthSampler.toggleMipMaps(false);
    depthSampler._useRefCompare = true; //< Use compare function
    depthSampler._cmpFunc = CMP_FUNC_GEQUAL; //< Use greater or equal
    depthDescriptor.setSampler(depthSampler);

    _renderTarget[RENDER_TARGET_DEPTH_RANGES]->AddAttachment(depthRangesDescriptor, TextureDescriptor::Color0);
    _renderTarget[RENDER_TARGET_DEPTH_RANGES]->toggleDepthBuffer(false);
    _renderTarget[RENDER_TARGET_DEPTH_RANGES]->setClearColor(vec4<F32>(0.0f, 1.0f, 0.0f, 1.0f));
    _renderTarget[RENDER_TARGET_DEPTH_RANGES]->Create(resTemp.x / tileSize.x - 1, resTemp.y / tileSize.y - 1);

    _renderTarget[RENDER_TARGET_DEPTH]->AddAttachment(depthDescriptorHiZ, TextureDescriptor::Depth);
    _renderTarget[RENDER_TARGET_DEPTH]->toggleColorWrites(false);
    _renderTarget[RENDER_TARGET_DEPTH]->Create(resolution.width, resolution.height);

    _renderTarget[RENDER_TARGET_SCREEN]->AddAttachment(screenDescriptor, TextureDescriptor::Color0);
    _renderTarget[RENDER_TARGET_SCREEN]->AddAttachment(depthDescriptor,  TextureDescriptor::Depth);
    _renderTarget[RENDER_TARGET_SCREEN]->Create(resolution.width, resolution.height);

    if(_enableAnaglyph){
        _renderTarget[RENDER_TARGET_ANAGLYPH] = newFB(true);
        _renderTarget[RENDER_TARGET_ANAGLYPH]->AddAttachment(screenDescriptor, TextureDescriptor::Color0);
        _renderTarget[RENDER_TARGET_ANAGLYPH]->AddAttachment(depthDescriptor,  TextureDescriptor::Depth);
        _renderTarget[RENDER_TARGET_ANAGLYPH]->Create(resolution.width, resolution.height);
    }

    _postFX.init(resolution);
    add2DRenderFunction(DELEGATE_BIND(&GFXDevice::previewDepthBuffer, this), 0);
    _kernel->getCameraMgr().addNewCamera("2DRenderCamera", _2DCamera);
    _kernel->getCameraMgr().addNewCamera("_gfxCubeCamera", _cubeCamera);
    _HIZConstructProgram = CreateResource<ShaderProgram>(ResourceDescriptor("HiZConstruct"));
    _HIZConstructProgram->UniformTexture("LastMip", 0);

    ResourceDescriptor rangesDesc("DepthRangesConstruct");
    rangesDesc.setPropertyList("LIGHT_GRID_TILE_DIM_X " + Util::toString(Config::Lighting::LIGHT_GRID_TILE_DIM_X) + "," + "LIGHT_GRID_TILE_DIM_Y " + Util::toString(Config::Lighting::LIGHT_GRID_TILE_DIM_Y));
    _depthRangesConstructProgram = CreateResource<ShaderProgram>(rangesDesc);
    _depthRangesConstructProgram->UniformTexture("depthTex", 0);

    _cachedSceneZPlanes.set(ParamHandler::getInstance().getParam<F32>("rendering.zNear"), ParamHandler::getInstance().getParam<F32>("rendering.zFar"));
    
    return NO_ERR;
}

void GFXDevice::closeRenderingApi(){
    _shaderManager.destroyInstance();
    _api.closeRenderingApi();
    _loaderThread->join();
    SAFE_DELETE(_loaderThread);

    FOR_EACH(RenderStateMap::value_type& it, _stateBlockMap)
        SAFE_DELETE(it.second);
    
    _stateBlockMap.clear();
    for(IMPrimitive*& priv : _imInterfaces)
        SAFE_DELETE(priv);
    
    _imInterfaces.clear(); //<Should call all destructors

    //Destroy all rendering Passes
    RenderPassManager::getInstance().destroyInstance();

    for (Framebuffer*& renderTarget : _renderTarget)
        SAFE_DELETE(renderTarget);

    SAFE_DELETE(_gfxDataBuffer);
    SAFE_DELETE(_nodeMaterialsBuffer);
    SAFE_DELETE(_nodeMatricesBuffer);
}

void GFXDevice::closeRenderer(){
    RemoveResource(_HIZConstructProgram);
    RemoveResource(_depthRangesConstructProgram);
    PRINT_FN(Locale::get("STOP_POST_FX"));
    _postFX.destroyInstance();
    _shaderManager.Destroy();
    PRINT_FN(Locale::get("CLOSING_RENDERER"));
    SAFE_DELETE(_renderer);
}

void GFXDevice::idle() {
    if (!_renderer) return;

    _cachedSceneZPlanes.set(ParamHandler::getInstance().getParam<F32>("rendering.zNear"), 
                            ParamHandler::getInstance().getParam<F32>("rendering.zFar"));

    _postFX.idle();
    _shaderManager.idle();
    _api.idle();
}

void GFXDevice::beginFrame()    {
    _api.beginFrame();
    setStateBlock(_defaultStateBlockHash);
}

void GFXDevice::endFrame()      { 
    //Remove dead primitives in 3 steps (or we could automate this with shared_ptr?):
    //1) Partition the vector in 2 parts: valid objects first, zombie objects second
    vectorImpl<IMPrimitive* >::iterator zombie = std::partition(_imInterfaces.begin(),
                                                                _imInterfaces.end(),
                                                                [](IMPrimitive* const priv){
                                                                    if(!priv->_canZombify) return true;
                                                                    return priv->zombieCounter() < IM_MAX_FRAMES_ZOMBIE_COUNT;
                                                                });
    //2) For every zombie object, free the memory it's using
    for( vectorImpl<IMPrimitive *>::iterator i = zombie ; i != _imInterfaces.end() ; ++i )
        SAFE_DELETE(*i);

    //3) Remove all the zombie objects once the memory is freed
    _imInterfaces.erase(zombie, _imInterfaces.end());

    FRAME_COUNT++;
    FRAME_DRAW_CALLS_PREV = FRAME_DRAW_CALLS;
    FRAME_DRAW_CALLS = 0;

    _api.endFrame();  
}