#include "Headers/PlatformDefines.h"

#include <SDL_syswm.h>

namespace {
    static LARGE_INTEGER g_time;
}

namespace Divide {
    void getWindowHandle(void* window, SysInfo& info) {
        SDL_SysWMinfo wmInfo;
        SDL_VERSION(&wmInfo.version);
        SDL_GetWindowWMInfo(static_cast<SDL_Window*>(window), &wmInfo);
        info._windowHandle = wmInfo.info.win.window;
    }

    bool CheckMemory(const U32 physicalRAMNeeded, SysInfo& info) {
        MEMORYSTATUSEX status; 
        GlobalMemoryStatusEx(&status);
        info._availableRam = status.ullAvailPhys;
        return info._availableRam > physicalRAMNeeded;
    }

    void getTicksPerSecond(TimeValue& ticksPerSecond) {
        bool queryAvailable = QueryPerformanceFrequency(&g_time) != 0;
        DIVIDE_ASSERT(queryAvailable,
            "Current system does not support 'QueryPerformanceFrequency calls!");
        ticksPerSecond = g_time.QuadPart;
    }

    void getCurrentTime(TimeValue& timeOut) {
        QueryPerformanceCounter(&g_time);
        timeOut = g_time.QuadPart;
    }

}; //namespace Divide
