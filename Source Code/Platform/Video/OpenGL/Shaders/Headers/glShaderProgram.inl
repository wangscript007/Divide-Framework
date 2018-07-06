/* Copyright (c) 2015 DIVIDE-Studio
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

#ifndef _PLATFORM_VIDEO_OPENGL_SHADERS_SHADER_PROGRAM_INL_
#define _PLATFORM_VIDEO_OPENGL_SHADERS_SHADER_PROGRAM_INL_

namespace Divide {

template <>
inline bool glShaderProgram::cachedValueUpdate(I32 location, const U8& value) {
    assert(location != -1);

    ShaderVarU8Map::iterator it = _shaderVarsU8.find(location);
    if (it != std::end(_shaderVarsU8)) {
        if (it->second == value) {
            return false;
        } else {
            it->second = value;
        }
    } 

    hashAlg::emplace(_shaderVarsU8, location, value);
    return true;
}

template <>
inline bool glShaderProgram::cachedValueUpdate(I32 location, const U16& value) {
    assert(location != -1);

    ShaderVarU16Map::iterator it = _shaderVarsU16.find(location);
    if (it != std::end(_shaderVarsU16)) {
        if (it->second == value) {
            return false;
        } else {
            it->second = value;
        }
    } 

    hashAlg::emplace(_shaderVarsU16, location, value);
    return true;
}

template <>
inline bool glShaderProgram::cachedValueUpdate(I32 location, const U32& value) {
    assert(location != -1);

    ShaderVarU32Map::iterator it = _shaderVarsU32.find(location);
    if (it != std::end(_shaderVarsU32)) {
        if (it->second == value) {
            return false;
        } else {
            it->second = value;
        }
    } 

    hashAlg::emplace(_shaderVarsU32, location, value);
    return true;
}

template <>
inline bool glShaderProgram::cachedValueUpdate(I32 location, const I32& value) {
    assert(location != -1);

    ShaderVarI32Map::iterator it = _shaderVarsI32.find(location);
    if (it != std::end(_shaderVarsI32)) {
        if (it->second == value) {
            return false;
        } else {
            it->second = value;
        }
    } 

    hashAlg::emplace(_shaderVarsI32, location, value);
    return true;
}

template <>
inline bool glShaderProgram::cachedValueUpdate(I32 location, const F32& value) {
    assert(location != -1);

    ShaderVarF32Map::iterator it = _shaderVarsF32.find(location);
    if (it != std::end(_shaderVarsF32)) {
        if (it->second == value) {
            return false;
        } else {
            it->second = value;
        }
    } 

    hashAlg::emplace(_shaderVarsF32, location, value);
    return true;
}

template <>
inline bool glShaderProgram::cachedValueUpdate(I32 location, const vec2<F32>& value) {
    assert(location != -1);

    ShaderVarVec2F32Map::iterator it = _shaderVarsVec2F32.find(location);
    if (it != std::end(_shaderVarsVec2F32)) {
        if (it->second == value) {
            return false;
        } else {
            it->second.set(value);
        }
    } 

    hashAlg::emplace(_shaderVarsVec2F32, location, value);
    return true;
}

template <>
inline bool glShaderProgram::cachedValueUpdate(I32 location, const vec2<I32>& value) {
    assert(location != -1);

    ShaderVarvec2I32Map::iterator it = _shaderVarsVec2I32.find(location);
    if (it != std::end(_shaderVarsVec2I32)) {
        if (it->second == value) {
            return false;
        } else {
            it->second.set(value);
        }
    } 

    hashAlg::emplace(_shaderVarsVec2I32, location, value);
    return true;
}
 
template <>
inline bool glShaderProgram::cachedValueUpdate(I32 location, const vec2<U16>& value) {
    assert(location != -1);

    ShaderVarVec2U16Map::iterator it = _shaderVarsVec2U16.find(location);
    if (it != std::end(_shaderVarsVec2U16)) {
        if (it->second == value) {
            return false;
        } else {
            it->second = value;
        }
    } 

    hashAlg::emplace(_shaderVarsVec2U16, location, value);
    return true;
}
 
template <>
inline bool glShaderProgram::cachedValueUpdate(I32 location, const vec3<F32>& value) {
    assert(location != -1);

    ShaderVarVec3F32Map::iterator it = _shaderVarsVec3F32.find(location);
    if (it != std::end(_shaderVarsVec3F32)) {
        if (it->second == value) {
            return false;
        } else {
            it->second = value;
        }
    } 

    hashAlg::emplace(_shaderVarsVec3F32, location, value);
    return true;
}

template <>
inline bool glShaderProgram::cachedValueUpdate(I32 location, const vec4<F32>& value) {
    assert(location != -1);

    ShaderVarVec4F32Map::iterator it = _shaderVarsVec4F32.find(location);
    if (it != std::end(_shaderVarsVec4F32)) {
        if (it->second == value) {
            return false;
        } else {
            it->second = value;
        }
    } 

    hashAlg::emplace(_shaderVarsVec4F32, location, value);
    return true;
}

template <>
inline bool glShaderProgram::cachedValueUpdate(I32 location, const mat3<F32>& value) {
    assert(location != -1);

    ShaderVarMat3Map::iterator it = _shaderVarsMat3.find(location);
    if (it != std::end(_shaderVarsMat3)) {
        if (it->second == value) {
            return false;
        } else {
            it->second = value;
        }
    } 

    hashAlg::emplace(_shaderVarsMat3, location, value);
    return true;
}

template <>
inline bool glShaderProgram::cachedValueUpdate(I32 location, const mat4<F32>& value) {
    assert(location != -1);

    ShaderVarMat4Map::iterator it = _shaderVarsMat4.find(location);
    if (it != std::end(_shaderVarsMat4)) {
        if (it->second == value) {
            return false;
        } else {
            it->second = value;
        }
    } 

    hashAlg::emplace(_shaderVarsMat4, location, value);
    return true;
}


void glShaderProgram::Uniform(const stringImpl& ext, U32 value) {
    Uniform(cachedLocation(ext), value);
}

void glShaderProgram::Uniform(const stringImpl& ext, I32 value) {
    Uniform(cachedLocation(ext), value);
}

void glShaderProgram::Uniform(const stringImpl& ext, F32 value) {
    Uniform(cachedLocation(ext), value);
}

void glShaderProgram::Uniform(const stringImpl& ext,
                              const vec2<F32>& value) {
    Uniform(cachedLocation(ext), value);
}

void glShaderProgram::Uniform(const stringImpl& ext,
                              const vec2<I32>& value) {
    Uniform(cachedLocation(ext), value);
}

void glShaderProgram::Uniform(const stringImpl& ext,
                              const vec2<U16>& value) {
    Uniform(cachedLocation(ext), value);
}

void glShaderProgram::Uniform(const stringImpl& ext,
                              const vec3<F32>& value) {
    Uniform(cachedLocation(ext), value);
}

void glShaderProgram::Uniform(const stringImpl& ext,
                              const vec4<F32>& value) {
    Uniform(cachedLocation(ext), value);
}

void glShaderProgram::Uniform(const stringImpl& ext,
                              const mat3<F32>& value,
                              bool rowMajor) {
    Uniform(cachedLocation(ext), value, rowMajor);
}

void glShaderProgram::Uniform(const stringImpl& ext,
                              const mat4<F32>& value,
                              bool rowMajor) {
    Uniform(cachedLocation(ext), value, rowMajor);
}

void glShaderProgram::Uniform(const stringImpl& ext,
                              const vectorImpl<I32>& values) {
    Uniform(cachedLocation(ext), values);
}

void glShaderProgram::Uniform(const stringImpl& ext,
                              const vectorImpl<F32>& values) {
    Uniform(cachedLocation(ext), values);
}

void glShaderProgram::Uniform(const stringImpl& ext,
                              const vectorImpl<vec2<F32> >& values) {
    Uniform(cachedLocation(ext), values);
}

void glShaderProgram::Uniform(const stringImpl& ext,
                              const vectorImpl<vec3<F32> >& values) {
    Uniform(cachedLocation(ext), values);
}

void glShaderProgram::Uniform(const stringImpl& ext,
                              const vectorImpl<vec4<F32> >& values) {
    Uniform(cachedLocation(ext), values);
}

void glShaderProgram::Uniform(const stringImpl& ext,
                              const vectorImpl<mat3<F32> >& values,
                              bool rowMajor) {
    Uniform(cachedLocation(ext), values, rowMajor);
}

void glShaderProgram::Uniform(const stringImpl& ext,
                              const vectorImpl<mat4<F32> >& values,
                              bool rowMajor) {
    Uniform(cachedLocation(ext), values, rowMajor);
}

void glShaderProgram::Uniform(const stringImpl& ext, U8 slot) {
    Uniform(cachedLocation(ext), slot);
}

}; //namespace Divide

#endif //_PLATFORM_VIDEO_OPENGL_SHADERS_SHADER_PROGRAM_INL_