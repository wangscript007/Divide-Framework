/*�Copyright 2009-2012 DIVIDE-Studio�*/
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

#ifndef _D3D_VERTEX_BUFFER_OBJECT_H
#define _D3D_VERTEX_BUFFER_OBJECT_H

#include "../VertexBufferObject.h"

class d3dVertexBufferObject : public VertexBufferObject {

public:
	bool        Create() {return true;}
	bool		Create(U32 usage) {return true;}

	void		Destroy() {}

	
	void		Enable() {}
	void		Disable() {}

	
	d3dVertexBufferObject(){}
	~d3dVertexBufferObject() {Destroy();}

private:
	void Enable_VA() {}	
	void Enable_VBO() {}	
	void Disable_VA() {}	
	void Disable_VBO() {}

private:
	bool _created; 

};

#endif

