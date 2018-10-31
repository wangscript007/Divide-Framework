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

#ifndef _CSM_GENERATOR_H_
#define _CSM_GENERATOR_H_

#include "ShadowMap.h"
#include "Platform/Video/Headers/PushConstants.h"
#include "Core/Math/BoundingVolumes/Headers/BoundingBox.h"

namespace Divide {

class Quad3D;
class Camera;
class GFXDevice;
class ShaderBuffer;
class DirectionalLightComponent;

struct DebugView;

FWD_DECLARE_MANAGED_CLASS(SceneGraphNode);

/// Directional lights can't deliver good quality shadows using a single shadow map.
/// This technique offers an implementation of the CSM method
class CascadedShadowMapsGenerator : public ShadowMapGenerator {
   public:
    explicit CascadedShadowMapsGenerator(GFXDevice& context);
    ~CascadedShadowMapsGenerator();

    void render(const Camera& playerCamera, Light& light, U32 lightIndex, GFX::CommandBuffer& bufferInOut) override;

   protected:
    typedef std::array<F32, Config::Lighting::MAX_SPLITS_PER_LIGHT> SplitDepths;

    void postRender(const DirectionalLightComponent& light, GFX::CommandBuffer& bufferInOut);
    SplitDepths calculateSplitDepths(const mat4<F32>& projMatrix,
                                     DirectionalLightComponent& light,
                                     const vec2<F32>& nearFarPlanes);
    void applyFrustumSplits(DirectionalLightComponent& light,
                            const mat4<F32>& invViewMatrix,
                            U8 numSplits,
                            const SplitDepths& splitDepths,
                            const vec2<F32>& nearFarPlanes,
                            std::array<vec3<F32>, 8>& frustumWS,
                            std::array<vec3<F32>, 8>& frustumVS);

   protected:
    U32 _vertBlur;
    U32 _horizBlur;
    ShaderProgram_ptr _blurDepthMapShader;
    PushConstants     _blurDepthMapConstants;
    RenderTargetHandle _drawBuffer;
    RenderTargetHandle _blurBuffer;
};

};  // namespace Divide
#endif //_CSM_GENERATOR_H_