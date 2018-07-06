#include "Headers/NavMeshContext.h"

#include "core.h"

namespace Navigation {
    void rcContextDivide::doLog(const rcLogCategory category, const char* msg, const I32 len){
        switch(category){
            default:
            case RC_LOG_PROGRESS:
                PRINT_FN(Locale::get("RECAST_CONTEXT_LOG_PROGRESS"),msg);
                break;
            case RC_LOG_WARNING:
                PRINT_FN(Locale::get("RECAST_CONTEXT_LOG_WARNING"),msg);
                break;
            case RC_LOG_ERROR:
                ERROR_FN(Locale::get("RECAST_CONTEXT_LOG_ERROR"),msg);
                break;
        }
    }

    void rcContextDivide::doResetTimers() {
        for (I32 i = 0; i < RC_MAX_TIMERS; ++i)
            _accTime[i] = -1;
    }

    void rcContextDivide::doStartTimer(const rcTimerLabel label){
        _startTime[label] = GETMSTIME();
    }

    void rcContextDivide::doStopTimer(const rcTimerLabel label){
        const D32 endTime = GETMSTIME();
        const D32 deltaTime = (D32)(endTime - _startTime[label]);
        if (_accTime[label] == -1)
            _accTime[label] = deltaTime;
        else
            _accTime[label] += deltaTime;
    }

    I32 rcContextDivide::doGetAccumulatedTime(const rcTimerLabel label) const {
        return _accTime[label];
    }
};