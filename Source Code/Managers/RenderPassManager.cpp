#include "stdafx.h"

#include "Headers/RenderPassManager.h"

#include "Core/Headers/TaskPool.h"
#include "Core/Headers/StringHelper.h"
#include "Managers/Headers/SceneManager.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Rendering/RenderPass/Headers/RenderQueue.h"
#include "Platform/Video/Textures/Headers/Texture.h"

#ifndef USE_COLOUR_WOIT
//#define USE_COLOUR_WOIT
#endif

namespace Divide {

RenderPassManager::RenderPassManager(Kernel& parent, GFXDevice& context)
    : KernelComponent(parent),
      _context(context),
      _renderQueue(nullptr),
      _OITCompositionShader(nullptr)
{
}

RenderPassManager::~RenderPassManager()
{
    destroy();
}

bool RenderPassManager::init() {
    if (_renderQueue == nullptr) {
        _renderQueue = MemoryManager_NEW RenderQueue(_context);

        ResourceDescriptor shaderDesc("OITComposition");
        _OITCompositionShader = CreateResource<ShaderProgram>(parent().resourceCache(), shaderDesc);
        return true;
    }

    return false;
}

void RenderPassManager::destroy() {
    MemoryManager::DELETE_VECTOR(_renderPasses);
    MemoryManager::DELETE(_renderQueue);
    _OITCompositionShader.reset();
}

void RenderPassManager::render(SceneRenderState& sceneRenderState) {
    for (RenderPass* rp : _renderPasses) {
        rp->render(sceneRenderState);
    }
}

RenderPass& RenderPassManager::addRenderPass(const stringImpl& renderPassName,
                                             U8 orderKey,
                                             RenderStage renderStage) {
    assert(!renderPassName.empty());

    _renderPasses.emplace_back(MemoryManager_NEW RenderPass(*this, _context, renderPassName, orderKey, renderStage));
    RenderPass& item = *_renderPasses.back();

    std::sort(std::begin(_renderPasses), std::end(_renderPasses),
              [](RenderPass* a, RenderPass* b)
                  -> bool { return a->sortKey() < b->sortKey(); });

    assert(item.sortKey() == orderKey);

    return item;
}

void RenderPassManager::removeRenderPass(const stringImpl& name) {
    for (vector<RenderPass*>::iterator it = std::begin(_renderPasses);
         it != std::end(_renderPasses); ++it) {
        if ((*it)->name().compare(name) == 0) {
            _renderPasses.erase(it);
            break;
        }
    }
}

U16 RenderPassManager::getLastTotalBinSize(RenderStage renderStage) const {
    return getPassForStage(renderStage)->getLastTotalBinSize();
}

RenderPass* RenderPassManager::getPassForStage(RenderStage renderStage) const {
    for (RenderPass* pass : _renderPasses) {
        if (pass->stageFlag() == renderStage) {
            return pass;
        }
    }

    DIVIDE_UNEXPECTED_CALL();
    return nullptr;
}

const RenderPass::BufferData& 
RenderPassManager::getBufferData(RenderStage renderStage, I32 bufferIndex) const {
    return getPassForStage(renderStage)->getBufferData(bufferIndex);
}

RenderPass::BufferData&
RenderPassManager::getBufferData(RenderStage renderStage, I32 bufferIndex) {
    return getPassForStage(renderStage)->getBufferData(bufferIndex);
}

void RenderPassManager::prePass(const PassParams& params, const RenderTarget& target, GFX::CommandBuffer& bufferInOut) {
    static const vector<RenderBinType> depthExclusionList
    {
        RenderBinType::RBT_DECAL,
        RenderBinType::RBT_TRANSLUCENT
    };

    GFX::BeginDebugScopeCommand beginDebugScopeCmd;
    beginDebugScopeCmd._scopeID = 0;
    beginDebugScopeCmd._scopeName = Util::StringFormat("Custom pass ( %s ): PrePass", TypeUtil::renderStageToString(params._stage)).c_str();
    GFX::EnqueueCommand(bufferInOut, beginDebugScopeCmd);

    // PrePass requires a depth buffer
    bool doPrePass = params._doPrePass && target.getAttachment(RTAttachmentType::Depth, 0).used();

    if (doPrePass) {
        SceneManager& sceneManager = parent().sceneManager();

        RenderStagePass stagePass(params._stage, RenderPassType::DEPTH_PASS);

        Attorney::SceneManagerRenderPass::populateRenderQueue(sceneManager,
                                                              stagePass,
                                                              *params._camera,
                                                              true,
                                                              params._pass);

        if (params._target._usage != RenderTargetUsage::COUNT) {
           
            if (params._bindTargets) {
                GFX::BeginRenderPassCommand beginRenderPassCommand;
                beginRenderPassCommand._target = params._target;
                beginRenderPassCommand._descriptor = RenderTarget::defaultPolicyDepthOnly();
                beginRenderPassCommand._name = "DO_PRE_PASS";
                GFX::EnqueueCommand(bufferInOut, beginRenderPassCommand);
            }

            for (RenderBinType type : RenderBinType::_values()) {
                if (std::find(std::begin(depthExclusionList),
                    std::end(depthExclusionList),
                    type) == std::cend(depthExclusionList))
                {
                    if (type._value != RenderBinType::RBT_TRANSLUCENT) {
                        _context.renderQueueToSubPasses(type, bufferInOut);
                    }
                }
            }

            Attorney::SceneManagerRenderPass::postRender(sceneManager, stagePass, *params._camera, bufferInOut);

            if (params._bindTargets) {
                GFX::EndRenderPassCommand endRenderPassCommand;
                GFX::EnqueueCommand(bufferInOut, endRenderPassCommand);
            }
        }
        
    }

    GFX::EndDebugScopeCommand endDebugScopeCmd;
    GFX::EnqueueCommand(bufferInOut, endDebugScopeCmd);
}

void RenderPassManager::mainPass(const PassParams& params, RenderTarget& target, GFX::CommandBuffer& bufferInOut) {
    static const vector<RenderBinType> shadowExclusionList
    {
        RenderBinType::RBT_DECAL,
        RenderBinType::RBT_SKY,
        RenderBinType::RBT_TRANSLUCENT
    };

    GFX::BeginDebugScopeCommand beginDebugScopeCmd;
    beginDebugScopeCmd._scopeID = 1;
    beginDebugScopeCmd._scopeName = Util::StringFormat("Custom pass ( %s ): RenderPass", TypeUtil::renderStageToString(params._stage)).c_str();
    GFX::EnqueueCommand(bufferInOut, beginDebugScopeCmd);

    SceneManager& sceneManager = parent().sceneManager();

    RenderStagePass stagePass(params._stage, RenderPassType::COLOUR_PASS);
    Attorney::SceneManagerRenderPass::populateRenderQueue(sceneManager,
                                                          stagePass,
                                                          *params._camera,
                                                          !params._doPrePass,
                                                          params._pass);

    if (params._target._usage != RenderTargetUsage::COUNT) {
        bool drawToDepth = true;
        if (params._stage != RenderStage::SHADOW) {
            Attorney::SceneManagerRenderPass::preRender(sceneManager, stagePass, *params._camera, target, bufferInOut);
            if (params._doPrePass) {
                drawToDepth = Config::DEBUG_HIZ_CULLING;
            }
        }

        if (params._stage == RenderStage::DISPLAY) {
            GFX::BindDescriptorSetsCommand bindDescriptorSets;
            bindDescriptorSets._set = _context.newDescriptorSet();
            // Bind the depth buffers
            TextureData depthBufferTextureData = target.getAttachment(RTAttachmentType::Depth, 0).texture()->getData();
            bindDescriptorSets._set->_textureData.addTexture(depthBufferTextureData, to_U8(ShaderProgram::TextureUsage::DEPTH));

            const RTAttachment& velocityAttachment = target.getAttachment(RTAttachmentType::Colour, to_U8(GFXDevice::ScreenTargets::VELOCITY));
            if (velocityAttachment.used()) {
                const Texture_ptr& prevDepthTexture = _context.getPrevDepthBuffer();
                TextureData prevDepthData = (prevDepthTexture ? prevDepthTexture->getData() : depthBufferTextureData);
                bindDescriptorSets._set->_textureData.addTexture(prevDepthData, to_U8(ShaderProgram::TextureUsage::DEPTH_PREV));
            }

            GFX::EnqueueCommand(bufferInOut, bindDescriptorSets);
        }

        RTDrawDescriptor& drawPolicy = 
            params._drawPolicy ? *params._drawPolicy
                               : (!Config::DEBUG_HIZ_CULLING ? RenderTarget::defaultPolicyKeepDepth()
                                                             : RenderTarget::defaultPolicy());

        drawPolicy.drawMask().setEnabled(RTAttachmentType::Depth, 0, drawToDepth);

        if (params._bindTargets) {
            GFX::BeginRenderPassCommand beginRenderPassCommand;
            beginRenderPassCommand._target = params._target;
            beginRenderPassCommand._descriptor = drawPolicy;
            beginRenderPassCommand._name = "DO_MAIN_PASS";
            GFX::EnqueueCommand(bufferInOut, beginRenderPassCommand);
        }

        if (params._stage == RenderStage::SHADOW) {
            for (RenderBinType type : RenderBinType::_values()) {
                if (std::find(std::begin(shadowExclusionList),
                              std::end(shadowExclusionList),
                              type) == std::cend(shadowExclusionList))
                {
                    if (type._value != RenderBinType::RBT_TRANSLUCENT) {
                        _context.renderQueueToSubPasses(type, bufferInOut);
                    }
                }
            }
        } else {
            for (RenderBinType type : RenderBinType::_values()) {
                if (type._value != RenderBinType::RBT_TRANSLUCENT) {
                    _context.renderQueueToSubPasses(type, bufferInOut);
                }
            }
        }

        Attorney::SceneManagerRenderPass::postRender(sceneManager, stagePass, *params._camera, bufferInOut);

        if (params._stage == RenderStage::DISPLAY) {
            /// These should be OIT rendered as well since things like debug nav meshes have translucency
            Attorney::SceneManagerRenderPass::debugDraw(sceneManager, stagePass, *params._camera, bufferInOut);
        }

        if (params._bindTargets) {
            GFX::EndRenderPassCommand endRenderPassCommand;
            GFX::EnqueueCommand(bufferInOut, endRenderPassCommand);
        }
    }

    GFX::EndDebugScopeCommand endDebugScopeCmd;
    GFX::EnqueueCommand(bufferInOut, endDebugScopeCmd);
}

void RenderPassManager::woitPass(const PassParams& params, const RenderTarget& target, GFX::CommandBuffer& bufferInOut) {
    // Weighted Blended Order Independent Transparency
    if (_context.renderQueueSize(RenderBinType::RBT_TRANSLUCENT) > 0) {
        RenderTarget& oitTarget = _context.renderTargetPool().renderTarget(RenderTargetID(RenderTargetUsage::OIT));

        // Step1: Draw translucent items into the accumulation and revealage buffers

        GFX::BeginRenderPassCommand beginRenderPassOitCmd;
        beginRenderPassOitCmd._name = "DO_OIT_PASS_1";
        beginRenderPassOitCmd._target = RenderTargetID(RenderTargetUsage::OIT);
        RTBlendState& state0 = beginRenderPassOitCmd._descriptor.blendState(0);
        RTBlendState& state1 = beginRenderPassOitCmd._descriptor.blendState(1);

        state0._blendEnable = true;
        state0._blendProperties._blendSrc = BlendProperty::ONE;
        state0._blendProperties._blendDest = BlendProperty::ONE;
        state0._blendProperties._blendOp = BlendOperation::ADD;

        state1._blendEnable = true;
        state1._blendProperties._blendSrc = BlendProperty::ZERO;
        state1._blendProperties._blendDest = BlendProperty::INV_SRC_COLOR;
        state1._blendProperties._blendOp = BlendOperation::ADD;

        // Don't clear our screen target. That would be BAD.
        beginRenderPassOitCmd._descriptor.clearColour(to_U8(GFXDevice::ScreenTargets::MODULATE), false);
        // Don't clear and don't write to depth buffer
        beginRenderPassOitCmd._descriptor.disableState(RTDrawDescriptor::State::CLEAR_DEPTH_BUFFER);
        beginRenderPassOitCmd._descriptor.drawMask().setEnabled(RTAttachmentType::Depth, 0, false);
        GFX::EnqueueCommand(bufferInOut, beginRenderPassOitCmd);

        _context.renderQueueToSubPasses(RenderBinType::RBT_TRANSLUCENT, bufferInOut);

        GFX::EndRenderPassCommand endRenderPassOitCmd;
        GFX::EnqueueCommand(bufferInOut, endRenderPassOitCmd);

        // Step2: Composition pass
        // Don't clear depth & colours and do not write to the depth buffer
        GFX::BeginRenderPassCommand beginRenderPassCompCmd;
        beginRenderPassCompCmd._name = "DO_OIT_PASS_2";
        beginRenderPassCompCmd._target = RenderTargetID(RenderTargetUsage::SCREEN);
        beginRenderPassCompCmd._descriptor.disableState(RTDrawDescriptor::State::CLEAR_DEPTH_BUFFER);
        beginRenderPassCompCmd._descriptor.disableState(RTDrawDescriptor::State::CLEAR_COLOUR_BUFFERS);
        beginRenderPassCompCmd._descriptor.drawMask().setEnabled(RTAttachmentType::Depth, 0, false);
        beginRenderPassCompCmd._descriptor.blendState(0)._blendEnable = true;
        beginRenderPassCompCmd._descriptor.blendState(0)._blendProperties._blendSrc = BlendProperty::ONE;
        beginRenderPassCompCmd._descriptor.blendState(0)._blendProperties._blendDest = BlendProperty::INV_SRC_ALPHA;
        GFX::EnqueueCommand(bufferInOut, beginRenderPassCompCmd);

        GFX::BindPipelineCommand bindPipelineCmd;
        PipelineDescriptor pipelineDescriptor;
        pipelineDescriptor._stateHash = _context.get2DStateBlock();
        pipelineDescriptor._shaderProgramHandle = _OITCompositionShader->getID();
        bindPipelineCmd._pipeline = _context.newPipeline(pipelineDescriptor);
        GFX::EnqueueCommand(bufferInOut, bindPipelineCmd);

        TextureData accum = oitTarget.getAttachment(RTAttachmentType::Colour, to_U8(GFXDevice::ScreenTargets::ACCUMULATION)).texture()->getData();
        TextureData revealage = oitTarget.getAttachment(RTAttachmentType::Colour, to_U8(GFXDevice::ScreenTargets::REVEALAGE)).texture()->getData();

        GFX::BindDescriptorSetsCommand descriptorSetCmd;
        descriptorSetCmd._set = _context.newDescriptorSet();
        descriptorSetCmd._set->_textureData.addTexture(accum, 0u);
        descriptorSetCmd._set->_textureData.addTexture(revealage, 1u);
        GFX::EnqueueCommand(bufferInOut, descriptorSetCmd);

        GFX::DrawCommand drawCmd;
        GenericDrawCommand drawCommand;
        drawCommand._primitiveType = PrimitiveType::TRIANGLES;
        drawCommand._drawCount = 1;
        drawCmd._drawCommands.push_back(drawCommand);
        GFX::EnqueueCommand(bufferInOut, drawCmd);

        GFX::EndRenderPassCommand endRenderPassCompCmd;
        GFX::EnqueueCommand(bufferInOut, endRenderPassCompCmd);
    }
}

void RenderPassManager::doCustomPass(PassParams& params, GFX::CommandBuffer& bufferInOut) {
    // Tell the Rendering API to draw from our desired PoV
    GFX::SetCameraCommand setCameraCommand;
    setCameraCommand._camera = params._camera;
    GFX::EnqueueCommand(bufferInOut, setCameraCommand);

    GFX::SetClipPlanesCommand setClipPlanesCommand;
    setClipPlanesCommand._clippingPlanes = params._clippingPlanes;
    GFX::EnqueueCommand(bufferInOut, setClipPlanesCommand);

    RenderTarget& target = _context.renderTargetPool().renderTarget(params._target);
    prePass(params, target, bufferInOut);

    if (params._occlusionCull) {
        _context.constructHIZ(params._target, bufferInOut);
        _context.occlusionCull(getBufferData(params._stage, params._pass), target.getAttachment(RTAttachmentType::Depth, 0).texture(), bufferInOut);
    }

    mainPass(params, target, bufferInOut);
    woitPass(params, target, bufferInOut);
}

};