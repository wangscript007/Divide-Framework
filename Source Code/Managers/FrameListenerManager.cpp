#include "stdafx.h"

#include "Headers/FrameListenerManager.h"

#include "Core/Headers/Console.h"
#include "Core/Headers/StringHelper.h"
#include "Core/Math/Headers/MathHelper.h"

#include "Utility/Headers/Localization.h"

namespace Divide {

FrameListenerManager::FrameListenerManager()
    : Singleton()
{
}


/// Register a new Frame Listener to be processed every frame
void FrameListenerManager::registerFrameListener(FrameListener* listener,
                                                 U32 callOrder) {
    assert(listener != nullptr);

    // Check if the listener has a name or we should assign an id
    if (listener->getListenerName().empty()) {
        listener->name(Util::StringFormat("generic_f_listener_%d", listener->getGUID()).c_str());
    }

    listener->setCallOrder(callOrder);

    UniqueLockShared w_lock(_listenerLock);
    insert_sorted(_listeners, listener, eastl::less<>());
}

/// Remove an existent Frame Listener from our collection
void FrameListenerManager::removeFrameListener(FrameListener* const listener) {
    assert(listener != nullptr);

    I64 targetGUID = listener->getGUID();

    UniqueLockShared lock(_listenerLock);
    vectorEASTL<FrameListener*>::const_iterator it;
    it = eastl::find_if(eastl::cbegin(_listeners), eastl::cend(_listeners),
                      [targetGUID](FrameListener const* fl) -> bool
                      {
                        return fl->getGUID() == targetGUID;
                      });

    if (it != eastl::cend(_listeners)) {
        _listeners.erase(it);
    } else {
        Console::errorfn(Locale::get(_ID("ERROR_FRAME_LISTENER_REMOVE")), listener->getListenerName().c_str());
    }
}

/// For each listener, notify of current event and check results
/// If any Listener returns false, the whole manager returns false for this specific step
/// If the manager returns false at any step, the application exists
bool FrameListenerManager::frameEvent(const FrameEvent& evt) {
    switch (evt._type) {
        case FrameEventType::FRAME_EVENT_STARTED     : return frameStarted(evt);
        case FrameEventType::FRAME_PRERENDER_START   : return framePreRenderStarted(evt);
        case FrameEventType::FRAME_PRERENDER_END     : return framePreRenderEnded(evt);
        case FrameEventType::FRAME_SCENERENDER_START : return frameSceneRenderStarted(evt);
        case FrameEventType::FRAME_SCENERENDER_END   : return frameSceneRenderEnded(evt);
        case FrameEventType::FRAME_POSTRENDER_START  : return framePostRenderStarted(evt);
        case FrameEventType::FRAME_POSTRENDER_END    : return framePostRenderEnded(evt);
        case FrameEventType::FRAME_EVENT_PROCESS     : return frameRenderingQueued(evt);
        case FrameEventType::FRAME_EVENT_ENDED       : return frameEnded(evt);
    };

    return false;
}

bool FrameListenerManager::frameStarted(const FrameEvent& evt) {
    SharedLock r_lock(_listenerLock);
    for (FrameListener* listener : _listeners) {
        if (!listener->frameStarted(evt)) {
            return false;
        }
    }
    return true;
}

bool FrameListenerManager::framePreRenderStarted(const FrameEvent& evt) {
    SharedLock r_lock(_listenerLock);
    for (FrameListener* listener : _listeners) {
        if (!listener->framePreRenderStarted(evt)) {
            return false;
        }
    }
    return true;
}

bool FrameListenerManager::framePreRenderEnded(const FrameEvent& evt) {
    SharedLock r_lock(_listenerLock);
    for (FrameListener* listener : _listeners) {
        if (!listener->framePreRenderEnded(evt)) {
            return false;
        }
    }
    return true;
}

bool FrameListenerManager::frameSceneRenderStarted(const FrameEvent& evt) {
    SharedLock r_lock(_listenerLock);
    for (FrameListener* listener : _listeners) {
        if (!listener->frameSceneRenderStarted(evt)) {
            return false;
        }
    }
    return true;
}

bool FrameListenerManager::frameSceneRenderEnded(const FrameEvent& evt) {
    SharedLock r_lock(_listenerLock);
    for (FrameListener* listener : _listeners) {
        if (!listener->frameSceneRenderEnded(evt)) {
            return false;
        }
    }
    return true;
}

bool FrameListenerManager::frameRenderingQueued(const FrameEvent& evt) {
    SharedLock r_lock(_listenerLock);
    for (FrameListener* listener : _listeners) {
        if (!listener->frameRenderingQueued(evt)) {
            return false;
        }
    }
    return true;
}

bool FrameListenerManager::framePostRenderStarted(const FrameEvent& evt) {
    SharedLock r_lock(_listenerLock);
    for (FrameListener* listener : _listeners) {
        if (!listener->framePostRenderStarted(evt)) {
            return false;
        }
    }
    return true;
}

bool FrameListenerManager::framePostRenderEnded(const FrameEvent& evt) {
    SharedLock r_lock(_listenerLock);
    for (FrameListener* listener : _listeners) {
        if (!listener->framePostRenderEnded(evt)) {
            return false;
        }
    }
    return true;
}

bool FrameListenerManager::frameEnded(const FrameEvent& evt) {
    SharedLock r_lock(_listenerLock);
    for (FrameListener* listener : _listeners) {
        if (!listener->frameEnded(evt)) {
            return false;
        }
    }
    return true;
}

/// When the application is idle, we should really clear up old events
void FrameListenerManager::idle() {
}

/// Please see the Ogre3D documentation about this
void FrameListenerManager::createEvent(const U64 currentTimeUS, FrameEventType type, FrameEvent& evt) {
    evt._currentTimeUS = currentTimeUS;
    evt._timeSinceLastEventUS = calculateEventTime(evt._currentTimeUS, FrameEventType::FRAME_EVENT_ANY);
    evt._timeSinceLastFrameUS = calculateEventTime(evt._currentTimeUS, type);
    evt._type = type;
}

bool FrameListenerManager::createAndProcessEvent(const U64 currentTimeUS, FrameEventType type, FrameEvent& evt) {
    createEvent(currentTimeUS, type, evt);
    return frameEvent(evt);
}

U64 FrameListenerManager::calculateEventTime(const U64 currentTimeUS, FrameEventType type) {
    EventTimeMap& times = _eventTimers[to_U32(type)];
    times.push_back(currentTimeUS);

    if (times.size() == 1) {
        return 0;
    }

    EventTimeMap::const_iterator it = eastl::cbegin(times);
    EventTimeMap::const_iterator end = eastl::cend(times) - 2;

    while (it != end) {
        if (currentTimeUS - *it > 0) {
            ++it;
        } else {
            break;
        }
    }

    times.erase(eastl::cbegin(times), it);
    return (times.back() - times.front());
}
};