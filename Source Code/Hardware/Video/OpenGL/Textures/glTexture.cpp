#include "Hardware/Video/OpenGL/Headers/glResources.h"

#include "core.h"
#include "Headers/glTexture.h"
#include "Hardware/Video/Headers/GFXDevice.h"

glTexture::glTexture(GLuint type, bool flipped) : Texture(flipped),
												  _type(type),
												  _depth(GL_RGBA)
{
	_reservedStorage = false;
	_fixedPipeline = false;
	_canReserveStorage = glewIsSupported("GL_ARB_texture_storage");
}

glTexture::~glTexture(){}

bool glTexture::generateHWResource(const std::string& name) {

	Destroy();
	GLCheck(glGenTextures(1, &_handle));
	if(_handle == 0) return false;

	Bind();
	GLCheck(glPixelStorei(GL_UNPACK_ALIGNMENT,1));
	GLCheck(glPixelStorei(GL_PACK_ALIGNMENT,1));
	GLCheck(glTexParameteri(_type, GL_TEXTURE_BASE_LEVEL, 0));
	GLCheck(glTexParameteri(_type, GL_TEXTURE_MAX_LEVEL, 1000));
	GLCheck(glTexParameteri(_type, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	GLCheck(glTexParameteri(_type, GL_TEXTURE_MIN_FILTER, _generateMipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR));
	if (_maxAnisotrophy > 1 && _generateMipmaps) {
		if(!glewIsSupported("GL_EXT_texture_filter_anisotropic")){
			ERROR_FN(Locale::get("ERROR_NO_ANISO_SUPPORT"));
		}else{
			GLCheck(glTexParameteri(_type, GL_TEXTURE_MAX_ANISOTROPY_EXT,_maxAnisotrophy));
		}
    }
	if(_type == GL_TEXTURE_2D){
		GLCheck(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, glWrapTable[/*_wrapU*/TEXTURE_REPEAT]));
		GLCheck(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, glWrapTable[/*_wrapV*/TEXTURE_REPEAT]));
		if(!LoadFile(_type,name))	return false;

	}else if(_type == GL_TEXTURE_CUBE_MAP){
		GLCheck(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, glWrapTable[/*_wrapU*/TEXTURE_CLAMP_TO_EDGE]));
		GLCheck(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, glWrapTable[/*_wrapV*/TEXTURE_CLAMP_TO_EDGE]));
		GLCheck(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, glWrapTable[/*_wrapW*/TEXTURE_CLAMP_TO_EDGE]));

		GLbyte i=0;
		std::stringstream ss( name );
		std::string it;
		while(std::getline(ss, it, ' ')) {
			if(!LoadFile(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, it))
				return false;
			i++;
		}
	}  
	if(_generateMipmaps){
		if(glGenerateMipmap != NULL)
			GLCheck(glGenerateMipmap(_type));
		else 
			ERROR_FN(Locale::get("ERROR_NO_MIP_MAPS"));
	}

	Unbind();
	return Texture::generateHWResource(name);
}

void glTexture::LoadData(GLuint target, GLubyte* ptr, GLushort& w, GLushort& h, GLubyte d) {

	///If the current texture is a 2D one, than converting it to n^2 by n^2 dimensions will result in faster
	///rendering for the cost of a slightly higher loading overhead
	///The conversion code is based on the glmimg code from the glm library;
	GLubyte* img = New GLubyte[(size_t)(w) * (size_t)(h) * (size_t)(d)];
	memcpy(img, ptr, (size_t)(w) * (size_t)(h) * (size_t)(d));

	switch(d){
		case 1:
			_depth = GL_LUMINANCE;
			break;
		case 2:
		case 3:
		default:
			_depth = GL_RGB;
			break;
		case 4:
			_depth = GL_RGBA;
			break;
	}

    if (target == GL_TEXTURE_2D) {

		GLushort xSize2 = w, ySize2 = h;
		GLdouble xPow2 = log((GLdouble)xSize2) / log(2.0);
		GLdouble yPow2 = log((GLdouble)ySize2) / log(2.0);

		GLushort ixPow2 = (GLushort)xPow2;
		GLushort iyPow2 = (GLushort)yPow2;

		if (xPow2 != (GLdouble)ixPow2)   ixPow2++;
		if (yPow2 != (GLdouble)iyPow2)   iyPow2++;

		xSize2 = 1 << ixPow2;
		ySize2 = 1 << iyPow2;
    
		if((w != xSize2) || (h != ySize2)) {
			GLubyte* rdata = New GLubyte[xSize2*ySize2*d];
			GLCheck(gluScaleImage(_depth, w, h,GL_UNSIGNED_BYTE, img, xSize2, ySize2, GL_UNSIGNED_BYTE, rdata));
			delete[] img;
			img = rdata;
			w = xSize2; h = ySize2;
		}
	}
	if(_canReserveStorage && _type != GL_TEXTURE_CUBE_MAP){
		if(!_reservedStorage) reserveStorage(w,h);
		GLCheck(glTexSubImage2D(target,0,0,0,w,h,_depth,GL_UNSIGNED_BYTE,img));
	}else{
		GLCheck(glTexImage2D(target, 0, _depth, w, h, 0, _depth, GL_UNSIGNED_BYTE, img));
	}
}

void glTexture::reserveStorage(GLint w, GLint h){
	if(_canReserveStorage){
		GLint levels = 1;
		while ((w|h) >> levels) levels += 1;
		GLCheck(glTexStorage2D(_type,levels,_depth,w,h));
	}
	_reservedStorage = true;
}

void glTexture::Destroy(){
	if(_handle > 0){
		GLCheck(glDeleteTextures(1, &_handle));
		_handle = 0;
	}
}

void glTexture::Bind() const {
	GLCheck(glBindTexture(_type, _handle));
	Texture::Bind();
}

void glTexture::Bind(GLushort unit, bool fixedPipeline)  {
	if(checkBinding(unit,_handle)){ //prevent double bind
		if(fixedPipeline){
			GLCheck(glEnable(_type));
			_fixedPipeline = true;
		}
        GL_API::setActiveTextureUnit(unit);
		GLCheck(glBindTexture(_type, _handle));
	}
	Texture::Bind(unit);
}

void glTexture::Unbind() const {
	GLCheck(glBindTexture(_type, 0));
	Texture::Unbind();
}

void glTexture::Unbind(GLushort unit) {
    GL_API::setActiveTextureUnit(unit);
	GLCheck(glBindTexture(_type, 0));
    if(_fixedPipeline){
		GLCheck(glDisable(_type));
		_fixedPipeline = false;
	}
	Texture::Unbind(unit);
}

