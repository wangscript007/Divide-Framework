#include "Headers/glMemoryManager.h"
#include "Platform/Video/OpenGL/Headers/GLWrapper.h"

#include "Core/Headers/Console.h"

namespace Divide {
namespace GLUtil {

bufferPtr allocPersistentBuffer(GLuint bufferId,
                                GLsizeiptr bufferSize,
                                BufferStorageMask storageMask,
                                BufferAccessMask accessMask,
                                const bufferPtr data) {
    glNamedBufferStorage(bufferId, bufferSize, data, storageMask);
    bufferPtr ptr = glMapNamedBufferRange(bufferId, 0, bufferSize, accessMask);
    assert(ptr != NULL);
    return ptr;
}

bufferPtr createAndAllocPersistentBuffer(GLsizeiptr bufferSize,
                                         BufferStorageMask storageMask,
                                         BufferAccessMask accessMask,
                                         GLuint& bufferIdOut,
                                         bufferPtr const data) {
    glCreateBuffers(1, &bufferIdOut);
    DIVIDE_ASSERT(bufferIdOut != 0,
                  "GLUtil::allocPersistentBuffer error: buffer creation failed");

    return allocPersistentBuffer(bufferIdOut, bufferSize, storageMask, accessMask,
                                 data);
}

void createAndAllocBuffer(GLsizeiptr bufferSize,
                          GLenum usageMask,
                          GLuint& bufferIdOut,
                          const bufferPtr data) {
    glCreateBuffers(1, &bufferIdOut);
    DIVIDE_ASSERT(bufferIdOut != 0, "GLUtil::allocBuffer error: buffer creation failed");
    glNamedBufferData(bufferIdOut, bufferSize, data, usageMask);
}

void freeBuffer(GLuint& bufferId, bufferPtr mappedPtr) {
    if (bufferId > 0) {
        if (mappedPtr != nullptr) {
            GLboolean result = glUnmapNamedBuffer(bufferId);
            DIVIDE_ASSERT(result != GL_FALSE, "GLUtil::freeBuffer error: buffer unmaping failed");
            mappedPtr = nullptr;
        }
        glDeleteBuffers(1, &bufferId);
        bufferId = 0;
    }
}

};  // namespace GLUtil
};  // namespace Divide