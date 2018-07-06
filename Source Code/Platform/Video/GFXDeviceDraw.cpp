#include "stdafx.h"

#include "config.h"

#include "Headers/GFXDevice.h"

#include "GUI/Headers/GUI.h"
#include "GUI/Headers/GUIText.h"
#include "GUI/Headers/GUIFlash.h"

#include "Rendering/Headers/Renderer.h"

#include "Managers/Headers/SceneManager.h"
#include "Managers/Headers/RenderPassManager.h"

#include "Core/Time/Headers/ProfileTimer.h"
#include "Platform/Video/Headers/IMPrimitive.h"
#include "Platform/Video/Textures/Headers/Texture.h"
#include "Platform/Video/Shaders/Headers/ShaderProgram.h"
#include "Platform/Video/Buffers/ShaderBuffer/Headers/ShaderBuffer.h"

namespace Divide {

void GFXDevice::uploadGPUBlock() {
    if (_gpuBlock._needsUpload) {
        // We flush the entire buffer on update to inform the GPU that we don't
        // need the previous data. Might avoid some driver sync
        _gfxDataBuffer->writeData(&_gpuBlock._data);
        _gfxDataBuffer->bind(ShaderBufferLocation::GPU_BLOCK);
        _api->updateClipPlanes();
        _gpuBlock._needsUpload = false;
    }
}

void GFXDevice::renderQueueToSubPasses(RenderBinType queueType, RenderSubPassCmd& subPassCmd) {
    RenderPackageQueue& renderQueue = _renderQueues[queueType._to_integral()];

    assert(renderQueue.locked() == false);
    if (!renderQueue.empty()) {
        U32 queueSize = renderQueue.size();
        for (U32 idx = 0; idx < queueSize; ++idx) {
            RenderPackage& package = renderQueue.getPackage(idx);
            GenericDrawCommands& drawCommands = package._drawCommands;
            if (drawCommands.size() > 0 && batchCommands(drawCommands)) {
                std::for_each(std::begin(drawCommands),
                                std::end(drawCommands),
                                [&](GenericDrawCommand& cmd) -> void {
                                    cmd.enableOption(GenericDrawCommand::RenderOptions::RENDER_INDIRECT);
                                });

                for (ShaderBufferList::value_type& it : package._shaderBuffers) {
                    subPassCmd._shaderBuffers.emplace_back(it._buffer, it._slot, it._range);
                }

                subPassCmd._textures.set(package._textureData);
                subPassCmd._commands.insert(std::cbegin(subPassCmd._commands),
                                            std::cbegin(drawCommands),
                                            std::cend(drawCommands));
            }
        }
        renderQueue.clear();
    }
}

void GFXDevice::flushCommandBuffer(const CommandBuffer& commandBuffer) {
    uploadGPUBlock();
    _api->flushCommandBuffer(commandBuffer);
}

void GFXDevice::lockQueue(RenderBinType type) {
    _renderQueues[type._to_integral()].lock();
}

void GFXDevice::unlockQueue(RenderBinType type) {
    _renderQueues[type._to_integral()].unlock();
}

void GFXDevice::addToRenderQueue(RenderBinType queueType, const RenderPackage& package) {
    U32 queueIndex = queueType._to_integral();

    assert(_renderQueues[queueIndex].locked() == true);

    if (!package.isRenderable()) {
        return;
    }

    RenderPackageQueue& queue = _renderQueues[queueIndex];

    if (!queue.empty()) {
        RenderPackage& previous = queue.back();

        if (previous.isCompatible(package)) {
            previous._drawCommands.insert(std::cend(previous._drawCommands),
                                          std::cbegin(package._drawCommands),
                                          std::cend(package._drawCommands));
            return;
        }
    } else {
        queue.reserve(Config::MAX_VISIBLE_NODES);
    }

    queue.push_back(package);
}

/// Prepare the list of visible nodes for rendering
GFXDevice::NodeData& GFXDevice::processVisibleNode(const SceneGraphNode& node, U32 dataIndex) {
    NodeData& dataOut = _matricesData[dataIndex];

    RenderingComponent* const renderable = node.get<RenderingComponent>();
    AnimationComponent* const animComp = node.get<AnimationComponent>();
    PhysicsComponent* const transform = node.get<PhysicsComponent>();

    // Extract transform data (if available)
    // (Nodes without transforms are considered as using identity matrices)
    if (transform) {
        // ... get the node's world matrix properly interpolated
        dataOut._worldMatrix.set(transform->getWorldMatrix(getFrameInterpolationFactor()));

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
        dataOut._normalMatrixWV *= getMatrix(MATRIX::VIEW);
    }

    // Since the normal matrix is 3x3, we can use the extra row and column to store additional data
    dataOut._normalMatrixWV.element(0, 3) = to_F32(animComp ? animComp->boneCount() : 0);
    dataOut._normalMatrixWV.setRow(3, node.get<BoundsComponent>()->getBoundingSphere().asVec4());
    // Get the material property matrix (alpha test, texture count, texture operation, etc.)
    renderable->getRenderingProperties(dataOut._properties, dataOut._normalMatrixWV.element(1, 3), dataOut._normalMatrixWV.element(2, 3));
    // Get the colour matrix (diffuse, specular, etc.)
    renderable->getMaterialColourMatrix(dataOut._colourMatrix);

    return dataOut;
}

void GFXDevice::buildDrawCommands(const RenderQueue::SortedQueues& sortedNodes,
                                  SceneRenderState& sceneRenderState,
                                  RenderPass::BufferData& bufferData,
                                  bool refreshNodeData)
{
    Time::ScopedTimer timer(_commandBuildTimer);
    // If there aren't any nodes visible in the current pass, don't update anything (but clear the render queue

    RenderStagePass currentStage = getRenderStage();
    if (refreshNodeData) {
        bufferData._lastCommandCount = 0;
        bufferData._lasNodeCount = 0;
    }

    if (currentStage.stage() == RenderStage::SHADOW) {
        Light* shadowLight = LightPool::currentShadowCastingLight();
        assert(shadowLight != nullptr);
        if (!COMPARE(_gpuBlock._data._renderProperties.x, shadowLight->getShadowProperties()._arrayOffset.x)) {
            _gpuBlock._data._renderProperties.x = to_F32(shadowLight->getShadowProperties()._arrayOffset.x);
            _gpuBlock._needsUpload = true;
        }
        U8 shadowPasses = shadowLight->getLightType() == LightType::DIRECTIONAL
                                                       ? shadowLight->getShadowMapInfo()->numLayers()
                                                       : 1;
        if (!COMPARE(_gpuBlock._data._renderProperties.y, to_F32(shadowPasses))) {
            _gpuBlock._data._renderProperties.y = to_F32(shadowPasses);
            _gpuBlock._needsUpload = true;
        }
    }

    U32 nodeCount = 0;
    U32 cmdCount = 0;

    for (const vectorImpl<SceneGraphNode*>& queue : sortedNodes) {
        std::for_each(std::begin(queue), std::end(queue),
            [&](SceneGraphNode* node) -> void
            {
                RenderingComponent* renderable = node->get<RenderingComponent>();
                Attorney::RenderingCompGFXDevice::prepareDrawPackage(*renderable, sceneRenderState, currentStage);
            });

        std::for_each(std::begin(queue), std::end(queue),
            [&](SceneGraphNode* node) -> void
            {
                RenderingComponent* renderable = node->get<RenderingComponent>();
                RenderPackage& pkg = 
                    Attorney::RenderingCompGFXDevice::getDrawPackage(*renderable,
                                                                     sceneRenderState,
                                                                     currentStage,
                                                                     refreshNodeData ? cmdCount
                                                                                     : renderable->commandOffset(),
                                                                     refreshNodeData ? nodeCount
                                                                                     : renderable->commandIndex());
                if (pkg.isRenderable()) {
                    if (refreshNodeData) {
                        NodeData& dataOut = processVisibleNode(*node, nodeCount);
                        //set properties.w to -1 to skip occlusion culling for the node
                        dataOut._properties.w = pkg.isOcclusionCullable() ? 1.0f : -1.0f;
                        for (GenericDrawCommand& cmd : pkg._drawCommands) {
                            for (U32 i = 0; i < cmd.drawCount(); ++i) {
                                _drawCommandsCache[cmdCount++].set(cmd.cmd());
                            }
                        }
                    }
                    nodeCount++;
                }
            });
    }

    if (refreshNodeData) {
        bufferData._lastCommandCount = cmdCount;
        bufferData._lasNodeCount = nodeCount;

        assert(cmdCount >= nodeCount);
        // If the buffer update required is large enough, just replace the entire thing
        if (nodeCount > Config::MAX_VISIBLE_NODES / 2) {
            bufferData._renderData->writeData(_matricesData.data());
        } else {
            // Otherwise, just update the needed range to save bandwidth
            bufferData._renderData->writeData(0, nodeCount, _matricesData.data());
        }

        ShaderBuffer& cmdBuffer = *bufferData._cmdBuffer;
        cmdBuffer.writeData(_drawCommandsCache.data());
        _api->registerCommandBuffer(cmdBuffer);

        // This forces a sync for each buffer to make sure all data is properly uploaded in VRAM
        bufferData._renderData->bind(ShaderBufferLocation::NODE_INFO);
    }
}

void GFXDevice::occlusionCull(const RenderPass::BufferData& bufferData, const Texture_ptr& depthBuffer) {
    static const U32 GROUP_SIZE_AABB = 64;
    uploadGPUBlock();

    bufferData._cmdBuffer->bind(ShaderBufferLocation::GPU_COMMANDS);
    bufferData._cmdBuffer->bindAtomicCounter();

    depthBuffer->bind(to_U8(ShaderProgram::TextureUsage::DEPTH));
    U32 cmdCount = bufferData._lastCommandCount;

    _HIZCullProgram->bind();
    _HIZCullProgram->Uniform("dvd_numEntities", cmdCount);
    _HIZCullProgram->DispatchCompute((cmdCount + GROUP_SIZE_AABB - 1) / GROUP_SIZE_AABB, 1, 1);
    _HIZCullProgram->SetMemoryBarrier(ShaderProgram::MemoryBarrierType::COUNTER);
}

U32 GFXDevice::getLastCullCount() const {
    const RenderPass::BufferData& bufferData = parent().renderPassManager().getBufferData(RenderStage::DISPLAY, 0);

    U32 cullCount = bufferData._cmdBuffer->getAtomicCounter();
    if (cullCount > 0) {
        bufferData._cmdBuffer->resetAtomicCounter();
    }
    return cullCount;
}

bool GFXDevice::batchCommands(GenericDrawCommands& commands) const {
    auto batch = [](GenericDrawCommand& previousIDC,
                    GenericDrawCommand& currentIDC)  -> bool {
            if (previousIDC.compatible(currentIDC) &&
                // Batchable commands must share the same buffer
                previousIDC.sourceBuffer()->getGUID() == currentIDC.sourceBuffer()->getGUID())
            {
                U32 prevCount = previousIDC.drawCount();
                if (previousIDC.cmd().baseInstance + prevCount != currentIDC.cmd().baseInstance) {
                    return false;
                }
                // If the rendering commands are batchable, increase the draw count for the previous one
                previousIDC.drawCount(to_U16(prevCount + currentIDC.drawCount()));
                // And set the current command's draw count to zero so it gets removed from the list later on
                currentIDC.drawCount(0);

                return true;
            }

            return false;
    };


    vectorAlg::vecSize previousCommandIndex = 0;
    vectorAlg::vecSize currentCommandIndex = 1;
    const vectorAlg::vecSize commandCount = commands.size();
    for (; currentCommandIndex < commandCount; ++currentCommandIndex) {
        GenericDrawCommand& previousCommand = commands[previousCommandIndex];
        GenericDrawCommand& currentCommand = commands[currentCommandIndex];
        if (!batch(previousCommand, currentCommand))
        {
            previousCommandIndex = currentCommandIndex;
        }
    }

    commands.erase(std::remove_if(std::begin(commands),
                   std::end(commands),
                   [](const GenericDrawCommand& cmd) -> bool {
                       return cmd.drawCount() == 0;
                   }),
                   std::end(commands));
    return true;
}

bool GFXDevice::draw(const GenericDrawCommand& cmd) {
    uploadGPUBlock();
    if (_api->draw(cmd)) {
        if (cmd.isEnabledOption(GenericDrawCommand::RenderOptions::RENDER_GEOMETRY)) {
            registerDrawCall();
        }
        if (cmd.isEnabledOption(GenericDrawCommand::RenderOptions::RENDER_WIREFRAME)) {
            registerDrawCall();
        }
        return true;
    }

    return false;
}


void GFXDevice::flushDisplay(const vec4<I32>& targetViewport) {
    PipelineDescriptor pipelineDescriptor;
    pipelineDescriptor._stateHash = getDefaultStateBlock(true);
    pipelineDescriptor._shaderProgram = _displayShader;

    GenericDrawCommand triangleCmd;
    triangleCmd.primitiveType(PrimitiveType::TRIANGLES);
    triangleCmd.drawCount(1);
    triangleCmd.pipeline(newPipeline(pipelineDescriptor));

    RenderTarget& screen = renderTarget(RenderTargetID(RenderTargetUsage::SCREEN));
    screen.bind(to_U8(ShaderProgram::TextureUsage::UNIT0),
                RTAttachment::Type::Colour,
                to_U8(ScreenTargets::ALBEDO));


    GFX::ScopedViewport targetArea(*this, targetViewport);

    // Blit render target to screen
    draw(triangleCmd);


    // Render all 2D debug info and call API specific flush function
    if (Application::instance().mainLoopActive()) {
        GFX::Scoped2DRendering scoped2D(*this);
        ReadLock r_lock(_2DRenderQueueLock);
        for (std::pair<U32, GUID2DCbk>& callbackFunction : _2dRenderQueue) {
            callbackFunction.second.second();
        }
    }
}

};
