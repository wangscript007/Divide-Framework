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

#ifndef _D3D_RENDER_STATE_BLOCK_H
#define _D3D_RENDER_STATE_BLOCK_H

#include "Hardware/Video/Headers/RenderStateBlock.h"

class d3dRenderStateBlock : public RenderStateBlock {
public:

   d3dRenderStateBlock(const RenderStateBlockDescriptor& descriptor);
   virtual ~d3dRenderStateBlock();

   void activate(d3dRenderStateBlock* oldState);

   inline I64 getGUID() const {return _hashValue;}

   inline RenderStateBlockDescriptor& getDescriptor() {return _descriptor;}

private:
   RenderStateBlockDescriptor _descriptor;
   I64 _hashValue;
};
#endif