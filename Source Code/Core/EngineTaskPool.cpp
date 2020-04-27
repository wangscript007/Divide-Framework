#include "stdafx.h"

#include "Headers/EngineTaskPool.h"

#include "Headers/Kernel.h"
#include "Headers/PlatformContext.h"

namespace Divide {

void WaitForAllTasks(PlatformContext& context, bool yield, bool flushCallbacks, bool foceClear) {
    WaitForAllTasks(context.taskPool(TaskPoolType::HIGH_PRIORITY), yield, flushCallbacks, foceClear);
}

}; //namespace Divide