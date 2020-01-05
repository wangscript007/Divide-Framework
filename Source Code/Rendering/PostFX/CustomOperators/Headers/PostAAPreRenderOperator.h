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

/// shader from:
/// http://www.geeks3d.com/20110405/fxaa-fast-approximate-anti-aliasing-demo-glsl-opengl-test-radeon-geforce/

#ifndef _POST_AA_PRE_RENDER_OPERATOR_H_
#define _POST_AA_PRE_RENDER_OPERATOR_H_

#include "Rendering/PostFX/Headers/PreRenderOperator.h"

namespace Divide {

class PostAAPreRenderOperator final : public PreRenderOperator {
   public:
    PostAAPreRenderOperator(GFXDevice& context, PreRenderBatch& parent, ResourceCache& cache);
    ~PostAAPreRenderOperator();

    void prepare(const Camera& camera, GFX::CommandBuffer& bufferInOut) final;
    void execute(const Camera& camera, GFX::CommandBuffer& bufferInOut) final;
    void reshape(U16 width, U16 height) final;

    inline U8 aaSamples() const { return _postAASamples; }
    void setAASamples(U8 postAASamples);

    inline void useSMAA(const bool state);
    inline bool usesSMAA() const { return _useSMAA; }

   private:
    PushConstants _fxaaConstants;
    ShaderProgram_ptr _fxaa;
    ShaderProgram_ptr _smaa;
    Pipeline* _aaPipeline;
    U8 _postAASamples;
    bool _useSMAA;
};

};  // namespace Divide

#endif