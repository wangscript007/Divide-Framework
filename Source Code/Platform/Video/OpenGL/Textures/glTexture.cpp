#include "Platform/Video/OpenGL/Headers/glResources.h"

#include "Headers/glTexture.h"
#include "Platform/Video/Headers/GFXDevice.h"

#include "Core/Headers/Console.h"
#include "Utility/Headers/Localization.h"

namespace Divide {

glTexture::glTexture(TextureType type, bool flipped)
    : Texture(type, flipped)

{
    _format = _internalFormat = GL_NONE;
    _allocatedStorage = false;
    U32 tempHandle = 0;
    glCreateTextures(GLUtil::glTextureTypeTable[to_uint(type)], 1, &tempHandle);
    DIVIDE_ASSERT(tempHandle != 0,
                  "glTexture error: failed to generate new texture handle!");
    _textureData.setHandleHigh(tempHandle);
    _mipMaxLevel = _mipMinLevel = 0;
}

glTexture::~glTexture() {
}

bool glTexture::unload() {
    U32 textureID = _textureData.getHandleHigh();
    if (textureID > 0) {
        glDeleteTextures(1, &textureID);
        _textureData.setHandleHigh((U32)0);
    }

    return true;
}

void glTexture::threadedLoad(const stringImpl& name) {
    updateSampler();
    Texture::generateHWResource(name);
    Resource::threadedLoad(name);
}

void glTexture::setMipMapRange(GLushort base, GLushort max) {
    if (!_samplerDescriptor.generateMipMaps()) {
        _mipMaxLevel = 1;
        return;
    }

    if (_mipMinLevel == base && _mipMaxLevel == max) {
        return;
    }

    _mipMinLevel = base;
    _mipMaxLevel = max;

    glTextureParameterf(_textureData.getHandleHigh(), GL_TEXTURE_BASE_LEVEL,
                        base);
    glTextureParameterf(_textureData.getHandleHigh(), GL_TEXTURE_MAX_LEVEL,
                        max);
}

void glTexture::updateMipMaps() {
    if (_mipMapsDirty && _samplerDescriptor.generateMipMaps()) {
        glGenerateTextureMipmap(_textureData.getHandleHigh());
        _mipMapsDirty = false;
    }
}

void glTexture::updateSampler() {
    if (_samplerDirty) {
        GL_API::getOrCreateSamplerObject(_samplerDescriptor);
        _samplerDirty = false;
    }
}

bool glTexture::generateHWResource(const stringImpl& name) {
    GFX_DEVICE.loadInContext(
        _threadedLoading ? CurrentContext::GFX_LOADING_CTX
                         : CurrentContext::GFX_RENDERING_CTX,
        DELEGATE_BIND(&glTexture::threadedLoad, this, name));

    return true;
}

void glTexture::reserveStorage() {
    DIVIDE_ASSERT(
        !(_textureData._textureType == TextureType::TEXTURE_CUBE_MAP &&
          _width != _height),
        "glTexture::reserverStorage error: width and heigth for "
        "cube map texture do not match!");

    switch (_textureData._textureType) {
        case TextureType::TEXTURE_1D: {
            glTextureStorage1D(_textureData.getHandleHigh(), _mipMaxLevel,
                               _internalFormat, _width);
        } break;
        case TextureType::TEXTURE_2D:
        case TextureType::TEXTURE_CUBE_MAP: {
            glTextureStorage2D(_textureData.getHandleHigh(), _mipMaxLevel,
                               _internalFormat, _width, _height);
        } break;
        case TextureType::TEXTURE_2D_MS: {
            glTextureStorage2DMultisample(_textureData.getHandleHigh(),
                                          GFX_DEVICE.gpuState().MSAASamples(),
                                          _internalFormat, _width, _height,
                                          GL_TRUE);
        } break;
        case TextureType::TEXTURE_2D_ARRAY_MS: {
            glTextureStorage3DMultisample(_textureData.getHandleHigh(),
                                          GFX_DEVICE.gpuState().MSAASamples(),
                                          _internalFormat, _width, _height,
                                          _numLayers, GL_TRUE);
        } break;
        case TextureType::TEXTURE_2D_ARRAY:
        case TextureType::TEXTURE_CUBE_ARRAY:
        case TextureType::TEXTURE_3D: {
            // Use _imageLayers as depth for GL_TEXTURE_3D
            glTextureStorage3D(_textureData.getHandleHigh(), _mipMaxLevel,
                               _internalFormat, _width, _height, _numLayers);
        } break;
        default:
            return;
    };

    _allocatedStorage = true;
}

void glTexture::loadData(GLuint target,
                         const GLubyte* const ptr,
                         const vec2<GLushort>& dimensions,
                         const vec2<GLushort>& mipLevels,
                         GFXImageFormat format,
                         GFXImageFormat internalFormat,
                         bool usePOW2) {
    bool isTextureLayer =
        (_textureData._textureType == TextureType::TEXTURE_2D_ARRAY &&
         target > 0);

    if (!isTextureLayer) {
        _format = GLUtil::glImageFormatTable[to_uint(format)];
        _internalFormat =
            internalFormat == GFXImageFormat::DEPTH_COMPONENT
                ? GL_DEPTH_COMPONENT32
                : GLUtil::glImageFormatTable[to_uint(internalFormat)];
        setMipMapRange(mipLevels.x, mipLevels.y);

        if (Config::Profile::USE_2x2_TEXTURES) {
            _width = _height = 2;
        } else {
            _width = dimensions.width;
            _height = dimensions.height;
            assert(_width > 0 && _height > 0);
        }
    } else {
        DIVIDE_ASSERT(
            _width == dimensions.width && _height == dimensions.height,
            "glTexture error: Invalid dimensions for texture array layer");
    }

    if (ptr) {
        assert(_bitDepth != 0);
        if (usePOW2) {
            GLushort xSize2 = _width, ySize2 = _height;
            GLdouble xPow2 = log((GLdouble)xSize2) / log(2.0);
            GLdouble yPow2 = log((GLdouble)ySize2) / log(2.0);

            GLushort ixPow2 = (GLushort)xPow2;
            GLushort iyPow2 = (GLushort)yPow2;

            if (xPow2 != (GLdouble)ixPow2) {
                ixPow2++;
            }
            if (yPow2 != (GLdouble)iyPow2) {
                yPow2++;
            }
            xSize2 = 1 << ixPow2;
            ySize2 = 1 << iyPow2;
            _power2Size = (_width == xSize2) && (_height == ySize2);
        }
        if (!_allocatedStorage) {
            reserveStorage();
        }
        assert(_allocatedStorage);

        GL_API::setPixelPackUnpackAlignment();
        switch (_textureData._textureType) {
            case TextureType::TEXTURE_1D: {
                glTextureSubImage1D(_textureData.getHandleHigh(), 0, 0, _width,
                                    _format, GL_UNSIGNED_BYTE, ptr);
            } break;
            case TextureType::TEXTURE_3D:
            case TextureType::TEXTURE_CUBE_MAP:
            case TextureType::TEXTURE_CUBE_ARRAY:
            case TextureType::TEXTURE_2D_ARRAY:
            case TextureType::TEXTURE_2D_ARRAY_MS: {
                glTextureSubImage3D(_textureData.getHandleHigh(), 0, 0, 0,
                                    target, _width, _height, 1, _format,
                                    GL_UNSIGNED_BYTE, ptr);
            } break;
            default:
            case TextureType::TEXTURE_2D:
            case TextureType::TEXTURE_2D_MS: {
                glTextureSubImage2D(_textureData.getHandleHigh(), 0, 0, 0,
                                    _width, _height, _format, GL_UNSIGNED_BYTE,
                                    ptr);
            } break;
        }
        _mipMapsDirty = true;
        updateMipMaps();
    } else {
        if (!_allocatedStorage) {
            reserveStorage();
        }
        assert(_allocatedStorage);
    }

    DIVIDE_ASSERT(_width > 0 && _height > 0,
                  "glTexture error: Invalid texture dimensions!");
}

void glTexture::Bind(GLubyte unit) {
    updateSampler();
    updateMipMaps();
    GL_API::bindTexture(
        unit, _textureData.getHandleHigh(),
        GLUtil::glTextureTypeTable[to_uint(_textureData._textureType)],
        _textureData._samplerHash);
}
};