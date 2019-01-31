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
    class Texture;

    struct TextureView {
        Texture* _texture = nullptr;
        vec2<U16> _mipLevels = {};
        vec2<U16> _layerRange = {};

        inline bool operator==(const TextureView& other) const {
            return _mipLevels == other._mipLevels &&
                   _layerRange == other._layerRange &&
                   _texture == other._texture;
        }

        inline bool operator!=(const TextureView& other) const {
            return _mipLevels != other._mipLevels ||
                   _layerRange != other._layerRange ||
                   _texture != other._texture;
        }
    };

    struct TextureViewEntry {
        U8 _binding = 0;
        TextureView _view = {};

        inline bool operator==(const TextureViewEntry& other) const {
            return _binding == other._binding &&
                   _view == other._view;
        }

        inline bool operator!=(const TextureViewEntry& other) const {
            return _binding != other._binding ||
                   _view != other._view;
        }
    };

    class ShaderBuffer;
    struct ShaderBufferBinding {
        ShaderBufferLocation _binding = ShaderBufferLocation::COUNT;
        std::pair<bool, vec2<U8>> _atomicCounter = { false, {0u, 0u} };
        vec2<U32>     _elementRange = {};
        ShaderBuffer* _buffer = nullptr;

        bool set(const ShaderBufferBinding& other);
        bool set(ShaderBufferLocation binding, ShaderBuffer* buffer, const vec2<U32>& elementRange, const std::pair<bool, vec2<U32>>& atomicCounter = std::make_pair(false, vec2<U32>(0u)));

        bool operator==(const ShaderBufferBinding& other) const;
        bool operator!=(const ShaderBufferBinding& other) const;
    };

    typedef vectorEASTL<ShaderBufferBinding> ShaderBufferList;

    typedef vectorEASTL<TextureViewEntry> TextureViews;

    struct DescriptorSet {
        //This needs a lot more work!
        TextureDataContainer _textureData = {};

        ShaderBufferList _shaderBuffers = {};
        TextureViews _textureViews = {};

        void addShaderBuffer(const ShaderBufferBinding& entry);
        void addShaderBuffers(const ShaderBufferList& entries);
        const ShaderBufferBinding* findBinding(ShaderBufferLocation slot) const;
        const TextureData* findTexture(U8 binding) const;
        const TextureView* findTextureView(U8 binding) const;

        inline bool operator==(const DescriptorSet &other) const {
            return _shaderBuffers == other._shaderBuffers &&
                   _textureData == other._textureData &&
                   _textureViews == other._textureViews;
        }

        inline bool operator!=(const DescriptorSet &other) const {
            return _shaderBuffers != other._shaderBuffers ||
                   _textureData != other._textureData ||
                   _textureViews != other._textureViews;
        }
    };

    bool Merge(DescriptorSet &lhs, DescriptorSet &rhs, bool& partial);

    typedef MemoryPool<DescriptorSet, nextPOW2(sizeof(DescriptorSet) * 256)> DescriptorSetPool;

    struct DeleteDescriptorSet {
        DeleteDescriptorSet(std::mutex& lock, DescriptorSetPool& context)
            : _lock(lock),
              _context(context)
        {
        }

        inline void operator()(DescriptorSet* res) {
            UniqueLock w_lock(_lock);
            _context.deleteElement(res);
        }

        std::mutex& _lock;
        DescriptorSetPool& _context;
    };

}; //namespace Divide

#endif //_DESCRIPTOR_SETS_H_
