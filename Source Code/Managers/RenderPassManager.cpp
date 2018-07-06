#include "stdafx.h"

#include "Headers/RenderPassManager.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/TaskPool.h"
#include "Core/Headers/StringHelper.h"
#include "Core/Headers/PlatformContext.h"
#include "Core/Time/Headers/ProfileTimer.h"
#include "Managers/Headers/SceneManager.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Rendering/PostFX/Headers/PostFX.h"
#include "Rendering/RenderPass/Headers/RenderQueue.h"
#include "Platform/Video/Textures/Headers/Texture.h"
#include "Platform/Video/Buffers/ShaderBuffer/Headers/ShaderBuffer.h"

#ifndef USE_COLOUR_WOIT
//#define USE_COLOUR_WOIT
#endif

namespace Divide {

namespace {
    thread_local vectorEASTL<GFXDevice::NodeData> g_nodeData;
    thread_local vectorEASTL<IndirectDrawCommand> g_drawCommands;
};

RenderPassManager::RenderPassManager(Kernel& parent, GFXDevice& context)
    : KernelComponent(parent),
     _renderQueue(parent),
     _context(context),
     _postFxRenderTimer(&Time::ADD_TIMER("PostFX Timer"))
{
    ResourceDescriptor shaderDesc("OITComposition");
    _OITCompositionShader = CreateResource<ShaderProgram>(parent.resourceCache(), shaderDesc);
    _renderPassCommandBuffer.push_back(GFX::allocateCommandBuffer(true));
}

RenderPassManager::~RenderPassManager()
{
    for (GFX::CommandBuffer*& buf : _renderPassCommandBuffer) {
        GFX::deallocateCommandBuffer(buf, true);
    }
}

void RenderPassManager::render(SceneRenderState& sceneRenderState) {
    TaskPriority priority = Config::USE_THREADED_COMMAND_GENERATION ? TaskPriority::DONT_CARE : TaskPriority::REALTIME;

    TaskPool& pool = parent().platformContext().taskPool();
    TaskHandle renderTask = CreateTask(pool, DELEGATE_CBK<void, const Task&>());

    U8 renderPassCount = to_U8(_renderPasses.size());

    for (U8 i = 0; i < renderPassCount; ++i) {
        std::shared_ptr<RenderPass>& rp = _renderPasses[i];
        GFX::CommandBuffer& buf = *_renderPassCommandBuffer[i];
        CreateTask(pool,
                   &renderTask,
                   [&rp, &buf, &sceneRenderState](const Task& parentTask) {
                       buf.clear();
                       rp->render(sceneRenderState, buf);
                       buf.batch();
                   }).startTask(priority);
    }

    CreateTask(pool,
               &renderTask,
               [this](const Task& parentTask) {
                  Time::ScopedTimer time3(*_postFxRenderTimer);
                  GFX::CommandBuffer& buf = *_renderPassCommandBuffer.back();
                  buf.clear();
                  PostFX::instance().apply(buf);
                  buf.batch();
               }).startTask(priority);

    renderTask.startTask(priority).wait();

    //ToDo: Maybe handle this differently?
#if 0 //Direct render and clear
    for (vec_size_eastl i = 0; i < _renderPassCommandBuffer.size(); ++i) {
        _context.flushCommandBuffer(*_renderPassCommandBuffer[i]);
    }
#else // Maybe better batching and validation?
    GFX::ScopedCommandBuffer sBuffer(GFX::allocateScopedCommandBuffer());
    for (vec_size_eastl i = 0; i < _renderPassCommandBuffer.size(); ++i) {
        GFX::CommandBuffer& buf = *_renderPassCommandBuffer[i];
        sBuffer().add(buf);
    }
    _context.flushCommandBuffer(sBuffer());
#endif
}

RenderPass& RenderPassManager::addRenderPass(const stringImpl& renderPassName,
                                             U8 orderKey,
                                             RenderStage renderStage) {
    assert(!renderPassName.empty());

    _renderPasses.push_back(std::make_shared<RenderPass>(*this, _context, renderPassName, orderKey, renderStage));
    //Secondary command buffers. Used in a threaded fashion
    _renderPassCommandBuffer.push_back(GFX::allocateCommandBuffer(true));

    std::shared_ptr<RenderPass>& item = _renderPasses.back();

    eastl::sort(eastl::begin(_renderPasses),
                eastl::end(_renderPasses),
                [](const std::shared_ptr<RenderPass>& a, const std::shared_ptr<RenderPass>& b) -> bool {
                      return a->sortKey() < b->sortKey();
                });

    return *item;
}

void RenderPassManager::removeRenderPass(const stringImpl& name) {
    for (vectorEASTL<std::shared_ptr<RenderPass>>::iterator it = eastl::begin(_renderPasses);
         it != eastl::end(_renderPasses); ++it) {
        if ((*it)->name().compare(name) == 0) {
            _renderPasses.erase(it);
            // Remove one command buffer
            GFX::CommandBuffer* buf = _renderPassCommandBuffer.back();
            GFX::deallocateCommandBuffer(buf, true);
            _renderPassCommandBuffer.pop_back();
            break;
        }
    }
}

U16 RenderPassManager::getLastTotalBinSize(RenderStage renderStage) const {
    return getPassForStage(renderStage).getLastTotalBinSize();
}

RenderPass& RenderPassManager::getPassForStage(RenderStage renderStage) {
    for (std::shared_ptr<RenderPass>& pass : _renderPasses) {
        if (pass->stageFlag() == renderStage) {
            return *pass;
        }
    }

    DIVIDE_UNEXPECTED_CALL();
    return *_renderPasses.front();
}

const RenderPass& RenderPassManager::getPassForStage(RenderStage renderStage) const {
    for (const std::shared_ptr<RenderPass>& pass : _renderPasses) {
        if (pass->stageFlag() == renderStage) {
            return *pass;
        }
    }

    DIVIDE_UNEXPECTED_CALL();
    return *_renderPasses.front();
}

const RenderPass::BufferData& 
RenderPassManager::getBufferData(RenderStage renderStage, I32 bufferIndex) const {
    return getPassForStage(renderStage).getBufferData(bufferIndex);
}

RenderPass::BufferData&
RenderPassManager::getBufferData(RenderStage renderStage, I32 bufferIndex) {
    return getPassForStage(renderStage).getBufferData(bufferIndex);
}

/// Prepare the list of visible nodes for rendering
GFXDevice::NodeData RenderPassManager::processVisibleNode(SceneGraphNode* node, bool isOcclusionCullable, const SceneRenderState& sceneRenderState, const mat4<F32>& viewMatrix) const {
    GFXDevice::NodeData dataOut;

    RenderingComponent* const renderable = node->get<RenderingComponent>();
    TransformComponent* const transform = node->get<TransformComponent>();

    // Extract transform data (if available)
    // (Nodes without transforms are considered as using identity matrices)
    if (transform) {
        // ... get the node's world matrix properly interpolated
        if (Config::USE_FIXED_TIMESTEP) {
            dataOut._worldMatrix.set(transform->getWorldMatrix(_context.getFrameInterpolationFactor()));
        } else {
            dataOut._worldMatrix.set(transform->getWorldMatrix());
        }

        dataOut._normalMatrixWV.set(dataOut._worldMatrix);
        if (!transform->isUniformScaled()) {
            // Non-uniform scaling requires an inverseTranspose to negate
            // scaling contribution but preserve rotation
            dataOut._normalMatrixWV.setRow(3, 0.0f, 0.0f, 0.0f, 1.0f);
            dataOut._normalMatrixWV.inverseTranspose();
            dataOut._normalMatrixWV.mat[15] = 0.0f;
        }
        dataOut._normalMatrixWV.setRow(3, 0.0f, 0.0f, 0.0f, 0.0f);

        // Calculate the normal matrix (world * view)
        dataOut._normalMatrixWV *= viewMatrix;
    }

    // Since the normal matrix is 3x3, we can use the extra row and column to store additional data
    if (sceneRenderState.isEnabledOption(SceneRenderState::RenderOptions::PLAY_ANIMATIONS)) {
        AnimationComponent* const animComp = node->get<AnimationComponent>();
        dataOut._normalMatrixWV.element(0, 3) = to_F32(animComp && animComp->playAnimations() ? animComp->boneCount() : 0);
    } else {
        dataOut._normalMatrixWV.element(0, 3) = 0.0f;
    }
    dataOut._normalMatrixWV.setRow(3, node->get<BoundsComponent>()->getBoundingSphere().asVec4());
    // Get the material property matrix (alpha test, texture count, texture operation, etc.)
    renderable->getRenderingProperties(dataOut._properties, dataOut._normalMatrixWV.element(1, 3), dataOut._normalMatrixWV.element(2, 3));
    // Get the colour matrix (diffuse, specular, etc.)
    renderable->getMaterialColourMatrix(dataOut._colourMatrix);

    //set properties.w to -1 to skip occlusion culling for the node
    dataOut._properties.w = isOcclusionCullable ? 1.0f : -1.0f;

    return dataOut;
}

void RenderPassManager::refreshNodeData(RenderStagePass stagePass, const PassParams& params, GFX::CommandBuffer& bufferInOut) {
    const SceneRenderState& sceneRenderState = parent().sceneManager().getActiveScene().renderState();

    U16 queueSize = 0;
    RenderQueue::SortedQueues sortedQueues = getQueue().getSortedQueues(stagePass._stage, queueSize);

    const mat4<F32>& viewMatrix = params._camera->getViewMatrix();
    g_nodeData.resize(0);
    g_nodeData.reserve(sortedQueues.size() * Config::MAX_VISIBLE_NODES);

    g_drawCommands.resize(0);
    g_drawCommands.reserve(Config::MAX_VISIBLE_NODES);

    g_drawCommands.resize(0);
    g_drawCommands.reserve(Config::MAX_VISIBLE_NODES);

    for (const vectorEASTL<SceneGraphNode*>& queue : sortedQueues) {
        for (SceneGraphNode* node : queue) {
            RenderingComponent& renderable = *node->get<RenderingComponent>();
            Attorney::RenderingCompRenderPass::setDataIndex(renderable, to_U32(g_nodeData.size()));
            if (Attorney::RenderingCompRenderPass::hasDrawCommands(renderable, stagePass._stage)) {
                g_nodeData.push_back(processVisibleNode(node, renderable.renderOptionEnabled(RenderingComponent::RenderOptions::IS_OCCLUSION_CULLABLE), sceneRenderState, viewMatrix));
                Attorney::RenderingCompRenderPass::updateDrawCommands(renderable, stagePass._stage, g_drawCommands);
            }
        }
    }

    RenderPass::BufferData& bufferData = getBufferData(stagePass._stage, params._pass);
    bufferData._lastCommandCount = to_U32(g_drawCommands.size());

    assert(bufferData._lastCommandCount >= to_U32(g_nodeData.size()));
    // If the buffer update required is large enough, just replace the entire thing
    if (g_nodeData.size() > Config::MAX_VISIBLE_NODES / 2) {
        bufferData._renderData->writeData(g_nodeData.data());
    } else { // Otherwise, just update the needed range to save bandwidth
        bufferData._renderData->writeData(0, g_nodeData.size(), g_nodeData.data());
    }

    bufferData._cmdBuffer->writeData(g_drawCommands.data());

    ShaderBufferBinding shaderBufferCmds;
    shaderBufferCmds._binding = ShaderBufferLocation::CMD_BUFFER;
    shaderBufferCmds._buffer = bufferData._cmdBuffer;

    ShaderBufferBinding shaderBufferData;
    shaderBufferData._binding = ShaderBufferLocation::NODE_INFO;
    shaderBufferData._buffer = bufferData._renderData;
    shaderBufferData._range = vec2<U16>(0, to_U16(g_nodeData.size()));

    GFX::BindDescriptorSetsCommand descriptorSetCmd;
    descriptorSetCmd._set = _context.newDescriptorSet();
    descriptorSetCmd._set->_shaderBuffers.emplace_back(shaderBufferCmds);
    descriptorSetCmd._set->_shaderBuffers.emplace_back(shaderBufferData);
    GFX::EnqueueCommand(bufferInOut, descriptorSetCmd);
}

void RenderPassManager::buildDrawCommands(RenderStagePass stagePass, const PassParams& params, bool refresh, GFX::CommandBuffer& bufferInOut)
{
    const SceneRenderState& sceneRenderState = parent().sceneManager().getActiveScene().renderState();

    U16 queueSize = 0;
    RenderQueue::SortedQueues sortedQueues = getQueue().getSortedQueues(stagePass._stage, queueSize);
    for (const vectorEASTL<SceneGraphNode*>& queue : sortedQueues) {
        for (SceneGraphNode* node : queue) {
            Attorney::RenderingCompRenderPass::prepareDrawPackage(*node->get<RenderingComponent>(), *params._camera, sceneRenderState, stagePass);
        }
    }

    if (refresh) {
        refreshNodeData(stagePass, params, bufferInOut);
    }
}

void RenderPassManager::prepareRenderQueues(RenderStagePass stagePass, const PassParams& params, bool refreshNodeData, GFX::CommandBuffer& bufferInOut) {
    const RenderPassCuller::VisibleNodeList& visibleNodes = refreshNodeData ? Attorney::SceneManagerRenderPass::cullScene(parent().sceneManager(), stagePass, *params._camera)
                                                                            : parent().sceneManager().getVisibleNodesCache(params._stage);

    RenderQueue& queue = getQueue();
    queue.refresh(stagePass._stage);
    const vec3<F32>& eyePos = params._camera->getEye();
    for (const RenderPassCuller::VisibleNode& node : visibleNodes) {
        queue.addNodeToQueue(*node._node, stagePass, eyePos);
    }
    // Sort all bins
    queue.sort(stagePass);

    vectorEASTL<RenderPackage*>& packageQueue = _renderQueues[to_base(stagePass._stage)];
    packageQueue.resize(0);
    packageQueue.reserve(Config::MAX_VISIBLE_NODES);

    queue.populateRenderQueues(stagePass,
                               stagePass._passType == RenderPassType::OIT_PASS
                                                   ? RenderBinType::RBT_TRANSLUCENT
                                                   : RenderBinType::RBT_COUNT,
                               packageQueue);

    buildDrawCommands(stagePass, params, refreshNodeData, bufferInOut);
}

void RenderPassManager::prePass(const PassParams& params, const RenderTarget& target, GFX::CommandBuffer& bufferInOut) {
    GFX::BeginDebugScopeCommand beginDebugScopeCmd;
    beginDebugScopeCmd._scopeID = 0;
    beginDebugScopeCmd._scopeName = " - PrePass";
    GFX::EnqueueCommand(bufferInOut, beginDebugScopeCmd);

    // PrePass requires a depth buffer
    bool doPrePass = params._doPrePass && target.getAttachment(RTAttachmentType::Depth, 0).used();

    if (doPrePass && params._target._usage != RenderTargetUsage::COUNT) {
        RenderStagePass stagePass(params._stage, RenderPassType::DEPTH_PASS);
        prepareRenderQueues(stagePass, params, true, bufferInOut);

        if (params._bindTargets) {
            GFX::BeginRenderPassCommand beginRenderPassCommand;
            beginRenderPassCommand._target = params._target;
            beginRenderPassCommand._descriptor = RenderTarget::defaultPolicyDepthOnly();
            beginRenderPassCommand._name = "DO_PRE_PASS";
            GFX::EnqueueCommand(bufferInOut, beginRenderPassCommand);
        }

        renderQueueToSubPasses(stagePass, bufferInOut);

        Attorney::SceneManagerRenderPass::postRender(parent().sceneManager(), stagePass, *params._camera, bufferInOut);

        if (params._bindTargets) {
            GFX::EndRenderPassCommand endRenderPassCommand;
            GFX::EnqueueCommand(bufferInOut, endRenderPassCommand);
        }
    }

    GFX::EndDebugScopeCommand endDebugScopeCmd;
    GFX::EnqueueCommand(bufferInOut, endDebugScopeCmd);
}

void RenderPassManager::mainPass(const PassParams& params, RenderTarget& target, GFX::CommandBuffer& bufferInOut) {
    GFX::BeginDebugScopeCommand beginDebugScopeCmd;
    beginDebugScopeCmd._scopeID = 1;
    beginDebugScopeCmd._scopeName = " - MainPass";
    GFX::EnqueueCommand(bufferInOut, beginDebugScopeCmd);

    SceneManager& sceneManager = parent().sceneManager();

    RenderStagePass stagePass(params._stage, RenderPassType::COLOUR_PASS);
    prepareRenderQueues(stagePass, params, !params._doPrePass, bufferInOut);

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

        // We try and render translucent items in the shadow pass and due some alpha-discard tricks
        renderQueueToSubPasses(stagePass, bufferInOut);

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
    PipelineDescriptor pipelineDescriptor;
    pipelineDescriptor._stateHash = _context.get2DStateBlock();
    pipelineDescriptor._shaderProgramHandle = _OITCompositionShader->getID();
    Pipeline* pipeline = _context.newPipeline(pipelineDescriptor);

    RenderStagePass stagePass(params._stage, RenderPassType::OIT_PASS);
    prepareRenderQueues(stagePass, params, false, bufferInOut);

    // Weighted Blended Order Independent Transparency
    for (U8 i = 0; i < /*2*/1; ++i) {
        GFX::BeginDebugScopeCommand beginDebugScopeCmd;
        beginDebugScopeCmd._scopeID = 2;
        beginDebugScopeCmd._scopeName = Util::StringFormat(" - W-OIT Pass %d", to_U32(i)).c_str();
        GFX::EnqueueCommand(bufferInOut, beginDebugScopeCmd);

        //RenderPackage::MinQuality quality = i == 0 ? RenderPackage::MinQuality::FULL : RenderPackage::MinQuality::LOW;
        if (renderQueueSize(stagePass) > 0) {
            RenderTargetUsage rtUsage = i == 0 ? RenderTargetUsage::OIT_FULL_RES : RenderTargetUsage::OIT_QUARTER_RES;

            RenderTarget& oitTarget = _context.renderTargetPool().renderTarget(RenderTargetID(rtUsage));

            // Step1: Draw translucent items into the accumulation and revealage buffers
            GFX::BeginRenderPassCommand beginRenderPassOitCmd;
            beginRenderPassOitCmd._name = Util::StringFormat("DO_OIT_PASS_1_%s", (i == 0 ? "HIGH" : "LOW")).c_str();
            beginRenderPassOitCmd._target = RenderTargetID(rtUsage);
            {
                RTBlendState& state0 = beginRenderPassOitCmd._descriptor.blendState(to_U8(GFXDevice::ScreenTargets::ACCUMULATION));
                state0._blendProperties._blendSrc = BlendProperty::ONE;
                state0._blendProperties._blendDest = BlendProperty::ONE;
                state0._blendProperties._blendOp = BlendOperation::ADD;

                RTBlendState& state1 = beginRenderPassOitCmd._descriptor.blendState(to_U8(GFXDevice::ScreenTargets::REVEALAGE));
                state1._blendProperties._blendSrc = BlendProperty::ZERO;
                state1._blendProperties._blendDest = BlendProperty::INV_SRC_COLOR;
                state1._blendProperties._blendOp = BlendOperation::ADD;
            }
            // Don't clear our screen target. That would be BAD.
            beginRenderPassOitCmd._descriptor.clearColour(to_U8(GFXDevice::ScreenTargets::MODULATE), false);
            // Don't clear and don't write to depth buffer
            beginRenderPassOitCmd._descriptor.disableState(RTDrawDescriptor::State::CLEAR_DEPTH_BUFFER);
            beginRenderPassOitCmd._descriptor.drawMask().setEnabled(RTAttachmentType::Depth, 0, false);
            GFX::EnqueueCommand(bufferInOut, beginRenderPassOitCmd);

            renderQueueToSubPasses(stagePass, bufferInOut/*, quality*/);

            GFX::EndRenderPassCommand endRenderPassOitCmd;
            GFX::EnqueueCommand(bufferInOut, endRenderPassOitCmd);

            // Step2: Composition pass
            // Don't clear depth & colours and do not write to the depth buffer
            GFX::BeginRenderPassCommand beginRenderPassCompCmd;
            beginRenderPassCompCmd._name = Util::StringFormat("DO_OIT_PASS_2_%s", (i == 0 ? "HIGH" : "LOW")).c_str();
            beginRenderPassCompCmd._target = RenderTargetID(RenderTargetUsage::SCREEN);
            beginRenderPassCompCmd._descriptor.disableState(RTDrawDescriptor::State::CLEAR_DEPTH_BUFFER);
            beginRenderPassCompCmd._descriptor.disableState(RTDrawDescriptor::State::CLEAR_COLOUR_BUFFERS);
            beginRenderPassCompCmd._descriptor.drawMask().setEnabled(RTAttachmentType::Depth, 0, false);
            {
                RTBlendState& state0 = beginRenderPassCompCmd._descriptor.blendState(to_U8(GFXDevice::ScreenTargets::ALBEDO));
                state0._blendProperties._blendOp = BlendOperation::ADD;
#if defined(USE_COLOUR_WOIT)
                state0._blendProperties._blendSrc = BlendProperty::INV_SRC_ALPHA;
                state0._blendProperties._blendDest = BlendProperty::ONE;
#else
                state0._blendProperties._blendSrc = BlendProperty::SRC_ALPHA;
                state0._blendProperties._blendDest = BlendProperty::INV_SRC_ALPHA;
#endif
            }
            GFX::EnqueueCommand(bufferInOut, beginRenderPassCompCmd);

            GFX::BindPipelineCommand bindPipelineCmd;
            bindPipelineCmd._pipeline = pipeline;
            GFX::EnqueueCommand(bufferInOut, bindPipelineCmd);

            TextureData accum = oitTarget.getAttachment(RTAttachmentType::Colour, to_U8(GFXDevice::ScreenTargets::ACCUMULATION)).texture()->getData();
            TextureData revealage = oitTarget.getAttachment(RTAttachmentType::Colour, to_U8(GFXDevice::ScreenTargets::REVEALAGE)).texture()->getData();

            GFX::BindDescriptorSetsCommand descriptorSetCmd;
            descriptorSetCmd._set = _context.newDescriptorSet();
            descriptorSetCmd._set->_textureData.addTexture(accum, to_base(ShaderProgram::TextureUsage::UNIT0));
            descriptorSetCmd._set->_textureData.addTexture(revealage, to_base(ShaderProgram::TextureUsage::UNIT1));
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

        GFX::EndDebugScopeCommand endDebugScopeCmd;
        GFX::EnqueueCommand(bufferInOut, endDebugScopeCmd);
    }
}

void RenderPassManager::doCustomPass(PassParams& params, GFX::CommandBuffer& bufferInOut) {
    Attorney::SceneManagerRenderPass::prepareLightData(parent().sceneManager(), params._stage, *params._camera);

    // Tell the Rendering API to draw from our desired PoV
    GFX::SetCameraCommand setCameraCommand;
    setCameraCommand._camera = params._camera;
    GFX::EnqueueCommand(bufferInOut, setCameraCommand);

    GFX::SetClipPlanesCommand setClipPlanesCommand;
    setClipPlanesCommand._clippingPlanes = params._clippingPlanes;
    GFX::EnqueueCommand(bufferInOut, setClipPlanesCommand);

    RenderTarget& target = _context.renderTargetPool().renderTarget(params._target);

    GFX::BeginDebugScopeCommand beginDebugScopeCmd;
    beginDebugScopeCmd._scopeID = 0;
    beginDebugScopeCmd._scopeName = Util::StringFormat("Custom pass ( %s )", TypeUtil::renderStageToString(params._stage)).c_str();
    GFX::EnqueueCommand(bufferInOut, beginDebugScopeCmd);

    prePass(params, target, bufferInOut);

    if (params._occlusionCull) {
        _context.constructHIZ(params._target, bufferInOut);
        _context.occlusionCull(getBufferData(params._stage, params._pass), target.getAttachment(RTAttachmentType::Depth, 0).texture(), bufferInOut);
        if (params._stage == RenderStage::DISPLAY) {
            _context.updateCullCount(bufferInOut);
        }
    }

    mainPass(params, target, bufferInOut);
    woitPass(params, target, bufferInOut);

    GFX::EndDebugScopeCommand endDebugScopeCmd;
    GFX::EnqueueCommand(bufferInOut, endDebugScopeCmd);
}


// TEMP
U32 RenderPassManager::renderQueueSize(RenderStagePass stagePass, RenderPackage::MinQuality qualityRequirement) const {
    const vectorEASTL<RenderPackage*>& queue = _renderQueues[to_base(stagePass._stage)];
    if (qualityRequirement == RenderPackage::MinQuality::COUNT) {
        return to_U32(queue.size());
    }

    U32 size = 0;
    for (const RenderPackage* item : queue) {
        if (item->qualityRequirement() == qualityRequirement) {
            ++size;
        }
    }

    return size;
}

void RenderPassManager::renderQueueToSubPasses(RenderStagePass stagePass, GFX::CommandBuffer& commandsInOut, RenderPackage::MinQuality qualityRequirement) const {
    const vectorEASTL<RenderPackage*>& queue = _renderQueues[to_base(stagePass._stage)];

    bool cacheMiss = false;
    if (qualityRequirement == RenderPackage::MinQuality::COUNT) {
        for (RenderPackage* item : queue) {
            commandsInOut.add(Attorney::RenderPackageRenderPassManager::buildAndGetCommandBuffer(*item, cacheMiss));
        }
    } else {
        for (RenderPackage* item : queue) {
            if (item->qualityRequirement() == qualityRequirement) {
                commandsInOut.add(Attorney::RenderPackageRenderPassManager::buildAndGetCommandBuffer(*item, cacheMiss));
            }
        }
    }
}

};