#include "Headers/Mesh.h"
#include "Headers/SubMesh.h"

#include "Managers/Headers/SceneManager.h"
#include "Core/Headers/ParamHandler.h"
#include "Core/Math/Headers/Transform.h"
#include "Geometry/Animations/Headers/SceneAnimator.h"

namespace Divide {

Mesh::Mesh(ObjectFlag flag)
    : Object3D(ObjectType::MESH, flag),
      _visibleToNetwork(true),
      _animator(nullptr)
{
}

Mesh::~Mesh()
{
}

/// Mesh bounding box is built from all the SubMesh bounding boxes
void Mesh::computeBoundingBox() {
    _boundingBox.reset();
}

void Mesh::addSubMesh(SubMesh* const subMesh) {
    // Hold a reference to the submesh by ID (used for animations)
    _subMeshList.push_back(subMesh);
    Attorney::SubMeshMesh::setParentMesh(*subMesh, this);
    computeBoundingBox();
}

/// After we loaded our mesh, we need to add submeshes as children nodes
void Mesh::postLoad(SceneGraphNode& sgn) {
    static const U32 normalMask = to_const_uint(SGNComponent::ComponentType::NAVIGATION) |
                                  to_const_uint(SGNComponent::ComponentType::PHYSICS) |
                                  to_const_uint(SGNComponent::ComponentType::BOUNDS) |
                                  to_const_uint(SGNComponent::ComponentType::RENDERING);

    static const U32 skinnedMask = normalMask | 
                                   to_const_uint(SGNComponent::ComponentType::ANIMATION) |
                                   to_const_uint(SGNComponent::ComponentType::INVERSE_KINEMATICS) |
                                   to_const_uint(SGNComponent::ComponentType::RAGDOLL);

    for (SubMesh* submesh : _subMeshList) {
        sgn.addNode(*submesh,
                    submesh->hasFlag(ObjectFlag::OBJECT_FLAG_SKINNED) ? skinnedMask : normalMask,
                    Util::StringFormat("%s_%d",
                                        sgn.getName().c_str(),
                                        submesh->getID()));
    }
    sgn.get<BoundsComponent>()->lockBBTransforms(true);
    Object3D::postLoad(sgn);
}

/// Called from SceneGraph "sceneUpdate"
void Mesh::sceneUpdate(const U64 deltaTime, SceneGraphNode& sgn,
                       SceneState& sceneState) {
    if (hasFlag(ObjectFlag::OBJECT_FLAG_SKINNED)) {
        bool playAnimations = sceneState.renderState().playAnimations() && _playAnimations;
        U32 childCount = sgn.getChildCount();
        for (U32 i = 0; i < childCount; ++i) {
            AnimationComponent* comp = sgn.getChild(i, childCount).get<AnimationComponent>();
            // Not all submeshes are necessarily animated. (e.g. flag on the back of a character)
            if (comp) {
                comp->playAnimations(playAnimations && comp->playAnimations());
                comp->incParentTimeStamp(deltaTime);
            }
        }
    }

    SceneNode::sceneUpdate(deltaTime, sgn, sceneState);
}
};