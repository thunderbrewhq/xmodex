#include "xmodex/os/CriticalSection.hpp"
#include "xmodex/Memory.hpp"
#include <pthread.h>

pthread_mutex_t gMemoryCrit;

FMOD_RESULT FMOD_OS_CriticalSection_Create(FMOD_OS_CRITICALSECTION** crit, bool memorycrit) {
    pthread_mutex_t* mutex;
    pthread_mutexattr_t mutexattr;

    if (!crit) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (memorycrit) {
        mutex = &gMemoryCrit;
    } else {
        mutex = reinterpret_cast<pthread_mutex_t*>(FMOD_Memory_Alloc(sizeof(pthread_mutex_t)));
        if (!mutex) {
            return FMOD_ERR_MEMORY;
        }
    }

    pthread_mutexattr_init(&mutexattr);
    if (pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE) == 0) {
        if (pthread_mutex_init(mutex, &mutexattr) == 0) {
            *crit = reinterpret_cast<FMOD_OS_CRITICALSECTION*>(mutex);
            return FMOD_OK;
        }
    }

    if (memorycrit) {
        return FMOD_ERR_MEMORY;
    }

    FMOD_Memory_Free(mutex);
    return FMOD_ERR_MEMORY;
}

FMOD_RESULT FMOD_OS_CriticalSection_Free(FMOD_OS_CRITICALSECTION* crit) {
    if (!crit) {
        return FMOD_ERR_INVALID_PARAM;
    }

    auto mutex = reinterpret_cast<pthread_mutex_t*>(crit);
    pthread_mutex_destroy(mutex);
    if (mutex != &gMemoryCrit) {
        FMOD_Memory_Free(crit);
    }

    return FMOD_OK;
}

FMOD_RESULT FMOD_OS_CriticalSection_Enter(FMOD_OS_CRITICALSECTION* crit) {
    if (!crit) {
        return FMOD_ERR_INVALID_PARAM;
    }

    return pthread_mutex_lock(reinterpret_cast<pthread_mutex_t*>(crit)) == 0 ? FMOD_OK : FMOD_ERR_MEMORY;
}

FMOD_RESULT FMOD_OS_CriticalSection_Leave(FMOD_OS_CRITICALSECTION* crit) {
    if (!crit) {
        return FMOD_ERR_INVALID_PARAM;
    }

    return pthread_mutex_unlock(reinterpret_cast<pthread_mutex_t*>(crit)) == 0 ? FMOD_OK : FMOD_ERR_MEMORY;
}
