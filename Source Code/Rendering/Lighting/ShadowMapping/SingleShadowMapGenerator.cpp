#include "stdafx.h"

#include "Headers/SingleShadowMapGenerator.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/Configuration.h"
#include "Core/Headers/StringHelper.h"

#include "Scenes/Headers/SceneState.h"
#include "Managers/Headers/SceneManager.h"
#include "Managers/Headers/RenderPassManager.h"

#include "Rendering/Headers/Renderer.h"
#include "Rendering/Camera/Headers/Camera.h"

#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Textures/Headers/Texture.h"
#include "Geometry/Shapes/Predefined/Headers/Quad3D.h"
#include "Platform/Video/Shaders/Headers/ShaderProgram.h"

#include "ECS/Components/Headers/TransformComponent.h"
#include "ECS/Components/Headers/SpotLightComponent.h"

namespace Divide {

namespace {
    const RenderTargetID g_depthMapID(RenderTargetUsage::SHADOW, to_base(ShadowType::SINGLE));
    Configuration::Rendering::ShadowMapping g_shadowSettings;
};

SingleShadowMapGenerator::SingleShadowMapGenerator(GFXDevice& context)
    : ShadowMapGenerator(context, ShadowType::SINGLE) {
    Console::printfn(Locale::get(_ID("LIGHT_CREATE_SHADOW_FB")), "Single Shadow Map");

    g_shadowSettings = context.context().config().rendering.shadowMapping;
    {
        ShaderModuleDescriptor vertModule = {};
        vertModule._moduleType = ShaderType::VERTEX;
        vertModule._sourceFile = "baseVertexShaders.glsl";
        vertModule._variant = "FullScreenQuad";

        ShaderModuleDescriptor geomModule = {};
        geomModule._moduleType = ShaderType::GEOMETRY;
        geomModule._sourceFile = "blur.glsl";
        geomModule._variant = "GaussBlur";
        geomModule._defines.emplace_back(Util::StringFormat("GS_MAX_INVOCATIONS %d", 1).c_str(), true);

        ShaderModuleDescriptor fragModule = {};
        fragModule._moduleType = ShaderType::FRAGMENT;
        fragModule._sourceFile = "blur.glsl";
        fragModule._variant = "GaussBlur";

        ShaderProgramDescriptor shaderDescriptor = {};
        shaderDescriptor._modules.push_back(vertModule);
        shaderDescriptor._modules.push_back(geomModule);
        shaderDescriptor._modules.push_back(fragModule);

        ResourceDescriptor blurDepthMapShader(Util::StringFormat("GaussBlur_%d_invocations", 1).c_str());
        blurDepthMapShader.waitForReady(true);
        blurDepthMapShader.propertyDescriptor(shaderDescriptor);

        _blurDepthMapShader = CreateResource<ShaderProgram>(context.parent().resourceCache(), blurDepthMapShader);
        _blurDepthMapShader->addStateCallback(ResourceState::RES_LOADED, [this](CachedResource* res) {
            PipelineDescriptor pipelineDescriptor = {};
            pipelineDescriptor._stateHash = _context.get2DStateBlock();
            pipelineDescriptor._shaderProgramHandle = _blurDepthMapShader->getGUID();
            _blurPipeline = _context.newPipeline(pipelineDescriptor);
        });
    }

    PipelineDescriptor pipelineDescriptor = {};
    pipelineDescriptor._stateHash = _context.get2DStateBlock();

    _shaderConstants.set(_ID("layerCount"), GFX::PushConstantType::INT, 1);
    _shaderConstants.set(_ID("layerOffsetRead"), GFX::PushConstantType::INT, (I32)0);
    _shaderConstants.set(_ID("layerOffsetWrite"), GFX::PushConstantType::INT, (I32)0);

    std::array<vec2<F32>, Config::Lighting::MAX_CSM_SPLITS_PER_LIGHT> blurSizes;
    blurSizes.fill(vec2<F32>(1.0f / g_shadowSettings.spot.shadowMapResolution));

    for (U8 i = 1; i < Config::Lighting::MAX_CSM_SPLITS_PER_LIGHT; ++i) {
        blurSizes[i] = blurSizes[i - 1] / 2;
    }

    _shaderConstants.set(_ID("blurSizes"), GFX::PushConstantType::VEC2, blurSizes);
    SamplerDescriptor sampler = {};
    sampler.wrapU(TextureWrap::CLAMP_TO_EDGE);
    sampler.wrapV(TextureWrap::CLAMP_TO_EDGE);
    sampler.wrapW(TextureWrap::CLAMP_TO_EDGE);
    sampler.minFilter(TextureFilter::LINEAR);
    sampler.magFilter(TextureFilter::LINEAR);
    sampler.anisotropyLevel(0);

    const RenderTarget& rt = _context.renderTargetPool().renderTarget(g_depthMapID);
    const TextureDescriptor& texDescriptor = rt.getAttachment(RTAttachmentType::Colour, 0).texture()->descriptor();

    //Draw FBO
    {
        RenderTargetDescriptor desc = {};
        desc._resolution = rt.getResolution();

        TextureDescriptor colourDescriptor(TextureType::TEXTURE_2D_MS, texDescriptor.baseFormat(), texDescriptor.dataType());
        colourDescriptor.samplerDescriptor(sampler);
        colourDescriptor.msaaSamples(g_shadowSettings.spot.MSAASamples);

        TextureDescriptor depthDescriptor(TextureType::TEXTURE_2D_MS, GFXImageFormat::DEPTH_COMPONENT, GFXDataFormat::UNSIGNED_INT);
        depthDescriptor.samplerDescriptor(sampler);
        depthDescriptor.msaaSamples(g_shadowSettings.spot.MSAASamples);

        vectorEASTL<RTAttachmentDescriptor> att = {
            { colourDescriptor, RTAttachmentType::Colour },
            { depthDescriptor, RTAttachmentType::Depth }
        };

        desc._name = "Single_ShadowMap_Draw";
        desc._attachmentCount = to_U8(att.size());
        desc._attachments = att.data();
        desc._msaaSamples = g_shadowSettings.spot.MSAASamples;

        _drawBufferDepth = context.renderTargetPool().allocateRT(desc);
    }

    //Blur FBO
    {
        TextureDescriptor blurMapDescriptor(TextureType::TEXTURE_2D, texDescriptor.baseFormat(), texDescriptor.dataType());
        blurMapDescriptor.layerCount(1);
        blurMapDescriptor.samplerDescriptor(sampler);

        vectorEASTL<RTAttachmentDescriptor> att = {
            { blurMapDescriptor, RTAttachmentType::Colour }
        };

        RenderTargetDescriptor desc = {};
        desc._name = "Single_Blur";
        desc._resolution = rt.getResolution();
        desc._attachmentCount = to_U8(att.size());
        desc._attachments = att.data();

        _blurBuffer = _context.renderTargetPool().allocateRT(desc);
    }

    WAIT_FOR_CONDITION(_blurPipeline != nullptr);
}

SingleShadowMapGenerator::~SingleShadowMapGenerator()
{
    _context.renderTargetPool().deallocateRT(_drawBufferDepth);
}

void SingleShadowMapGenerator::render(const Camera& playerCamera, Light& light, U32 lightIndex, GFX::CommandBuffer& bufferInOut) {
    ACKNOWLEDGE_UNUSED(playerCamera);
    SpotLightComponent& spotLight = static_cast<SpotLightComponent&>(light);

    const vec3<F32> lightPos = light.positionCache();
    const F32 farPlane = light.range() * 1.2f;

    auto& shadowCameras = ShadowMap::shadowCameras(ShadowType::SINGLE);
    const mat4<F32> viewMatrix = shadowCameras[0]->lookAt(lightPos, lightPos + light.directionCache() * farPlane);
    const mat4<F32> projectionMatrix = shadowCameras[0]->setProjection(1.0f, 90.0f, vec2<F32>(0.01f, farPlane));
    shadowCameras[0]->updateLookAt();

    mat4<F32>& lightVP = light.getShadowVPMatrix(0);
    mat4<F32>::Multiply(viewMatrix, projectionMatrix, lightVP);
    light.setShadowLightPos(0, lightPos);
    light.setShadowFloatValue(0, shadowCameras[0]->getZPlanes().y);
    light.setShadowVPMatrix(0, mat4<F32>::Multiply(lightVP, MAT4_BIAS));

    RenderPassManager::PassParams params = {};
    params._sourceNode = &light.getSGN();
    params._camera = shadowCameras[0];
    params._stagePass = { RenderStage::SHADOW, RenderPassType::COUNT, to_U8(light.getLightType()), lightIndex };
    params._target = _drawBufferDepth._targetID;
    params._passName = "SingleShadowMap";
    params._bindTargets = false;
    params._minLoD = -1;

    GFX::BeginRenderPassCommand beginRenderPassCmd = {};
    beginRenderPassCmd._target = params._target;
    beginRenderPassCmd._name = "DO_SINGLE_SHADOW_MAP";
    beginRenderPassCmd._descriptor = {};
    GFX::EnqueueCommand(bufferInOut, beginRenderPassCmd);

    RTClearDescriptor clearDescriptor = {};
    clearDescriptor.clearDepth(true);
    clearDescriptor.clearColours(true);
    clearDescriptor.resetToDefault(true);

    GFX::ClearRenderTargetCommand clearMainTarget = {};
    clearMainTarget._target = params._target;
    clearMainTarget._descriptor = clearDescriptor;
    GFX::EnqueueCommand(bufferInOut, clearMainTarget);

    _context.parent().renderPassManager()->doCustomPass(params, bufferInOut);

    GFX::EnqueueCommand(bufferInOut, GFX::EndRenderPassCommand{});

    postRender(spotLight, bufferInOut);
}

void SingleShadowMapGenerator::postRender(const SpotLightComponent& light, GFX::CommandBuffer& bufferInOut) {
    const RenderTarget& shadowMapRT = _context.renderTargetPool().renderTarget(g_depthMapID);
    const U16 layerOffset = light.getShadowOffset();
    const I32 layerCount = 1;

    GFX::BlitRenderTargetCommand blitRenderTargetCommand = {};
    blitRenderTargetCommand._source = _drawBufferDepth._targetID;
    blitRenderTargetCommand._destination = g_depthMapID;
    blitRenderTargetCommand._blitColours[0].set(0u, 0u, 0u, layerOffset);
    GFX::EnqueueCommand(bufferInOut, blitRenderTargetCommand);

    // Now we can either blur our target or just skip to mipmap computation
    if (g_shadowSettings.spot.enableBlurring) {
        _shaderConstants.set(_ID("layerCount"), GFX::PushConstantType::INT, layerCount);

        GenericDrawCommand pointsCmd = {};
        pointsCmd._primitiveType = PrimitiveType::API_POINTS;
        pointsCmd._drawCount = 1;

        GFX::DrawCommand drawCmd = { pointsCmd };
        GFX::BindDescriptorSetsCommand descriptorSetCmd = {};
        GFX::BeginRenderPassCommand beginRenderPassCmd = {};
        GFX::SendPushConstantsCommand pushConstantsCommand = {};

        // Blur horizontally
        beginRenderPassCmd._target = _blurBuffer._targetID;
        beginRenderPassCmd._name = "DO_CSM_BLUR_PASS_HORIZONTAL";

        GFX::EnqueueCommand(bufferInOut, beginRenderPassCmd);

        GFX::EnqueueCommand(bufferInOut, GFX::BindPipelineCommand{ _blurPipeline });

        TextureData texData = shadowMapRT.getAttachment(RTAttachmentType::Colour, 0).texture()->data();
        descriptorSetCmd._set._textureData.setTexture(texData, TextureUsage::UNIT0);
        GFX::EnqueueCommand(bufferInOut, descriptorSetCmd);

        _shaderConstants.set(_ID("layered"), GFX::PushConstantType::BOOL, true);
        _shaderConstants.set(_ID("verticalBlur"), GFX::PushConstantType::BOOL, false);
        _shaderConstants.set(_ID("layerOffsetRead"), GFX::PushConstantType::INT, layerOffset);
        _shaderConstants.set(_ID("layerOffsetWrite"), GFX::PushConstantType::INT, 0);

        pushConstantsCommand._constants = _shaderConstants;
        GFX::EnqueueCommand(bufferInOut, pushConstantsCommand);

        GFX::EnqueueCommand(bufferInOut, drawCmd);

        GFX::EnqueueCommand(bufferInOut, GFX::EndRenderPassCommand{});

        // Blur vertically
        texData = _blurBuffer._rt->getAttachment(RTAttachmentType::Colour, 0).texture()->data();
        descriptorSetCmd._set._textureData.setTexture(texData, TextureUsage::UNIT0);
        GFX::EnqueueCommand(bufferInOut, descriptorSetCmd);

        beginRenderPassCmd._target = g_depthMapID;
        beginRenderPassCmd._descriptor = {};
        beginRenderPassCmd._name = "DO_CSM_BLUR_PASS_VERTICAL";
        GFX::EnqueueCommand(bufferInOut, beginRenderPassCmd);

        pushConstantsCommand._constants.set(_ID("verticalBlur"), GFX::PushConstantType::BOOL, true);
        pushConstantsCommand._constants.set(_ID("layerOffsetRead"), GFX::PushConstantType::INT, 0);
        pushConstantsCommand._constants.set(_ID("layerOffsetWrite"), GFX::PushConstantType::INT, layerOffset);

        GFX::EnqueueCommand(bufferInOut, pushConstantsCommand);

        GFX::EnqueueCommand(bufferInOut, drawCmd);

        GFX::EnqueueCommand(bufferInOut, GFX::EndRenderPassCommand{});
    }

    GFX::ComputeMipMapsCommand computeMipMapsCommand = {};
    computeMipMapsCommand._texture = shadowMapRT.getAttachment(RTAttachmentType::Colour, 0).texture().get();
    computeMipMapsCommand._layerRange = vec2<U32>(light.getShadowOffset(), layerCount);
    computeMipMapsCommand._defer = false;
    GFX::EnqueueCommand(bufferInOut, computeMipMapsCommand);
}

void SingleShadowMapGenerator::updateMSAASampleCount(U8 sampleCount) {
    if (_context.context().config().rendering.shadowMapping.spot.MSAASamples != sampleCount) {
        _context.context().config().rendering.shadowMapping.spot.MSAASamples = sampleCount;
        _drawBufferDepth._rt->updateSampleCount(sampleCount);
    }
}

};