/*
   Copyright (c) 2013 DIVIDE-Studio
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

#ifndef _GL_IM_EMULATION_H_
#define _GL_IM_EMULATION_H_

#include "Hardware/Video/Headers/RenderAPIEnums.h"
#include "Hardware/Video/Headers/ImmediateModeEmulation.h"

namespace NS_GLIM {
    class GLIM_BATCH;
    enum GLIM_ENUM;
};

extern NS_GLIM::GLIM_ENUM glimPrimitiveType[PrimitiveType_PLACEHOLDER];

class glIMPrimitive : public IMPrimitive {
protected:
    friend class GLWrapper;
    glIMPrimitive();
    ~glIMPrimitive();

public:
    void beginBatch();
    void begin(PrimitiveType type);
    void vertex(const vec3<F32>& vert);
    ///Specify each attribute at least once(even with dummy values) before calling begin!
    void attribute4ub(const std::string& attribName, const vec4<U8>& value);
    void attribute4f(const std::string& attribName, const vec4<F32>& value);
    void end();
    void endBatch();
    void clear();

protected:
    friend class GL_API;
    void renderBatch(bool wireframe = false);
    void renderBatchInstanced(I32 count, bool wireframe = false);
protected:
    NS_GLIM::GLIM_BATCH*  _imInterface;//< Rendering API specific implementation
};

#endif