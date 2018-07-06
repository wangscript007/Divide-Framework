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

#ifndef _WRAPPER_DX_H_
#define _WRAPPER_DX_H_

#include "Platform/Video/Headers/RenderAPIWrapper.h"

#include "Platform/Video/Direct3D/Buffers/Framebuffer/Headers/d3dRenderTarget.h"
#include "Platform/Video/Direct3D/Buffers/VertexBuffer/Headers/d3dVertexBuffer.h"
#include "Platform/Video/Direct3D/Buffers/VertexBuffer/Headers/d3dGenericVertexData.h"
#include "Platform/Video/Direct3D/Buffers/ShaderBuffer/Headers/d3dConstantBuffer.h"
#include "Platform/Video/Direct3D/Buffers/PixelBuffer/Headers/d3dPixelBuffer.h"
#include "Platform/Video/Direct3D/Shaders/Headers/d3dShaderProgram.h"
#include "Platform/Video/Direct3D/Shaders/Headers/d3dShader.h"
#include "Platform/Video/Direct3D/Textures/Headers/d3dTexture.h"
#include "Platform/Video/Direct3D/Headers/d3dEnumTable.h"

namespace Divide {

DEFINE_SINGLETON_EXT1_W_SPECIFIER(DX_API, RenderAPIWrapper, final)
  protected:
    DX_API() : RenderAPIWrapper() {}

    ErrorCode initRenderingAPI(const vec2<U16>& resolution, I32 argc,
                               char** argv) override;
    void closeRenderingAPI() override;
    void changeResolution(U16 w, U16 h) override;
    void changeViewport(const vec4<I32>& newViewport) const override;
    void setCursorPosition(U16 x, U16 y) const override;
    void uploadDrawCommands(
        const vectorImpl<IndirectDrawCommand>& drawCommands) const override;
    /// Change the window's position
    void setWindowPos(U16 w, U16 h) const override;
    void beginFrame() override;
    void endFrame() override;

    inline ShaderBuffer* newSB(
        const stringImpl& bufferName, const bool unbound = false,
        const bool persistentMapped = true) const override {
        return MemoryManager_NEW d3dConstantBuffer(bufferName, unbound,
                                                   persistentMapped);
    }

    inline IMPrimitive* newIMP() const override { return nullptr; }

    inline Framebuffer* newFB(bool multisampled) const override {
        return MemoryManager_NEW d3dRenderTarget(multisampled);
    }

    inline GenericVertexData* newGVD(
        const bool persistentMapped = false) const override {
        return MemoryManager_NEW d3dGenericVertexData(persistentMapped);
    }

    inline VertexBuffer* newVB() const override {
        return MemoryManager_NEW d3dVertexBuffer();
    }

    inline PixelBuffer* newPB(const PBType& type) const override {
        return MemoryManager_NEW d3dPixelBuffer(type);
    }

    inline Texture* newTextureArray(const bool flipped = false) const override {
        return MemoryManager_NEW d3dTexture(TEXTURE_2D_ARRAY, flipped);
    }

    inline Texture* newTexture2D(const bool flipped = false) const override {
        return MemoryManager_NEW d3dTexture(TEXTURE_2D, flipped);
    }

    inline Texture* newTextureCubemap(const bool flipped = false) const override {
        return MemoryManager_NEW d3dTexture(TEXTURE_CUBE_MAP, flipped);
    }

    inline ShaderProgram* newShaderProgram(
        const bool optimise = false) const override {
        return MemoryManager_NEW d3dShaderProgram(optimise);
    }

    inline Shader* newShader(const stringImpl& name, const ShaderType& type,
                             const bool optimise = false) const override {
        return MemoryManager_NEW d3dShader(name, type, optimise);
    }

    bool initShaders() override;
    bool deInitShaders() override;

    void toggleRasterization(bool state) override;
    void setLineWidth(F32 width) override;

    void drawText(const TextLabel& textLabel, const vec2<I32>& position) override;
    void drawPoints(U32 numPoints) override;

    void updateClipPlanes() override;

    void threadedLoadCallback() override;

    U64 getFrameDurationGPU() override { return 0; }

    void activateStateBlock(const RenderStateBlock& newBlock,
                            RenderStateBlock* const oldBlock) const override;

END_SINGLETON

};  // namespace Divide
#endif