#include "Headers/ForwardPlusRenderer.h"

#include "Core/Headers/Console.h"
#include "Managers/Headers/LightManager.h"
#include "Core/Resources/Headers/ResourceCache.h"

namespace Divide {

ForwardPlusRenderer::ForwardPlusRenderer() : Renderer(RENDERER_FORWARD_PLUS)
{
    _opaqueGrid.reset(/*MemoryManager_NEW*/new LightGrid());
    _transparentGrid.reset(/*MemoryManager_NEW*/new LightGrid());
    /// Initialize our depth ranges construction shader (see LightManager.cpp for more documentation)
    ResourceDescriptor rangesDesc("DepthRangesConstruct");

    stringImpl gridDim("LIGHT_GRID_TILE_DIM_X ");
    gridDim.append(Util::toString(Config::Lighting::LIGHT_GRID_TILE_DIM_X).c_str());
    gridDim.append(",LIGHT_GRID_TILE_DIM_Y ");
    gridDim.append(Util::toString(Config::Lighting::LIGHT_GRID_TILE_DIM_Y).c_str());
    rangesDesc.setPropertyList(gridDim);

    _depthRangesConstructProgram = CreateResource<ShaderProgram>(rangesDesc);
    _depthRangesConstructProgram->UniformTexture("depthTex", 0);
    /// Depth ranges are used for grid based light culling
    SamplerDescriptor depthRangesSampler;
    depthRangesSampler.setFilters(TEXTURE_FILTER_NEAREST);
    depthRangesSampler.setWrapMode(TEXTURE_CLAMP_TO_EDGE);
    depthRangesSampler.toggleMipMaps(false);
    TextureDescriptor depthRangesDescriptor(TEXTURE_2D, RGBA32F, FLOAT_32);
    depthRangesDescriptor.setSampler(depthRangesSampler);
    // The down-sampled depth buffer is used to cull screen space lights for our Forward+ rendering algorithm. 
    // It's only updated on demand.
    _depthRanges = GFX_DEVICE.newFB(false);
    _depthRanges->AddAttachment(depthRangesDescriptor, TextureDescriptor::Color0);
    _depthRanges->toggleDepthBuffer(false);
    _depthRanges->setClearColor(vec4<F32>(0.0f, 1.0f, 0.0f, 1.0f));
    vec2<U16> screenRes = GFX_DEVICE.getRenderTarget(GFXDevice::RENDER_TARGET_DEPTH)->getResolution();
    vec2<U16> tileSize(Config::Lighting::LIGHT_GRID_TILE_DIM_X, Config::Lighting::LIGHT_GRID_TILE_DIM_Y);
    vec2<U16> resTemp(screenRes + tileSize);
    _depthRanges->Create(resTemp.x / tileSize.x - 1, resTemp.y / tileSize.y - 1);
}

ForwardPlusRenderer::~ForwardPlusRenderer()
{
    MemoryManager::DELETE(_depthRanges);
    RemoveResource(_depthRangesConstructProgram);
}

void ForwardPlusRenderer::processVisibleNodes(const vectorImpl<SceneGraphNode* >& visibleNodes,
                                              const GFXDevice::GPUBlock& gpuBlock) {
    buildLightGrid(gpuBlock);
}

void ForwardPlusRenderer::render(const DELEGATE_CBK<>& renderCallback, 
                                 const SceneRenderState& sceneRenderState) {
    renderCallback();
}

void ForwardPlusRenderer::updateResolution(U16 width, U16 height) {
    vec2<U16> tileSize(Config::Lighting::LIGHT_GRID_TILE_DIM_X, 
                       Config::Lighting::LIGHT_GRID_TILE_DIM_Y);
    vec2<U16> resTemp(GFX_DEVICE.getRenderTarget(GFXDevice::RENDER_TARGET_DEPTH)->getResolution() + 
                      tileSize);
    _depthRanges->Create(resTemp.x / tileSize.x - 1, 
                         resTemp.y / tileSize.y - 1);
}

bool ForwardPlusRenderer::buildLightGrid(const GFXDevice::GPUBlock& gpuBlock) {
    const Light::LightMap& lights = LightManager::getInstance().getLights();

    _omniLightList.clear();
    _omniLightList.reserve(static_cast<vectorAlg::vecSize>(lights.size()));

    for (const Light::LightMap::value_type& it : lights) {
        const Light& light = *it.second;
        if ( light.getLightType() == LIGHT_TYPE_POINT ) {
            _omniLightList.push_back( LightGrid::makeLight(light.getPosition(),
                                                           light.getDiffuseColor(), 
                                                           light.getRange() ) );
        }
    }

    if ( !_omniLightList.empty() ) {
        _transparentGrid->build(vec2<U16>(Config::Lighting::LIGHT_GRID_TILE_DIM_X,
                                          Config::Lighting::LIGHT_GRID_TILE_DIM_Y),
                                // render target resolution
                                vec2<U16>(gpuBlock._ViewPort.z, 
                                          gpuBlock._ViewPort.w), 
                                _omniLightList,
                                gpuBlock._ViewMatrix,
                                gpuBlock._ProjectionMatrix,
                                // current near plane
                                gpuBlock._ZPlanesCombined.x, 
                                vectorImpl<vec2<F32> >());

         
        downSampleDepthBuffer(_depthRangesCache);
        // We take a copy of this, and prune the grid using depth ranges found from pre-z pass 
        // (for opaque geometry).
        STUBBED("ADD OPTIMIZED COPY!!!");
        *_opaqueGrid = *_transparentGrid;
        // Note that the pruning does not occur if the pre-z pass was not performed
        // (depthRanges is empty in this case).
        _opaqueGrid->prune(_depthRangesCache);
        _transparentGrid->pruneFarOnly(gpuBlock._ZPlanesCombined.x, _depthRangesCache);
        return true;
    }

    return false;
}

/// Extract the depth ranges and store them in a different render target used to cull lights against
void ForwardPlusRenderer::downSampleDepthBuffer(vectorImpl<vec2<F32>> &depthRanges) {
    depthRanges.resize(_depthRanges->getWidth() * _depthRanges->getHeight());

    _depthRanges->Begin(Framebuffer::defaultPolicy());
    {
        _depthRangesConstructProgram->bind();
        _depthRangesConstructProgram->Uniform("dvd_ProjectionMatrixInverse",
                                              GFX_DEVICE.getMatrix(PROJECTION_INV_MATRIX));
        GFX_DEVICE.getRenderTarget(GFXDevice::RENDER_TARGET_DEPTH)->Bind(ShaderProgram::TEXTURE_UNIT0,
                                                                         TextureDescriptor::Depth);    
        GFX_DEVICE.drawPoints(1, GFX_DEVICE.getDefaultStateBlock(true), _depthRangesConstructProgram);

        _depthRanges->ReadData(RG, FLOAT_32, &depthRanges[0]);
    }
    _depthRanges->End();
}

};