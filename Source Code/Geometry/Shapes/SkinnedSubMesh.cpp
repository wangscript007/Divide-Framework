#include "Headers/SkinnedSubMesh.h"
#include "Headers/Mesh.h"

#include "Core/Headers/ParamHandler.h"
#include "Core/Math/Headers/Transform.h"
#include "Managers/Headers/SceneManager.h"

#include "Geometry/Animations/Headers/SceneAnimator.h"
#include "Graphs/Components/Headers/AnimationComponent.h"

namespace Divide {

const static bool USE_MUTITHREADED_LOADING = false;

SkinnedSubMesh::SkinnedSubMesh(const stringImpl& name)
    : SubMesh(name, Object3D::ObjectFlag::OBJECT_FLAG_SKINNED),
    _parentAnimatorPtr(nullptr)
{
    _buildingBoundingBoxes = false;
}

SkinnedSubMesh::~SkinnedSubMesh()
{
}

/// After we loaded our mesh, we need to add submeshes as children nodes
void SkinnedSubMesh::postLoad(SceneGraphNode& sgn) {
    if (_parentAnimatorPtr == nullptr) {
        _parentAnimatorPtr = _parentMesh->getAnimator();
    }

    sgn.setComponent(
        SGNComponent::ComponentType::ANIMATION,
        MemoryManager_NEW AnimationComponent(*_parentAnimatorPtr, sgn));

    SubMesh::postLoad(sgn);
}

/// update possible animations
bool SkinnedSubMesh::updateAnimations(SceneGraphNode& sgn) {
    assert(sgn.getComponent<AnimationComponent>());

    return getBoundingBoxForCurrentFrame(sgn);
}

void SkinnedSubMesh::buildBoundingBoxesForAnimCompleted(U32 animationIndex) {
    _buildingBoundingBoxes = false;
    std::atomic_bool& currentBBStatus = _boundingBoxesAvailable[animationIndex];
    currentBBStatus = true;
    std::atomic_bool& currentBBComputing = _boundingBoxesComputing[animationIndex];
    currentBBComputing = false;
}

void SkinnedSubMesh::buildBoundingBoxesForAnim(
    U32 animationIndex, AnimationComponent* const animComp) {
    const AnimEvaluator& currentAnimation =
        animComp->GetAnimationByIndex(animationIndex);

    boundingBoxPerFrame& currentBBs = _boundingBoxes[animationIndex];
    std::atomic_bool& currentBBStatus = _boundingBoxesAvailable[animationIndex];
    std::atomic_bool& currentBBComputing = _boundingBoxesComputing[animationIndex];
    // We might need to recompute BBs so clear any possible old values
    currentBBs.clear();
    currentBBStatus = false;
    currentBBComputing = true;

    VertexBuffer* parentVB = _parentMesh->getGeometryVB();
    U32 partitionOffset = parentVB->getPartitionOffset(_geometryPartitionID);
    U32 partitionCount =
        parentVB->getPartitionCount(_geometryPartitionID) + partitionOffset;

    const vectorImpl<vec3<F32> >& verts = parentVB->getPosition();
    const vectorImpl<P32 >& indices = parentVB->getBoneIndices();
    const vectorImpl<vec4<F32> >& weights = parentVB->getBoneWeights();

    I32 frameCount = animComp->frameCount(animationIndex);
    //#pragma omp parallel for
    for (I32 i = 0; i < frameCount; ++i) {
        BoundingBox& bb = currentBBs[i];
        bb.reset();

        const vectorImpl<mat4<F32> >& transforms =
            currentAnimation.transforms(to_uint(i));

        // loop through all vertex weights of all bones
        for (U32 j = partitionOffset; j < partitionCount; ++j) {
            U32 idx = parentVB->getIndex(j);
            P32 ind = indices[idx];
            const vec4<F32>& wgh = weights[idx];
            const vec3<F32>& curentVert = verts[idx];

            F32 fwgh = 1.0f - (wgh.x + wgh.y + wgh.z);

            bb.Add((wgh.x * (transforms[ind.b[0]] * curentVert)) +
                   (wgh.y * (transforms[ind.b[1]] * curentVert)) +
                   (wgh.z * (transforms[ind.b[2]] * curentVert)) +
                   (fwgh  * (transforms[ind.b[3]] * curentVert)));
        }

        bb.setComputed(true);
    }
}

bool SkinnedSubMesh::getBoundingBoxForCurrentFrame(SceneGraphNode& sgn) {
    AnimationComponent* animComp = sgn.getComponent<AnimationComponent>();
    // If anymations are paused or unavailable, keep the current BB
    if (!animComp->playAnimations()) {
        return true;
    }
    // Attempt to get the map of BBs for the current animation
    U32 animationIndex = animComp->animationIndex();
    boundingBoxPerFrame& animBB = _boundingBoxes[animationIndex];
    // If the BBs are computed, set the BB for the current frame as the node BB
    if (!animBB.empty()) {
        // Update the BB, only if the calculation task has finished
        if (!_boundingBoxesComputing[animationIndex]) {
            sgn.setInitialBoundingBox(animBB[animComp->frameIndex()]);
            return true;
        }
        return false;
    }

    if (!_buildingBoundingBoxes) {
        _buildingBoundingBoxes = true;
        if (USE_MUTITHREADED_LOADING) {
            Kernel& kernel = Application::getInstance().getKernel();
            Task* task = kernel.AddTask(
                1, 1,
                DELEGATE_BIND(&SkinnedSubMesh::buildBoundingBoxesForAnim,
                                this, animationIndex, animComp),
                DELEGATE_BIND(
                    &SkinnedSubMesh::buildBoundingBoxesForAnimCompleted,
                    this, animationIndex));
            task->startTask();
        } else {
            buildBoundingBoxesForAnim(animationIndex, animComp);
            buildBoundingBoxesForAnimCompleted(animationIndex);
        }
    }

    return true;
}
};