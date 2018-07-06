#include "resource.h"
#include "glFrameBufferObject.h"

glFrameBufferObject::glFrameBufferObject()
{
	if(!glewIsSupported("glBindFramebuffer"))
	{
		glBindFramebuffer = GLEW_GET_FUN(__glewBindFramebufferEXT);
		glDeleteFramebuffers = GLEW_GET_FUN(__glewDeleteFramebuffersEXT);
		glFramebufferTexture2D = GLEW_GET_FUN(__glewFramebufferTexture2DEXT);
		glGenFramebuffers   = GLEW_GET_FUN(__glewGenFramebuffersEXT);
		glCheckFramebufferStatus = GLEW_GET_FUN(__glewCheckFramebufferStatusEXT);

	}
	_frameBufferHandle=0;
	_depthBufferHandle=0;
	_textureId = 0;
	_width = 0;
	_height = 0;
	_useFBO = true;
}

void glFrameBufferObject::Destroy()
{
	glDeleteTextures(1, &_textureId);
	glDeleteFramebuffers(1, &_frameBufferHandle);
	if(_useDepthBuffer)
		glDeleteRenderbuffers(1, &_depthBufferHandle);

	_textureId = 0;
	_width = 0;
	_height = 0;
	_useFBO = true;
}


void glFrameBufferObject::Begin(U32 nFace) const
{
	assert(nFace<6);
	glPushAttrib(GL_VIEWPORT_BIT);
	glViewport(0, 0, _width, _height);

	if(_useFBO) {
		glBindFramebuffer(GL_FRAMEBUFFER, _frameBufferHandle);

		if(_textureType == GL_TEXTURE_CUBE_MAP) {
			glFramebufferTexture2D(GL_FRAMEBUFFER, _attachment, GL_TEXTURE_CUBE_MAP_POSITIVE_X+nFace, _textureId, 0);
		}
	}
	else {
        glPushAttrib(GL_COLOR_BUFFER_BIT | GL_PIXEL_MODE_BIT);
        glDrawBuffer(GL_BACK);
        glReadBuffer(GL_BACK);
	}
}

void glFrameBufferObject::End(U32 nFace) const
{
	assert(nFace<6);
	if(_useFBO) {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
	else {
        glBindTexture(_textureType, _textureId);
        glCopyTexSubImage2D(_textureType, 0, 0, 0, 0, 0, _width, _height);
        glBindTexture(_textureType, 0);

        glPopAttrib();	
	}
	glPopAttrib();
}

void glFrameBufferObject::Bind(int unit) const
{
	glActiveTexture(GL_TEXTURE0 + unit);
	glEnable(_textureType);
	glBindTexture(_textureType, _textureId);
}

void glFrameBufferObject::Unbind(int unit) const
{
	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture( _textureType, 0 );
	glDisable(_textureType);
}


bool glFrameBufferObject::Create(FBO_TYPE type, U32 width, U32 height)
{
	Destroy();


	_width = width;
	_height = height;
	_useFBO = true;
	_useDepthBuffer = true;
	_textureType = type == FBO_CUBE_COLOR? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;


	
	// Texture
	glGenTextures( 1, &_textureId );
	glBindTexture(_textureType, _textureId);
	glTexParameterf(_textureType, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(_textureType, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(_textureType, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(_textureType, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);


//	if(type==FBO_2D_DEPTH)
//		glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_LUMINANCE);

	U32 eTarget;
	U32 nFrames;
	if(type!=FBO_CUBE_COLOR) {
		eTarget = GL_TEXTURE_2D;
		nFrames = 1;
	}
	else {
		eTarget = GL_TEXTURE_CUBE_MAP_POSITIVE_X;
		nFrames = 6;
	}

	for(U32 i=0; i<nFrames; i++)
	{
		if(type==FBO_2D_DEPTH)
			glTexImage2D(eTarget+i, 0, GL_DEPTH_COMPONENT24, _width, _height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0);
		else
			glTexImage2D(eTarget+i, 0, GL_RGB, _width, _height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	}



	if(_useFBO)
	{
		// Frame buffer
		glGenFramebuffers(1, &_frameBufferHandle);
		glBindFramebuffer(GL_FRAMEBUFFER, _frameBufferHandle);
		if(_useDepthBuffer /*&& format!=GL_DEPTH_COMPONENT*/)
		{
			// Depth buffer
			glGenRenderbuffers(1, &_depthBufferHandle);
			glBindRenderbuffer(GL_RENDERBUFFER, _depthBufferHandle);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, _width, _height);

			// atasarea frame bufferului de depth buffer
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _depthBufferHandle);
		}

		//  attach the framebuffer to our texture, which may be a depth texture
		if(type==FBO_2D_DEPTH) {
			_attachment = GL_DEPTH_ATTACHMENT;
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, eTarget, _textureId, 0);
			//  disable drawing to any buffers, we only want the depth
			glDrawBuffer(GL_NONE);
			glReadBuffer(GL_NONE);
		} else {
			_attachment = GL_COLOR_ATTACHMENT0;
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, eTarget, _textureId, 0);
		}
		_useFBO = checkStatus();
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	return true;
}



bool glFrameBufferObject::checkStatus()
{
	//Not defined in GLEW
	#define GL_FRAMEBUFFER_INCOMPLETE_FORMATS 0x8CDA
	#define GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS 0x8CD9

    // check FBO status
    U32 status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    switch(status)
    {
    case GL_FRAMEBUFFER_COMPLETE:
//      std::cout << "Framebuffer complete." << std::endl;
        return true;

    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
        std::cout << "[ERROR] Framebuffer incomplete: Attachment is NOT complete." << std::endl;
        return false;

    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
        std::cout << "[ERROR] Framebuffer incomplete: No image is attached to FBO." << std::endl;
        return false;

    case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
        std::cout << "[ERROR] Framebuffer incomplete: Attached images have different dimensions." << std::endl;
        return false;

    case GL_FRAMEBUFFER_INCOMPLETE_FORMATS:
        std::cout << "[ERROR] Framebuffer incomplete: Color attached images have different internal formats." << std::endl;
        return false;

    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
        std::cout << "[ERROR] Framebuffer incomplete: Draw buffer." << std::endl;
        return false;

    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
        std::cout << "[ERROR] Framebuffer incomplete: Read buffer." << std::endl;
        return false;

    case GL_FRAMEBUFFER_UNSUPPORTED:
        std::cout << "[ERROR] Unsupported by FBO implementation." << std::endl;
        return false;

    default:
        std::cout << "[ERROR] Unknow error." << std::endl;
        return false;
    }
}




