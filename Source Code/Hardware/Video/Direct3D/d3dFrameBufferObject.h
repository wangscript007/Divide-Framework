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

#ifndef _D3D_FRAME_BUFFER_OBJECT_H_
#define _D3D_FRAME_BUFFER_OBJECT_H_

#include "../FrameBufferObject.h"

class d3dFrameBufferObject : public FrameBufferObject
{
public:

	d3dFrameBufferObject(){}
	~d3dFrameBufferObject() {Destroy();}

	bool Create(FBO_TYPE type, U16 width, U16 height, TEXTURE_FORMAT_INTERNAL internalFormatEnum = RGBA8, TEXTURE_FORMAT formatEnum = RGBA) {return true;}
				
	void Destroy() {}

	void Begin(U8 nFace=0) const {}
	void End(U8 nFace=0) const {}

	void Bind(U8 unit=0, U8 texture = 0) {}	
	void Unbind(U8 unit=0) {}

private:
	bool checkStatus() {return true;}

};

#endif