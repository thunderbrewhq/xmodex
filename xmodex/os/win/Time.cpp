#include "xmodex/os/Time.hpp"
// clang-format off
#include <windows.h>
#include <timeapi.h>
// clang-format on

FMOD_RESULT FMOD_OS_Time_GetMs(uint32_t* ms) {
    *ms = timeGetTime();
    return FMOD_OK;
}

FMOD_RESULT FMOD_OS_Time_GetNs(uint32_t* ns) {
    LARGE_INTEGER val;
    LARGE_INTEGER freq;

    if (QueryPerformanceCounter(&val)) {
        QueryPerformanceFrequency(&freq);
        val.QuadPart *= 1000;
        val.QuadPart *= 1000;
        val.QuadPart /= freq.QuadPart;

        *ns = val.LowPart;
    } else {
        *ns = timeGetTime() * 1000;
    }

    return FMOD_OK;
}

FMOD_RESULT FMOD_OS_Time_Sleep(uint32_t ms) {
    Sleep(ms);
    return FMOD_OK;
}
