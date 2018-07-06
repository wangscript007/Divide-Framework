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

#ifndef _D3D_TEXTURE_H_
#define _D3D_TEXTURE_H_

#include "core.h"
#include "Hardware/Video/Textures/Headers/Texture.h"

class d3dTexture : public Texture {
public:
	d3dTexture(U32 type, bool flipped = false) : Texture(flipped), _type(type) {}
	~d3dTexture() {}

	bool generateHWResource(const std::string& name){return true;}
	bool unload() {Destroy(); return true;}

	void Bind(U16 unit){}
	void Unbind(U16 unit){}
	void setMipMapRange(U32 base = 0, U32 max = 1000){}
	void loadData(U32 target, const U8* const ptr, const vec2<U16>& dimensions, U8 bpp, GFXImageFormat format){}

private:

	void Bind() const {}
	void Unbind() const {}
	void Destroy() {}

private:
	U32 _type;
};

#endif