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

#ifndef _GL_ENUM_TABLE_H_
#define _GL_ENUM_TABLE_H_

#include "Hardware/Video/Headers/RenderAPIEnums.h"

namespace GL_ENUM_TABLE
{
   ///Populate enumaration tables with apropriate API values
   void fill();
};

extern unsigned int glBlendTable[BlendProperty_PLACEHOLDER];
extern unsigned int glBlendOpTable[BlendOperation_PLACEHOLDER];
extern unsigned int glCompareFuncTable[ComparisonFunction_PLACEHOLDER];
extern unsigned int glStencilOpTable[StencilOperation_PLACEHOLDER];
extern unsigned int glCullModeTable[CullMode_PLACEHOLDER];
extern unsigned int glFillModeTable[FillMode_PLACEHOLDER];
extern unsigned int glTextureTypeTable[TextureType_PLACEHOLDER];
extern unsigned int glImageFormatTable[GFXImageFormat_PLACEHOLDER];
extern unsigned int glPrimitiveTypeTable[PrimitiveType_PLACEHOLDER];
extern unsigned int glDataFormat[GDF_PLACEHOLDER];
extern unsigned int glWrapTable[TextureWrap_PLACEHOLDER];
extern unsigned int glTextureFilterTable[TextureFilter_PLACEHOLDER];

#endif