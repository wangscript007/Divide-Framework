/*
Copyright (c) 2018 DIVIDE-Studio
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

#pragma once
#ifndef _DESCRIPTOR_SETS_H_
#define _DESCRIPTOR_SETS_H_

#include "ClipPlanes.h"
#include "TextureData.h"
#include "Core/Math/Headers/MathVectors.h"

namespace Divide {
    class ShaderBuffer;
    struct ShaderBufferBinding {
        ShaderBufferLocation _binding;
        ShaderBuffer* _buffer;
        vec2<U32>     _range;
        std::pair<bool, vec2<U32>> _atomicCounter;

        ShaderBufferBinding();
        ShaderBufferBinding(ShaderBufferLocation slot,
                            ShaderBuffer* buffer,
                            const vec2<U32>& range);
        ShaderBufferBinding(ShaderBufferLocation slot,
                            ShaderBuffer* buffer,
                            const vec2<U32>& range,
                            const std::pair<bool, vec2<U32>>& atomicCounter);

        void set(const ShaderBufferBinding& other);
        void set(ShaderBufferLocation binding,
                 ShaderBuffer* buffer,
                 const vec2<U32>& range);
        void set(ShaderBufferLocation binding,
                 ShaderBuffer* buffer,
                 const vec2<U32>& range,
                 const std::pair<bool, vec2<U32>>& atomicCounter);

        bool operator==(const ShaderBufferBinding& other) const;
        bool operator!=(const ShaderBufferBinding& other) const;
    };

    typedef vectorEASTL<ShaderBufferBinding> ShaderBufferList;


    struct DescriptorSet {
        //This needs a lot more work!
        ShaderBufferList _shaderBuffers;
        TextureDataContainer _textureData;

        bool merge(const DescriptorSet &other);

        bool operator==(const DescriptorSet &other) const;
        bool operator!=(const DescriptorSet &other) const;
    };
}; //namespace Divide

#endif //_DESCRIPTOR_SETS_H_
