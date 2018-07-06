#include "Platform/Video/OpenGL/Headers/glResources.h"

#include "Headers/glTexture.h"
#include "Platform/Video/Headers/GFXDevice.h"

#include "Core/Headers/Console.h"
#include "Utility/Headers/Localization.h"

namespace Divide {

glTexture::glTexture(TextureType type, bool flipped)
    : Texture(type, flipped)

{
    _type = GLUtil::GL_ENUM_TABLE::glTextureTypeTable[to_uint(type)];
    _format = _internalFormat = GL_NONE;
    _allocatedStorage = false;
    _samplerCreated = false;
    U32 tempHandle = 0;
    glGenTextures(1, &tempHandle);
    DIVIDE_ASSERT(tempHandle != 0,
                  "glTexture error: failed to generate new texture handle!");
    _handle = tempHandle;
    _mipMaxLevel = _mipMinLevel = 0;
}

glTexture::~glTexture() 
{
}

bool glTexture::unload() {
    if (_handle > 0) {
        U32 textureID = _handle;
        glDeleteTextures(1, &textureID);
        _handle = 0;
    }

    return true;
}

void glTexture::threadedLoad(const stringImpl& name) {
    if (!_samplerCreated) {
        _samplerHash = GL_API::getOrCreateSamplerObject(_samplerDescriptor);
        _samplerCreated = true;
    }

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

    glTextureParameterf(_handle, GL_TEXTURE_BASE_LEVEL, base);
    glTextureParameterf(_handle, GL_TEXTURE_MAX_LEVEL, max);
}

void glTexture::updateMipMaps() {
    if (_mipMapsDirty && _samplerDescriptor.generateMipMaps()) {
        gl45ext::glGenerateTextureMipmapEXT(_handle, _type);
        _mipMapsDirty = false;
    }
}

void glTexture::updateSampler() {
    if (_samplerDirty) {
        _samplerHash = GL_API::getOrCreateSamplerObject(_samplerDescriptor);
        _samplerDirty = false;
    }
}

bool glTexture::generateHWResource(const stringImpl& name) {
    GFX_DEVICE.loadInContext(
        _threadedLoading ? CurrentContext::GFX_LOADING_CONTEXT :
                           CurrentContext::GFX_RENDERING_CONTEXT,
        DELEGATE_BIND(&glTexture::threadedLoad, this, name));

    return true;
}

void glTexture::reserveStorage() {
    // generate empty texture data using each texture type's specific function
    switch (_type) {
        case GL_TEXTURE_1D: {
            gl45ext::glTextureStorage1DEXT(_handle, _type, _mipMaxLevel,
                                           _internalFormat, _width);
        } break;
        case GL_TEXTURE_2D_MULTISAMPLE: {
            gl45ext::glTextureStorage2DMultisampleEXT(
                _handle, _type, GFX_DEVICE.gpuState().MSAASamples(),
                _internalFormat, _width, _height, GL_TRUE);
        } break;
        case GL_TEXTURE_2D:
        case GL_TEXTURE_CUBE_MAP: {
            gl45ext::glTextureStorage2DEXT(_handle, _type, _mipMaxLevel,
                                           _internalFormat, _width, _height);
        } break;
        case GL_TEXTURE_2D_MULTISAMPLE_ARRAY: {
            gl45ext::glTextureStorage3DMultisampleEXT(
                _handle, _type, GFX_DEVICE.gpuState().MSAASamples(),
                _internalFormat, _width, _height, _numLayers, GL_TRUE);
        } break;
        case GL_TEXTURE_2D_ARRAY:
        case GL_TEXTURE_CUBE_MAP_ARRAY:
        case GL_TEXTURE_3D: {
            // Use _imageLayers as depth for GL_TEXTURE_3D
            gl45ext::glTextureStorage3DEXT(_handle, _type, _mipMaxLevel,
                                           _internalFormat, _width, _height,
                                           _numLayers);
        } break;
        default:
            return;
    };

    _allocatedStorage = true;
}

void glTexture::loadData(GLuint target, const GLubyte* const ptr,
                         const vec2<GLushort>& dimensions,
                         const vec2<GLushort>& mipLevels, GFXImageFormat format,
                         GFXImageFormat internalFormat, bool usePOW2) {
    bool isTextureLayer = (_type == GL_TEXTURE_2D_ARRAY && target > 0);

    if (!isTextureLayer) {
        _format = GLUtil::GL_ENUM_TABLE::glImageFormatTable[to_uint(format)];
        _internalFormat =
            internalFormat == GFXImageFormat::DEPTH_COMPONENT
                ? GL_DEPTH_COMPONENT32
                : GLUtil::GL_ENUM_TABLE::glImageFormatTable[to_uint(internalFormat)];
        setMipMapRange(mipLevels.x, mipLevels.y);

        if (Config::Profile::USE_2x2_TEXTURES) {
            _width = _height = 2;
        } else {
            _width = dimensions.width;
            _height = dimensions.height;
        }
    } else {
        DIVIDE_ASSERT(
            _width == dimensions.width && _height == dimensions.height,
            "glTexture error: Invalid dimensions for texture array layer");
    }

    if (ptr) {
        assert(_bitDepth != 0);
        if (_type != GL_TEXTURE_CUBE_MAP && usePOW2 && !isTextureLayer) {
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
        if (_type == GL_TEXTURE_2D_ARRAY) {
            gl45ext::glTextureSubImage3DEXT(_handle, GL_TEXTURE_2D_ARRAY, 0, 0,
                                            0, target, _width, _height, 1,
                                            _format, GL_UNSIGNED_BYTE, ptr);
        } else {
            gl45ext::glTextureSubImage2DEXT(
                _handle, _type == GL_TEXTURE_CUBE_MAP
                             ? GL_TEXTURE_CUBE_MAP_POSITIVE_X + target
                             : _type,
                0, 0, 0, _width, _height, _format, GL_UNSIGNED_BYTE, ptr);
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

void glTexture::Bind(GLushort unit) {
    updateSampler();
    GL_API::bindTexture(unit, _handle, _type, _samplerHash);
    updateMipMaps();
}
};