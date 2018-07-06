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

#ifndef _GL_TEXTURE_H_
#define _GL_TEXTURE_H_

#include "resource.h"
#include "../Texture.h"

class glTexture : public Texture {

public:
	glTexture(U32 type, bool flipped = false) : Texture(flipped), _type(type) {}
	~glTexture() {}

	bool load(const std::string& name);
	bool unload() {Destroy(); return true;}

	void Bind(U16 unit);
	void Unbind(U16 unit);

	void LoadData(U32 target, U8* ptr, U16& w, U16& h, U8 d);

private:

	void Bind() const;
	void Unbind() const;
	void Destroy();
	void Gen();

private:
	U32 _type;
};

#endif