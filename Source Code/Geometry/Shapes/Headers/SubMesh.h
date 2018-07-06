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

#ifndef _SUB_MESH_H_
#define _SUB_MESH_H_

#include "Object3D.h"

/*
DIVIDE-Engine: 21.10.2010 (Ionut Cava)

A SubMesh is a geometry wrapper used to build a mesh. Just like a mesh, it can be rendered locally or across
the server or disabled from rendering alltogheter. 

Objects created from this class have theyr position in relative space based on the parent mesh position.
(Same for scale,rotation and so on).

The SubMesh is composed of a VBO object that contains vertx,normal and textcoord data, a vector of materials,
and a name.
*/

#include "resource.h"
#include "Hardware/Video/VertexBufferObject.h"

class Mesh;
class SubMesh : public Object3D {

public:
	SubMesh(const std::string& name) : Object3D(name,SUBMESH),
									   _visibleToNetwork(true),
									   _render(true),
									   _id(0),
									   _parentMesh(NULL)
	{
		/// 3D objects usually requires the VBO to be recomputed on creation. mesh objects do not.
		_refreshVBO = false;
	}



	~SubMesh(){}

	bool load(const std::string& name) {return true;}
	bool unload();
	bool computeBoundingBox(SceneGraphNode* const sgn);
	inline U32  getId() {return _id;}
	/// When loading a submesh, the ID is the node index from the imported scene
	/// scene->mMeshes[n] == (SubMesh with _id == n)
	inline void setId(U32 id) {_id = id;}

	inline Mesh* getParentMesh() {return _parentMesh;}

	void onDraw();
protected:
	friend class Mesh;
	inline void setParentMesh(Mesh* const parentMesh) {_parentMesh = parentMesh;}

private:
	bool _visibleToNetwork, _render;
	U32 _id;
	Mesh* _parentMesh;
};

#endif