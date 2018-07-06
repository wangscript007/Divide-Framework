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

#ifndef _TERRAIN_H_
#define _TERRAIN_H_

#include "Geometry/Shapes/Headers/Object3D.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Core/Math/BoundingVolumes/Headers/BoundingBox.h"
#include "Environment/Vegetation/Headers/Vegetation.h"
#include "Hardware/Video/Buffers/VertexBuffer/Headers/VertexBuffer.h"

struct TerrainTextureLayer {
    TerrainTextureLayer()
    {
        _lastOffset = 0;
        _blendMap = nullptr;
        _tileMaps = nullptr;
    }

    ~TerrainTextureLayer();

    enum TerrainTextureChannel {
        TEXTURE_RED_CHANNEL = 0,
        TEXTURE_GREEN_CHANNEL = 1,
        TEXTURE_BLUE_CHANNEL = 2,
        TEXTURE_ALPHA_CHANNEL = 3
    };

    inline void setBlendMap(Texture* texture) { _blendMap = texture; }
    inline void setTileMaps(Texture* texture) { _tileMaps = texture; }
    inline void setDiffuseScale(TerrainTextureChannel textureChannel, F32 scale) { _diffuseUVScale[textureChannel] = scale; }
    inline void setDetailScale(TerrainTextureChannel textureChannel, F32 scale)  { _detailUVScale[textureChannel]  = scale; }

    inline const vec4<F32>& getDiffuseScales() const { return _diffuseUVScale; }
    inline const vec4<F32>& getDetailScales()  const { return _detailUVScale;  }

    Texture* const blendMap() const { return _blendMap; }
    Texture* const tileMaps() const { return _tileMaps; }

private:
    U32 _lastOffset;
    vec4<F32> _diffuseUVScale;
    vec4<F32> _detailUVScale;
    Texture*  _blendMap;
    Texture*  _tileMaps;
};

class Quad3D;
class Quadtree;
class Transform;
class VertexBuffer;
class ShaderProgram;
class TerrainDescriptor;

class Terrain : public Object3D {
    friend class TerrainLoader;
public:

    Terrain();
    ~Terrain();

    bool unload();

    void drawBoundingBox(SceneGraphNode* const sgn) const;
    inline void toggleBoundingBoxes(){ _drawBBoxes = !_drawBBoxes; }

    vec3<F32>  getPositionFromGlobal(F32 x, F32 z) const;
    vec3<F32>  getPosition(F32 x_clampf, F32 z_clampf) const;
    vec3<F32>  getNormal(F32 x_clampf, F32 z_clampf) const;
    vec3<F32>  getTangent(F32 x_clampf, F32 z_clampf) const;
    vec2<F32>  getDimensions(){return vec2<F32>((F32)_terrainWidth, (F32)_terrainHeight);}

           void  terrainSmooth(F32 k);
           void  initializeVegetation(TerrainDescriptor* const terrain, SceneGraphNode* const terrainSGN);

    inline Quadtree&           getQuadtree()   const {return *_terrainQuadtree;}

    bool computeBoundingBox(SceneGraphNode* const sgn);

protected:

    void postDraw(SceneGraphNode* const sgn, const RenderStage& currentStage) {}

    void render(SceneGraphNode* const sgn, const SceneRenderState& sceneRenderState, const RenderStage& currentRenderStage);

    void sceneUpdate(const U64 deltaTime, SceneGraphNode* const sgn, SceneState& sceneState);

    void postLoad(SceneGraphNode* const sgn);

    inline void setUnderwaterDiffuseScale(F32 diffuseScale) {_underwaterDiffuseScale = diffuseScale;}

    size_t getDrawStateHash(RenderStage renderStage);
    ShaderProgram* const getDrawShader(RenderStage renderStage = FINAL_STAGE);
    bool isInView(const SceneRenderState& sceneRenderState, const BoundingBox& boundingBox, const BoundingSphere& sphere, const bool distanceCheck = true);

protected:
    friend class TerrainChunk;
    VegetationDetails _vegDetails;

    U8            _lightCount;
    U16			  _terrainWidth;
    U16           _terrainHeight;
    U32           _chunkSize;
    Quadtree*	  _terrainQuadtree;
 
    vec2<F32> _terrainScaleFactor;
    F32	 _farPlane;
    bool _alphaTexturePresent;
    bool _drawBBoxes;
    bool _terrainInView;
    bool _planeInView;
    SceneGraphNode* _planeSGN;
    SceneGraphNode* _vegetationGrassNode;
    BoundingBox     _boundingBox;
    Quad3D*		    _plane;
    F32             _underwaterDiffuseScale;
    vectorImpl<TerrainTextureLayer* > _terrainTextures;

    ///Normal rendering state
    size_t _terrainRenderStateHash;
    ///Depth map rendering state
    size_t _terrainDepthRenderStateHash;
    ///Reflection rendering state
    size_t _terrainReflectionRenderStateHash;
};

#endif
