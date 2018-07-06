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

#ifndef D3D_UNIFORM_BUFFER_OBJECT_H_
#define D3D_UNIFORM_BUFFER_OBJECT_H_

#include "Platform/Video/Buffers/ShaderBuffer/Headers/ShaderBuffer.h"
namespace Divide {

/// Base class for shader constant buffers
class d3dConstantBuffer final : public ShaderBuffer {
   public:
    d3dConstantBuffer(const stringImpl& bufferName, bool unbound,
                      bool persistentMapped);
    ~d3dConstantBuffer();

    /// Reserve primitiveCount * implementation specific primitive size of space
    /// in the buffer and fill it with NULL values
    void Create(U32 primitiveCount, ptrdiff_t primitiveSize);

    void DiscardAllData() const;

    void DiscardSubData(ptrdiff_t offset, ptrdiff_t size) const;

    void UpdateData(ptrdiff_t offset, ptrdiff_t size,
                    const bufferPtr data) const;

    bool BindRange(U32 bindIndex, U32 offsetElementCount,
                   U32 rangeElementCount);

    bool CheckBindRange(U32 bindIndex, U32 offsetElementCount,
                        U32 rangeElementCount);

    bool Bind(U32 bindIndex);
    bool CheckBind(U32 bindIndex);

    void PrintInfo(const ShaderProgram* shaderProgram, U32 bindIndex);
};

};  // namespace Divide
#endif