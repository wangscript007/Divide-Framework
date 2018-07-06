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

#ifndef HLSL_H_
#define HLSL_H_

#include "Platform/Video/Shaders/Headers/ShaderProgram.h"

namespace Divide {

class d3dShaderProgram : public ShaderProgram {
   public:
    d3dShaderProgram(const bool optimise = false) : ShaderProgram(optimise) {}

    ~d3dShaderProgram() {}

    bool unload() {
        unbind();
        return true;
    }
    bool bind() { return false; }
    void unbind(bool resetActiveProgram = true) {}

    bool isValid() const { return false; }
    void attachShader(Shader* const shader, const bool refresh = false) {}
    void detachShader(Shader* const shader) {}

    void registerShaderBuffer(ShaderBuffer& buffer) {}

    // Subroutines
    void SetSubroutines(ShaderType type, const vectorImpl<U32>& indices) const {
    }
    void SetSubroutine(ShaderType type, U32 index) const {}
    U32 GetSubroutineIndex(ShaderType type, const stringImpl& name) const {
        return 0;
    }
    U32 GetSubroutineUniformLocation(ShaderType type,
                                     const stringImpl& name) const {
        return 0;
    }
    U32 GetSubroutineUniformCount(ShaderType type) const { return 0; }
    // Uniforms
    void Uniform(const stringImpl& ext, U8 slot) {}
    void Uniform(const stringImpl& ext, U32 value) {}
    void Uniform(const stringImpl& ext, I32 value) {}
    void Uniform(const stringImpl& ext, F32 value) {}
    void Uniform(const stringImpl& ext, const vec2<F32>& value) {}
    void Uniform(const stringImpl& ext, const vec2<I32>& value) {}
    void Uniform(const stringImpl& ext, const vec2<U16>& value) {}
    void Uniform(const stringImpl& ext, const vec3<F32>& value) {}
    void Uniform(const stringImpl& ext, const vec4<F32>& value) {}
    void Uniform(const stringImpl& ext,
                 const mat3<F32>& value,
                 bool rowMajor = false) {}
    void Uniform(const stringImpl& ext,
                 const mat4<F32>& value,
                 bool rowMajor = false) {}
    void Uniform(const stringImpl& ext, const vectorImpl<I32>& values) {}
    void Uniform(const stringImpl& ext, const vectorImpl<F32>& values) {}
    void Uniform(const stringImpl& ext, const vectorImpl<vec2<F32> >& values) {}
    void Uniform(const stringImpl& ext, const vectorImpl<vec3<F32> >& values) {}
    void Uniform(const stringImpl& ext, const vectorImpl<vec4<F32> >& values) {}
    void Uniform(const stringImpl& ext,
                 const vectorImpl<mat3<F32> >& values,
                 bool rowMajor = false) {}
    void Uniform(const stringImpl& ext,
                 const vectorImpl<mat4<F32> >& values,
                 bool rowMajor = false) {}

    void Uniform(I32 location, U8 slot) {}
    void Uniform(I32 location, U32 value) {}
    void Uniform(I32 location, I32 value) {}
    void Uniform(I32 location, F32 value) {}
    void Uniform(I32 location, const vec2<F32>& value) {}
    void Uniform(I32 location, const vec2<I32>& value) {}
    void Uniform(I32 location, const vec2<U16>& value) {}
    void Uniform(I32 location, const vec3<F32>& value) {}
    void Uniform(I32 location, const vec4<F32>& value) {}
    void Uniform(I32 location, const mat3<F32>& value, bool rowMajor = false) {}
    void Uniform(I32 location, const mat4<F32>& value, bool rowMajor = false) {}
    void Uniform(I32 location, const vectorImpl<I32>& values) {}
    void Uniform(I32 location, const vectorImpl<F32>& values) {}
    void Uniform(I32 location, const vectorImpl<vec2<F32> >& values) {}
    void Uniform(I32 location, const vectorImpl<vec3<F32> >& values) {}
    void Uniform(I32 location, const vectorImpl<vec4<F32> >& values) {}
    void Uniform(I32 location,
                 const vectorImpl<mat3<F32> >& values,
                 bool rowMajor = false) {}
    void Uniform(I32 location,
                 const vectorImpl<mat4<F32> >& values,
                 bool rowMajor = false) {}

   protected:
    inline bool generateHWResource(const stringImpl& name) {
        return ShaderProgram::generateHWResource(name);
    }
};

};  // namespace Divide
#endif