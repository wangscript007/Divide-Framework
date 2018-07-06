#include "Headers/SkinnedSubMesh.h"
#include "Headers/Mesh.h"

#include "Core/Headers/ParamHandler.h"
#include "Core/Math/Headers/Transform.h"
#include "Managers/Headers/SceneManager.h"

#include "Geometry/Animations/Headers/SceneAnimator.h"
#include "Graphs/Components/Headers/AnimationComponent.h"

namespace Divide {

namespace {
    const bool MULTITHREADED_BOUNDING_BOX_CALCULATION = false;
};

SkinnedSubMesh::SkinnedSubMesh(const stringImpl& name)
    : SubMesh(name, Object3D::ObjectFlag::OBJECT_FLAG_SKINNED),
    _parentAnimatorPtr(nullptr)
{
}

SkinnedSubMesh::~SkinnedSubMesh()
{
}

/// After we loaded our mesh, we need to add submeshes as children nodes
void SkinnedSubMesh::postLoad(SceneGraphNode& sgn) {
    if (_parentAnimatorPtr == nullptr) {
        _parentAnimatorPtr = _parentMesh->getAnimator();
        vectorAlg::vecSize animationCount = _parentAnimatorPtr->animations().size();
        _boundingBoxesAvailable.resize(animationCount);
        _boundingBoxesComputing.resize(animationCount);
        _boundingBoxes.resize(animationCount);
        for (vectorAlg::vecSize idx = 0; idx < animationCount; ++idx) {
            _boundingBoxes.at(idx).resize(_parentAnimatorPtr->frameCount(to_int(idx)));
        }
    }

    
    sgn.get<AnimationComponent>()->updateAnimator(_parentAnimatorPtr);

    SubMesh::postLoad(sgn);
}

/// update possible animations
void SkinnedSubMesh::updateAnimations(SceneGraphNode& sgn) {
    assert(sgn.get<AnimationComponent>());

    computeBoundingBoxForCurrentFrame(sgn);
}

void SkinnedSubMesh::buildBoundingBoxesForAnimCompleted(U32 animationIndex) {
    _boundingBoxesComputing.at(animationIndex) = false;
    _boundingBoxesAvailable.at(animationIndex) = true;
}

void SkinnedSubMesh::buildBoundingBoxesForAnim(U32 animationIndex, AnimationComponent* const animComp) {

    if (_boundingBoxesComputing.at(animationIndex) == true) {
        return;
    }

    _boundingBoxesComputing.at(animationIndex) = true;
    _boundingBoxesAvailable.at(animationIndex) = false;

    const vectorImpl<vectorImpl<mat4<F32>>>& currentAnimation = 
        animComp->getAnimationByIndex(animationIndex).transforms();

    VertexBuffer* parentVB = _parentMesh->getGeometryVB();
    U32 partitionOffset = parentVB->getPartitionOffset(_geometryPartitionID);
    U32 partitionCount = parentVB->getPartitionIndexCount(_geometryPartitionID);

    U32 i = 0;
    BoundingBoxPerFrame& currentBBs = _boundingBoxes.at(animationIndex);
    for (BoundingBox& bb : currentBBs) {
        bb.reset();
        const vectorImpl<mat4<F32> >& transforms = currentAnimation[i++];
        // loop through all vertex weights of all bones
        for (U32 j = 0; j < partitionCount; ++j) {
            U32 idx = parentVB->getIndex(j + partitionOffset);
            P32 ind = parentVB->getBoneIndices(idx);
            const vec4<F32>& wgh = parentVB->getBoneWeights(idx);
            const vec3<F32>& curentVert = parentVB->getPosition(idx);

            bb.add((wgh.x * (transforms[ind.b[0]] * curentVert)) +
                   (wgh.y * (transforms[ind.b[1]] * curentVert)) +
                   (wgh.z * (transforms[ind.b[2]] * curentVert)) +
                   (wgh.w * (transforms[ind.b[3]] * curentVert)));
        }
    }
}

void SkinnedSubMesh::computeBoundingBoxForCurrentFrame(SceneGraphNode& sgn) {
    AnimationComponent* animComp = sgn.get<AnimationComponent>();
    // If animations are paused or unavailable, keep the current BB
    if (!animComp->playAnimations()) {
        return;
    }
    // Attempt to get the map of BBs for the current animation
    U32 animationIndex = animComp->animationIndex();

    if (_boundingBoxesAvailable.at(animationIndex) == false) {
        if (_boundingBoxesComputing.at(animationIndex) == false) {
            if (MULTITHREADED_BOUNDING_BOX_CALCULATION) {
                DELEGATE_CBK<> buildBB = DELEGATE_BIND(&SkinnedSubMesh::buildBoundingBoxesForAnim,
                                                       this, animationIndex, animComp);
                DELEGATE_CBK<> builBBComplete = DELEGATE_BIND(&SkinnedSubMesh::buildBoundingBoxesForAnimCompleted,
                                                              this, animationIndex);
                _bbBuildTasks.push_back(Application::getInstance()
                    .getKernel()
                    .AddTask(1, 1, buildBB, builBBComplete));
                _bbBuildTasks.back().lock()->startTask(Task::TaskPriority::DONT_CARE);
            } else {
                buildBoundingBoxesForAnim(animationIndex, animComp);
                buildBoundingBoxesForAnimCompleted(animationIndex);
            }
        }
    }
}

bool SkinnedSubMesh::checkBoundingBox(const SceneGraphNode& sgn) {
    AnimationComponent* animComp = sgn.get<AnimationComponent>();

    // If animations are paused or unavailable, keep the current BB
    if (animComp->playAnimations()) {
        // Attempt to get the map of BBs for the current animation
        U32 animationIndex = animComp->animationIndex();
        if (_boundingBoxesAvailable.at(animationIndex) == true) {
            _boundingBox.set(_boundingBoxes.at(animationIndex).at(animComp->frameIndex()));
            return true;
        }
    }

    return SceneNode::checkBoundingBox(sgn);
}

};