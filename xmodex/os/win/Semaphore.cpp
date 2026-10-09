#include "xmodex/os/Semaphore.hpp"
#include "xmodex/os/win/MeteredSection.hpp"
#include <windows.h>

FMOD_RESULT FMOD_OS_Semaphore_Create(FMOD_OS_SEMAPHORE** sema) {
    if (!sema) {
        return FMOD_ERR_INVALID_PARAM;
    }

    *sema = reinterpret_cast<FMOD_OS_SEMAPHORE*>(CreateMeteredSection(0, 0xFFFF, nullptr));
    if (!*sema) {
        return FMOD_ERR_MEMORY;
    }

    return FMOD_OK;
}

FMOD_RESULT FMOD_OS_Semaphore_Free(FMOD_OS_SEMAPHORE* sema) {
    if (!sema) {
        return FMOD_ERR_INVALID_PARAM;
    }

    CloseMeteredSection(reinterpret_cast<LPMETERED_SECTION>(sema));

    return FMOD_OK;
}

FMOD_RESULT FMOD_OS_Semaphore_Signal(FMOD_OS_SEMAPHORE* sema, bool interrupt) {
    if (!sema) {
        return FMOD_ERR_INVALID_PARAM;
    }

    LeaveMeteredSection(reinterpret_cast<LPMETERED_SECTION>(sema), 1, nullptr);

    return FMOD_OK;
}

FMOD_RESULT FMOD_OS_Semaphore_Wait(FMOD_OS_SEMAPHORE* sema) {
    if (!sema) {
        return FMOD_ERR_INVALID_PARAM;
    }

    EnterMeteredSection(reinterpret_cast<LPMETERED_SECTION>(sema), INFINITE);
    return FMOD_OK;
}
