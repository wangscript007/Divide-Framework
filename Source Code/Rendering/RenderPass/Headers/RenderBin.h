/*
   Copyright (c) 2018 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software
   and associated documentation files (the "Software"), to deal in the Software
   without restriction,
   including without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
   PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
   DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
   IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

/*The system is similar to the one used in Torque3D (RenderPassMgr / RenderBinManager) as it was used as an inspiration.
  All credit goes to GarageGames for the idea:
  - http://garagegames.com/
  - https://github.com/GarageGames/Torque3D
  */

#pragma once
#ifndef _RENDER_BIN_H_
#define _RENDER_BIN_H_

#include "Platform/Video/Headers/RenderAPIEnums.h"

namespace Divide {

struct Task;
class GFXDevice;
class SceneGraphNode;
class RenderingComponent;
struct RenderStagePass;

namespace GFX {
    class CommandBuffer;
};

struct RenderBinItem {
    RenderingComponent* _renderable = nullptr;
    size_t _stateHash = 0;
    I64 _sortKeyA = -1;
    I32 _sortKeyB = -1;
    F32 _distanceToCameraSq = 0.0f;
};

enum class RenderingOrder : U8 {
    NONE = 0,
    FRONT_TO_BACK,
    BACK_TO_FRONT,
    BY_STATE,
    WATER_FIRST, //Hack, but improves rendering perf :D
    COUNT
};

//Bins can hold certain node types. This is also the order in which nodes will be rendered!
BETTER_ENUM(RenderBinType, U8,
    RBT_OPAQUE,      //< Opaque objects will occlude a lot of the terrain and terrain is REALLY expensive to render, so maybe draw them first?
    RBT_TERRAIN_AUX, //< Water, infinite ground plane, etc. Ground, but not exactly ground. Still oclude a lot of terrain AND cheaper to render
    RBT_TERRAIN,     //< Actual terrain. It should cover most of the remaining empty screen space
    RBT_SKY,         //< Sky needs to be drawn after ALL opaque geometry to save on fillrate
    RBT_TRANSLUCENT, //< Translucent items use a [0.0...1.0] alpha values supplied via an opacity map or the albedo's alpha channel
    RBT_IMPOSTOR,    //< Impostors should be overlayed over everything since they are a debugging tool
    RBT_COUNT);

class RenderPackage;
class SceneRenderState;
class RenderPassManager;

enum class RenderQueueListType : U8 {
    OCCLUDERS = 0,
    OCCLUDEES,
    COUNT
};

class RenderBin;
using RenderQueuePackages = vectorEASTLFast<RenderPackage*>;

/// This class contains a list of "RenderBinItem"'s and stores them sorted
/// depending on designation
class RenderBin {
   public:
    using RenderBinStack = vectorEASTL<RenderBinItem>;
    using SortedQueue = vectorEASTL<RenderingComponent*>;
    using SortedQueues = std::array<SortedQueue, RenderBinType::RBT_COUNT>;

    friend class RenderQueue;

    explicit RenderBin(RenderBinType rbType);
    ~RenderBin() = default;

    void sort(RenderStage stage, RenderingOrder renderOrder);
    void populateRenderQueue(RenderStagePass stagePass, RenderQueuePackages& queueInOut) const;
    void postRender(const SceneRenderState& renderState, RenderStagePass stagePass, GFX::CommandBuffer& bufferInOut);
    void refresh(RenderStage stage);

    void addNodeToBin(const SceneGraphNode* sgn,
                      const RenderStagePass& renderStagePass,
                      F32 minDistToCameraSq);

    const RenderBinItem& getItem(RenderStage stage, U16 index) const;

    U16 getSortedNodes(RenderStage stage, SortedQueue& nodes) const;

    U16 getBinSize(RenderStage stage) const;

    bool empty(RenderStage stage) const;

    RenderBinType getType() const noexcept { return _rbType; }

   private:
    const RenderBinType _rbType;

    std::array<RenderBinStack, to_base(RenderStage::COUNT)> _renderBinStack;
};

};  // namespace Divide
#endif