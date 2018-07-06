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

#ifndef _D3D_PIXEL_BUFFER_OBJECT_H
#define _D3D_PIXEL_BUFFER_OBJECT_H

#include "Platform/Video/Buffers/PixelBuffer/Headers/PixelBuffer.h"

namespace Divide {

class d3dPixelBuffer : public PixelBuffer {
   public:
    d3dPixelBuffer(PBType type) : PixelBuffer(type) {}
    ~d3dPixelBuffer() { Destroy(); }

    bool Create(U16 width, U16 height, U16 depth = 0,
                GFXImageFormat internalFormatEnum = GFXImageFormat::RGBA8,
                GFXImageFormat formatEnum = GFXImageFormat::RGBA,
                GFXDataFormat dataTypeEnum = GFXDataFormat::FLOAT_32) {
        return true;
    }

    void Destroy(){};

    void* Begin() const { return 0; };
    void End() const {}

    void Bind(U8 unit = 0) const {}

    void updatePixels(const F32* const pixels, U32 pixelCount) {}

   private:
    bool checkStatus() { return true; }
};

};  // namespace Divide
#endif