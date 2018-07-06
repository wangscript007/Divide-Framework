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

#ifndef _TASK_POOL_H_
#define _TASK_POOL_H_

#include "Platform/Threading/Headers/Task.h"
#include <boost/lockfree/queue.hpp>

namespace Divide {

class TaskPool {
  public:
    TaskPool();
    ~TaskPool();

    bool init();
    void flushCallbackQueue();

    TaskHandle getTaskHandle(I64 taskGUID);
    Task& getAvailableTask();
    void setTaskCallback(const TaskHandle& handle,
                         const DELEGATE_CBK<>& callback);

  private:
    //ToDo: replace all friend class declarations with attorneys -Ionut;
    friend class Task;
    void taskCompleted(I64 onExitTaskID);
    inline ThreadPool& threadPool() {
        return _mainTaskPool;
    }
  private:
    ThreadPool _mainTaskPool;

    typedef hashMapImpl<I64, DELEGATE_CBK<> > CallbackFunctions;
    CallbackFunctions _taskCallbacks;

    typedef hashMapImpl<I64, bool > TaskState;
    TaskState _taskStates;

    boost::lockfree::queue<I64> _threadedCallbackBuffer;

    std::array<Task, Config::MAX_POOLED_TASKS> _tasksPool;
    std::atomic<U32> _allocatedJobs;
};
};

#endif _TASK_POOL_H_