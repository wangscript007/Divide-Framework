/*�Copyright 2009-2011 DIVIDE-Studio�*/
/* This file is part of DIVIDE Framework.

   DIVIDE Framework is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   DIVIDE Framework is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with DIVIDE Framework.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _TEXTURE_H
#define _TEXTURE_H

#include "resource.h"
#include "Utility/Headers/ImageTools.h"
#include "Core/Headers/BaseClasses.h"

class Texture : public Resource
{
/*Abstract interface*/
public:
	virtual void Bind(U16 slot);
	virtual void Unbind(U16 slot);
	virtual void Destroy() = 0;
	virtual void LoadData(U32 target, U8* ptr, U16& w, U16& h, U8 d) = 0;
	virtual ~Texture() {_img.Destroy();}

	enum TextureFilters{
		LINEAR					= 0x0000,
		NEAREST					= 0x0001,
		NEAREST_MIPMAP_NEAREST  = 0x0002,
		LINEAR_MIPMAP_NEAREST   = 0x0003,
		NEAREST_MIPMAP_LINEAR   = 0x0004,
		LINEAR_MIPMAP_LINEAR    = 0x0005
	};

protected:
	virtual void Bind() const;
	virtual void Unbind() const;

public:
	virtual void createCopy() {incRefCount();}
	virtual void removeCopy(){decRefCount();}
			void resize(U16 width, U16 height);

	inline	U32 getHandle() const {return _handle;} 
	inline	U16 getWidth() const {return _width;}
	inline	U16 getHeight() const {return _height;}
	inline	U8  getBitDepth() const {return _bitDepth;}
	inline	bool isFlipped()  {return _flipped;}
	inline  bool hasTransparency() {return _hasTransparency;}
	static void enableGenerateMipmaps(bool generateMipMaps) {_generateMipmaps=generateMipMaps;}

	bool LoadFile(U32 target, const std::string& name);
	inline 	void setTextureWrap(bool repeatS, bool repeatT){_repeatS = repeatS; _repeatT = repeatT;}
	inline	void setTextureFilters(U8 minFilter, U8 magFilter) {_minFilter = minFilter; _magFilter = magFilter;}
	
protected:
	Texture(bool flipped = false);
	static bool checkBinding(U16 unit, U32 handle);

protected:
	U32	_handle;
	U16 _width,_height;
	U8  _bitDepth;			
	bool _flipped;
	bool _bound;
	bool _hasTransparency;
	ImageTools::ImageData _img;
	static bool _generateMipmaps;	
	mat4  _transformMatrix;
	bool  _repeatS, _repeatT;
	U8 _minFilter,_magFilter;
	static unordered_map<U8/*slot*/, U32/*textureHandle*/> textureBoundMap;
};


#endif
