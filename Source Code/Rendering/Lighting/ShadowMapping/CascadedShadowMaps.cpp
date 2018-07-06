#include "Headers/CascadedShadowMaps.h"

#include "Core/Headers/ParamHandler.h"

#include "Scenes/Headers/SceneState.h"

#include "Rendering/Headers/Renderer.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Rendering/Lighting/Headers/Light.h"

#include "Managers/Headers/SceneManager.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"

#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Shaders/Headers/ShaderProgram.h"
#include "Platform/Video/Buffers/ShaderBuffer/Headers/ShaderBuffer.h"

namespace Divide {

CascadedShadowMaps::CascadedShadowMaps(Light* light, Camera* shadowCamera, U8 numSplits)
    : ShadowMap(light, shadowCamera, ShadowType::LAYERED)
{
    _dirLight = dynamic_cast<DirectionalLight*>(_light);
    _splitLogFactor = 0.0f;
    _nearClipOffset = 0.0f;
    _numSplits = numSplits;
    _splitDepths.resize(_numSplits + 1);
    _frustumCornersWS.resize(8);
    _frustumCornersVS.resize(8);
    _frustumCornersLS.resize(8);
    _splitFrustumCornersVS.resize(8);
    _horizBlur = 0;
    _vertBlur = 0;
    _renderPolicy = MemoryManager_NEW RTDrawDescriptor(RenderTarget::defaultPolicy());
    _renderPolicy->_clearDepthBufferOnBind = true;
    _renderPolicy->_clearColourBuffersOnBind = true;

    ResourceDescriptor shadowPreviewShader("fbPreview.Layered.LinearDepth.ESM.ScenePlanes");
    shadowPreviewShader.setThreadedLoading(false);
    shadowPreviewShader.setPropertyList("USE_SCENE_ZPLANES");
    _previewDepthMapShader = CreateResource<ShaderProgram>(shadowPreviewShader);
    _previewDepthMapShader->Uniform("useScenePlanes", false);

    ResourceDescriptor blurDepthMapShader("blur.GaussBlur");
    blurDepthMapShader.setThreadedLoading(false);
    _blurDepthMapShader = CreateResource<ShaderProgram>(blurDepthMapShader);
    _blurDepthMapShader->Uniform("layerTotalCount", (I32)_numSplits);
    _blurDepthMapShader->Uniform("layerCount",  (I32)_numSplits);

    Console::printfn(Locale::get(_ID("LIGHT_CREATE_SHADOW_FB")), light->getGUID(), "EVCSM");

    SamplerDescriptor blurMapSampler;
    blurMapSampler.setFilters(TextureFilter::LINEAR);
    blurMapSampler.setWrapMode(TextureWrap::CLAMP_TO_EDGE);
    blurMapSampler.setAnisotropy(0);
    blurMapSampler.toggleMipMaps(false);
    TextureDescriptor blurMapDescriptor(TextureType::TEXTURE_2D_ARRAY,
                                        GFXImageFormat::RG32F,
                                        GFXDataFormat::FLOAT_32);
    blurMapDescriptor.setLayerCount(Config::Lighting::MAX_SPLITS_PER_LIGHT);
    blurMapDescriptor.setSampler(blurMapSampler);
    
    _blurBuffer = GFX_DEVICE.newRT(false);
    _blurBuffer->addAttachment(blurMapDescriptor,
                               RTAttachment::Type::Colour, 0);
    _blurBuffer->setClearColour(RTAttachment::Type::COUNT, 0, DefaultColours::WHITE());

    _shadowMatricesBuffer = GFX_DEVICE.newSB(1, false, false, BufferUpdateFrequency::OFTEN);
    _shadowMatricesBuffer->create(Config::Lighting::MAX_SPLITS_PER_LIGHT, sizeof(mat4<F32>));

    STUBBED("Migrate to this: http://www.ogldev.org/www/tutorial49/tutorial49.html");
}

CascadedShadowMaps::~CascadedShadowMaps()
{
    MemoryManager::DELETE(_blurBuffer);
    MemoryManager::DELETE(_renderPolicy);
    MemoryManager::DELETE(_shadowMatricesBuffer);
}

void CascadedShadowMaps::init(ShadowMapInfo* const smi) {
    _numSplits = smi->numLayers();
    RenderTarget* depthMap = getDepthMap();
    _blurBuffer->create(depthMap->getWidth(), depthMap->getHeight());
    _horizBlur = _blurDepthMapShader->GetSubroutineIndex(ShaderType::GEOMETRY, "computeCoordsH");
    _vertBlur = _blurDepthMapShader->GetSubroutineIndex(ShaderType::GEOMETRY, "computeCoordsV");

    vectorImpl<vec2<F32>> blurSizes(_numSplits);
    blurSizes[0].set(1.0f / (depthMap->getWidth() * 1.0f), 1.0f / (depthMap->getHeight() * 1.0f));
    for (int i = 1; i < _numSplits; ++i) {
        blurSizes[i].set(blurSizes[i - 1] / 2.0f);
    }

    _blurDepthMapShader->Uniform("blurSizes", blurSizes);

    _init = true;
}

void CascadedShadowMaps::render(SceneRenderState& renderState) {
    _splitLogFactor = _dirLight->csmSplitLogFactor();
    _nearClipOffset = _dirLight->csmNearClipOffset();
    _lightPosition.set(_light->getPosition());

    Camera& camera = renderState.getCamera();
    _sceneZPlanes.set(camera.getZPlanes());
    camera.getWorldMatrix(_viewInvMatrixCache);
    camera.getFrustum().getCornersWorldSpace(_frustumCornersWS);
    camera.getFrustum().getCornersViewSpace(_frustumCornersVS);

    calculateSplitDepths(camera);
    applyFrustumSplits();

    /*const RenderPassCuller::VisibleNodeList& nodes = 
        SceneManager::instance().cullSceneGraph(RenderStage::SHADOW);

    _previousFrustumBB.reset();
    for (SceneGraphNode_wptr node_wptr : nodes) {
        SceneGraphNode_ptr node_ptr = node_wptr.lock();
        if (node_ptr) {
            _previousFrustumBB.add(node_ptr->getBoundingBoxConst());
        }
    }
    _previousFrustumBB.transformHomogeneous(_shadowCamera->getViewMatrix());*/

    _shadowMatricesBuffer->setData(_shadowMatrices.data());
    _shadowMatricesBuffer->bind(ShaderBufferLocation::LIGHT_SHADOW);
    _shadowMatricesBuffer->incQueue();

    renderState.getCameraMgr().pushActiveCamera(_shadowCamera);
    getDepthMap()->begin(*_renderPolicy);
        SceneManager::instance().renderVisibleNodes(RenderStage::SHADOW, true);
    getDepthMap()->end();
    renderState.getCameraMgr().popActiveCamera();

    postRender();
}

void CascadedShadowMaps::calculateSplitDepths(const Camera& cam) {
    const mat4<F32>& projMatrixCache = cam.getProjectionMatrix();

    F32 fd = _sceneZPlanes.y;//std::min(_sceneZPlanes.y, _previousFrustumBB.getExtent().z);
    F32 nd = _sceneZPlanes.x;//std::max(_sceneZPlanes.x, std::fabs(_previousFrustumBB.nearestDistanceFromPoint(cam.getEye())));

    F32 i_f = 1.0f, cascadeCount = (F32)_numSplits;
    _splitDepths[0] = nd;
    for (U32 i = 1; i < to_uint(_numSplits); ++i, i_f += 1.0f)
    {
        _splitDepths[i] = Lerp(nd + (i_f / cascadeCount)*(fd - nd),  nd * std::powf(fd / nd, i_f / cascadeCount), _splitLogFactor);
    }
    _splitDepths[_numSplits] = fd;

    for (U8 i = 0; i < _numSplits; ++i) {
        F32 crtSplitDepth = 0.5f * (-_splitDepths[i + 1] * projMatrixCache.mat[10] + projMatrixCache.mat[14]) / _splitDepths[i + 1] + 0.5f;
       _light->setShadowFloatValue(i, crtSplitDepth);
    }
}

void CascadedShadowMaps::applyFrustumSplits() {
    for (U8 pass = 0 ; pass < _numSplits; ++pass) {
        F32 minZ = _splitDepths[pass];
        F32 maxZ = _splitDepths[pass + 1];

        for (U8 i = 0; i < 4; ++i) {
            _splitFrustumCornersVS[i] =
                _frustumCornersVS[i + 4] * (minZ / _sceneZPlanes.y);
        }

        for (U8 i = 4; i < 8; ++i) {
            _splitFrustumCornersVS[i] =
                _frustumCornersVS[i] * (maxZ / _sceneZPlanes.y);
        }

        for (U8 i = 0; i < 8; ++i) {
            _frustumCornersWS[i].set(
                _viewInvMatrixCache.transformHomogeneous(_splitFrustumCornersVS[i]));
        }

        vec3<F32> frustumCentroid(0.0f);

        for (U8 i = 0; i < 8; ++i) {
            frustumCentroid += _frustumCornersWS[i];
        }
        // Find the centroid
        frustumCentroid /= 8;

        // Position the shadow-caster camera so that it's looking at the centroid,
        // and backed up in the direction of the sunlight
        F32 distFromCentroid = std::max((maxZ - minZ),
                     _splitFrustumCornersVS[4].distance(_splitFrustumCornersVS[5])) + _nearClipOffset;
    
        vec3<F32> currentEye = frustumCentroid - (_lightPosition * distFromCentroid);

        const mat4<F32>& viewMatrix = _shadowCamera->lookAt(currentEye, frustumCentroid);
        // Determine the position of the frustum corners in light space
        for (U8 i = 0; i < 8; ++i) {
            _frustumCornersLS[i].set(viewMatrix.transformHomogeneous(_frustumCornersWS[i]));
        }

        F32 frustumSphereRadius = BoundingSphere(_frustumCornersLS).getRadius();
        vec2<F32> clipPlanes(std::max(1.0f, minZ - _nearClipOffset), frustumSphereRadius * 1.75f + _nearClipOffset);
        _shadowCamera->setProjection(UNIT_RECT * frustumSphereRadius,
                                     clipPlanes,
                                     true);
        _shadowMatrices[pass].set(viewMatrix * _shadowCamera->getProjectionMatrix());

        // http://www.gamedev.net/topic/591684-xna-40---shimmering-shadow-maps/
        F32 halfShadowMapSize = (getDepthMap()->getWidth())*0.5f;
        vec3<F32> testPoint = _shadowMatrices[pass].transformHomogeneous(VECTOR3_ZERO) * halfShadowMapSize;
        vec3<F32> testPointRounded(testPoint);
        testPointRounded.round();
        _shadowMatrices[pass].translate(vec3<F32>(((testPointRounded - testPoint) / halfShadowMapSize).xy(), 0.0f));

        _light->setShadowVPMatrix(pass, _shadowMatrices[pass] * MAT4_BIAS);
        _light->setShadowLightPos(pass, currentEye);
    }
}

void CascadedShadowMaps::postRender() {
    if (GFX_DEVICE.shadowDetailLevel() == RenderDetailLevel::LOW) {
        return;
    }

    _blurDepthMapShader->bind();

    GenericDrawCommand pointsCmd;
    pointsCmd.primitiveType(PrimitiveType::API_POINTS);
    pointsCmd.drawCount(1);
    pointsCmd.stateHash(GFX_DEVICE.getDefaultStateBlock(true));
    pointsCmd.shaderProgram(_blurDepthMapShader);

    // Blur horizontally
    _blurDepthMapShader->Uniform("layerOffsetRead", (I32)getArrayOffset());
    _blurDepthMapShader->Uniform("layerOffsetWrite", 0);

    _blurDepthMapShader->SetSubroutine(ShaderType::GEOMETRY, _horizBlur);
    _blurBuffer->begin(RenderTarget::defaultPolicy());
    getDepthMap()->bind(0, RTAttachment::Type::Colour, 0, false);
        GFX_DEVICE.draw(pointsCmd);
    getDepthMap()->end();

    // Blur vertically
    _blurDepthMapShader->Uniform("layerOffsetRead", 0);
    _blurDepthMapShader->Uniform("layerOffsetWrite", (I32)getArrayOffset());

    _blurDepthMapShader->SetSubroutine(ShaderType::GEOMETRY, _vertBlur);
    getDepthMap()->begin(RenderTarget::defaultPolicy());
    _blurBuffer->bind(0, RTAttachment::Type::Colour, 0);
        GFX_DEVICE.draw(pointsCmd);
    getDepthMap()->end();
}

void CascadedShadowMaps::previewShadowMaps(U32 rowIndex) {
    if (_previewDepthMapShader->getState() != ResourceState::RES_LOADED) {
        return;
    }


    getDepthMap()->bind(0, RTAttachment::Type::Colour, 0);

    GenericDrawCommand triangleCmd;
    triangleCmd.primitiveType(PrimitiveType::TRIANGLES);
    triangleCmd.drawCount(1);
    triangleCmd.stateHash(GFX_DEVICE.getDefaultStateBlock(true));
    triangleCmd.shaderProgram(_previewDepthMapShader);

    const vec4<I32> viewport = getViewportForRow(rowIndex);
    for (U32 i = 0; i < _numSplits; ++i) {
        _previewDepthMapShader->Uniform("layer", i + _arrayOffset);
        GFX::ScopedViewport sViewport(viewport.x * i, viewport.y, viewport.z, viewport.w);
        GFX_DEVICE.draw(triangleCmd);
    }
}
};