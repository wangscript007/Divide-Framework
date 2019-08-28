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
#ifndef _RENDER_TARGET_DRAW_DESCRIPTOR_H_
#define _RENDER_TARGET_DRAW_DESCRIPTOR_H_

#include "RTAttachment.h"
#include "Platform/Video/Headers/BlendingProperties.h"

namespace Divide {

// 4 should be more than enough even for batching multiple render targets together
constexpr U8 MAX_RT_COLOUR_ATTACHMENTS = 4;

class RTDrawMask {
  public:
    RTDrawMask();

    bool isEnabled(RTAttachmentType type) const;
    bool isEnabled(RTAttachmentType type, U8 index) const;
    void setEnabled(RTAttachmentType type, U8 index, const bool state);

    void enableAll();
    void disableAll();

    inline bool operator==(const RTDrawMask& other) const;
    inline bool operator!=(const RTDrawMask& other) const;

  private:
    std::array<bool, MAX_RT_COLOUR_ATTACHMENTS> _disabledColours;
    bool _disabledDepth;
    bool _disabledStencil;
};

struct RTBlendState {
    UColour4 _blendColour = {};
    BlendingProperties _blendProperties;

    inline bool operator==(const RTBlendState& other) const;
    inline bool operator!=(const RTBlendState& other) const;
};

class RTDrawDescriptor {
  public: 
    enum class State : U8 {
        CLEAR_COLOUR_BUFFERS = toBit(1),
        CLEAR_DEPTH_BUFFER = toBit(2),
        CHANGE_VIEWPORT = toBit(3),
        COUNT = 3
    };

  public:
    RTDrawDescriptor();

    void stateMask(U32 stateMask);
    void enableState(State state);
    void disableState(State state);
    bool isEnabledState(State state) const;

    inline RTDrawMask& drawMask() { return _drawMask; }
    inline const RTDrawMask& drawMask() const { return _drawMask; }
    inline U32 stateMask() const { return _stateMask; }

    inline RTBlendState& blendState(U8 index) { return _blendStates[index]; }
    inline const RTBlendState& blendState(U8 index) const{ return _blendStates[index]; }

    inline void clearColour(U8 index, const bool state) { _clearColourAttachment[index] = state; }
    inline bool clearColour(U8 index) const { return _clearColourAttachment[index]; }

    void clearExternalColour(bool state) { _clearExternalColour = state; }
    bool clearExternalColour() const { return _clearExternalColour; }

    void clearExternalDepth(bool state) { _clearExternalDepth = state; }
    bool clearExternalDepth() const { return _clearExternalDepth; }

    void markDirtyLayer(RTAttachmentType type, U8 index, U16 layer);
    const std::unordered_set<U16>& getDirtyLayers(RTAttachmentType type, U8 index = 0) const;

    inline bool operator==(const RTDrawDescriptor& other) const;
    inline bool operator!=(const RTDrawDescriptor& other) const;

  protected:

    typedef std::unordered_set<U16> DirtyLayers;
    typedef std::pair<U8, DirtyLayers> DirtyLayersEntry;

    RTDrawMask _drawMask;
    std::array<vectorEASTL<DirtyLayersEntry>, to_base(RTAttachmentType::COUNT)> _dirtyLayers;
    std::array<RTBlendState, MAX_RT_COLOUR_ATTACHMENTS> _blendStates;
    std::array<bool, MAX_RT_COLOUR_ATTACHMENTS> _clearColourAttachment;
    U32 _stateMask = 0;
    bool _clearExternalColour = false;
    bool _clearExternalDepth = false;
};

}; //namespace Divide

#endif //_RENDER_TARGET_DRAW_DESCRIPTOR_H_


#include "RTDrawDescriptor.inl"