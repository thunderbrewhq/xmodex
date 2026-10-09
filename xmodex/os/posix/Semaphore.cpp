#include "xmodex/os/Semaphore.hpp"
#include "fmod.hpp"
#include "xmodex/Memory.hpp"
#include <semaphore.h>

FMOD_RESULT FMOD_OS_Semaphore_Create(FMOD_OS_SEMAPHORE** sema) {
    if (!sema) {
        return FMOD_ERR_INVALID_PARAM;
    }

    auto sem = reinterpret_cast<sem_t*>(FMOD_Memory_Alloc(sizeof(sem_t)));
    if (!sem) {
        return FMOD_ERR_MEMORY;
    }

    if (sem_init(sem, 0, 0) < 0) {
        FMOD_Memory_Free(sem);
        return FMOD_ERR_MEMORY;
    }

    *sema = reinterpret_cast<FMOD_OS_SEMAPHORE*>(sem);

    return FMOD_OK;
}

FMOD_RESULT FMOD_OS_Semaphore_Free(FMOD_OS_SEMAPHORE* sema) {
    sem_destroy(reinterpret_cast<sem_t*>(sema));
    FMOD_Memory_Free(sema);
    return FMOD_OK;
}

FMOD_RESULT FMOD_OS_Semaphore_Signal(FMOD_OS_SEMAPHORE* sema, bool interrupt) {
    if (!sema) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (sem_post(reinterpret_cast<sem_t*>(sema)) < 0) {
        return FMOD_ERR_MEMORY;
    }

    return FMOD_OK;
}

FMOD_RESULT FMOD_OS_Semaphore_Wait(FMOD_OS_SEMAPHORE* sema) {
    if (!sema) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (sem_wait(reinterpret_cast<sem_t*>(sema)) < 0) {
        return FMOD_ERR_MEMORY;
    }

    return FMOD_OK;
}
