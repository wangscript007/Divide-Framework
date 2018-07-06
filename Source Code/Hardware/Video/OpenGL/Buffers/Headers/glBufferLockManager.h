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

#ifndef _GL_BUFFER_LOCK_MANAGER_H
#define _GL_BUFFER_LOCK_MANAGER_H

#include "Hardware/Video/OpenGL/Headers/GLWrapper.h"
//https://github.com/nvMcJohn/apitest
// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------
struct BufferRange
{
    size_t mStartOffset;
    size_t mLength;

    bool Overlaps(const BufferRange& _rhs) const {
        return mStartOffset < (_rhs.mStartOffset + _rhs.mLength)
            && _rhs.mStartOffset < (mStartOffset + mLength);
    }
};

// --------------------------------------------------------------------------------------------------------------------
struct BufferLock
{
    BufferRange mRange;
    GLsync mSyncObj;
};

// --------------------------------------------------------------------------------------------------------------------
class glBufferLockManager
{
public:
    glBufferLockManager(bool _cpuUpdates);
    ~glBufferLockManager();

    void WaitForLockedRange(size_t _lockBeginBytes, size_t _lockLength);
    void LockRange(size_t _lockBeginBytes, size_t _lockLength);

private:
    void wait(GLsync* _syncObj);
    void cleanup(BufferLock* _bufferLock);

    vectorImpl<BufferLock> mBufferLocks;

    // Whether it's the CPU (true) that updates, or the GPU (false)
    bool mCPUUpdates;
};

#endif