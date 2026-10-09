#include "xmodex/os/Thread.hpp"
#include "fmod.hpp"
#include <pthread.h>
#include <sched.h>

FMOD_RESULT FMOD_OS_Thread_GetCurrentID(FMOD_UINT_NATIVE* id) {
    *id = reinterpret_cast<FMOD_UINT_NATIVE>(pthread_self());
    return FMOD_OK;
}

FMOD_RESULT FMOD_OS_Thread_Destroy(void* handle) {
    pthread_detach(reinterpret_cast<pthread_t>(handle));
    return FMOD_OK;
}

FMOD_RESULT FMOD_OS_Thread_Create(const char* name, FMOD_THREAD_CALLBACK func, void* param, FMOD_THREAD_PRIORITY priority, void* stack, int32_t stacksize, void** handle) {
    pthread_t thread;
    sched_param sp;
    int policy;

    if (!handle) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (pthread_create(&thread, nullptr, reinterpret_cast<void* (*)(void*)>(func), param)) {
        return FMOD_ERR_MEMORY;
    }

    policy = SCHED_OTHER;

    switch (priority) {
    case FMOD_THREAD_PRIORITY_HIGH:
        sp.sched_priority = 90;
        policy = SCHED_FIFO;
        break;
    case FMOD_THREAD_PRIORITY_VERYHIGH:
        sp.sched_priority = 94;
        policy = SCHED_FIFO;
        break;
    case FMOD_THREAD_PRIORITY_CRITICAL:
        sp.sched_priority = 99;
        policy = SCHED_FIFO;
        break;
    case FMOD_THREAD_PRIORITY_VERYLOW:
    case FMOD_THREAD_PRIORITY_LOW:
    case FMOD_THREAD_PRIORITY_NORMAL:
        sp.sched_priority = 0;
        policy = SCHED_OTHER;
        break;
    }

    pthread_setschedparam(thread, policy, &sp);

    *handle = reinterpret_cast<void*>(thread);
    return FMOD_OK;
}
