/*
   Copyright (c) 2016 DIVIDE-Studio
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

#ifndef _DIVIDE_CONFIG_H_
#define _DIVIDE_CONFIG_H_

namespace Divide {
namespace Config {
/// Use OpenGL/OpenGL ES for rendering
const bool USE_OPENGL_RENDERING = true;
/// Select between desktop GL and ES GL
const bool USE_OPENGL_ES = false;
/// if this is false, a variable timestep will be used for the game loop
const bool USE_FIXED_TIMESTEP = true;
/// How many textures to store per material.
/// bump(0) + opacity(1) + spec(2) + tex[3..MAX_TEXTURE_STORAGE - 1]
const unsigned int MAX_TEXTURE_STORAGE = 6;
/// Application desired framerate for physics simulations
const unsigned int TARGET_FRAME_RATE = 60;
/// Minimum required RAM size (in bytes) for the current build
const unsigned int REQUIRED_RAM_SIZE = 2 * 1024 * 1024; //2Gb
/// Application update rate divisor (how many draw calls per render call
/// e.g. 2 = 30Hz update rate at 60Hz rendering)
const unsigned int TICK_DIVISOR = 2;
/// Application update rate
const unsigned int TICKS_PER_SECOND = TARGET_FRAME_RATE / TICK_DIVISOR;
/// Maximum frameskip
const unsigned int MAX_FRAMESKIP = 5;
const unsigned long long SKIP_TICKS = (1000 * 1000) / Config::TICKS_PER_SECOND;
/// The minimum threshold needed for a threaded loop to use sleep
/// Update intervals bellow this threshold will not use sleep!
const unsigned int MIN_SLEEP_THRESHOLD_MS = 5;
/// How many tasks should we keep in a pool to avoid using new/delete (must be power of two)
const unsigned int MAX_POOLED_TASKS = 4096;
/// AI update frequency
const unsigned int AI_THREAD_UPDATE_FREQUENCY = TICKS_PER_SECOND;
/// Toggle multi-threaded resource loading on or off
const bool USE_GPU_THREADED_LOADING = true;
/// Maximum number of instances of a single mesh with a single draw call
const unsigned int MAX_INSTANCE_COUNT = 512;
/// Maximum number of points that can be sent to the GPU per batch
const unsigned int MAX_POINTS_PER_BATCH = static_cast<unsigned int>(1 << 31);
/// Maximum number of bones available per node
const unsigned int MAX_BONE_COUNT_PER_NODE = 256;
/// Estimated maximum number of visible objects per render pass
//(This includes debug primitives)
const unsigned int MAX_VISIBLE_NODES = 1024;
/// How many clip planes should the shaders us
/// How many reflective objects are we allowed to display on screen at the same time
#   if defined(_DEBUG)
const unsigned int MAX_REFLECTIVE_NODES_IN_VIEW = 3;
#   else
const unsigned int  MAX_REFLECTIVE_NODES_IN_VIEW = 5;
#   endif
/// Reflection render target resolution
const unsigned int REFLECTION_TARGET_RESOLUTION = 256;
const unsigned int MAX_CLIP_PLANES = 6;
/// Generic index value used to separate primitives within the same vertex
/// buffer
const unsigned int PRIMITIVE_RESTART_INDEX_L = 0xFFFFFFFF;
const unsigned int PRIMITIVE_RESTART_INDEX_S = 0xFFFF;
/// Terrain LOD configuration
/// Camera distance to the terrain chunk is calculated as follows:
///    vector EyeToChunk = terrainBoundingBoxCenter - EyePos; cameraDistance =
///    EyeToChunk.length();
/// Number of LOD levels for the terrain
const unsigned int TERRAIN_CHUNKS_LOD = 3;
/// How many grass elements (3 quads p.e.) to add to each terrain element
const unsigned int MAX_GRASS_BATCHES = 2000000;
/// SceneNode LOD selection
/// Distance computation is identical to the of the terrain (using SceneNode's
/// bounding box)
const unsigned int SCENE_NODE_LOD = 3;
/// Relative distance for LOD0->LOD1 selection
const unsigned int SCENE_NODE_LOD0 = 100;
/// Relative distance for LOD0->LOD2 selection
const unsigned int SCENE_NODE_LOD1 = 180;
/// Use "precompiled" shaders if possible
const bool USE_SHADER_BINARY = true;
/// Use HW AA'ed lines
const bool USE_HARDWARE_AA_LINES = true;
/// Multi-draw causes some problems with profiling software (e.g.
/// GPUPerfStudio2)
const bool BATCH_DRAW_COMMANDS = false;
/// If true, load shader source coude from cache files
/// If false, materials recompute shader source code from shader atoms
/// If true, clear shader cache to apply changes to shader atom source code
#if defined(_DEBUG)
const bool USE_SHADER_TEXT_CACHE = false;
#else
const bool USE_SHADER_TEXT_CACHE = true;
#endif
/// If true, Hi-Z based occlusion culling is used
const bool USE_HIZ_CULLING = true;
/// If true, Hi-Z culling is disabled and potentially culled nodes are drawn in bright red and double in size
const bool DEBUG_HIZ_CULLING = true;
/// If true, the depth pass acts as a zPrePass for the main draw pass as well
/// If false, the main draw pass will clear the depth buffer and populate a new one instead
const bool USE_Z_PRE_PASS = true;

/// Compute related options
namespace Compute {

}; //namespace Compute

/// Profiling options
namespace Profile {
/// enable function level profiling
#if defined(_DEBUG) || defined(_PROFILE)
const bool ENABLE_FUNCTION_PROFILING = true;
#else
const bool ENABLE_FUNCTION_PROFILING = true;
#endif
/// run performance profiling code
#if defined(_DEBUG) || defined(_PROFILE)
const bool BENCHMARK_PERFORMANCE = true;
#else
const bool BENCHMARK_PERFORMANCE = false;
#endif
/// Benchmark reset frequency in seconds
const unsigned int BENCHMARK_FREQUENCY = 10;
/// use only a basic shader
const bool DISABLE_SHADING = false;
/// skip all draw calls
const bool DISABLE_DRAWS = false;
/// every viewport call is overridden with 1x1 (width x height)
const bool USE_1x1_VIEWPORT = false;
/// how many profiling timers are we allowed to use in our applications
const unsigned int MAX_PROFILE_TIMERS = 2048;
/// textures are capped at 2x2 when uploaded to the GPU
const bool USE_2x2_TEXTURES = false;
/// disable persistently mapped buffers
const bool DISABLE_PERSISTENT_BUFFER = false;
};  // namespace Profile

namespace Assert {
#if defined(_DEBUG)
/// Log assert fails messages to the error log file
const bool LOG_ASSERTS = true;
/// Do not call the platform "assert" function in order to continue application
/// execution
const bool CONTINUE_ON_ASSERT = false;
/// Popup a GUI Message Box on asserts;
const bool SHOW_MESSAGE_BOX = true;
#elif defined(_PROFILE)
const bool LOG_ASSERTS = true;
const bool CONTINUE_ON_ASSERT = false;
const bool SHOW_MESSAGE_BOX = false;
#else  //_RELEASE
const bool LOG_ASSERTS = false;
const bool CONTINUE_ON_ASSERT = false;
const bool SHOW_MESSAGE_BOX = false;
#endif
};  // namespace Assert

namespace Lighting {
// How many lights (in order as passed to the shader for the node) should cast shadows
const unsigned int MAX_SHADOW_CASTING_LIGHTS = 5;
/// Used for CSM or PSSM to determine the maximum number of frustum splits
/// And cube map shadows as well
const unsigned int MAX_SPLITS_PER_LIGHT = 6;
/// How many "units" away should a directional light source be from the camera's
/// position
const unsigned int DIRECTIONAL_LIGHT_DISTANCE = 500;

/// the maximum number of lights supported, this is limited by constant buffer
/// size, commonly this is 64kb, but AMD only seem to allow 2048 lights...
const unsigned int MAX_POSSIBLE_LIGHTS = 1024;

/// The following parameters control the behaviour of the Forward+ renderer
const unsigned int FORWARD_PLUS_TILE_RES = 16;
const unsigned int FORWARD_PLUS_MAX_LIGHTS_PER_TILE = 544;
const unsigned int FORWARD_PLUS_LIGHT_INDEX_BUFFER_SENTINEL = 0x7fffffff;
};  // namespace Lighting
};  // namespace Config
};  // namespace Divide

/// If the target machine uses the nVidia Optimus layout (IntelHD + nVidiadiscreet GPU)
/// or the AMD PowerXPress system, this forces the client to use the high performance GPU
#ifndef FORCE_HIGHPERFORMANCE_GPU
#define FORCE_HIGHPERFORMANCE_GPU
#endif

/// Please enter the desired log file name
#ifndef OUTPUT_LOG_FILE
#define OUTPUT_LOG_FILE "console.log"
#endif  // OUTPUT_LOG_FILE

#ifndef ERROR_LOG_FILE
#define ERROR_LOG_FILE "errors.log"
#endif  // ERROR_LOG_FILE

#ifndef SERVER_LOG_FILE
#define SERVER_LOG_FILE "server.log"
#endif  // SERVER_LOG_FILE

#define BOOST_IMP 0
#define STL_IMP 1
#define EASTL_IMP 2

/// Specify the string implementation to use
#ifndef STRING_IMP
#define STRING_IMP STL_IMP
#endif  // STRING_IMP

/// Specify the vector implementation to use
#ifndef VECTOR_IMP
#define VECTOR_IMP STL_IMP
#endif  // VECTOR_IMP

/// Specify the hash maps / unordered maps implementation to use
#ifndef HASH_MAP_IMP
#define HASH_MAP_IMP STL_IMP
#endif  // HASH_MAP_IMP

#ifndef GPU_VALIDATION_IN_RELEASE_BUILD
//#define GPU_VALIDATION_IN_RELEASE_BUILD
#endif

#ifndef GPU_VALIDATION_IN_PROFILE_BUILD
//#define GPU_VALIDATION_IN_PROFILE_BUILD
#endif

#ifndef GPU_VALIDATION_IN_DEBUG_BUILD
#define GPU_VALIDATION_IN_DEBUG_BUILD
#endif

#ifndef ENABLE_GPU_VALIDATION
    #if defined(_RELEASE) && defined(GPU_VALIDATION_IN_RELEASE_BUILD)
    #define ENABLE_GPU_VALIDATION
    #endif

    #if defined(_PROFILE) && defined(GPU_VALIDATION_IN_PROFILE_BUILD)
    #define ENABLE_GPU_VALIDATION
    #endif

    #if defined(_DEBUG) && defined(GPU_VALIDATION_IN_DEBUG_BUILD)
    #define ENABLE_GPU_VALIDATION
    #endif
#endif

#endif  //_DIVIDE_CONFIG_H_
