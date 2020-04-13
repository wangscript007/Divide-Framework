#include "stdafx.h"

#include "config.h"

#include "Headers/RenderingComponent.h"
#include "Headers/BoundsComponent.h"
#include "Headers/AnimationComponent.h"
#include "Headers/TransformComponent.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/Configuration.h"
#include "Core/Headers/PlatformContext.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/StringHelper.h"
#include "Core/Headers/PlatformContext.h"

#include "Scenes/Headers/SceneState.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Headers/IMPrimitive.h"

#include "Platform/Video/Headers/RenderStateBlock.h"
#include "Geometry/Shapes/Headers/Mesh.h"
#include "Geometry/Material/Headers/Material.h"
#include "Dynamics/Entities/Headers/Impostor.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Rendering/Lighting/Headers/LightPool.h"

namespace Divide {

namespace {
    constexpr U8 MAX_LOD_LEVEL = 4u;

    constexpr I16 g_renderRangeLimit = std::numeric_limits<I16>::max();

    I32 getUsageIndex(RenderTargetUsage usage) noexcept {
        for (I32 i = 0; i < (sizeof(g_texUsage) / sizeof(g_texUsage[0])); ++i) {
            if (g_texUsage[i].first == usage) {
                return i;
            }
        }

        return -1;
    }
};

hashMap<U32, DebugView*> RenderingComponent::s_debugViews[2];

RenderingComponent::RenderingComponent(SceneGraphNode& parentSGN, PlatformContext& context) 
    : BaseComponentType<RenderingComponent, ComponentType::RENDERING>(parentSGN, context),
      _context(context.gfx()),
      _config(context.config()),
      _cullFlagValue(1.0f),
      _renderMask(0),
      _reflectionIndex(-1),
      _refractionIndex(-1),
      _reflectorType(ReflectorType::COUNT),
      _refractorType(RefractorType::COUNT),
      _primitivePipeline{nullptr, nullptr, nullptr},
      _materialInstance(nullptr),
      _materialInstanceCache(nullptr),
      _boundingBoxPrimitive{ nullptr, nullptr },
      _boundingSpherePrimitive{ nullptr, nullptr },
      _skeletonPrimitive(nullptr),
      _axisGizmo(nullptr)
{
    _lodLevels.fill(0u);
    _lodLockLevels.fill({ false, to_U8(0u) });

    _renderRange.min = -1.0f * g_renderRangeLimit;
    _renderRange.max =  1.0f* g_renderRangeLimit;

    setMaterialTpl(parentSGN.getNode().getMaterialTpl());

    toggleRenderOption(RenderOptions::RENDER_GEOMETRY, true);
    toggleRenderOption(RenderOptions::CAST_SHADOWS, true);
    toggleRenderOption(RenderOptions::RECEIVE_SHADOWS, true);
    toggleRenderOption(RenderOptions::IS_VISIBLE, true);

    defaultReflectionTexture(nullptr, 0);
    defaultRefractionTexture(nullptr, 0);

    RenderStateBlock primitiveStateBlock = {};
    PipelineDescriptor pipelineDescriptor = {};
    pipelineDescriptor._stateHash = primitiveStateBlock.getHash();
    pipelineDescriptor._shaderProgramHandle = ShaderProgram::defaultShader()->getGUID();

    const SceneNode& node = _parentSGN.getNode();
    if (node.type() == SceneNodeType::TYPE_OBJECT3D) {
        // Prepare it for rendering lines
        // Do not cull the sky
        if (static_cast<const Object3D&>(node).type() == SceneNodeType::TYPE_SKY) {
            _cullFlagValue = -1.0f;
        }
        if (static_cast<const Object3D&>(node).getObjectFlag(Object3D::ObjectFlag::OBJECT_FLAG_SKINNED)) {
            RenderStateBlock primitiveStateBlockNoZRead = {};
            primitiveStateBlockNoZRead.depthTestEnabled(false);
            pipelineDescriptor._stateHash = primitiveStateBlockNoZRead.getHash();
            _primitivePipeline[1] = _context.newPipeline(pipelineDescriptor);
        }

        _primitivePipeline[0] = _context.newPipeline(pipelineDescriptor);
    }

    if_constexpr (Config::Build::ENABLE_EDITOR) {
        // Prepare it for line rendering
        pipelineDescriptor._stateHash = _context.getDefaultStateBlock(true);
        _primitivePipeline[2] = _context.newPipeline(pipelineDescriptor);
    }

    for (U8 s = 0; s < to_U8(RenderStage::COUNT); ++s) {
        if (s == to_U8(RenderStage::SHADOW)) {
            _renderPackages[s][to_base(RenderPassType::MAIN_PASS)].resize(ShadowMap::MAX_SHADOW_PASSES);
        } else {
            PackagesPerPassType & perPassPkgs = _renderPackages[s];
            for (U8 p = 0; p < to_U8(RenderPassType::COUNT); ++p) {
                perPassPkgs[p].resize(1);
            }
        }
    }
}

RenderingComponent::~RenderingComponent()
{
    for (U8 i = 0; i < 2; ++i) {
        if (_boundingBoxPrimitive[i]) {
            _context.destroyIMP(_boundingBoxPrimitive[i]);
        }
        if (_boundingSpherePrimitive[i]) {
            _context.destroyIMP(_boundingSpherePrimitive[i]);
        }
    }

    if (_skeletonPrimitive) {
        _context.destroyIMP(_skeletonPrimitive);
    }

    if_constexpr (Config::Build::ENABLE_EDITOR) {
        if (_axisGizmo) {
            _context.destroyIMP(_axisGizmo);
        }
    }
}

void RenderingComponent::setMaterialTpl(const Material_ptr& material) {
    _materialInstance = material;
    if (_materialInstance != nullptr) {
        assert(!_materialInstance->resourceName().empty());

        EditorComponentField materialField = {};
        materialField._name = "Material";
        materialField._data = _materialInstance.get();
        materialField._type = EditorComponentFieldType::MATERIAL;
        materialField._readOnly = false;
        // should override any existing entry
        _editorComponent.registerField(std::move(materialField));

        EditorComponentField lockLodField = {};
        lockLodField._name = "Rendered LOD Level";
        lockLodField._type = EditorComponentFieldType::PUSH_TYPE;
        lockLodField._basicTypeSize = GFX::PushConstantSize::BYTE;
        lockLodField._basicType = GFX::PushConstantType::UINT;
        lockLodField._data = &_lodLevels[to_base(RenderStage::DISPLAY)];
        lockLodField._readOnly = true;
        _editorComponent.registerField(std::move(lockLodField));

        EditorComponentField renderLodField = {};
        renderLodField._name = "Lock LoD";
        renderLodField._type = EditorComponentFieldType::PUSH_TYPE;
        renderLodField._basicType = GFX::PushConstantType::BOOL;
        renderLodField._data = &_lodLockLevels[to_base(RenderStage::DISPLAY)].first;
        renderLodField._readOnly = false;
        _editorComponent.registerField(std::move(renderLodField));

        EditorComponentField lockLodLevelField = {};
        lockLodLevelField._name = "Lock LoD Level";
        lockLodLevelField._type = EditorComponentFieldType::PUSH_TYPE;
        lockLodLevelField._range = { 0.0f, to_F32(MAX_LOD_LEVEL) };
        lockLodLevelField._basicType = GFX::PushConstantType::UINT;
        lockLodLevelField._basicTypeSize = GFX::PushConstantSize::BYTE;
        lockLodLevelField._data = &_lodLockLevels[to_base(RenderStage::DISPLAY)].second;
        lockLodLevelField._readOnly = false;
        _editorComponent.registerField(std::move(lockLodLevelField));

        _materialInstanceCache = _materialInstance.get();
        if (_materialInstanceCache != nullptr) {
            _materialInstanceCache->setStatic(_parentSGN.usageContext() == NodeUsageContext::NODE_STATIC);
        }
    }
}

void RenderingComponent::useUniqueMaterialInstance() {
    if (_materialInstance == nullptr) {
        return;
    }

    _materialInstance = _materialInstance->clone("_instance_" + _parentSGN.name());
    _materialInstanceCache = _materialInstance.get();
    if (_materialInstanceCache != nullptr) {
        _materialInstanceCache->setStatic(_parentSGN.usageContext() == NodeUsageContext::NODE_STATIC);
    }
}

void RenderingComponent::setMinRenderRange(F32 minRange) {
    _renderRange.min = std::max(minRange, -1.0f * g_renderRangeLimit);
}

void RenderingComponent::setMaxRenderRange(F32 maxRange) {
    _renderRange.max = std::min(maxRange,  1.0f * g_renderRangeLimit);
}

void RenderingComponent::rebuildDrawCommands(const RenderStagePass& stagePass, RenderPackage& pkg) {
    OPTICK_EVENT();
    pkg.clear();

    // The following commands are needed for material rendering
    // In the absence of a material, use the SceneNode buildDrawCommands to add all of the needed commands
    if (_materialInstanceCache != nullptr) {
        PipelineDescriptor pipelineDescriptor = {};
        pipelineDescriptor._stateHash = _materialInstanceCache->getRenderStateBlock(stagePass);
        pipelineDescriptor._shaderProgramHandle = _materialInstanceCache->getProgramGUID(stagePass);

        pkg.addPipelineCommand(GFX::BindPipelineCommand{ _context.newPipeline(pipelineDescriptor) });

        GFX::BindDescriptorSetsCommand bindDescriptorSetsCommand = {};
        for (const ShaderBufferBinding& binding : getShaderBuffers()) {
            bindDescriptorSetsCommand._set.addShaderBuffer(binding);
        }
        pkg.addDescriptorSetsCommand(bindDescriptorSetsCommand);
    }

    _parentSGN.getNode().buildDrawCommands(_parentSGN, stagePass, pkg);
}

void RenderingComponent::Update(const U64 deltaTimeUS) {
    OPTICK_EVENT();

    if (_materialInstanceCache != nullptr && _materialInstanceCache->update(deltaTimeUS)) {
        onMaterialChanged();
    }

    const SceneNode& node = _parentSGN.getNode();
    if (node.type() == SceneNodeType::TYPE_OBJECT3D) {
        const Object3D& node3D = static_cast<const Object3D&>(node);
        if (node3D.getObjectType()._value == ObjectType::SUBMESH) {
            _parentSGN.parent()->clearFlag(SceneGraphNode::Flags::BOUNDING_BOX_RENDERED);

            if (node3D.getObjectFlag(Object3D::ObjectFlag::OBJECT_FLAG_SKINNED)) {
                _parentSGN.parent()->clearFlag(SceneGraphNode::Flags::SKELETON_RENDERED);
            }
        }
    }

    BaseComponentType<RenderingComponent, ComponentType::RENDERING>::Update(deltaTimeUS);
}


void RenderingComponent::onMaterialChanged() {
    OPTICK_EVENT();

    for (U8 s = 0u; s < to_U8(RenderStage::COUNT); ++s) {
        if (s == to_U8(RenderStage::SHADOW)) {
            continue;
        }
        PackagesPerPassType& perPassPkg = _renderPackages[s];
        for (U8 p = 0u; p < to_U8(RenderPassType::COUNT); ++p) {
            PackagesPerIndex& perIndexPkg = perPassPkg[p];
            for (RenderPackage& pkg : perIndexPkg) {
                pkg.textureDataDirty(true);
            }
        }
    }
}

bool RenderingComponent::canDraw(const RenderStagePass& renderStagePass, U8 LoD, bool refreshData) {
    OPTICK_EVENT();
    OPTICK_TAG("Node", (_parentSGN.name().c_str()));

    if (Attorney::SceneGraphNodeComponent::getDrawState(_parentSGN, renderStagePass, LoD)) {
        // Can we render without a material? Maybe. IDK.
        if (_materialInstanceCache == nullptr || _materialInstanceCache->canDraw(renderStagePass)) {
            return renderOptionEnabled(RenderOptions::IS_VISIBLE);
        }
    }

    return false;
}

void RenderingComponent::onParentUsageChanged(NodeUsageContext context) {
    if (_materialInstanceCache != nullptr) {
        _materialInstanceCache->setStatic(context == NodeUsageContext::NODE_STATIC);
    }
}

void RenderingComponent::rebuildMaterial() {
    if (_materialInstanceCache != nullptr) {
        _materialInstanceCache->rebuild();
        onMaterialChanged();
    }

    _parentSGN.forEachChild([](const SceneGraphNode* child, I32 /*childIdx*/) {
        RenderingComponent* const renderable = child->get<RenderingComponent>();
        if (renderable) {
            renderable->rebuildMaterial();
        }
    });
}

void RenderingComponent::onRender(const RenderStagePass& renderStagePass) {
    OPTICK_EVENT();

    RenderPackage& pkg = getDrawPackage(renderStagePass);
    if (pkg.textureDataDirty()) {
        TextureDataContainer<>& textures = pkg.descriptorSet(0)._textureData;

        if (_materialInstanceCache != nullptr) {
            _materialInstanceCache->getTextureData(renderStagePass, textures);
        }

        if (!renderStagePass.isDepthPass()) {
            for (U8 i = 0; i < _externalTextures.size(); ++i) {
                const Texture_ptr& crtTexture = _externalTextures[i];
                if (crtTexture != nullptr) {
                    textures.setTexture(crtTexture->data(), to_base(g_texUsage[i].second));
                }
            }
        }

        pkg.textureDataDirty(false);
    }
}

bool RenderingComponent::onQuickRefreshNodeData(RefreshNodeDataParams& refreshParams) {
    OPTICK_EVENT();
    Attorney::SceneGraphNodeComponent::onRefreshNodeData(_parentSGN, *refreshParams._stagePass, *refreshParams._camera, true, *refreshParams._bufferInOut);
    return true;
}

bool RenderingComponent::onRefreshNodeData(RefreshNodeDataParams& refreshParams, const TargetDataBufferParams& bufferParams) {
    OPTICK_EVENT();

    RenderPackage& pkg = getDrawPackage(*refreshParams._stagePass);
    if (pkg.empty()) {
        return false;
    }

    const I32 drawCommandCount = pkg.drawCommandCount();
    if (drawCommandCount == 0) {
        return false;
    }

    const RenderStage stage = refreshParams._stagePass->_stage;
    const U8 stageIdx = to_base(stage);
    const U8 lodLevel = _lodLevels[stageIdx];

    RenderStagePass tempStagePass = *refreshParams._stagePass;

    const U32 startOffset = to_U32(refreshParams._drawCommandsInOut->size());

    const U32 dataIndex = bufferParams._dataIndex;
    Attorney::RenderPackageRenderingComponent::updateDrawCommands(pkg, dataIndex, startOffset, lodLevel);

    if (stage != RenderStage::SHADOW) {
        for (U8 i = 0; i < to_base(RenderPassType::COUNT); ++i) {
            if (i == to_U8(refreshParams._stagePass->_passType)) {
                continue;
            }

            tempStagePass._passType = static_cast<RenderPassType>(i);
            Attorney::RenderPackageRenderingComponent::updateDrawCommands(getDrawPackage(tempStagePass), dataIndex, startOffset, lodLevel);
        }
    }

    for (I32 i = 0; i < drawCommandCount; ++i) {
        refreshParams._drawCommandsInOut->push_back(pkg.drawCommand(i, 0)._cmd);
    }

    Attorney::SceneGraphNodeComponent::onRefreshNodeData(_parentSGN, *refreshParams._stagePass, *refreshParams._camera, false, *refreshParams._bufferInOut);
    return true;
}

void RenderingComponent::getMaterialColourMatrix(mat4<F32>& matOut) const {
    matOut.zero();

    if (_materialInstanceCache != nullptr) {
        _materialInstanceCache->getMaterialMatrix(matOut);
    }
}

void RenderingComponent::getRenderingProperties(const RenderStagePass& stagePass, NodeRenderingProperties& propertiesOut) const {
    propertiesOut._isHovered = _parentSGN.hasFlag(SceneGraphNode::Flags::HOVERED);
    propertiesOut._isSelected = _parentSGN.hasFlag(SceneGraphNode::Flags::SELECTED);
    propertiesOut._receivesShadows = _config.rendering.shadowMapping.enabled && renderOptionEnabled(RenderOptions::RECEIVE_SHADOWS);
    propertiesOut._lod = _lodLevels[to_base(stagePass._stage)];
    propertiesOut._cullFlagValue = _cullFlagValue;
    propertiesOut._reflectionIndex = defaultReflectionTextureIndex();
    propertiesOut._reflectionIndex = defaultRefractionTextureIndex();
    if (_materialInstanceCache != nullptr) {
        propertiesOut._texOperation = _materialInstanceCache->getTextureOperation();
        propertiesOut._bumpMethod = _materialInstanceCache->getBumpMethod();
    }
}

/// Called after the current node was rendered
void RenderingComponent::postRender(const SceneRenderState& sceneRenderState, const RenderStagePass& renderStagePass, GFX::CommandBuffer& bufferInOut) {
    
    if (renderStagePass._stage != RenderStage::DISPLAY || renderStagePass._passType == RenderPassType::PRE_PASS) {
        return;
    }

    const SceneNode& node = _parentSGN.getNode();

    if_constexpr(Config::Build::IS_DEBUG_BUILD) {
        switch(sceneRenderState.gizmoState()){
            case SceneRenderState::GizmoState::ALL_GIZMO: {
                if (node.type() == SceneNodeType::TYPE_OBJECT3D) {
                    if (static_cast<const Object3D&>(node).getObjectType()._value == ObjectType::SUBMESH) {
                        drawDebugAxis();
                        bufferInOut.add(_axisGizmo->toCommandBuffer());
                    }
                }
            } break;
            case SceneRenderState::GizmoState::SELECTED_GIZMO: {
                if (_parentSGN.hasFlag(SceneGraphNode::Flags::SELECTED)) {
                    drawDebugAxis();
                    bufferInOut.add(_axisGizmo->toCommandBuffer());
                }
            } break;
            case SceneRenderState::GizmoState::NO_GIZMO: {
                if (_axisGizmo) {
                    _context.destroyIMP(_axisGizmo);
                }
            } break;
        }
    }

    SceneGraphNode* grandParent = _parentSGN.parent();

    // Draw bounding box if needed and only in the final stage to prevent Shadow/PostFX artifacts
    const bool renderBBox = renderOptionEnabled(RenderOptions::RENDER_BOUNDS_AABB) || sceneRenderState.isEnabledOption(SceneRenderState::RenderOptions::RENDER_AABB);
    const bool renderBSphere = renderOptionEnabled(RenderOptions::RENDER_BOUNDS_SPHERE) || sceneRenderState.isEnabledOption(SceneRenderState::RenderOptions::RENDER_BSPHERES);
    const bool isSubMesh = static_cast<const Object3D&>(node).getObjectType()._value == ObjectType::SUBMESH;

    bool setGrandparentFlag = !grandParent->hasFlag(SceneGraphNode::Flags::BOUNDING_BOX_RENDERED);
    if (renderBBox) {
        if (!_boundingBoxPrimitive[0]) {
            _boundingBoxPrimitive[0] = _context.newIMP();
            _boundingBoxPrimitive[0]->name("BoundingBox_" + _parentSGN.name());
            _boundingBoxPrimitive[0]->pipeline(*_primitivePipeline[0]);
            _boundingBoxPrimitive[0]->skipPostFX(true);
        }


        const BoundingBox& bb = _parentSGN.get<BoundsComponent>()->getBoundingBox();
        _boundingBoxPrimitive[0]->fromBox(bb.getMin(), bb.getMax(), UColour4(0, 0, 255, 255));
        bufferInOut.add(_boundingBoxPrimitive[0]->toCommandBuffer());

        if (isSubMesh) {
            if (!setGrandparentFlag) {
                if (!_boundingBoxPrimitive[1]) {
                    _boundingBoxPrimitive[1] = _context.newIMP();
                    _boundingBoxPrimitive[1]->name("BoundingBox_Parent_" + _parentSGN.name());
                    _boundingBoxPrimitive[1]->pipeline(*_primitivePipeline[0]);
                    _boundingBoxPrimitive[1]->skipPostFX(true);
                }

                const BoundingBox& bbGrandParent = grandParent->get<BoundsComponent>()->getBoundingBox();
                _boundingBoxPrimitive[1]->fromBox(bbGrandParent.getMin(), bbGrandParent.getMax(), UColour4(255, 0, 0, 255));
                bufferInOut.add(_boundingBoxPrimitive[1]->toCommandBuffer());
                setGrandparentFlag = true;
            }
        }
    } else if (_boundingBoxPrimitive[0]) {
        _context.destroyIMP(_boundingBoxPrimitive[0]);
        if (_boundingBoxPrimitive[1]) {
            _context.destroyIMP(_boundingBoxPrimitive[1]);
        }
    }

    if (renderBSphere) {
        if (!_boundingSpherePrimitive[0]) {
            _boundingSpherePrimitive[0] = _context.newIMP();
            _boundingSpherePrimitive[0]->name("BoundingSphere_" + _parentSGN.name());
            _boundingSpherePrimitive[0]->pipeline(*_primitivePipeline[0]);
            _boundingSpherePrimitive[0]->skipPostFX(true);
        }

        const BoundingSphere& bs = _parentSGN.get<BoundsComponent>()->getBoundingSphere();
        _boundingSpherePrimitive[0]->fromSphere(bs.getCenter(), bs.getRadius(), UColour4(0, 255, 0, 255));
        bufferInOut.add(_boundingSpherePrimitive[0]->toCommandBuffer());

        if (isSubMesh) {
            if (!setGrandparentFlag) {
                if (!_boundingSpherePrimitive[1]) {
                    _boundingSpherePrimitive[1] = _context.newIMP();
                    _boundingSpherePrimitive[1]->name("BoundingSphere_Parent_" + _parentSGN.name());
                    _boundingSpherePrimitive[1]->pipeline(*_primitivePipeline[0]);
                    _boundingSpherePrimitive[1]->skipPostFX(true);
                }

                const BoundingSphere& bsGrandParent = grandParent->get<BoundsComponent>()->getBoundingSphere();
                _boundingSpherePrimitive[1]->fromSphere(bsGrandParent.getCenter(), bsGrandParent.getRadius(), UColour4(255, 0, 0, 255));
                bufferInOut.add(_boundingSpherePrimitive[1]->toCommandBuffer());
                setGrandparentFlag = true;
            }
        }
    } else if (_boundingSpherePrimitive[0]) {
        _context.destroyIMP(_boundingSpherePrimitive[0]);
        if (_boundingSpherePrimitive[1]) {
            _context.destroyIMP(_boundingSpherePrimitive[1]);
        }
    }

    if (setGrandparentFlag) {
        grandParent->setFlag(SceneGraphNode::Flags::BOUNDING_BOX_RENDERED);
    }

    if (renderOptionEnabled(RenderOptions::RENDER_SKELETON) || sceneRenderState.isEnabledOption(SceneRenderState::RenderOptions::RENDER_SKELETONS)) {
        // Continue only for skinned 3D objects
        if (_parentSGN.getNode<Object3D>().getObjectFlag(Object3D::ObjectFlag::OBJECT_FLAG_SKINNED))
        {
            if (_skeletonPrimitive == nullptr) {
                _skeletonPrimitive = _context.newIMP();
                _skeletonPrimitive->skipPostFX(true);
                _skeletonPrimitive->name("Skeleton_" + _parentSGN.name());
                _skeletonPrimitive->pipeline(*_primitivePipeline[1]);
            }
            if (!grandParent->hasFlag(SceneGraphNode::Flags::SKELETON_RENDERED)) {
                // Get the animation component of any submesh. They should be synced anyway.
                const AnimationComponent* childAnimComp = _parentSGN.get<AnimationComponent>();
                // Get the skeleton lines from the submesh's animation component
                const vectorSTD<Line>& skeletonLines = childAnimComp->skeletonLines();
                _skeletonPrimitive->worldMatrix(_parentSGN.get<TransformComponent>()->getWorldMatrix());
                // Submit the skeleton lines to the GPU for rendering
                _skeletonPrimitive->fromLines(skeletonLines);
                bufferInOut.add(_skeletonPrimitive->toCommandBuffer());

                grandParent->setFlag(SceneGraphNode::Flags::SKELETON_RENDERED);
            }
        } else if (_skeletonPrimitive) {
            _context.destroyIMP(_skeletonPrimitive);
        }
    }
}

U8 RenderingComponent::getLoDLevel(const BoundsComponent& bComp, const vec3<F32>& cameraEye, RenderStage renderStage, const vec4<U16>& lodThresholds) {
    OPTICK_EVENT();

    const auto[state, level] = _lodLockLevels[to_base(renderStage)];

    if (state) {
        return CLAMPED<U8>(to_U8(level), 0u, MAX_LOD_LEVEL);
    }

    const F32 distSQtoCenter = std::max(bComp.getBoundingSphere().getCenter().distanceSquared(cameraEye), EPSILON_F32);

    if (distSQtoCenter <= to_F32(SQUARED(lodThresholds.x))) {
        return 0u;
    } else if (distSQtoCenter <= to_F32(SQUARED(lodThresholds.y))) {
        return 1u;
    } else if (distSQtoCenter <= to_F32(SQUARED(lodThresholds.z))) {
        return 2u;
    } else if (distSQtoCenter <= to_F32(SQUARED(lodThresholds.w))) {
        return 3u;
    }

    return MAX_LOD_LEVEL;
}

bool RenderingComponent::prepareDrawPackage(const Camera& camera, const SceneRenderState& sceneRenderState, const RenderStagePass& renderStagePass, bool refreshData) {
    OPTICK_EVENT();

    U8& lod = _lodLevels[to_base(renderStagePass._stage)];
    if (refreshData) {
        BoundsComponent* bComp = _parentSGN.get<BoundsComponent>();
        if (bComp != nullptr) {
            lod = getLoDLevel(*bComp, camera.getEye(), renderStagePass._stage, sceneRenderState.lodThresholds(renderStagePass._stage));
            const bool renderBS = renderOptionEnabled(RenderOptions::RENDER_BOUNDS_AABB);
            const bool renderAABB = renderOptionEnabled(RenderOptions::RENDER_BOUNDS_SPHERE);

            if (renderAABB != bComp->showAABB()) {
                bComp->showAABB(renderAABB);
            }
            if (renderBS != bComp->showBS()) {
                bComp->showAABB(renderBS);
            }
        }
    }

    if (canDraw(renderStagePass, lod, refreshData)) {
        RenderPackage& pkg = getDrawPackage(renderStagePass);

        bool rebuildCommandsOut = false;
        if (Attorney::SceneGraphNodeComponent::preRender(_parentSGN, camera, renderStagePass, refreshData, rebuildCommandsOut)) {
            if (pkg.empty() || rebuildCommandsOut) {
                rebuildDrawCommands(renderStagePass, pkg);
            }

            if (Attorney::SceneGraphNodeComponent::prepareRender(_parentSGN, *this, camera, renderStagePass, refreshData)) {
                pkg.setDrawOption(CmdRenderOptions::RENDER_GEOMETRY,
                                 (renderOptionEnabled(RenderOptions::RENDER_GEOMETRY)  && sceneRenderState.isEnabledOption(SceneRenderState::RenderOptions::RENDER_GEOMETRY)));
                pkg.setDrawOption(CmdRenderOptions::RENDER_WIREFRAME,
                                 (renderOptionEnabled(RenderOptions::RENDER_WIREFRAME) || sceneRenderState.isEnabledOption(SceneRenderState::RenderOptions::RENDER_WIREFRAME)));
            }
        }

        return true;
    }

    return false;
}

RenderPackage& RenderingComponent::getDrawPackage(const RenderStagePass& renderStagePass) {
    const U8 s = to_U8(renderStagePass._stage);
    const U8 p = to_U8(renderStagePass._stage == RenderStage::SHADOW ? RenderPassType::MAIN_PASS : renderStagePass._passType);
    const U32 i = renderStagePass._stage == RenderStage::SHADOW ? (renderStagePass._indexA * ShadowMap::MAX_PASSES_PER_LIGHT + renderStagePass._indexB) : renderStagePass._passIndex;

    return _renderPackages[s][p][i];
}

const RenderPackage& RenderingComponent::getDrawPackage(const RenderStagePass& renderStagePass) const {
    const U8 s = to_U8(renderStagePass._stage);
    const U8 p = to_U8(renderStagePass._stage == RenderStage::SHADOW ? RenderPassType::MAIN_PASS : renderStagePass._passType);
    const U32 i = renderStagePass._stage == RenderStage::SHADOW ? (renderStagePass._indexA * ShadowMap::MAX_PASSES_PER_LIGHT + renderStagePass._indexB) : renderStagePass._passIndex;

    return _renderPackages[s][p][i];
}

size_t RenderingComponent::getSortKeyHash(const RenderStagePass& renderStagePass) const {
    const RenderPackage& pkg = getDrawPackage(renderStagePass);
    return (pkg.empty() ? 0 : pkg.getSortKeyHash());
}

void RenderingComponent::updateReflectionIndex(ReflectorType type, I32 index) {
    _reflectionIndex = index;
    if (_reflectionIndex > -1) {
        RenderTarget& reflectionTarget = _context.renderTargetPool().renderTarget(RenderTargetID(type == ReflectorType::PLANAR ? RenderTargetUsage::REFLECTION_PLANAR
                                                                                                                               : RenderTargetUsage::REFLECTION_CUBE,
                                                     to_U16(index)));
        const Texture_ptr& refTex = reflectionTarget.getAttachment(RTAttachmentType::Colour, 0).texture();
        _externalTextures[getUsageIndex(type == ReflectorType::PLANAR ? RenderTargetUsage::REFLECTION_PLANAR
                                                                      : RenderTargetUsage::REFLECTION_CUBE)] = refTex;
    } else {
        _externalTextures[getUsageIndex(type == ReflectorType::PLANAR ? RenderTargetUsage::REFLECTION_PLANAR
                                                                      : RenderTargetUsage::REFLECTION_CUBE)] = _defaultReflection.first;
    }
}

bool RenderingComponent::clearReflection() {
    updateReflectionIndex(_reflectorType, -1);
    return true;
}

bool RenderingComponent::updateReflection(U16 reflectionIndex,
                                          Camera* camera,
                                          const SceneRenderState& renderState,
                                          GFX::CommandBuffer& bufferInOut)
{
    if (_reflectorType == ReflectorType::COUNT) {
        return false;
    }

    updateReflectionIndex(_reflectorType, reflectionIndex);

    RenderTargetID reflectRTID(_reflectorType == ReflectorType::PLANAR ? RenderTargetUsage::REFLECTION_PLANAR
                                                                       : RenderTargetUsage::REFLECTION_CUBE, 
                               reflectionIndex);

    if_constexpr(Config::Build::IS_DEBUG_BUILD) {
        const RenderTarget& target = _context.renderTargetPool().renderTarget(reflectRTID);

        DebugView* debugView = s_debugViews[0][reflectionIndex];
        if (debugView == nullptr) {
            DebugView_ptr viewPtr = eastl::make_shared<DebugView>();
            viewPtr->_texture = target.getAttachment(RTAttachmentType::Colour, 0).texture();
            viewPtr->_shader = _context.getRTPreviewShader(false);
            viewPtr->_shaderData.set(_ID("lodLevel"), GFX::PushConstantType::FLOAT, 0.0f);
            viewPtr->_shaderData.set(_ID("unpack1Channel"), GFX::PushConstantType::UINT, 0u);
            viewPtr->_shaderData.set(_ID("unpack2Channel"), GFX::PushConstantType::UINT, 0u);
            viewPtr->_shaderData.set(_ID("startOnBlue"), GFX::PushConstantType::UINT, 0u);

            viewPtr->_name = Util::StringFormat("REFLECTION_%d_%s", reflectRTID._index, _parentSGN.name().c_str());
            debugView = _context.addDebugView(viewPtr);
            s_debugViews[0][reflectionIndex] = debugView;
        } else {
            /*if (_context.getFrameCount() % (Config::TARGET_FRAME_RATE * 15) == 0) {
                if (debugView->_shader->getGUID() == _previewRenderTargetColour->getGUID()) {
                    debugView->_texture = target.getAttachment(RTAttachmentType::Depth, 0).texture();
                    debugView->_shader = _context.getRTPreviewShader(true);
                    debugView->_shaderData.set("zPlanes", GFX::PushConstantType::VEC2, camera->getZPlanes());
                } else {
                    debugView->_texture = target.getAttachment(RTAttachmentType::Colour, 0).texture();
                    debugView->_shader = _context.getRTPreviewShader(false);
                }
            }*/
        }
    }

    if (_reflectionCallback) {
        RenderCbkParams params(_context, _parentSGN, renderState, reflectRTID, reflectionIndex, to_U8(_reflectorType), camera);
        _reflectionCallback(params, bufferInOut);
    } else {
        if (_reflectorType == ReflectorType::CUBE) {
            const vec2<F32>& zPlanes = camera->getZPlanes();
            _context.generateCubeMap(reflectRTID,
                                     0,
                                     camera->getEye(),
                                     vec2<F32>(zPlanes.x, zPlanes.y * 0.25f),
                                     RenderStagePass{RenderStage::REFLECTION, RenderPassType::MAIN_PASS, to_U8(_reflectorType), reflectionIndex},
                                     bufferInOut);
        }
    }

    return true;
}

void RenderingComponent::updateRefractionIndex(ReflectorType type, I32 index) {
    _refractionIndex = index;
    if (_refractionIndex > -1) {
        RenderTarget& refractionTarget = _context.renderTargetPool().renderTarget(RenderTargetID(RenderTargetUsage::REFRACTION_PLANAR, to_U16(index)));
        const Texture_ptr& refTex = refractionTarget.getAttachment(RTAttachmentType::Colour, 0).texture();
        _externalTextures[getUsageIndex(RenderTargetUsage::REFRACTION_PLANAR)] = refTex;
    } else {
        _externalTextures[getUsageIndex(RenderTargetUsage::REFRACTION_PLANAR)] = _defaultRefraction.first;
    }
}

bool RenderingComponent::clearRefraction() {
    updateRefractionIndex(_reflectorType, -1);
    return true;
}

bool RenderingComponent::updateRefraction(U16 refractionIndex,
                                          Camera* camera,
                                          const SceneRenderState& renderState,
                                          GFX::CommandBuffer& bufferInOut) {
    // no default refraction system!
    if (_refractorType == RefractorType::COUNT) {
        return false;
    }

    updateRefractionIndex(_reflectorType, refractionIndex);

    RenderTargetID refractRTID(RenderTargetUsage::REFRACTION_PLANAR, refractionIndex);

    if_constexpr(Config::Build::IS_DEBUG_BUILD) {
        const RenderTarget& target = _context.renderTargetPool().renderTarget(refractRTID);

        DebugView* debugView = s_debugViews[1][refractionIndex];
        if (debugView == nullptr) {
            DebugView_ptr viewPtr = eastl::make_shared<DebugView>();
            viewPtr->_texture = target.getAttachment(RTAttachmentType::Colour, 0).texture();
            viewPtr->_shader = _context.getRTPreviewShader(false);
            viewPtr->_shaderData.set(_ID("lodLevel"), GFX::PushConstantType::FLOAT, 0.0f);
            viewPtr->_shaderData.set(_ID("unpack1Channel"), GFX::PushConstantType::UINT, 0u);
            viewPtr->_shaderData.set(_ID("unpack2Channel"), GFX::PushConstantType::UINT, 0u);
            viewPtr->_shaderData.set(_ID("startOnBlue"), GFX::PushConstantType::UINT, 0u);

            viewPtr->_name = Util::StringFormat("REFRACTION_%d_%s", refractRTID._index, _parentSGN.name().c_str());
            debugView = _context.addDebugView(viewPtr);
            s_debugViews[1][refractionIndex] = debugView;
        } else {
            /*if (_context.getFrameCount() % (Config::TARGET_FRAME_RATE * 15) == 0) {
                if (debugView->_shader->getGUID() == _previewRenderTargetColour->getGUID()) {
                    debugView->_texture = target.getAttachment(RTAttachmentType::Depth, 0).texture();
                    debugView->_shader = _context.getRTPreviewShader(true);
                    debugView->_shaderData.set("zPlanes", GFX::PushConstantType::VEC2, camera->getZPlanes());
                } else {
                    debugView->_texture = target.getAttachment(RTAttachmentType::Colour, 0).texture();
                    debugView->_shader = _context.getRTPreviewShader(false);
                }
            } */
        }
    }

    if (_refractionCallback) {
        RenderCbkParams params{ _context, _parentSGN, renderState, refractRTID, refractionIndex, to_U8(_refractorType), camera };
        _refractionCallback(params, bufferInOut);
        return true;
    }

    return false;
}

U32 RenderingComponent::defaultReflectionTextureIndex() const {
    return _reflectionIndex > -1 ? to_U32(_reflectionIndex) : _defaultReflection.second;
}

U32 RenderingComponent::defaultRefractionTextureIndex() const {
    return _refractionIndex > -1 ? to_U32(_refractionIndex) : _defaultRefraction.second;
}

void RenderingComponent::defaultReflectionTexture(const Texture_ptr& reflectionPtr, U32 arrayIndex) {
    _defaultReflection.first = reflectionPtr;
    _defaultReflection.second = arrayIndex;
}

void RenderingComponent::defaultRefractionTexture(const Texture_ptr& refractionPtr, U32 arrayIndex) {
    _defaultRefraction.first = refractionPtr;
    _defaultRefraction.second = arrayIndex;
}

void RenderingComponent::updateEnvProbeList(const EnvironmentProbeList& probes) {
    _envProbes.resize(0);
    if (probes.empty()) {
        return;
    }

    _envProbes.insert(std::cend(_envProbes), std::cbegin(probes), std::cend(probes));

    TransformComponent* const transform = _parentSGN.get<TransformComponent>();
    if (transform) {
        const vec3<F32>& nodePos = transform->getPosition();
        auto sortFunc = [&nodePos](const EnvironmentProbe_ptr& a, const EnvironmentProbe_ptr& b) -> bool {
            return a->distanceSqTo(nodePos) < b->distanceSqTo(nodePos);
        };

        std::sort(std::begin(_envProbes), std::end(_envProbes), sortFunc);
    }

    RenderTarget* rt = EnvironmentProbe::reflectionTarget()._rt;
    defaultReflectionTexture(rt->getAttachment(RTAttachmentType::Colour, 0).texture(), _envProbes.front()->getRTIndex());
}

/// Draw the axis arrow gizmo
void RenderingComponent::drawDebugAxis() {
    if (!Config::Build::IS_DEBUG_BUILD) {
        return;
    }

    if (!_axisGizmo) {
        Line temp;
        temp.widthStart(10.0f);
        temp.widthEnd(10.0f);
        temp.pointStart(VECTOR3_ZERO);

        Line axisLines[3];
        // Red X-axis
        temp.pointEnd(WORLD_X_AXIS * 4);
        temp.colourStart(UColour4(255, 0, 0, 255));
        temp.colourEnd(UColour4(255, 0, 0, 255));
        axisLines[0] = temp;

        // Green Y-axis
        temp.pointEnd(WORLD_Y_AXIS * 4);
        temp.colourStart(UColour4(0, 255, 0, 255));
        temp.colourEnd(UColour4(0, 255, 0, 255));
        axisLines[1] = temp;

        // Blue Z-axis
        temp.pointEnd(WORLD_Z_AXIS * 4);
        temp.colourStart(UColour4(0, 0, 255, 255));
        temp.colourEnd(UColour4(0, 0, 255, 255));
        axisLines[2] = temp;

        _axisGizmo = _context.newIMP();
        _axisGizmo->name("AxisGizmo_" + _parentSGN.name());
        _axisGizmo->skipPostFX(true);
        _axisGizmo->pipeline(*_primitivePipeline[2]);
        // Create the object containing all of the lines
        _axisGizmo->beginBatch(true, 3 * 2, 1);
        _axisGizmo->attribute4f(to_base(AttribLocation::COLOR), Util::ToFloatColour(axisLines[0].colourStart()));
        // Set the mode to line rendering
        _axisGizmo->begin(PrimitiveType::LINES);
        // Add every line in the list to the batch
        for (const Line& line : axisLines) {
            _axisGizmo->attribute4f(to_base(AttribLocation::COLOR), Util::ToFloatColour(line.colourStart()));
            _axisGizmo->vertex(line.pointStart());
            _axisGizmo->vertex(line.pointEnd());
        }
        _axisGizmo->end();
        // Finish our object
        _axisGizmo->endBatch();
    }

    TransformComponent* const transform = _parentSGN.get<TransformComponent>();
    if (transform) {
        mat4<F32> tempOffset(GetMatrix(transform->getOrientation()), false);
        tempOffset.setTranslation(transform->getPosition());
        _axisGizmo->worldMatrix(tempOffset);
    } else {
        _axisGizmo->resetWorldMatrix();
    }
}

void RenderingComponent::cullFlagValue(F32 newValue) {
    _cullFlagValue = newValue;
}

};
