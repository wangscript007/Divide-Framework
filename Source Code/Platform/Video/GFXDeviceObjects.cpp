#include "Headers/GFXDevice.h"

#include "Core/Headers/ParamHandler.h"
#include "Utility/Headers/Localization.h"

#include "Platform/Video/OpenGL/Buffers/RenderTarget/Headers/glFramebuffer.h"
#include "Platform/Video/Direct3D/Buffers/RenderTarget/Headers/d3dRenderTarget.h"

#include "Platform/Video/OpenGL/Headers/glIMPrimitive.h"

#include "Platform/Video/OpenGL/Buffers/VertexBuffer/Headers/glVertexArray.h"
#include "Platform/Video/Direct3D/Buffers/VertexBuffer/Headers/d3dVertexBuffer.h"

#include "Platform/Video/OpenGL/Buffers/PixelBuffer/Headers/glPixelBuffer.h"
#include "Platform/Video/Direct3D/Buffers/PixelBuffer/Headers/d3dPixelBuffer.h"

#include "Platform/Video/OpenGL/Buffers/VertexBuffer/Headers/glGenericVertexData.h"
#include "Platform/Video/Direct3D/Buffers/VertexBuffer/Headers/d3dGenericVertexData.h"

#include "Platform/Video/OpenGL/Textures/Headers/glTexture.h"
#include "Platform/Video/Direct3D/Textures/Headers/d3dTexture.h"

#include "Platform/Video/OpenGL/Shaders/Headers/glShaderProgram.h"
#include "Platform/Video/Direct3D/Shaders/Headers/d3dShaderProgram.h"

#include "Platform/Video/OpenGL/Buffers/ShaderBuffer/Headers/glUniformBuffer.h"
#include "Platform/Video/Direct3D/Buffers/ShaderBuffer/Headers/d3dConstantBuffer.h"

