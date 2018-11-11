#include "stdafx.h"

#include "Headers/ThreadPool.h"
#include "Core/Headers/TaskPool.h"
#include "Platform/Headers/PlatformDefines.h"

namespace Divide {

    ThreadPool::ThreadPool(TaskPool& parent, const U8 threadCount)
        : _parent(parent),
          _isRunning(true)
    {
        _tasksLeft.store(0);

        _threads.reserve(threadCount);
    }

    ThreadPool::~ThreadPool()
    {
        join();
    }

    void ThreadPool::join() {
        if (!_isRunning) {
            return;
        }

        _isRunning = false;

        const vec_size_eastl threadCount = _threads.size();
        for (vec_size_eastl idx = 0; idx < threadCount; ++idx) {
            addTask([] {});
        }

        for (std::thread& thread : _threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    }

    void ThreadPool::wait() {
        if (_isRunning) {
            // Busy wait
            while (_tasksLeft.load() > 0) {
                std::this_thread::yield();
            }
        }
    }

    eastl::vector<std::thread>& ThreadPool::threads() {
        return _threads;
    }

    void ThreadPool::onThreadCreate(const std::thread::id& threadID) {
        _parent.onThreadCreate(threadID);
    }

    BlockingThreadPool::BlockingThreadPool(TaskPool& parent, const U8 threadCount)
        : ThreadPool(parent, threadCount)
    {
        for (U8 idx = 0; idx < threadCount; ++idx) {
            _threads.push_back(std::thread([&]
            {
                onThreadCreate(std::this_thread::get_id());

                while (_isRunning) {
                    PoolTask task;
                    _queue.wait_dequeue(task);
                    task();

                    _tasksLeft.fetch_sub(1);
                }
            }));
        }
    }

    bool BlockingThreadPool::addTask(const PoolTask& job)  {
        if (_queue.enqueue(job)) {
            _tasksLeft.fetch_add(1);
            return true;
        }

        return false;
    }

    LockFreeThreadPool::LockFreeThreadPool(TaskPool& parent, const U8 threadCount)
        : ThreadPool(parent, threadCount)
    {
        for (U8 idx = 0; idx < threadCount; ++idx) {
            _threads.push_back(std::thread([&]
            {
                onThreadCreate(std::this_thread::get_id());

                while (_isRunning) {
                    PoolTask task;
                    while (!_queue.try_dequeue(task)) {
                        std::this_thread::yield();
                    }
                    task();

                    _tasksLeft.fetch_sub(1);
                }
            }));
        }
    }

    bool LockFreeThreadPool::addTask(const PoolTask& job) {
        if (_queue.enqueue(job)) {
            _tasksLeft.fetch_add(1);
            return true;
        }

        return false;
    }
   
};