/* Copyright (c) 2017 DIVIDE-Studio
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

#ifndef _PLATFORM_VIDEO_OPENGLS_PROGRAM_INL_
#define _PLATFORM_VIDEO_OPENGLS_PROGRAM_INL_

#include "Core/Headers/Console.h"

namespace Divide {
    namespace {

        bool compare(GFX::PushConstantType type, const vectorImpl<AnyParam>& lhsVec, const vectorImpl<AnyParam>& rhsVec) {
            for (vectorAlg::vecSize i = 0; i < lhsVec.size(); ++i) {
                const AnyParam& lhs = lhsVec[i];
                const AnyParam& rhs = rhsVec[i];

                switch (type) {
                    case GFX::PushConstantType::BOOL:  return lhs.constant_cast<bool>() == rhs.constant_cast<bool>();
                    case GFX::PushConstantType::INT:   return lhs.constant_cast<I32>() == rhs.constant_cast<I32>();
                    case GFX::PushConstantType::UINT:  return lhs.constant_cast<U32>() == rhs.constant_cast<U32>();
                    case GFX::PushConstantType::DOUBLE:return lhs.constant_cast<D64>() == rhs.constant_cast<D64>();
                    case GFX::PushConstantType::FLOAT: return lhs.constant_cast<F32>() == rhs.constant_cast<F32>();
                    case GFX::PushConstantType::IVEC2: return lhs.constant_cast<vec2<I32>>() == rhs.constant_cast<vec2<I32>>();
                    case GFX::PushConstantType::IVEC3: return lhs.constant_cast<vec3<I32>>() == rhs.constant_cast<vec3<I32>>();
                    case GFX::PushConstantType::IVEC4: return lhs.constant_cast<vec4<I32>>() == rhs.constant_cast<vec4<I32>>();
                    case GFX::PushConstantType::UVEC2: return lhs.constant_cast<vec2<U32>>() == rhs.constant_cast<vec2<U32>>();
                    case GFX::PushConstantType::UVEC3: return lhs.constant_cast<vec3<U32>>() == rhs.constant_cast<vec3<U32>>();
                    case GFX::PushConstantType::UVEC4: return lhs.constant_cast<vec4<U32>>() == rhs.constant_cast<vec4<U32>>();
                    case GFX::PushConstantType::DVEC2: return lhs.constant_cast<vec2<D64>>() == rhs.constant_cast<vec2<D64>>();
                    case GFX::PushConstantType::VEC2:  return lhs.constant_cast<vec2<F32>>() == rhs.constant_cast<vec2<F32>>();
                    case GFX::PushConstantType::DVEC3: return lhs.constant_cast<vec3<D64>>() == rhs.constant_cast<vec3<D64>>();
                    case GFX::PushConstantType::VEC3:  return lhs.constant_cast<vec3<F32>>() == rhs.constant_cast<vec3<F32>>();
                    case GFX::PushConstantType::DVEC4: return lhs.constant_cast<vec4<D64>>() == rhs.constant_cast<vec4<D64>>();
                    case GFX::PushConstantType::VEC4:  return lhs.constant_cast<vec4<F32>>() == rhs.constant_cast<vec4<F32>>();
                    case GFX::PushConstantType::IMAT2: return lhs.constant_cast<mat2<I32>>() == rhs.constant_cast<mat2<I32>>();
                    case GFX::PushConstantType::IMAT3: return lhs.constant_cast<mat3<I32>>() == rhs.constant_cast<mat3<I32>>();
                    case GFX::PushConstantType::IMAT4: return lhs.constant_cast<mat4<I32>>() == rhs.constant_cast<mat4<I32>>();
                    case GFX::PushConstantType::UMAT2: return lhs.constant_cast<mat2<U32>>() == rhs.constant_cast<mat2<U32>>();
                    case GFX::PushConstantType::UMAT3: return lhs.constant_cast<mat3<U32>>() == rhs.constant_cast<mat3<U32>>();
                    case GFX::PushConstantType::UMAT4: return lhs.constant_cast<mat4<U32>>() == rhs.constant_cast<mat4<U32>>();
                    case GFX::PushConstantType::MAT2:  return lhs.constant_cast<mat2<F32>>() == rhs.constant_cast<mat2<F32>>();
                    case GFX::PushConstantType::MAT3:  return lhs.constant_cast<mat3<F32>>() == rhs.constant_cast<mat3<F32>>();
                    case GFX::PushConstantType::MAT4:  return lhs.constant_cast<mat4<F32>>() == rhs.constant_cast<mat4<F32>>();
                    case GFX::PushConstantType::DMAT2: return lhs.constant_cast<mat2<D64>>() == rhs.constant_cast<mat2<D64>>();
                    case GFX::PushConstantType::DMAT3: return lhs.constant_cast<mat3<D64>>() == rhs.constant_cast<mat3<D64>>();
                    case GFX::PushConstantType::DMAT4: return lhs.constant_cast<mat4<D64>>() == rhs.constant_cast<mat4<D64>>();
                };
            }

            return false;
        }

        template<typename T_in>
        typename std::enable_if<!std::is_same<T_in, bool>::value, void>::type
        convert(const vectorImpl<AnyParam>& valuesIn, vectorImpl<T_in>& valuesOut) {
            valuesOut.reserve(valuesIn.size());

            std::transform(std::cbegin(valuesIn), std::cend(valuesIn), std::begin(valuesOut), [](const AnyParam& val) {
                return val.constant_cast<T_in>();
            });
        }

        template<typename T_in>
        typename std::enable_if<std::is_same<T_in, bool>::value, void>::type
        convert(const vectorImpl<AnyParam>& valuesIn, vectorImpl<T_in>& valuesOut) {
            valuesOut.reserve(valuesIn.size());

            std::transform(std::cbegin(valuesIn), std::cend(valuesIn), std::begin(valuesOut), [](const AnyParam& val) {
                return val.constant_cast<bool>() ? 1 : 0;
            });
        }

        //ToDo: REALLY SLOW! Find a faster way of handling this! -Ionut
        template<typename T_out, size_t T_out_count, typename T_in>
        struct castData {
            castData(const vectorImpl<AnyParam>& values)
                : _convertedData(values.size()),
                  _values(values)
            {
                convert(_values, _convertedData);
            }

            T_out* operator()() {
                return reinterpret_cast<T_out*>(_convertedData.data());
            }

            using vectorType = typename std::conditional<std::is_same<T_in, bool>::value, I32, T_in>::type;
            vectorImpl<vectorType> _convertedData;

            const vectorImpl<AnyParam>& _values;
        };
    };

    inline bool glShaderProgram::comparePushConstants(const GFX::PushConstant& lhs, const GFX::PushConstant& rhs) const {
        if (lhs._type == rhs._type) {
            if (lhs._binding == rhs._binding) {
                if (lhs._flag == rhs._flag) {
                    return compare(lhs._type, lhs._values, rhs._values);
                }
            }
        }

        return false;
    }

    void glShaderProgram::Uniform(I32 binding, GFX::PushConstantType type, const vectorImpl<AnyParam>& values, bool flag) const {
        GLsizei count = (GLsizei)values.size();
        switch (type) {
            case GFX::PushConstantType::BOOL:
                glProgramUniform1iv(_shaderProgramID, binding, count, castData<GLint, 1, bool>(values)());
                break;
            case GFX::PushConstantType::INT:
                glProgramUniform1iv(_shaderProgramID, binding, count, castData<GLint, 1, I32>(values)());
                break;
            case GFX::PushConstantType::UINT:
                glProgramUniform1uiv(_shaderProgramID, binding, count, castData<GLuint, 1, U32>(values)());
                break;
            case GFX::PushConstantType::DOUBLE:           // Warning! Downcasting to float -Ionut
                glProgramUniform1fv(_shaderProgramID, binding, count, castData<GLfloat, 1, D64>(values)());
                break;
            case GFX::PushConstantType::FLOAT:
                glProgramUniform1fv(_shaderProgramID, binding, count, castData<GLfloat, 1, F32>(values)());
                break;
            case GFX::PushConstantType::IVEC2:
                glProgramUniform2iv(_shaderProgramID, binding, count, castData<GLint, 2, vec2<I32>>(values)());
                break;
            case GFX::PushConstantType::IVEC3:
                glProgramUniform3iv(_shaderProgramID, binding, count, castData<GLint, 3, vec3<I32>>(values)());
                break;
            case GFX::PushConstantType::IVEC4:
                glProgramUniform4iv(_shaderProgramID, binding, count, castData<GLint, 4, vec4<I32>>(values)());
                break;
            case GFX::PushConstantType::UVEC2:
                glProgramUniform2uiv(_shaderProgramID, binding, count, castData<GLuint, 2, vec2<U32>>(values)());
                break;
            case GFX::PushConstantType::UVEC3:
                glProgramUniform3uiv(_shaderProgramID, binding, count, castData<GLuint, 3, vec3<U32>>(values)());
                break;
            case GFX::PushConstantType::UVEC4:
                glProgramUniform4uiv(_shaderProgramID, binding, count, castData<GLuint, 4, vec4<U32>>(values)());
                break;
            case GFX::PushConstantType::DVEC2:            // Warning! Downcasting to float -Ionut
                glProgramUniform2fv(_shaderProgramID, binding, count, castData<GLfloat, 2, vec2<D64>>(values)());
                break;
            case GFX::PushConstantType::VEC2:
                glProgramUniform2fv(_shaderProgramID, binding, count, castData<GLfloat, 2, vec2<F32>>(values)());
                break;
            case GFX::PushConstantType::DVEC3:            // Warning! Downcasting to float -Ionut
                glProgramUniform3fv(_shaderProgramID, binding, count, castData<GLfloat, 3, vec3<D64>>(values)());
                break;
            case GFX::PushConstantType::VEC3:
                glProgramUniform3fv(_shaderProgramID, binding, count, castData<GLfloat, 3, vec3<F32>>(values)());
                break;
            case GFX::PushConstantType::DVEC4:            // Warning! Downcasting to float -Ionut
                glProgramUniform4fv(_shaderProgramID, binding, count, castData<GLfloat, 4, vec4<D64>>(values)());
                break;
            case GFX::PushConstantType::VEC4:
                glProgramUniform4fv(_shaderProgramID, binding, count, castData<GLfloat, 4, vec4<F32>>(values)());
                break;
            case GFX::PushConstantType::IMAT2:
                glProgramUniformMatrix2fv(_shaderProgramID, binding, count, flag ? GL_TRUE : GL_FALSE, castData<GLfloat, 4, mat2<I32>>(values)());
                break;
            case GFX::PushConstantType::IMAT3:
                glProgramUniformMatrix3fv(_shaderProgramID, binding, count, flag ? GL_TRUE : GL_FALSE, castData<GLfloat, 9 , mat3<I32>>(values)());
                break;
            case GFX::PushConstantType::IMAT4:
                glProgramUniformMatrix4fv(_shaderProgramID, binding, count, flag ? GL_TRUE : GL_FALSE, castData<GLfloat, 16, mat4<I32>>(values)());
                break;
            case GFX::PushConstantType::UMAT2:
                glProgramUniformMatrix2fv(_shaderProgramID, binding, count, flag ? GL_TRUE : GL_FALSE, castData<GLfloat, 4, mat2<U32>>(values)());
                break;
            case GFX::PushConstantType::UMAT3:
                glProgramUniformMatrix3fv(_shaderProgramID, binding, count, flag ? GL_TRUE : GL_FALSE, castData<GLfloat, 9, mat3<U32>>(values)());
                break;
            case GFX::PushConstantType::UMAT4:
                glProgramUniformMatrix4fv(_shaderProgramID, binding, count, flag ? GL_TRUE : GL_FALSE, castData<GLfloat, 16, mat4<U32>>(values)());
                break;
            case GFX::PushConstantType::MAT2:
                glProgramUniformMatrix2fv(_shaderProgramID, binding, count, flag ? GL_TRUE : GL_FALSE, castData<GLfloat, 4, mat2<F32>>(values)());
                break;
            case GFX::PushConstantType::MAT3:
                glProgramUniformMatrix3fv(_shaderProgramID, binding, count, flag ? GL_TRUE : GL_FALSE, castData<GLfloat, 9, mat3<F32>>(values)());
                break;
            case GFX::PushConstantType::MAT4: {
                glProgramUniformMatrix4fv(_shaderProgramID, binding, count, flag ? GL_TRUE : GL_FALSE, castData<GLfloat, 16, mat4<F32>>(values)());
            }break;
            case GFX::PushConstantType::DMAT2:
                glProgramUniformMatrix2fv(_shaderProgramID, binding, count, flag ? GL_TRUE : GL_FALSE, castData<GLfloat, 4, mat2<D64>>(values)());
                break;
            case GFX::PushConstantType::DMAT3:
                glProgramUniformMatrix3fv(_shaderProgramID, binding, count, flag ? GL_TRUE : GL_FALSE, castData<GLfloat, 9, mat3<D64>>(values)());
                break;
            case GFX::PushConstantType::DMAT4:
                glProgramUniformMatrix4fv(_shaderProgramID, binding, count, flag ? GL_TRUE : GL_FALSE, castData<GLfloat, 16, mat4<D64>>(values)());
                break;
            default:
                DIVIDE_ASSERT(false, "glShaderProgram::Uniform error: Unhandled data type!");
        }
    }

    inline void glShaderProgram::UploadPushConstant(const GFX::PushConstant& constant) {
        I32 binding = cachedValueUpdate(constant);

        if (binding != -1) {
            Uniform(binding, constant._type, constant._values, constant._flag);
        }
    }

    inline void glShaderProgram::UploadPushConstants(const PushConstants& constants) {
        for (auto& constant : constants.data()) {
            if (!constant.second->_binding.empty() && constant.second->_type != GFX::PushConstantType::COUNT) {
                UploadPushConstant(*constant.second);
            }
        }
    }
}; //namespace Divide

#endif //_PLATFORM_VIDEO_OPENGLS_PROGRAM_INL_