namespace Divide {

RenderTarget* GFXDevice::newRT(bool multisampled) const {
    multisampled = multisampled &&
                   ParamHandler::instance().getParam<I32>(_ID("rendering.MSAAsampless"), 0) > 0;

    switch (_API_ID) {
        case RenderAPI::OpenGL:
        case RenderAPI::OpenGLES: {
            /// Create and return a new framebuffer.
            /// The callee is responsible for it's deletion!
            // If MSAA is disabled, this will be a simple colour / depth buffer
            // If we requested a MultiSampledFramebuffer and MSAA is enabled, we also
            // allocate a resolve framebuffer
            // We set the resolve framebuffer as the requested framebuffer's child.
            // The framebuffer is responsible for deleting it's own resolve child!
            return MemoryManager_NEW glFramebuffer(instance(), multisampled);
        } break;
        case RenderAPI::Direct3D: {
            return MemoryManager_NEW d3dRenderTarget(instance(), multisampled);
        } break;
        default: {
            DIVIDE_UNEXPECTED_CALL(Locale::get(_ID("ERROR_GFX_DEVICE_API")));
        } break;
    };

    return nullptr;
}

IMPrimitive* GFXDevice::newIMP() const {
    switch (_API_ID) {
        case RenderAPI::OpenGL:
        case RenderAPI::OpenGLES: {
            /// Create and return a new IM emulation primitive. The callee is responsible
            /// for it's deletion!
            return MemoryManager_NEW glIMPrimitive(instance());
        } break;
        default: {
            DIVIDE_UNEXPECTED_CALL(Locale::get(_ID("ERROR_GFX_DEVICE_API")));
        } break;
    };

    return nullptr;
}

VertexBuffer* GFXDevice::newVB() const {
    switch (_API_ID) {
        case RenderAPI::OpenGL:
        case RenderAPI::OpenGLES: {
            /// Create and return a new vertex array (VAO + VB + IB).
            /// The callee is responsible for it's deletion!
            return MemoryManager_NEW glVertexArray(instance());
        } break;
        case RenderAPI::Direct3D: {
            return MemoryManager_NEW d3dVertexBuffer(instance());
        } break;
        default: {
            DIVIDE_UNEXPECTED_CALL(Locale::get(_ID("ERROR_GFX_DEVICE_API")));
        } break;
    };

    return nullptr;
}

PixelBuffer* GFXDevice::newPB(PBType type) const {
    switch (_API_ID) {
        case RenderAPI::OpenGL:
        case RenderAPI::OpenGLES: {
            /// Create and return a new pixel buffer using the requested format.
            /// The callee is responsible for it's deletion!
            return MemoryManager_NEW glPixelBuffer(instance(), type);
        } break;
        case RenderAPI::Direct3D: {
            return MemoryManager_NEW d3dPixelBuffer(instance(), type);
        } break;
        default: {
            DIVIDE_UNEXPECTED_CALL(Locale::get(_ID("ERROR_GFX_DEVICE_API")));
        } break;
    };

    return nullptr;
}

GenericVertexData* GFXDevice::newGVD(const U32 ringBufferLength) const {
    switch (_API_ID) {
        case RenderAPI::OpenGL:
        case RenderAPI::OpenGLES: {
            /// Create and return a new generic vertex data object
            /// The callee is responsible for it's deletion!
            return MemoryManager_NEW glGenericVertexData(instance(), ringBufferLength);
        } break;
        case RenderAPI::Direct3D: {
            return MemoryManager_NEW d3dGenericVertexData(instance(), ringBufferLength);
        } break;
        default: {
            DIVIDE_UNEXPECTED_CALL(Locale::get(_ID("ERROR_GFX_DEVICE_API")));
        } break;
    };

    return nullptr;
}

Texture* GFXDevice::newTexture(const stringImpl& name,
                               const stringImpl& resourceLocation,
                               TextureType type,
                               bool asyncLoad) const {
    switch (_API_ID) {
        case RenderAPI::OpenGL:
        case RenderAPI::OpenGLES: {
            /// Create and return a new texture. The callee is responsible for it's deletion!
            return MemoryManager_NEW glTexture(instance(), name, resourceLocation, type, asyncLoad);
        } break;
        case RenderAPI::Direct3D: {
            return MemoryManager_NEW d3dTexture(instance(), name, resourceLocation, type, asyncLoad);
        } break;
        default: {
            DIVIDE_UNEXPECTED_CALL(Locale::get(_ID("ERROR_GFX_DEVICE_API")));
        } break;
    };

    return nullptr;
}

ShaderProgram* GFXDevice::newShaderProgram(const stringImpl& name,
                                           const stringImpl& resourceLocation,
                                           bool asyncLoad) const {
    switch (_API_ID) {
        case RenderAPI::OpenGL:
        case RenderAPI::OpenGLES: {
            /// Create and return a new shader program.
            /// The callee is responsible for it's deletion!
            return MemoryManager_NEW glShaderProgram(instance(), name, resourceLocation, asyncLoad);
        } break;
        case RenderAPI::Direct3D: {
            return MemoryManager_NEW d3dShaderProgram(instance(), name, resourceLocation, asyncLoad);
        } break;
        default: {
            DIVIDE_UNEXPECTED_CALL(Locale::get(_ID("ERROR_GFX_DEVICE_API")));
        } break;
    };

    return nullptr;
}

ShaderBuffer* GFXDevice::newSB(const U32 ringBufferLength,
                               const bool unbound,
                               const bool persistentMapped ,
                               BufferUpdateFrequency frequency) const {
    switch (_API_ID) {
        case RenderAPI::OpenGL:
        case RenderAPI::OpenGLES: {
            /// Create and return a new shader buffer. The callee is responsible for it's deletion!
            /// The OpenGL implementation creates either an 'Uniform Buffer Object' if unbound is false
            /// or a 'Shader Storage Block Object' otherwise
            // The shader buffer can also be persistently mapped, if requested
            return MemoryManager_NEW glUniformBuffer(instance(),
                                                     ringBufferLength,
                                                     unbound,
                                                     persistentMapped,
                                                     frequency);
        } break;
        case RenderAPI::Direct3D: {
            return MemoryManager_NEW d3dConstantBuffer(instance(),
                                                       ringBufferLength,
                                                       unbound,
                                                       persistentMapped,
                                                       frequency);
        } break;
        default: {
            DIVIDE_UNEXPECTED_CALL(Locale::get(_ID("ERROR_GFX_DEVICE_API")));
        } break;
    };

    return nullptr;
}

}; // namespace Divide