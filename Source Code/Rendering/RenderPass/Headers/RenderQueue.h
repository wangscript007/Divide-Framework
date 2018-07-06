/*
   Copyright (c) 2015 DIVIDE-Studio
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

#ifndef _RENDER_QUEUE_H_
#define _RENDER_QUEUE_H_

#include "RenderBin.h"

namespace Divide {

class Material;
class SceneNode;

/// This class manages all of the RenderBins and renders them in the correct order
class RenderQueue {
    typedef hashMapImpl<RenderBin::RenderBinType, RenderBin*> RenderBinMap;
    typedef hashMapImpl<U16, RenderBin::RenderBinType> RenderBinIDType;

  public:
    RenderQueue();
    ~RenderQueue();

    void sort(RenderStage renderStage);
    void refresh();
    void addNodeToQueue(SceneGraphNode& sgn, const vec3<F32>& eyePos);
    U16 getRenderQueueStackSize() const;
    SceneGraphNode& getItem(U16 renderBin, U16 index);
    RenderBin* getBin(RenderBin::RenderBinType rbType);

    inline U16 getRenderQueueBinSize() { 
        return (U16)_sortedRenderBins.size(); 
    }

    inline RenderBin* getBinSorted(U16 renderBin) {
        return _sortedRenderBins[renderBin];
    }

    inline RenderBin* getBin(U16 renderBin) {
        return getBin(_renderBinID[renderBin]);
    }

  private:
    RenderBin* getBinForNode(SceneNode* const nodeType,
                             Material* const matInstance);

    RenderBin* getOrCreateBin(const RenderBin::RenderBinType& rbType);

  private:
    RenderBinMap _renderBins;
    RenderBinIDType _renderBinID;
    vectorImpl<RenderBin*> _sortedRenderBins;

};

};  // namespace Divide

#endif
