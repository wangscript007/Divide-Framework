#include "config.h"

#include "Headers/glFramebuffer.h"

#include "Platform/Video/OpenGL/Headers/GLWrapper.h"
#include "Platform/Video/OpenGL/Headers/glResources.h"
#include "Platform/Video/OpenGL/Textures/Headers/glSamplerObject.h"

#include "Core/Headers/ParamHandler.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Textures/Headers/Texture.h"

namespace Divide {
#if defined(ENABLE_GPU_VALIDATION)
    bool glFramebuffer::_bufferBound = false;
#endif

bool glFramebuffer::_viewportChanged = false;

namespace {
typedef TextureDescriptor::AttachmentType AttachmentType;

const char* getAttachmentName(AttachmentType type) {
    switch (type) {
        case AttachmentType::Depth:
            return "Depth";
        case AttachmentType::Colour0:
            return "Colour0";
        case AttachmentType::Colour1:
            return "Colour1";
        case AttachmentType::Colour2:
            return "Colour2";
        case AttachmentType::Colour3:
            return "Colour3";
    }
    return "ERROR_TYPE";
}
};

IMPLEMENT_ALLOCATOR(glFramebuffer, 0, 0)
glFramebuffer::glFramebuffer(GFXDevice& context, bool useResolveBuffer)
    : RenderTarget(context, useResolveBuffer),
      _resolveBuffer(useResolveBuffer ? MemoryManager_NEW glFramebuffer(context)
                                      : nullptr),
      _resolved(false),
      _isLayeredDepth(false),
      _isCreated(false)
{
    _attOffset.fill(0);
    _attDirty.fill(false);
    _attachmentState.fill(false);
    _previousMask.fill(true);
    _attachments.fill(std::make_pair(GL_NONE, 0u));
    _mipMapLevel.fill(vec2<U16>(0, 1));
}

glFramebuffer::~glFramebuffer() {
    destroy();
    MemoryManager::DELETE(_resolveBuffer);
}

void glFramebuffer::initAttachment(AttachmentType type,
                                   TextureDescriptor& texDescriptor,
                                   bool resize) {
    I32 slot = to_int(type);
    // If it changed
    bool shouldResize = resize && _attachmentTexture[slot] != nullptr;
    if (!_attachmentChanged[to_uint(type)] && !shouldResize) {
        return;
    }

    DIVIDE_ASSERT(_width != 0 && _height != 0,
                  "glFramebuffer error: Invalid frame buffer dimensions!");

    if (_multisampled) {
        if (texDescriptor._type == TextureType::TEXTURE_2D) {
            texDescriptor._type = TextureType::TEXTURE_2D_MS;
        }
        if (texDescriptor._type == TextureType::TEXTURE_2D_ARRAY) {
            texDescriptor._type = TextureType::TEXTURE_2D_ARRAY_MS;
        }
    } else {
        if (texDescriptor._type == TextureType::TEXTURE_2D_MS) {
            texDescriptor._type = TextureType::TEXTURE_2D;
        }
        if (texDescriptor._type == TextureType::TEXTURE_2D_ARRAY_MS) {
            texDescriptor._type = TextureType::TEXTURE_2D_ARRAY;
        }
    }

    if (texDescriptor.getSampler().srgb()) {
        if (texDescriptor._internalFormat == GFXImageFormat::RGBA8) {
            texDescriptor._internalFormat = GFXImageFormat::SRGBA8;
        }
        if (texDescriptor._internalFormat == GFXImageFormat::RGB8) {
            texDescriptor._internalFormat = GFXImageFormat::SRGB8;
        }
    } else {
        if (texDescriptor._internalFormat == GFXImageFormat::SRGBA8) {
            texDescriptor._internalFormat = GFXImageFormat::RGBA8;
        }
        if (texDescriptor._internalFormat == GFXImageFormat::SRGB8) {
            texDescriptor._internalFormat = GFXImageFormat::RGB8;
        }
    }

    if (_multisampled) {
        texDescriptor.getSampler().toggleMipMaps(false);
    }

    bool newTexture = false;
    Texture_ptr& tex = _attachmentTexture[slot];
    if (!tex) {
        stringImpl attachmentName = Util::StringFormat(
            "Framebuffer_Att_%s_%d", getAttachmentName(type), getGUID());

        ResourceDescriptor textureAttachment(attachmentName);
        textureAttachment.setThreadedLoading(false);
        textureAttachment.setPropertyDescriptor(texDescriptor.getSampler());
        textureAttachment.setEnumValue(to_uint(texDescriptor._type));
        tex = CreateResource<Texture>(textureAttachment);
        _attachmentTexture[slot] = tex;
        newTexture = true;
    }
    
    assert(tex != nullptr);

    _mipMapLevel[slot].set(0,
        texDescriptor.getSampler().generateMipMaps()
        ? 1 + (I16)floorf(log2f(fmaxf(to_float(_width), to_float(_height))))
        : 1);

    tex->setNumLayers(texDescriptor._layerCount);
    if (resize) {
        tex->resize(NULL, vec2<U16>(_width, _height), _mipMapLevel[slot]);
    } else {
        if (newTexture) {
            Texture::TextureLoadInfo info;
            info._type = texDescriptor._type;
            tex->loadData(info, texDescriptor, NULL, vec2<U16>(_width, _height), _mipMapLevel[slot]);
        }
    }

    // Attach to frame buffer
    U32 offset = 0;
    GLenum attachment;
    if (type == AttachmentType::Depth) {
        attachment = GL_DEPTH_ATTACHMENT;
        _isLayeredDepth = (texDescriptor._type == TextureType::TEXTURE_2D_ARRAY ||
            texDescriptor._type == TextureType::TEXTURE_2D_ARRAY_MS ||
            texDescriptor._type == TextureType::TEXTURE_CUBE_MAP ||
            texDescriptor._type == TextureType::TEXTURE_CUBE_ARRAY ||
            texDescriptor._type == TextureType::TEXTURE_3D);
    } else {
        if (slot > 0) {
            offset += _attOffset[slot - 1];
        }
        attachment = GLenum((U32)GL_COLOR_ATTACHMENT0 + offset);
    }
    _attOffset[slot] = offset + 1;

    _attachments[slot] = std::make_pair(attachment, tex->getHandle());
    _attachmentChanged[slot] = false;
    _attDirty[slot] = true;
    if (!resize) {
        if (type != AttachmentType::Depth) {
            _colourBuffers.push_back(attachment);
            _colourBufferEnabled.push_back(true);
            assert(_colourBuffers.size() <= to_const_uint(AttachmentType::COUNT) - 1);
        }
    }
}

void glFramebuffer::toggleAttachment(TextureDescriptor::AttachmentType type, bool state) {
    U32 slot = to_uint(type);
    std::pair<GLenum, U32> att = _attachments[slot];
    if (att.second != 0 && state != _attachmentState[slot]) {
        glNamedFramebufferTexture(_framebufferHandle, att.first, state ? att.second : 0, 0);
        _attachmentState[slot] = state;
    }
}

void glFramebuffer::addDepthBuffer() {
    TextureDescriptor desc = _attachment[to_const_uint(AttachmentType::Colour0)];
    TextureType texType = desc._type;

    GFXDataFormat dataType = desc.dataType();
    bool fpDepth = (dataType == GFXDataFormat::FLOAT_16 ||
                    dataType == GFXDataFormat::FLOAT_32);
    SamplerDescriptor screenSampler;
    screenSampler.setFilters(TextureFilter::NEAREST);
    screenSampler.setWrapMode(desc.getSampler().wrapU(),
                              desc.getSampler().wrapV(),
                              desc.getSampler().wrapW());
    
    TextureDescriptor depthDescriptor(
        texType,
        fpDepth ? GFXImageFormat::DEPTH_COMPONENT32F
                : GFXImageFormat::DEPTH_COMPONENT,
        fpDepth ? GFXDataFormat::FLOAT_32 
                : GFXDataFormat::UNSIGNED_INT);

    //screenSampler._useRefCompare = true;  //< Use compare function
    screenSampler._cmpFunc = ComparisonFunction::LEQUAL;  //< Use less or equal
    screenSampler.toggleMipMaps(desc.getSampler().generateMipMaps());

    depthDescriptor.setSampler(screenSampler);
    depthDescriptor.setLayerCount(desc._layerCount);
    _attachmentChanged[to_const_uint(AttachmentType::Depth)] = true;
    _attachment[to_const_uint(AttachmentType::Depth)] = depthDescriptor;
    initAttachment(AttachmentType::Depth, depthDescriptor, false);
}

bool glFramebuffer::create(U16 width, U16 height) {
    if (_resolveBuffer) {
        _resolveBuffer->_useDepthBuffer = _useDepthBuffer;
        for (U8 i = 0; i < to_const_uint(AttachmentType::COUNT); ++i) {
            _resolveBuffer->_clearColours[i].set(_clearColours[i]);
            if (_attachmentChanged[i]) {
                _resolveBuffer->addAttachment(_attachment[i], static_cast<AttachmentType>(i));
            }
        }
        _resolveBuffer->create(width, height);
    }

    bool shouldResize = false;
    if (Config::Profile::USE_2x2_TEXTURES) {
        _width = _height = 2;
    } else {
        shouldResize = (_width != width || _height != height);
        _width = width;
        _height = height;
    }

    _isCreated = _isCreated && !_shouldRebuild;

    if (!_isCreated) {
        assert(_framebufferHandle == 0);
        glCreateFramebuffers(1, &_framebufferHandle);
        shouldResize = false;
        _resolved = false;
        _isLayeredDepth = false;
        _colourBuffers.resize(0);
        _colourBufferEnabled.resize(0);
    }


    // For every attachment, be it a colour or depth attachment ...
    for (U8 i = 0; i < to_const_uint(AttachmentType::COUNT); ++i) {
        initAttachment(static_cast<AttachmentType>(i), _attachment[i], shouldResize);
    }
    
    // If we either specify a depth texture or request a depth buffer ...
    if (_useDepthBuffer && !hasDepth()) {
        addDepthBuffer();
    }
    
    if (!shouldResize) {
        setInitialAttachments();
    }

    // If colour writes are disabled, draw only depth info
    if (!_colourBuffers.empty()) {
        glNamedFramebufferDrawBuffers(_framebufferHandle, 
                                        static_cast<GLsizei>(_colourBuffers.size()),
                                        _colourBuffers.data());
    }

    clear(RenderTarget::defaultPolicy());

    _isCreated = checkStatus();
    _shouldRebuild = !_isCreated;
    return _isCreated;
}

void glFramebuffer::destroy() {
    _attachmentTexture.fill(nullptr);

    if (_framebufferHandle > 0) {
        glDeleteFramebuffers(1, &_framebufferHandle);
        _framebufferHandle = 0;
    }

    _width = _height = 0;

    if (_resolveBuffer) {
        _resolveBuffer->destroy();
    }
}

void glFramebuffer::resolve() {
    if (_resolveBuffer && !_resolved) {
        _resolveBuffer->blitFrom(this, AttachmentType::Colour0, hasColour(), hasDepth());
        _resolved = true;
    }
}

void glFramebuffer::blitFrom(RenderTarget* inputFB,
                             AttachmentType slot,
                             bool blitColour,
                             bool blitDepth) {
    if (!inputFB || (!blitColour && !blitDepth)) {
        return;
    }
    
    glFramebuffer* input = static_cast<glFramebuffer*>(inputFB);

    // prevent stack overflow
    if (_resolveBuffer && (inputFB->getGUID() != _resolveBuffer->getGUID())) {
        input->resolve();
    }

    GLuint previousFB = GL_API::getActiveFB(RenderTarget::RenderTargetUsage::RT_READ_WRITE);
    GL_API::setActiveFB(RenderTarget::RenderTargetUsage::RT_READ_ONLY, input->_framebufferHandle);
    GL_API::setActiveFB(RenderTarget::RenderTargetUsage::RT_WRITE_ONLY, this->_framebufferHandle);

    if (blitColour && hasColour()) {
        size_t colourCount = _colourBuffers.size();
        for (U8 i = 0; i < colourCount; ++i) {
            if (_colourBufferEnabled[i]) {
                glDrawBuffer(this->_colourBuffers[i]);
                glReadBuffer(input->_colourBuffers[i]);
                glBlitFramebuffer(0, 0, input->_width, input->_height, 0, 0,
                                  this->_width, this->_height, GL_COLOR_BUFFER_BIT,
                                  GL_NEAREST);
            }
        }
        _context.registerDrawCalls(to_uint(colourCount));
    }

    if (blitDepth && hasDepth()) {
        glBlitFramebuffer(0, 0, input->_width, input->_height, 0, 0,
                          this->_width, this->_height, GL_DEPTH_BUFFER_BIT,
                          GL_NEAREST);
        _context.registerDrawCall();
    }

    GL_API::setActiveFB(RenderTarget::RenderTargetUsage::RT_READ_WRITE, previousFB);
}

const Texture_ptr& glFramebuffer::getAttachment(AttachmentType slot, bool flushStateOnRequest) {
    if (_resolveBuffer) {
        resolve();
        return _resolveBuffer->getAttachment(slot, flushStateOnRequest);
    }

    return RenderTarget::getAttachment(slot, flushStateOnRequest);
}

void glFramebuffer::bind(U8 unit, AttachmentType slot, bool flushStateOnRequest) {
    const Texture_ptr& attachment = getAttachment(slot, flushStateOnRequest);
    if (attachment != nullptr) {
        attachment->Bind(unit, flushStateOnRequest);
    }
}

void glFramebuffer::resetMipMaps(const RenderTargetDrawDescriptor& drawPolicy) {
    for (U8 i = 0; i < to_const_uint(AttachmentType::COUNT); ++i) {
        if (drawPolicy._drawMask[i]) {
            if (_attachmentTexture[i] != nullptr) {
                _attachmentTexture[i]->refreshMipMaps();
            }
        }
    }
}

void glFramebuffer::begin(const RenderTargetDrawDescriptor& drawPolicy) {
    static vectorImpl<GLenum> colourBuffers;

    DIVIDE_ASSERT(_framebufferHandle != 0,
                  "glFramebuffer error: "
                  "Tried to bind an invalid framebuffer!");

#   if defined(ENABLE_GPU_VALIDATION)
        DIVIDE_ASSERT(!glFramebuffer::_bufferBound,
                      "glFramebuffer error: "
                      "Begin() called without a call to the "
                      "previous bound buffer's End()");
#   endif

    if (drawPolicy._changeViewport) {
        _viewportChanged = true;
        _context.setViewport(vec4<GLint>(0, 0, _width, _height));
    }

    GL_API::setActiveFB(RenderTarget::RenderTargetUsage::RT_READ_WRITE, _framebufferHandle);

    if (_resolveBuffer) {
        _resolved = false;
    }

    if (_previousMask != drawPolicy._drawMask) {
        // handle colour buffers first
        colourBuffers.resize(0);
        GLsizei bufferCount = static_cast<GLsizei>(_colourBuffers.size());
        for (GLsizei i = 0; i < bufferCount; ++i) {
            _colourBufferEnabled[i] = drawPolicy._drawMask[i];
            colourBuffers.push_back(_colourBufferEnabled[i] ? _colourBuffers[i] : GL_NONE);
        }

        glDrawBuffers(bufferCount, colourBuffers.data());
        
        checkStatus();
        _previousMask = drawPolicy._drawMask;
    }

    clear(drawPolicy);

    resetMipMaps(drawPolicy);

#   if defined(ENABLE_GPU_VALIDATION)
        glFramebuffer::_bufferBound = true;
#   endif
}

void glFramebuffer::end() {
#   if defined(ENABLE_GPU_VALIDATION)
        DIVIDE_ASSERT(glFramebuffer::_bufferBound,
                      "glFramebuffer error: "
                      "End() called without a previous call to Begin()");
#   endif

    GL_API::setActiveFB(RenderTarget::RenderTargetUsage::RT_READ_WRITE, 0);
    if (_viewportChanged) {
        _context.restoreViewport();
        _viewportChanged = false;
    }

    setInitialAttachments();
    resolve();

#   if defined(ENABLE_GPU_VALIDATION)
        glFramebuffer::_bufferBound = false;
#   endif
}

void glFramebuffer::setInitialAttachments() {
    // Reset attachments if they changed (e.g. after layered rendering);
    for (U32 i = 0; i < _attDirty.size(); ++i) {
        if (_attDirty[i]) {
            toggleAttachment(static_cast<AttachmentType>(i), true);
        }
    }
    _attDirty.fill(false);
}

void glFramebuffer::clear(const RenderTargetDrawDescriptor& drawPolicy) const {
    if (hasColour() && drawPolicy._clearColourBuffersOnBind) {
        GLuint index = 0;
        for (; index < _colourBuffers.size(); ++index) {
            if (_colourBufferEnabled[index]) {
                TextureDescriptor desc = _attachment[index];
                GFXDataFormat dataType = desc.dataType();
                if(dataType == GFXDataFormat::FLOAT_16 ||dataType == GFXDataFormat::FLOAT_32) {
                    glClearNamedFramebufferfv(_framebufferHandle, GL_COLOR, index, _clearColours[index]._v);
                } else {
                    glClearNamedFramebufferuiv(_framebufferHandle, GL_COLOR, index, Util::ToUIntColour(_clearColours[index])._v);
                }
                _context.registerDrawCall();
            }
        }
    }

    if (hasDepth() && drawPolicy._clearDepthBufferOnBind) {
        glClearNamedFramebufferfv(_framebufferHandle, GL_DEPTH, 0, &_depthValue);
        _context.registerDrawCall();
    }
}

void glFramebuffer::drawToLayer(TextureDescriptor::AttachmentType slot,
                                U32 layer,
                                bool includeDepth) {

    GLenum textureType = GLUtil::glTextureTypeTable[to_uint(
        _attachmentTexture[to_uint(slot)]->getTextureType())];
    // only for array textures (it's better to simply ignore the command if the
    // format isn't supported (debugging reasons)
    if (textureType != GL_TEXTURE_2D_ARRAY &&
        textureType != GL_TEXTURE_CUBE_MAP_ARRAY &&
        textureType != GL_TEXTURE_2D_MULTISAMPLE_ARRAY) {
        return;
    }

    bool useDepthLayer =
        (hasDepth() && includeDepth) ||
        (hasDepth() && slot == TextureDescriptor::AttachmentType::Depth);
    bool useColourLayer =
        (hasColour() && slot < TextureDescriptor::AttachmentType::Depth);

    if (useDepthLayer && _isLayeredDepth) {
        glFramebufferTextureLayer(GL_FRAMEBUFFER,
                                  GL_DEPTH_ATTACHMENT,
                                  _attachmentTexture[to_const_uint(TextureDescriptor::AttachmentType::Depth)]->getHandle(),
                                  0,
                                  layer);
        _attDirty[to_const_uint(TextureDescriptor::AttachmentType::Depth)] = true;
    }

    if (useColourLayer) {
        U32 offset = 0;
        if (to_uint(slot) > 0) {
            offset = _attOffset[to_uint(slot) - 1];
        }
        glFramebufferTextureLayer(GL_FRAMEBUFFER,
                                  _colourBuffers[offset],
                                  _attachmentTexture[to_uint(slot)]->getHandle(),
                                  0,
                                  layer);
        _attDirty[to_uint(slot)] = true;
    }

    _attachmentTexture[to_uint(slot)]->refreshMipMaps();

    checkStatus();
}

void glFramebuffer::setMipLevel(U16 mipMinLevel, U16 mipMaxLevel, U16 writeLevel, TextureDescriptor::AttachmentType slot) {
    for (U8 i = 0; i < to_uint(AttachmentType::COUNT); ++i) {
        AttachmentType tempSlot = static_cast<AttachmentType>(i);
        if (tempSlot != slot) {
            toggleAttachment(tempSlot, false);
            _attDirty[i] = true;
        }
    }

    Texture* attachment = _attachmentTexture[to_uint(slot)].get();
    attachment->setMipMapRange(mipMinLevel, mipMaxLevel);

    glFramebufferTexture(GL_FRAMEBUFFER,
                            slot == TextureDescriptor::AttachmentType::Depth
                            ? GL_DEPTH_ATTACHMENT
                            : _colourBuffers[to_uint(slot)],
                         attachment->getHandle(),
                         writeLevel);
    checkStatus();
}

void glFramebuffer::resetMipLevel(TextureDescriptor::AttachmentType slot) {
    setMipLevel(_mipMapLevel[to_uint(slot)].x, _mipMapLevel[to_uint(slot)].y, 0, slot);
}

void glFramebuffer::readData(const vec4<U16>& rect,
                             GFXImageFormat imageFormat,
                             GFXDataFormat dataType,
                             void* outData) {
    if (_resolveBuffer) {
        resolve();
        _resolveBuffer->readData(rect, imageFormat, dataType, outData);
    } else {
        GL_API::setPixelPackUnpackAlignment();
        GL_API::setActiveFB(RenderTarget::RenderTargetUsage::RT_READ_ONLY, _framebufferHandle);
        glReadPixels(
            rect.x, rect.y, rect.z, rect.w,
            GLUtil::glImageFormatTable[to_uint(imageFormat)],
            GLUtil::glDataFormat[to_uint(dataType)], outData);
    }
}

bool glFramebuffer::checkStatus() const {
#if defined(ENABLE_GPU_VALIDATION)
    // check FB status
    switch (glCheckNamedFramebufferStatus(_framebufferHandle, GL_FRAMEBUFFER))
    {
        case GL_FRAMEBUFFER_COMPLETE: {
            return true;
        }
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: {
            Console::errorfn(Locale::get(_ID("ERROR_RT_ATTACHMENT_INCOMPLETE")));
            return false;
        }
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: {
            Console::errorfn(Locale::get(_ID("ERROR_RT_NO_IMAGE")));
            return false;
        }
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER: {
            Console::errorfn(Locale::get(_ID("ERROR_RT_INCOMPLETE_DRAW_BUFFER")));
            return false;
        }
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER: {
            Console::errorfn(Locale::get(_ID("ERROR_RT_INCOMPLETE_READ_BUFFER")));
            return false;
        }
        case GL_FRAMEBUFFER_UNSUPPORTED: {
            Console::errorfn(Locale::get(_ID("ERROR_RT_UNSUPPORTED")));
            return false;
        }
        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE: {
            Console::errorfn(Locale::get(_ID("ERROR_RT_INCOMPLETE_MULTISAMPLE")));
            return false;
        }
        case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS: {
            Console::errorfn(Locale::get(_ID("ERROR_RT_INCOMPLETE_LAYER_TARGETS")));
            return false;
        }
        case gl::GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT: {
            Console::errorfn(Locale::get(_ID("ERROR_RT_DIMENSIONS")));
            return false;
        }
        case gl::GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT: {
             Console::errorfn(Locale::get(_ID("ERROR_RT_FORMAT")));
             return false;
        }
        default: {
            Console::errorfn(Locale::get(_ID("ERROR_UNKNOWN")));
            return false;
        }
    };
#else
    return true;
#endif
}

};  // namespace Divide