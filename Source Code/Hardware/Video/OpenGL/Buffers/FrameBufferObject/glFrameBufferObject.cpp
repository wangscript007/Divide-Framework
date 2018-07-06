#include "Headers/glFrameBufferObject.h"
#include "Hardware/Video/OpenGL/Headers/glResources.h"
#include "Hardware/Video/OpenGL/Textures/Headers/glSamplerObject.h"

#include "core.h"
#include "Core/Headers/ParamHandler.h"
#include "Hardware/Video/Headers/GFXDevice.h"

glFrameBufferObject::glFrameBufferObject(FBOType type) : FrameBufferObject(type)
{
    memset(_textureId, 0, 5 * sizeof(GLuint));
    memset(_mipMapEnabled, false, 5 * sizeof(bool));

    _imageLayers = 0;
    _hasDepth = _hasColor = false;

    switch(type){
        default:
        case FBO_2D_DEPTH:
        case FBO_2D_COLOR_MS:
        case FBO_2D_DEFERRED: _textureType = GL_TEXTURE_2D; break;
        case FBO_CUBE_DEPTH:
        case FBO_CUBE_COLOR: _textureType = GL_TEXTURE_CUBE_MAP; break;
        case FBO_2D_ARRAY_COLOR:
        case FBO_2D_ARRAY_DEPTH : _textureType = GL_TEXTURE_2D_ARRAY_EXT; break;
        case FBO_CUBE_COLOR_ARRAY:
        case FBO_CUBE_DEPTH_ARRAY: _textureType = GL_TEXTURE_CUBE_MAP_ARRAY; break;
    };

    if(type == FBO_CUBE_DEPTH || type == FBO_2D_ARRAY_DEPTH || type == FBO_CUBE_DEPTH_ARRAY || type == FBO_2D_DEPTH){
        toggleColorWrites(false);
    }

    if((_textureType == GL_TEXTURE_2D_ARRAY_EXT || _textureType == GL_TEXTURE_CUBE_MAP_ARRAY) &&
       !glewIsSupported("GL_EXT_texture_array") && !glewIsSupported("GL_MESA_texture_array")){
            ERROR_FN(Locale::get("ERROR_GL_NO_TEXTURE_ARRAY"));
    }
}

glFrameBufferObject::~glFrameBufferObject()
{
}

void glFrameBufferObject::DrawToLayer(TextureDescriptor::AttachmentType slot, U8 layer) const
{
    // only for array textures
    assert(_textureType == GL_TEXTURE_2D_ARRAY_EXT || _textureType == GL_TEXTURE_CUBE_MAP_ARRAY);

    if(_hasDepth)
        GLCheck(glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, _textureId[TextureDescriptor::Depth], 0, layer));
    if(_hasColor)
        GLCheck(glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + slot, _textureId[slot], 0, layer));

    if(_clearBuffersState)   GLCheck(glClear(_clearBufferMask));
}

