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

#ifndef _CORE_TIME_PROFILE_TIMER_INL_
#define _CORE_TIME_PROFILE_TIMER_INL_

namespace Divide {
namespace Time {

inline U64 ProfileTimer::get() const {
    return _timerAverage / std::max(_timerCounter, 1u);
}

inline const stringImpl& ProfileTimer::name() const {
    return _name;
}

inline ProfileTimer& ADD_TIMER(const char* timerName) {
    return ProfileTimer::getNewTimer(timerName);
}

inline void START_TIMER(ProfileTimer& timer) {
    timer.start();
}

inline void STOP_TIMER(ProfileTimer& timer) {
    timer.stop();
}

inline void PRINT_TIMER(ProfileTimer& timer) {
    timer.print();
}

inline void REMOVE_TIMER(ProfileTimer*& timer) { 
    ProfileTimer::removeTimer(*timer);
}

};  // namespace Time
};  // namespace Divide

#endif  //_CORE_TIME_PROFILE_TIMER_INL_