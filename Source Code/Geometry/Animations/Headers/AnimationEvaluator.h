/*
   Copyright (c) 2018 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software
   and associated documentation files (the "Software"), to deal in the Software
   without restriction,
   including without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
   PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
   DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
   IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

/*Code references:
    http://nolimitsdesigns.com/game-design/open-asset-import-library-animation-loader/
*/

#pragma once
#ifndef ANIMATION_EVALUATOR_H_
#define ANIMATION_EVALUATOR_H_

#include "Bone.h"

struct aiAnimation;
struct aiVectorKey;
struct aiQuatKey;
struct aiVectorKey;

namespace Divide {
class ByteBuffer;
class ShaderBuffer;

struct AnimationChannel {
    vectorEASTL<aiVectorKey> _positionKeys;
    vectorEASTL<aiQuatKey>   _rotationKeys;
    vectorEASTL<aiVectorKey> _scalingKeys;
    U64 _nameKey = 0ULL;
    stringImpl _name = "";
    /** The number of position keys */
    U32 _numPositionKeys = 0u;
    U32 _numRotationKeys = 0u;
    U32 _numScalingKeys = 0u;
};

struct BoneTransform
{
    using Container = vectorEASTL<mat4<F32>>;
    PROPERTY_RW(Container, matrices);

    inline size_t count() const noexcept { return matrices().size(); }
};

class GFXDevice;
class AnimEvaluator {
   public:
    AnimEvaluator() = default;
    ~AnimEvaluator() = default;

    explicit AnimEvaluator(const aiAnimation* pAnim, U32 idx) noexcept;

    void evaluate(const D64 dt, Bone* skeleton);

    I32 frameIndexAt(const D64 elapsedTime) const noexcept;

    inline U32 frameCount() const noexcept { return to_U32(_transforms.size()); }

    inline vectorEASTL<BoneTransform>& transforms() noexcept { return _transforms; }
    
    inline const vectorEASTL<BoneTransform>& transforms() const noexcept { return _transforms; }

    inline BoneTransform& transforms(const U32 frameIndex) {
        assert(frameIndex < to_U32(_transforms.size()));
        return _transforms[frameIndex];
    }

    inline const BoneTransform& transforms(const U32 frameIndex) const {
        assert(frameIndex < to_U32(_transforms.size()));
        return _transforms[frameIndex];
    }

    inline BoneTransform& transforms(const D64 elapsedTime, I32& resultingFrameIndex) {
        resultingFrameIndex = frameIndexAt(elapsedTime);
        return transforms(to_U32(resultingFrameIndex));
    }

    inline BoneTransform& transforms(const D64 elapsedTime) {
        I32 resultingFrameIndex = 0;
        return transforms(elapsedTime, resultingFrameIndex);
    }

    inline const BoneTransform& transforms(const D64 elapsedTime, I32& resultingFrameIndex) const {
        resultingFrameIndex = frameIndexAt(elapsedTime);
        return transforms(to_U32(resultingFrameIndex));
    }

    inline const BoneTransform& transforms(const D64 elapsedTime) const {
        I32 resultingFrameIndex = 0;
        return transforms(elapsedTime, resultingFrameIndex);
    }

    bool initBuffers(GFXDevice& context);

    static void save(const AnimEvaluator& evaluator, ByteBuffer& dataOut);
    static void load(AnimEvaluator& evaluator, ByteBuffer& dataIn);

    PROPERTY_RW(D64, ticksPerSecond, 0.0);
    PROPERTY_RW(bool, playAnimationForward, true);
    PROPERTY_R_IW(D64, duration, 0.0);
    PROPERTY_R_IW(stringImpl, name, "");

    /// GPU buffer to hold bone transforms
    POINTER_R_IW(ShaderBuffer, boneBuffer, nullptr);

   protected:
    /// Array to return transformations results inside.
    vectorEASTL<BoneTransform> _transforms;
    vectorEASTL<vec3<U32>> _lastPositions;
    /// vector that holds all bone channels
    vectorEASTL<AnimationChannel> _channels;
    D64 _lastTime = 0.0;
};

};  // namespace Divide

#endif