void glFrameBufferObject::InitAttachement(TextureDescriptor::AttachmentType type, const TextureDescriptor& texDescriptor){
    if(type != TextureDescriptor::Depth) _hasColor = true;
    else                                 _hasDepth = true;
        
    //If it changed
    if(_attachementDirty[type]){
        U8 slot = type;
        //And get the image formats and data type
        if(_textureType != glTextureTypeTable[texDescriptor._type]){
            ERROR_FN(Locale::get("ERROR_FBO_ATTACHEMENT_DIFFERENT"), (I32)slot);
        }

        GLenum format = glImageFormatTable[texDescriptor._format];
        GLenum internalFormat = glImageFormatTable[texDescriptor._internalFormat];
        GLenum dataType = glDataFormat[texDescriptor._dataType];

        const SamplerDescriptor& sampler = texDescriptor.getSampler();
        _mipMapEnabled[slot] = sampler.generateMipMaps();
            
        GLCheck(glGenTextures( 1, &_textureId[slot] ));
        GLCheck(glBindTexture(_textureType, _textureId[slot]));

        //generate a new texture attachement
        //anisotrophic filtering is only added to color attachements
        if (sampler.anisotrophyLevel() > 1 && _mipMapEnabled[slot] && type != TextureDescriptor::Depth) {
            if(!glewIsSupported("GL_EXT_texture_filter_anisotropic")){
                ERROR_FN(Locale::get("ERROR_NO_ANISO_SUPPORT"));
            }else{
                U8 anisoLevel = std::min<I32>((I32)sampler.anisotrophyLevel(), ParamHandler::getInstance().getParam<U8>("rendering.anisotropicFilteringLevel"));
                GLCheck(glTexParameteri(_textureType, GL_TEXTURE_MAX_ANISOTROPY_EXT,anisoLevel));
            }
        }

        //depth attachements may need comparison functions
        if(sampler._useRefCompare){
            GLCheck(glTexParameteri(_textureType, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE));
            GLCheck(glTexParameteri(_textureType, GL_TEXTURE_COMPARE_FUNC,  glCompareFuncTable[sampler._cmpFunc]));
            GLCheck(glTexParameteri(_textureType, GL_DEPTH_TEXTURE_MODE, glImageFormatTable[sampler._depthCompareMode]));
        }
  
        //General texture parameters for either color or depth
        GLCheck(glTexParameterf(_textureType, GL_TEXTURE_MAG_FILTER, glTextureFilterTable[sampler.magFilter()]));
        GLCheck(glTexParameterf(_textureType, GL_TEXTURE_MIN_FILTER, glTextureFilterTable[sampler.minFilter()]));
        GLCheck(glTexParameterf(_textureType, GL_TEXTURE_WRAP_S,     glWrapTable[sampler.wrapU()]));

        //Anything other then a 1D texture, needs a T-wrap mode
        if(_textureType != GL_TEXTURE_1D){
            GLCheck(glTexParameterf(_textureType, GL_TEXTURE_WRAP_T, glWrapTable[sampler.wrapV()]));
        }

        //3D texture types need a R-wrap mode
        if(_textureType == GL_TEXTURE_3D ||  _textureType == GL_TEXTURE_CUBE_MAP || _textureType == GL_TEXTURE_CUBE_MAP_ARRAY) {
            GLCheck(glTexParameterf(_textureType, GL_TEXTURE_WRAP_R, glWrapTable[sampler.wrapW()]));
        }

        //generate empty texture data using each texture type's specific function
        switch(_textureType){
            case GL_TEXTURE_1D:{
                GLCheck(glTexImage1D(GL_TEXTURE_1D, 0, internalFormat,
                                        _width,        0, format,
                                        dataType, NULL));
            }break;
            case GL_TEXTURE_2D_MULTISAMPLE:
            case GL_TEXTURE_2D:{
                GLCheck(glTexImage2D(_textureType,    0, internalFormat,
                                        _width, _height, 0, format,
                                        dataType, NULL));
            }break;
            case GL_TEXTURE_CUBE_MAP:{
                for(GLubyte j=0; j<6; j++){
                        GLCheck(glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X+j,
                                                0, internalFormat, _width, _height,
                                                0, format, dataType, NULL));
                    }
            }break;
            case GL_TEXTURE_2D_ARRAY_EXT:
            case GL_TEXTURE_3D:{
                GLCheck(glTexImage3D(_textureType,   0, internalFormat,
                                        _width, _height, _imageLayers,//Use as depth for GL_TEXTURE_3D
                                        0, format, dataType, NULL));
            }break;
            case GL_TEXTURE_CUBE_MAP_ARRAY: {
                assert(false); //not implemented yet
                for(GLubyte j=0; j<6; j++){
                    GLCheck(glTexImage3D(GL_TEXTURE_CUBE_MAP_POSITIVE_X+j,
                                            0, internalFormat, _width, _height,
                                            _imageLayers, 0, format, dataType, NULL));
                }
            }break;
        };
            
        //Generate mip maps if needed
        if(_mipMapEnabled[slot]) {
            GLCheck(glTexParameteri(_textureType, GL_TEXTURE_BASE_LEVEL, texDescriptor._mipMinLevel));
            GLCheck(glTexParameteri(_textureType, GL_TEXTURE_MAX_LEVEL,  texDescriptor._mipMaxLevel));
            GLCheck(glGenerateMipmap(_textureType));
        }
        
        //unbind the texture
        GLCheck(glBindTexture(_textureType, 0));

        GLenum attachment = (type == TextureDescriptor::Depth) ? GL_DEPTH_ATTACHMENT : GL_COLOR_ATTACHMENT0 + slot;

        //Attach to frame buffer
        switch(_textureType){
            case GL_TEXTURE_1D:
                GLCheck(glFramebufferTexture1D(GL_FRAMEBUFFER, attachment, _textureType, _textureId[slot], 0));
                break;
            case GL_TEXTURE_2D_MULTISAMPLE:
            case GL_TEXTURE_2D:
            case GL_TEXTURE_CUBE_MAP: 
                GLCheck(glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, _textureType, _textureId[slot], 0));
                break;
            case GL_TEXTURE_2D_ARRAY_EXT:
                GLCheck(glFramebufferTextureEXT(GL_FRAMEBUFFER, attachment, _textureId[slot], 0));
                break;
            case GL_TEXTURE_3D:
            case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
            /*case GL_TEXTURE_CUBE_MAP_ARRAY:*/
                GLCheck(glFramebufferTexture3D(GL_FRAMEBUFFER, attachment, _textureType, _textureId[slot], 0, 0 ));
                break;
            case GL_TEXTURE_CUBE_MAP_ARRAY:
                assert(false); ///not implemented yet
                break;
        };
    }
}

