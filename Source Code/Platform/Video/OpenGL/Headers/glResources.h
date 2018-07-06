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

#ifndef _GL_RESOURCES_H_
#define _GL_RESOURCES_H_

//#define _GLDEBUG_IN_RELEASE

#include "config.h"

#include "Platform/DataTypes/Headers/PlatformDefines.h"
#ifndef GLBINDING_STATIC
#define GLBINDING_STATIC
#endif

#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif

//#define GL_VERSION_4_5

#ifdef GL_VERSION_4_5
#include <glbinding/gl/gl45.h>
using namespace gl45;
#else
#include <glbinding/gl/gl44.h>
using namespace gl44;
#endif

#include <glbinding/Binding.h>
#include <GL/glfw3.h>
#include "Platform/Video/Headers/RenderAPIWrapper.h"

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4505)  ///<unreferenced local function removal
#pragma warning(disable : 4100)  ///<unreferenced formal parameter
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#//pragma GCC diagnostic ignored "-Wall"
#endif

namespace NS_GLIM {
    enum class GLIM_ENUM : int;
}; //namespace NS_GLIM

namespace Divide {
namespace GLUtil {

typedef void* bufferPtr;

/// Wrapper for glGetIntegerv
GLint getIntegerv(GLenum param);
/// This function is called when the window's close button is pressed
void glfw_close_callback(GLFWwindow* window);
/// This function is called when the window loses focus
void glfw_focus_callback(GLFWwindow* window, I32 focusState);
/// This function is called if GLFW throws an error
void glfw_error_callback(GLint error, const char* description);
/// Check the current operation for errors
void APIENTRY 
DebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
              GLsizei length, const char* message, const void* userParam);
/// Half float conversion from:
/// http://www.opengl.org/discussion_boards/archive/index.php/t-154530.html [thx
/// gking]
/// Half-float to float
static GLfloat htof(GLhalf val);
/// Float to half-float
static GLhalf ftoh(GLfloat val);
/// Pack a float value in an unsigned int
// static GLuint ftopacked(GLfloat val);
/// Invalid object value. Used to compare handles and determine if they were
/// properly created
extern GLuint _invalidObjectID;
/// Main rendering window
extern GLFWwindow* _mainWindow;
/// Background thread for loading resources
extern GLFWwindow* _loaderWindow;
/// GFXDevice enums to OpenGL enum translation tables
namespace GL_ENUM_TABLE {
/// Populate enumeration tables with appropriate API values
void fill();

extern GLenum
    glBlendTable[to_const_uint(BlendProperty::COUNT)];
extern GLenum glBlendOpTable[to_const_uint(
    BlendOperation::COUNT)];
extern GLenum glCompareFuncTable[to_const_uint(
    ComparisonFunction::COUNT)];
extern GLenum glStencilOpTable[to_const_uint(
    StencilOperation::COUNT)];
extern GLenum
    glCullModeTable[to_const_uint(CullMode::COUNT)];
extern GLenum
    glFillModeTable[to_const_uint(FillMode::COUNT)];
extern GLenum glTextureTypeTable[to_const_uint(
    TextureType::COUNT)];
extern GLenum glImageFormatTable[to_const_uint(
    GFXImageFormat::COUNT)];
extern GLenum glPrimitiveTypeTable[to_const_uint(
    PrimitiveType::COUNT)];
extern GLenum glDataFormat[to_const_uint(GFXDataFormat::COUNT)];
extern GLenum
    glWrapTable[to_const_uint(TextureWrap::COUNT)];
extern GLenum glTextureFilterTable[to_const_uint(
    TextureFilter::COUNT)];
extern NS_GLIM::GLIM_ENUM glimPrimitiveType[to_const_uint(
    PrimitiveType::COUNT)];

};  // namespace GL_ENUM_TABLE
};  // namespace GLUtil
};  // namespace Divide

/* ARB_vertex_type_2_10_10_10_rev */
#define P10(f) ((I32)((f)*0x1FF))
#define UP10(f) ((I32)((f)*0x3FF))
#define PN2(f)                                                          \
    ((I32)((f) < 0.333 ? 3 /* = -0.333 */ : (f) < 1 ? 0   /* = 0.333 */ \
                                                    : 1)) /* normalized */
#define P2(f) ((I32)(f))                                  /* unnormalized */
#define UP2(f) ((I32)((f)*0x3))
#define P1010102(x, y, z, w) \
    (P10(x) | (P10(y) << 10) | (P10(z) << 20) | (P2(w) << 30))
#define PN1010102(x, y, z, w) \
    (P10(x) | (P10(y) << 10) | (P10(z) << 20) | (PN2(w) << 30))
#define UP1010102(x, y, z, w) \
    (UP10(x) | (UP10(y) << 10) | (UP10(z) << 20) | (UP2(w) << 30))

#if defined(_MSC_VER)
#pragma warning(pop)
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#endif