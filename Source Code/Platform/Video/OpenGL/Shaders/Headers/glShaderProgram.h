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

#ifndef _PLATFORM_VIDEO_OPENGL_SHADERS_SHADER_PROGRAM_H_
#define _PLATFORM_VIDEO_OPENGL_SHADERS_SHADER_PROGRAM_H_

#include "Platform/Video/OpenGL/Headers/glResources.h"
#include "Platform/Video/Shaders/Headers/ShaderProgram.h"

namespace Divide {

/// OpenGL implementation of the Shader entity
class glShaderProgram : public ShaderProgram {
   public:
    glShaderProgram(const bool optimise = false);
    ~glShaderProgram();

    /// Make sure this program is ready for deletion
    inline bool unload() {
        unbind();
        return ShaderProgram::unload();
    }
    /// Bind this shader program
    bool bind();
    /// Unbinding this program, unless forced, just clears the _bound flag
    void unbind(bool resetActiveProgram = true);
    /// Check every possible combination of flags to make sure this program can
    /// be used for rendering
    bool isValid() const;
    /// Called once per frame. Used to update internal state
    bool update(const U64 deltaTime);
    /// Add a new shader stage to this program
    void attachShader(Shader* const shader, const bool refresh = false);
    /// Remove a shader stage from this program
    void detachShader(Shader* const shader);
    /// Register a shader buffer to be used every time the shader is
    /// used to render. Makes sure the buffer is bound to the proper location
    void registerShaderBuffer(ShaderBuffer& buffer);
    /// This is used to set all of the subroutine indices for the specified
    /// shader stage for this program
    void SetSubroutines(ShaderType type, const vectorImpl<U32>& indices) const;
    /// This works exactly like SetSubroutines, but for a single index.
    void SetSubroutine(ShaderType type, U32 index) const;
    /// Get the index of the specified subroutine name for the specified stage.
    /// Not cached!
    U32 GetSubroutineIndex(ShaderType type, const stringImpl& name) const;
    /// Get the uniform location of the specified subroutine uniform for the
    /// specified stage. Not cached!
    U32 GetSubroutineUniformLocation(ShaderType type,
                                     const stringImpl& name) const;
    U32 GetSubroutineUniformCount(ShaderType type) const;
    /// Set an uniform value
    inline void Uniform(const stringImpl& ext, U8 slot);
    inline void Uniform(const stringImpl& ext, U32 value);
    inline void Uniform(const stringImpl& ext, I32 value);
    inline void Uniform(const stringImpl& ext, F32 value);
    inline void Uniform(const stringImpl& ext, const vec2<F32>& value);
    inline void Uniform(const stringImpl& ext, const vec2<I32>& value);
    inline void Uniform(const stringImpl& ext, const vec2<U16>& value);
    inline void Uniform(const stringImpl& ext, const vec3<F32>& value);
    inline void Uniform(const stringImpl& ext, const vec4<F32>& value);
    inline void Uniform(const stringImpl& ext,
                        const mat3<F32>& value,
                        bool rowMajor = false);
    inline void Uniform(const stringImpl& ext,
                        const mat4<F32>& value,
                        bool rowMajor = false);
    inline void Uniform(const stringImpl& ext, const vectorImpl<I32>& values);
    inline void Uniform(const stringImpl& ext, const vectorImpl<F32>& values);
    inline void Uniform(const stringImpl& ext,
                        const vectorImpl<vec2<F32>>& values);
    inline void Uniform(const stringImpl& ext,
                        const vectorImpl<vec3<F32>>& values);
    inline void Uniform(const stringImpl& ext,
                        const vectorImpl<vec4<F32>>& values);
    inline void Uniform(const stringImpl& ext,
                        const vectorImpl<mat3<F32>>& values,
                        bool rowMajor = false);
    inline void Uniform(const stringImpl& ext,
                        const vectorImpl<mat4<F32>>& values,
                        bool rowMajor = false);