void glFrameBufferObject::AddDepthBuffer(){
        
    TextureType texType = (_textureType == GL_TEXTURE_2D_ARRAY_EXT || 
                           _textureType == GL_TEXTURE_CUBE_MAP_ARRAY) ? 
                           TEXTURE_2D_ARRAY : TEXTURE_2D;

    SamplerDescriptor screenSampler;
    screenSampler.setFilters(TEXTURE_FILTER_NEAREST, TEXTURE_FILTER_NEAREST);
    screenSampler.setWrapMode(TEXTURE_CLAMP_TO_EDGE);
    screenSampler.toggleMipMaps(false);
    TextureDescriptor depthDescriptor(texType,
                                      DEPTH_COMPONENT,
                                      DEPTH_COMPONENT24,
                                      UNSIGNED_BYTE);
        
    screenSampler._useRefCompare = true; //< Use compare function
    screenSampler._cmpFunc = CMP_FUNC_LEQUAL; //< Use less or equal
    screenSampler._depthCompareMode = LUMINANCE;
    depthDescriptor.setSampler(screenSampler);
    _attachementDirty[TextureDescriptor::Depth] = true;
    _attachement[TextureDescriptor::Depth] = depthDescriptor;
    InitAttachement(TextureDescriptor::Depth, depthDescriptor);

    _hasDepth = true;
}

bool glFrameBufferObject::Create(GLushort width, GLushort height, GLubyte imageLayers)
{
    _width = width;
    _height = height;
    _imageLayers = imageLayers;

    _clearBufferMask = 0;
    assert(!_attachement.empty());
                    
    if(glGenerateMipmap == NULL){
        ERROR_FN(Locale::get("ERROR_NO_MIP_MAPS"));
        assert(glGenerateMipmap);
    }

    if(_frameBufferHandle <= 0){
        GLCheck(glGenFramebuffers(1, &_frameBufferHandle));
    }

    GLCheck(glBindFramebuffer(GL_FRAMEBUFFER, _frameBufferHandle));
        
    std::vector<GLenum >buffers;

    //For every attachement, be it a color or depth attachement ...
    for_each(TextureAttachements::value_type& it, _attachement){
       InitAttachement(it.first, it.second);
       if(it.first != TextureDescriptor::Depth)
           buffers.insert(buffers.begin(), GL_COLOR_ATTACHMENT0 + it.first);
    }

    if(!buffers.empty())
        GLCheck(glDrawBuffers(buffers.size(), &buffers[0]));

    //If we either specify a depth texture or request a depth buffer ...
    if(_useDepthBuffer && !_hasDepth){
       AddDepthBuffer();
    }

    //If color writes are disabled, draw only depth info
    if(_disableColorWrites){
        GLCheck(glDrawBuffer(GL_NONE));
        GLCheck(glReadBuffer(GL_NONE));
        _hasColor = false;
    }

    if(_hasColor)
        _clearBufferMask = _clearBufferMask | GL_COLOR_BUFFER_BIT;
    if(_hasDepth)
        _clearBufferMask = _clearBufferMask | GL_DEPTH_BUFFER_BIT;

    checkStatus();

    if(_clearColorState)     GL_API::clearColor( _clearColor );

    GLCheck(glClear(_clearBufferMask));

    GLCheck(glBindFramebuffer(GL_FRAMEBUFFER, 0));
    return true;
}

