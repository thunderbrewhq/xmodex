#include "xmodex/os/Time.hpp"
#include "xmodex/Global.hpp"
#include <sys/time.h>
#include <unistd.h>

FMOD_RESULT FMOD_OS_Time_GetMs(uint32_t* ms) {
    timeval v;
    gettimeofday(&v, nullptr);

    if (!FMOD::gGlobal->gStartTimeSeconds) {
        FMOD::gGlobal->gStartTimeSeconds = v.tv_sec;
    }

    *ms = ((v.tv_sec - FMOD::gGlobal->gStartTimeSeconds) * 1000) + (v.tv_usec / 1000);

    return FMOD_OK;
}

FMOD_RESULT FMOD_OS_Time_GetNs(uint32_t* ns) {
    timeval v;
    gettimeofday(&v, nullptr);

    if (!FMOD::gGlobal->gStartTimeSeconds) {
        FMOD::gGlobal->gStartTimeSeconds = v.tv_sec;
    }

    *ns = ((v.tv_sec - FMOD::gGlobal->gStartTimeSeconds) * 1000000) + v.tv_usec;

    return FMOD_OK;
}

FMOD_RESULT FMOD_OS_Time_Sleep(uint32_t ms) {
    usleep(ms * 1000);

    return FMOD_OK;
}
