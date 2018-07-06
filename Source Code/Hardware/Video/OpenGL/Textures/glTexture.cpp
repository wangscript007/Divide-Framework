#include "Hardware/Video/OpenGL/Headers/glResources.h"

#include "core.h"
#include "Headers/glTexture.h"
#include "Hardware/Video/Headers/GFXDevice.h"

glTexture::glTexture(GLuint type, bool flipped) : Texture(flipped),
                                                  _type(type),
                                                  _internalFormat(GL_RGBA8),
                                                  _format(GL_RGBA)
{

}

glTexture::~glTexture()
{
}

void glTexture::threadedLoad(const std::string& name){
    Destroy();

    U32 tempHandle = 0;
    glGenTextures(1, &tempHandle);
    assert(tempHandle != 0);

    glBindTexture(_type,  tempHandle);

    glPixelStorei(GL_UNPACK_ALIGNMENT,1);
    glPixelStorei(GL_PACK_ALIGNMENT,1);

    if(_type == GL_TEXTURE_2D){
        if(!LoadFile(_type,name))	return;
    }else if(_type == GL_TEXTURE_CUBE_MAP){
        GLbyte i=0;
        std::stringstream ss( name );
        std::string it;
        while(std::getline(ss, it, ' ')) {
            if(!LoadFile(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, it))	return;
            i++;
        }
        if(i != 6){
            ERROR_FN(Locale::get("ERROR_TEXTURE_LOADER_CUBMAP_INIT_COUNT"), name.c_str());
            return;
        }
    }else if (_type == GL_TEXTURE_2D_ARRAY){
        GLbyte i = 0;
        std::stringstream ss(name);
        std::string it;
        while (std::getline(ss, it, ' ')) {
            if (!it.empty()){
                if(!LoadFile(i, it))
                	return;
                i++;
            }
        }
    }

    if(_samplerDescriptor.generateMipMaps()){
        if(glGenerateMipmap != nullptr){
            glGenerateMipmap(_type);
        }else{
            ERROR_FN(Locale::get("ERROR_NO_MIP_MAPS"));
            assert(glGenerateMipmap);
        }
    }

    glBindTexture(_type, 0);
    Texture::generateHWResource(name);
    Resource::threadedLoad(name);
    //Our "atomic swap"
    _handle = tempHandle;
}

void glTexture::setMipMapRange(U32 base, U32 max){
    if(!_samplerDescriptor.generateMipMaps()) return;

    glBindTexture(_type, _handle);
    glTexParameterf(_type, GL_TEXTURE_BASE_LEVEL, base);
    glTexParameterf(_type, GL_TEXTURE_MAX_LEVEL,  max);
    glBindTexture(_type, 0);
}

void glTexture::createSampler() {
    _sampler.Create(_samplerDescriptor);
}

bool glTexture::generateHWResource(const std::string& name) {
    //create the sampler first, even if it is the default one
    createSampler();
    GFX_DEVICE.loadInContext(_threadedLoading ? GFX_LOADING_CONTEXT : GFX_RENDERING_CONTEXT,
                             DELEGATE_BIND(&glTexture::threadedLoad, this, name));

    return true;
}

void glTexture::loadData(GLuint target, const GLubyte* const ptr, const vec2<U16>& dimensions, GLubyte bpp, GFXImageFormat format) {
    //If the current texture is a 2D one, than converting it to n^2 by n^2 dimensions will result in faster
    //rendering for the cost of a slightly higher loading overhead
    //The conversion code is based on the glmimg code from the glm library;
    size_t imageSize = (size_t)(dimensions.width) * (size_t)(dimensions.height) * (size_t)(bpp);
    GLubyte* img = New GLubyte[imageSize];
    memcpy(img, ptr, imageSize);
    _format = glImageFormatTable[format];
    _internalFormat = _format;
    GLuint width = dimensions.width;
    GLuint height = dimensions.height;

    switch (_format){
        case GL_RGB:  _internalFormat = GL_RGB8;  break;
        case GL_RGBA: _internalFormat = GL_RGBA8; break;
    }

    if (target != GL_TEXTURE_CUBE_MAP) {
        GLushort xSize2 = width, ySize2 = height;
        GLdouble xPow2 = log((GLdouble)xSize2) / log(2.0);
        GLdouble yPow2 = log((GLdouble)ySize2) / log(2.0);

        GLushort ixPow2 = (GLushort)xPow2;
        GLushort iyPow2 = (GLushort)yPow2;

        if (xPow2 != (GLdouble)ixPow2)   ixPow2++;
        if (yPow2 != (GLdouble)iyPow2)   iyPow2++;

        xSize2 = 1 << ixPow2;
        ySize2 = 1 << iyPow2;

        if((width != xSize2) || (height != ySize2)) {
            GLubyte* rdata = New GLubyte[xSize2*ySize2*bpp];
            gluScaleImage(_format, width, height, GL_UNSIGNED_BYTE, img, xSize2, ySize2, GL_UNSIGNED_BYTE, rdata);
            SAFE_DELETE_ARRAY(img);
            img = rdata;
            width = xSize2; height = ySize2;
        }
    }
    assert(width > 0 && height > 0);
    GLint numLevels = 1 + (GLint)floorf(log2f(fmaxf(width, height)));
    glTexParameterf(_type, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameterf(_type, GL_TEXTURE_MAX_LEVEL, numLevels - 1);
    if (target <= _numLayers){ // texture array
        if (target == 0){
            glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, _internalFormat, width, height, _numLayers, 0, _format, GL_UNSIGNED_BYTE, NULL);
        }
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, target, width, height, 1, _format, GL_UNSIGNED_BYTE, img);
    }else{
        glTexImage2D(target, 0, _internalFormat, width, height, 0, _format, GL_UNSIGNED_BYTE, img);
    }
    SAFE_DELETE_ARRAY(img);
}

void glTexture::Destroy(){
    if(_handle > 0){
        U32 textureId = _handle;
        glDeleteTextures(1, &textureId);
        _handle = 0;
        _sampler.Destroy();
    }
}

void glTexture::Bind(GLushort unit)  {
    GL_API::bindTexture(unit, _handle, _type, _sampler.getObjectHandle());
}