void glFrameBufferObject::Destroy() {
    Unbind();

    for(U8 i = 0; i < 5; i++){
        if(_textureId[i] > 0){
            GLCheck(glDeleteTextures(1, &_textureId[i]));
            _textureId[i] = 0;
        }
    }

    if(_frameBufferHandle > 0){
        GLCheck(glDeleteFramebuffers(1, &_frameBufferHandle));
        _frameBufferHandle = 0;
    }

    _width = _height = 0;
}

void glFrameBufferObject::BlitFrom(FrameBufferObject* inputFBO) const {
    GLCheck(glBindFramebuffer(GL_READ_FRAMEBUFFER, inputFBO->getHandle()));
    GLCheck(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->_frameBufferHandle));
    GLCheck(glBlitFramebuffer(0, 0, inputFBO->getWidth(), inputFBO->getHeight(), 0, 0, 
                              this->_width, this->_height, GL_COLOR_BUFFER_BIT, GL_LINEAR));
    GLCheck(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

void glFrameBufferObject::Bind(GLubyte unit, TextureDescriptor::AttachmentType slot) const {
    FrameBufferObject::Bind(unit, slot);
    GL_API::setActiveTextureUnit(unit);
    GLCheck(glBindTexture(_textureType, _textureId[slot]));
}

void glFrameBufferObject::Unbind(GLubyte unit) const {
    FrameBufferObject::Unbind(unit);
    GL_API::setActiveTextureUnit(unit);
    GLCheck(glBindTexture(_textureType, 0 ));
}

void glFrameBufferObject::Begin(GLubyte nFace) const {
    assert(nFace<6);
    GL_API::setViewport(vec4<U32>(0,0,_width,_height));
    GLCheck(glBindFramebuffer(GL_FRAMEBUFFER, _frameBufferHandle));

    if(_textureType == GL_TEXTURE_CUBE_MAP) {
        GLCheck(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X+nFace, _textureId[TextureDescriptor::Color0], 0));
    }

    if(_clearColorState)     GL_API::clearColor( _clearColor );//< this is checked so it isn't called twice on the GPU
    if(_clearBuffersState)   GLCheck(glClear(_clearBufferMask));
}

void glFrameBufferObject::End(GLubyte nFace) const {
    assert(nFace<6);
    GLCheck(glBindFramebuffer(GL_FRAMEBUFFER, 0));
    GL_API::restoreViewport();
}

void glFrameBufferObject::UpdateMipMaps(TextureDescriptor::AttachmentType slot) const {
    if(!_mipMapEnabled[slot] || !_bound)
        return;

    GLCheck(glGenerateMipmap(_textureType));
}

bool glFrameBufferObject::checkStatus() const {
    //Not defined in GLEW
    #define GL_FRAMEBUFFER_INCOMPLETE_FORMATS 0x8CDA
    #define GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS 0x8CD9

    // check FBO status
    GLuint status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    switch(status)
    {
    case GL_FRAMEBUFFER_COMPLETE:
        return true;

    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:{
        ERROR_FN(Locale::get("ERROR_FBO_ATTACHMENT_INCOMPLETE"));
        return false;
                                              }
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:{
        ERROR_FN(Locale::get("ERROR_FBO_NO_IMAGE"));
        return false;
                                              }
    case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:{
        ERROR_FN(Locale::get("ERROR_FBO_DIMENSIONS"));
        return false;
                                              }
    case GL_FRAMEBUFFER_INCOMPLETE_FORMATS:{
        ERROR_FN(Locale::get("ERROR_FBO_FORMAT"));
        return false;
                                           }
    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:{
        ERROR_FN(Locale::get("ERROR_FBO_INCOMPLETE_DRAW_BUFFER"));
        return false;
                                               }
    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:{
        ERROR_FN(Locale::get("ERROR_FBO_INCOMPLETE_READ_BUFFER"));
        return false;
                                               }
    case GL_FRAMEBUFFER_UNSUPPORTED:{
        ERROR_FN(Locale::get("ERROR_FBO_UNSUPPORTED"));
        return false;
                                    }
    case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_EXT:{
        ERROR_FN(Locale::get("ERROR_FBO_INCOMPLETE_MULTISAMPLE"));
        return false;
                                                   }
    default:{
        ERROR_FN(Locale::get("ERROR_UNKNOWN"));
        return false;
        }
    };
}