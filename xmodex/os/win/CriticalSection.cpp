#include "xmodex/os/CriticalSection.hpp"
#include "xmodex/Memory.hpp"
#include <windows.h>

FMOD_OS_CRITICALSECTION* gResolveCrit;
CRITICAL_SECTION gMemoryCrit;

FMOD_RESULT FMOD_OS_CriticalSection_Create(FMOD_OS_CRITICALSECTION** crit, bool memorycrit) {
    if (!crit) {
        return FMOD_ERR_INVALID_PARAM;
    }

    CRITICAL_SECTION* mutex;

    if (memorycrit) {
        mutex = &gMemoryCrit;
    } else {
        mutex = reinterpret_cast<CRITICAL_SECTION*>(FMOD_Memory_Alloc(sizeof(CRITICAL_SECTION)));
        if (!mutex) {
            return FMOD_ERR_MEMORY;
        }
    }

    *crit = reinterpret_cast<FMOD_OS_CRITICALSECTION*>(mutex);
    InitializeCriticalSection(mutex);
    return FMOD_OK;
}

FMOD_RESULT FMOD_OS_CriticalSection_Free(FMOD_OS_CRITICALSECTION* crit) {
    if (!crit) {
        return FMOD_ERR_INVALID_PARAM;
    }

    DeleteCriticalSection(reinterpret_cast<CRITICAL_SECTION*>(crit));
    if (reinterpret_cast<CRITICAL_SECTION*>(crit) != &gMemoryCrit) {
        FMOD_Memory_Free(crit);
    }
    return FMOD_OK;
}

FMOD_RESULT FMOD_OS_CriticalSection_Enter(FMOD_OS_CRITICALSECTION* crit) {
    if (!crit) {
        return FMOD_ERR_INVALID_PARAM;
    }

    EnterCriticalSection(reinterpret_cast<CRITICAL_SECTION*>(crit));

    return FMOD_OK;
}

FMOD_RESULT FMOD_OS_CriticalSection_Leave(FMOD_OS_CRITICALSECTION* crit) {
    if (!crit) {
        return FMOD_ERR_INVALID_PARAM;
    }

    LeaveCriticalSection(reinterpret_cast<CRITICAL_SECTION*>(crit));

    return FMOD_OK;
}
