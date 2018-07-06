/*
   Copyright (c) 2014 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy of this software
   and associated documentation files (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef GL_UNIFORM_BUFFER_OBJECT_H_
#define GL_UNIFORM_BUFFER_OBJECT_H_

#include "Hardware/Video/OpenGL/Headers/glResources.h"
#include "Hardware/Video/Buffers/ShaderBuffer/Headers/ShaderBuffer.h"

///Base class for shader uniform blocks
class glUniformBuffer : public ShaderBuffer {
public:
    glUniformBuffer(const bool unbound);
    ~glUniformBuffer();
    ///Create a new buffer object to hold our uniform shader data
    ///if "dynamic" is false, the buffer will be created using GL_STATIC_DRAW
    ///if "dynamic" is true, the buffer will use either GL_STREAM_DRAW or GL_DYNAMIC_DRAW depending on the "stream" param
    ///default value will be a GL_DYNAMIC_DRAW, as most data will change once every few frames
    ///(lights might change per frame, so stream will be better in that case)
    void Create(bool dynamic = true, bool stream = false);
    ///Reserve primitiveCount * implementation specific primitive size of space in the buffer and fill it with nullptr values
    virtual void ReserveBuffer(GLuint primitiveCount, GLsizeiptr primitiveSize) const;
    virtual void ChangeSubData(GLintptr offset,	GLsizeiptr size, const GLvoid *data) const;
    virtual bool bindRange(GLuint bindIndex, GLintptr offset, GLsizeiptr size) const;
    virtual bool bind(GLuint bindIndex) const;

    void printUniformBlockInfo(GLint prog, GLint block_index);

protected:
    void setActive() const;

protected:
    GLuint _UBOid;
    GLenum _usage;
    GLenum _target;
};
#endif