    void Uniform(I32 location, U8 slot);
    void Uniform(I32 location, U32 value);
    void Uniform(I32 location, I32 value);
    void Uniform(I32 location, F32 value);
    void Uniform(I32 location, const vec2<F32>& value);
    void Uniform(I32 location, const vec2<I32>& value);
    void Uniform(I32 location, const vec2<U16>& value);
    void Uniform(I32 location, const vec3<F32>& value);
    void Uniform(I32 location, const vec4<F32>& value);
    void Uniform(I32 location, const mat3<F32>& value, bool rowMajor = false);
    void Uniform(I32 location, const mat4<F32>& value, bool rowMajor = false);
    void Uniform(I32 location, const vectorImpl<I32>& values);
    void Uniform(I32 location, const vectorImpl<F32>& values);
    void Uniform(I32 location, const vectorImpl<vec2<F32>>& values);
    void Uniform(I32 location, const vectorImpl<vec3<F32>>& values);
    void Uniform(I32 location, const vectorImpl<vec4<F32>>& values);
    void Uniform(I32 location,
                 const vectorImpl<mat3<F32>>& values,
                 bool rowMajor = false);
    void Uniform(I32 location,
                 const vectorImpl<mat4<F32>>& values,
                 bool rowMajor = false);

   protected:
    /// Creation of a new shader program. Pass in a shader token and use glsw to
    /// load the corresponding effects
    bool generateHWResource(const stringImpl& name);
    /// Linking a shader program also sets up all pre-link properties for the
    /// shader (varying locations, attrib bindings, etc)
    void link();
    /// This should be called in the loading thread, but some issues are still
    /// present, and it's not recommended (yet)
    void threadedLoad(const stringImpl& name);
    /// Cache uniform/attribute locations for shader programs
    GLint cachedLocation(const stringImpl& name);
    template <typename T>
    inline bool cachedValueUpdate(I32 location, const T& value) {
        static_assert(
            false,
            "glShaderProgram::cachedValue error: unsupported data type!");
    }
    /// Basic OpenGL shader program validation (both in debug and in release)
    void validateInternal();
    /// Retrieve the program's validation log if we need it
    stringImpl getLog() const;
    /// Prevent binding multiple textures to the same slot.
    bool checkSlotUsage(GLint location, GLushort slot);

   private:
    typedef hashMapImpl<stringImpl, I32> ShaderVarMap;
    typedef hashMapImpl<I32, U8> ShaderVarU8Map;
    typedef hashMapImpl<I32, U16> ShaderVarU16Map;
    typedef hashMapImpl<I32, U32> ShaderVarU32Map;
    typedef hashMapImpl<I32, I32> ShaderVarI32Map;
    typedef hashMapImpl<I32, F32> ShaderVarF32Map;
    typedef hashMapImpl<I32, vec2<F32>> ShaderVarVec2F32Map;
    typedef hashMapImpl<I32, vec2<I32>> ShaderVarvec2I32Map;
    typedef hashMapImpl<I32, vec2<U16>> ShaderVarVec2U16Map;
    typedef hashMapImpl<I32, vec3<F32>> ShaderVarVec3F32Map;
    typedef hashMapImpl<I32, vec4<F32>> ShaderVarVec4F32Map;
    typedef hashMapImpl<I32, mat3<F32>> ShaderVarMat3Map;
    typedef hashMapImpl<I32, mat4<F32>> ShaderVarMat4Map;

    ShaderVarU8Map _shaderVarsU8;
    ShaderVarU16Map _shaderVarsU16;
    ShaderVarU32Map _shaderVarsU32;
    ShaderVarI32Map _shaderVarsI32;
    ShaderVarF32Map _shaderVarsF32;
    ShaderVarVec2F32Map _shaderVarsVec2F32;
    ShaderVarvec2I32Map _shaderVarsVec2I32;
    ShaderVarVec2U16Map _shaderVarsVec2U16;
    ShaderVarVec3F32Map _shaderVarsVec3F32;
    ShaderVarVec4F32Map _shaderVarsVec4F32;
    ShaderVarMat3Map _shaderVarsMat3;
    ShaderVarMat4Map _shaderVarsMat4;

    ShaderVarMap _shaderVarLocation;
    std::atomic_bool _validationQueued;
    GLenum _binaryFormat;
    bool _validated;
    bool _loadedFromBinary;
    Shader* _shaderStage[to_const_uint(ShaderType::COUNT)];
    GLenum _shaderStageTable[to_const_uint(ShaderType::COUNT)];
    GLuint _shaderProgramIDTemp;
    static stringImpl _lastPathPrefix;
    static stringImpl _lastPathSuffix;
};

};  // namespace Divide

#endif  //_PLATFORM_VIDEO_OPENGL_SHADERS_SHADER_PROGRAM_H_

#include "glShaderProgram.inl"