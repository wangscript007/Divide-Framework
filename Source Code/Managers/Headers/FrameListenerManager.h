/*
   Copyright (c) 2018 DIVIDE-Studio
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

#pragma once
#ifndef _FRAME_LISTENER_MANAGER_H_
#define _FRAME_LISTENER_MANAGER_H_

/// Add this include here so that any FrameListner derived class only needs to
/// include the manager

#include "Core/Headers/Singleton.h"
#include "Rendering/Headers/FrameListener.h"
#include "Platform/Headers/PlatformDefines.h"

namespace Divide {

class FrameListenerManager : public Singleton<FrameListenerManager> {
    friend class Singleton<FrameListenerManager>;

    using EventTimeMap = eastl::fixed_vector<U64, 16, false>;

  public:
    void registerFrameListener(FrameListener* listener, U32 callOrder);
    void removeFrameListener(FrameListener* const listener);
    void idle();

    bool frameEvent(const FrameEvent& evt);

    /// pass the current time in microseconds as the first parameter
    void createEvent(const U64 currentTimeUS, FrameEventType type, FrameEvent& evt);

    /// Calls createEvent and frameEvent
    bool createAndProcessEvent(const U64 currentTimeUS, FrameEventType type, FrameEvent& evt);
  private:
    FrameListenerManager() : Singleton() {}
    ~FrameListenerManager() = default;

    bool frameStarted(const FrameEvent& evt);
    bool framePreRenderStarted(const FrameEvent& evt);
    bool framePreRenderEnded(const FrameEvent& evt);
    bool frameSceneRenderStarted(const FrameEvent& evt);
    bool frameSceneRenderEnded(const FrameEvent& evt);
    bool frameRenderingQueued(const FrameEvent& evt);
    bool framePostRenderStarted(const FrameEvent& evt);
    bool framePostRenderEnded(const FrameEvent& evt);
    bool frameEnded(const FrameEvent& evt);

    /// pass the current time in microseconds as the first parameter
    /// returns the event time in microseconds
    U64 calculateEventTime(const U64 currentTimeUS, FrameEventType type);

   private:
    mutable SharedMutex _listenerLock;
    vectorEASTL<FrameListener*> _listeners;
    std::array<EventTimeMap, to_base(FrameEventType::FRAME_EVENT_ENDED) + 1> _eventTimers;

};

inline void REGISTER_FRAME_LISTENER(FrameListener* listener, U32 callOrder) {
    FrameListenerManager::instance().registerFrameListener(listener,
                                                              callOrder);
}

inline void UNREGISTER_FRAME_LISTENER(FrameListener* const listener) {
    FrameListenerManager::instance().removeFrameListener(listener);
}

};  // namespace Divide

#endif //_FRAME_LISTENER_MANAGER_H_
