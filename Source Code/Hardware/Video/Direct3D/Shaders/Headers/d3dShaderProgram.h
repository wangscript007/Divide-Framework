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

#ifndef HLSL_H_
#define HLSL_H_

#include "Hardware/Video/Shaders/Headers/ShaderProgram.h"

class d3dShaderProgram : public ShaderProgram {
public:
    d3dShaderProgram(const bool optimise = false) : ShaderProgram(optimise) {};
    ~d3dShaderProgram(){};

    bool unload(){unbind(); return true;}
    void bind(){}
    void unbind(bool resetActiveProgram = true){}
    void attachShader(Shader* const shader,const bool refresh = false){}
    void detachShader(Shader* const shader) {}
    //Attributes
    void Attribute(I32 location, D32 value){}
    void Attribute(I32 location, F32 value){}
    void Attribute(I32 location, const vec2<F32>& value){}
    void Attribute(I32 location, const vec3<F32>& value){}
    void Attribute(I32 location, const vec4<F32>& value){}
    //Uniforms
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
    void Uniform(I32 location, const vectorImpl<I32 >& values) {}
    void Uniform(I32 location, const vectorImpl<F32 >& values) {}
    void Uniform(I32 location, const vectorImpl<vec2<F32> >& values) {}
    void Uniform(I32 location, const vectorImpl<vec3<F32> >& values) {}
    void Uniform(I32 location, const vectorImpl<vec4<F32> >& values) {}
    void Uniform(I32 location, const vectorImpl<mat4<F32> >& values, bool rowMajor = false) {}
    //Uniform Texture
    void UniformTexture(I32 location, U16 slot){}

private:
    I32  cachedLoc(const std::string& name, const bool uniform = true) { return -1; }
    bool flushLocCache(){return true;}

private:
    Unordered_map<std::string, I32 > _shaderVars;

protected:
    inline bool generateHWResource(const std::string& name){return ShaderProgram::generateHWResource(name);}
    void validate(){}
    void link(){}
};

#endif
