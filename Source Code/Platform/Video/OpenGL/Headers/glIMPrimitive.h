/*
   Copyright (c) 2015 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software
   and associated documentation files (the "Software"), to deal in the Software
   without restriction,
   including without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
   PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
   DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
   IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef _GL_IM_EMULATION_H_
#define _GL_IM_EMULATION_H_

#include "Platform/Video/Headers/RenderAPIWrapper.h"
#include "Platform/Video/Headers/IMPrimitive.h"

#include <memory>

namespace NS_GLIM {
    class GLIM_BATCH;
}; //namespace NS_GLIM

namespace Divide {

/// An Implementation of the NS_GLIM library.
class glIMPrimitive : public IMPrimitive {
   public:
    glIMPrimitive();
    ~glIMPrimitive();

   public:
    /// Begins defining one piece of geometry that can later be rendered with
    /// one set of states.
    void beginBatch();
    /// Ends defining the batch. After this call "RenderBatch" can be called to
    /// actually render it.
    void endBatch();
    /// Begins gathering information about the given type of primitives.
    void begin(PrimitiveType type);
    /// Ends gathering information about the primitives.
    void end();
    /// Specify the position of a vertex belonging to this primitive
    void vertex(F32 x, F32 y, F32 z);
    /// Specify each attribute at least once(even with dummy values) before
    /// calling begin!
    /// Specify an attribute that will be applied to all vertex calls after this
    void attribute1i(const stringImpl& attribName, I32 value);
    /// Specify an attribute that will be applied to all vertex calls after this
    void attribute4ub(const stringImpl& attribName, U8 x, U8 y, U8 z, U8 w);
    /// Specify an attribute that will be applied to all vertex calls after this
    void attribute4f(const stringImpl& attribName, F32 x, F32 y, F32 z, F32 w);
    /// Restore the primitive to it's initial state
    void clear();
    /// Submit the created batch to the GPU for rendering
    void render(bool forceWireframe = false, U32 instanceCount = 1);

   protected:
    /// Rendering API specific implementation
    std::unique_ptr<NS_GLIM::GLIM_BATCH> _imInterface;
};

};  // namespace Divide
#endif