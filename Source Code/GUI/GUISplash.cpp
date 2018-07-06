#include "Headers/GUISplash.h"

#include "Core/Headers/ParamHandler.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Textures/Headers/Texture.h"
#include "Platform/Video/Shaders/Headers/ShaderProgram.h"

namespace Divide {

GUISplash::GUISplash(const stringImpl& splashImageName,
                     const vec2<U16>& dimensions) 
    : _dimensions(dimensions)
{
    SamplerDescriptor splashSampler;
    splashSampler.toggleMipMaps(false);
    splashSampler.setAnisotropy(0);
    splashSampler.setWrapMode(TextureWrap::CLAMP);
    splashSampler.toggleSRGBColourSpace(true);
    ResourceDescriptor splashImage("SplashScreen Texture");
    splashImage.setThreadedLoading(false);
    splashImage.setPropertyDescriptor<SamplerDescriptor>(splashSampler);
    stringImpl splashImageLocation = 
        Util::StringFormat("%s/misc_images/%s",
                           ParamHandler::instance().getParam<stringImpl>(_ID("assetsLocation")).c_str(),
                           splashImageName.c_str());
    splashImage.setResourceLocation(splashImageLocation);
    splashImage.setEnumValue(to_const_uint(TextureType::TEXTURE_2D));
    _splashImage = CreateResource<Texture>(splashImage);
    ResourceDescriptor splashShader("fbPreview");
    splashShader.setThreadedLoading(false);
    _splashShader = CreateResource<ShaderProgram>(splashShader);
}

GUISplash::~GUISplash()
{
}

void GUISplash::render() {
    GFX::ScopedViewport splashViewport(vec4<I32>(0, 0, _dimensions.width, _dimensions.height));
    _splashImage->bind(to_const_ubyte(ShaderProgram::TextureUsage::UNIT0));


    GenericDrawCommand triangleCmd;
    triangleCmd.primitiveType(PrimitiveType::TRIANGLES);
    triangleCmd.drawCount(1);
    triangleCmd.stateHash(GFXDevice::instance().getDefaultStateBlock(true));
    triangleCmd.shaderProgram(_splashShader);

    GFXDevice::instance().draw(triangleCmd);
}

};