#include "stdafx.h"

#include "Headers/CubeShadowMapGenerator.h"

#include "Core/Headers/StringHelper.h"
#include "Graphs/Headers/SceneGraph.h"
#include "Managers/Headers/SceneManager.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Rendering/Lighting/Headers/Light.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Shaders/Headers/ShaderProgram.h"

namespace Divide {

CubeShadowMapGenerator::CubeShadowMapGenerator(GFXDevice& context)
    : ShadowMapGenerator(context)
{
    Console::printfn(Locale::get(_ID("LIGHT_CREATE_SHADOW_FB")), "Single Shadow Map");
}

void CubeShadowMapGenerator::render(const Camera& playerCamera, Light& light, U32 lightIndex, GFX::CommandBuffer& bufferInOut) {
    ACKNOWLEDGE_UNUSED(playerCamera);

    _context.generateCubeMap(RenderTargetID(RenderTargetUsage::SHADOW, to_base(ShadowType::CUBEMAP)),
                             light.getShadowOffset(),
                             light.getPosition(),
                             vec2<F32>(0.1f, light.getRange()),
                             RenderStagePass(RenderStage::SHADOW, RenderPassType::DEPTH_PASS, to_U8(light.getLightType())),
                             lightIndex,
                             bufferInOut,
                             light.shadowCameras()[0]);
}

};