/*
Copyright (c) 2015 DIVIDE-Studio
Copyright (c) 2009 Ionut Cava

This file is part of DIVIDE Framework.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software
and associated documentation files (the "Software"), to deal in the Software
without restriction,
including without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the Software
is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE
OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#ifndef _SHADER_BUFFER_H_
#define _SHADER_BUFFER_H_

#include "Platform/Video/Headers/RenderAPIWrapper.h"
#include "Utility/Headers/GUIDWrapper.h"

namespace Divide {

class ShaderProgram;
class ShaderBuffer : private NonCopyable, public GUIDWrapper {
   public:
    typedef void* bufferPtr;

    ShaderBuffer(const stringImpl& bufferName, bool unbound,
                 bool persistentMapped)
        : GUIDWrapper(),
          _bufferName(bufferName),
          _unbound(unbound),
          _primitiveSize(0),
          _primitiveCount(0),
          _bufferSize(0),
          _persistentMapped(persistentMapped &&
                            !Config::Profile::DISABLE_PERSISTENT_BUFFER) {}

    virtual ~ShaderBuffer()
    {
    }

    /// Create a new buffer to hold our shader data
    virtual void Create(U32 primitiveCount, ptrdiff_t primitiveSize) {
        _primitiveCount = primitiveCount;
        _primitiveSize = primitiveSize;
        _bufferSize = primitiveSize * primitiveCount;
    }

    virtual void DiscardAllData() const = 0;
    virtual void DiscardSubData(ptrdiff_t offset, ptrdiff_t size) const = 0;
    virtual void UpdateData(ptrdiff_t offset,
                            ptrdiff_t size,
                            const bufferPtr data) const = 0;
    inline void SetData(const bufferPtr data) {
        UpdateData(0, _bufferSize, data);
    }

    virtual bool BindRange(U32 bindIndex,
                           U32 offsetElementCount,
                           U32 rangeElementCount) = 0;

    inline bool BindRange(ShaderBufferLocation bindIndex,
                          U32 offsetElementCount,
                          U32 rangeElementCount) {
        return BindRange(to_uint(bindIndex),
                         offsetElementCount,
                         rangeElementCount);

    }

    virtual bool Bind(U32 bindIndex) = 0;

    inline bool Bind(ShaderBufferLocation bindIndex) {
        return Bind(to_uint(bindIndex));
    }

    inline void PrintInfo(const ShaderProgram *shaderProgram,
                          ShaderBufferLocation bindIndex) {
        PrintInfo(shaderProgram, to_uint(bindIndex));
    }

    virtual void PrintInfo(const ShaderProgram *shaderProgram,
                           U32 bindIndex) = 0;

    inline size_t getPrimitiveSize() const { return _primitiveSize; }
    inline U32 getPrimitiveCount() const { return _primitiveCount; }

   protected:
    size_t _bufferSize;
    size_t _primitiveSize;
    U32 _primitiveCount;

    const bool _unbound;
    const bool _persistentMapped;

    stringImpl _bufferName;
};

};  // namespace Divide
#endif