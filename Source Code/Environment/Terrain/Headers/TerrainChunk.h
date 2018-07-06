/*�Copyright 2009-2013 DIVIDE-Studio�*/
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

#ifndef _TERRAIN_CHUNK_H
#define _TERRAIN_CHUNK_H

#include "core.h"

class Mesh;
class Terrain;
struct FileData;
class ShaderProgram;
class SceneGraphNode;
class VertexBufferObject;

class TerrainChunk{
public:
	void Destroy();
	int  DrawGround(I8 lod,ShaderProgram* const program, VertexBufferObject* const vbo);
	void DrawGrass(I8 lod, F32 d,VertexBufferObject* const grassVBO);
	void Load(U8 depth, vec2<U32> pos, vec2<U32> HMsize,VertexBufferObject* const groundVBO);

	inline vectorImpl<U32>&			getIndiceArray(I8 lod)		   {return _indice[lod];}
	inline vectorImpl<U16>&			getGrassIndiceArray()		   {return _grassIndice;}

	void addObject(Mesh* obj);
	void addTree(const vec4<F32>& pos,F32 scale, const FileData& tree,SceneGraphNode* parentNode);

	TerrainChunk() {}
	~TerrainChunk() {Destroy();}

private:
	void ComputeIndicesArray(I8 lod, U8 depth, vec2<U32> pos, vec2<U32> HMsize);

private:
	vectorImpl<U32> 	_indice[TERRAIN_CHUNKS_LOD];
	U16					_indOffsetW[TERRAIN_CHUNKS_LOD];
	U16  				_indOffsetH[TERRAIN_CHUNKS_LOD];
	F32                 _grassVisibility;
	vectorImpl<U16> 	_grassIndice;
};

#endif

