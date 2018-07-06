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

#ifndef _MESH_H_
#define _MESH_H_

/**
DIVIDE-Engine: 21.10.2010 (Ionut Cava)

Mesh class. This class wraps all of the renderable geometry drawn by the engine.
The only exceptions are: Terrain (including TerrainChunk) and Vegetation.

Meshes are composed of at least 1 submesh that contains vertex data, texture info and so on.
A mesh has a name, position, rotation, scale and a boolean value that enables or disables rendering
across the network and one that disables rendering alltogheter;

Note: all transformations applied to the mesh affect every submesh that compose the mesh.
*/

#include "Object3D.h"
#include <assimp/anim.h>

class SubMesh;
class Mesh : public Object3D {
public:

    Mesh(ObjectFlag flag = OBJECT_FLAG_NONE) : Object3D(MESH,TRIANGLES,flag),
                                              _visibleToNetwork(true)
    {
    }

    virtual ~Mesh() {}

    bool computeBoundingBox(SceneGraphNode* const sgn);
    virtual void updateTransform(SceneGraphNode* const sgn);
    virtual void updateBBatCurrentFrame(SceneGraphNode* const sgn);

    /// Called from SceneGraph "sceneUpdate"
    virtual void sceneUpdate(const D32 deltaTime, SceneGraphNode* const sgn, SceneState& sceneState);
    virtual void postLoad(SceneGraphNode* const sgn);
    virtual	void onDraw(const RenderStage& currentStage);
    inline  void render(SceneGraphNode* const sgn){};
    virtual void preFrameDrawEnd() {}

    inline void  addSubMesh(const std::string& subMesh) {_subMeshes.push_back(subMesh);}
    inline const BoundingBox& getMaxBoundingBox() const { return _maxBoundingBox; }

protected:
    typedef Unordered_map<std::string, SceneGraphNode*> childrenNodes;
    typedef Unordered_map<U32, SubMesh*> subMeshRefMap;

    bool _visibleToNetwork;

    vectorImpl<std::string > _subMeshes;
    subMeshRefMap            _subMeshRefMap;
    BoundingBox              _maxBoundingBox;
};

#endif