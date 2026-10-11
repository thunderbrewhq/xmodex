#ifndef XMODEX_OS_THREAD_HPP
#define XMODEX_OS_THREAD_HPP

#include "fmod.hpp"
#include "xmodex/Types.hpp"

enum FMOD_THREAD_PRIORITY {
    FMOD_THREAD_PRIORITY_VERYLOW = -2,
    FMOD_THREAD_PRIORITY_LOW = -1,
    FMOD_THREAD_PRIORITY_NORMAL = 0,
    FMOD_THREAD_PRIORITY_HIGH = 1,
    FMOD_THREAD_PRIORITY_VERYHIGH = 2,
    FMOD_THREAD_PRIORITY_CRITICAL = 3
};

typedef uintptr_t(F_CALLBACK* FMOD_THREAD_CALLBACK)(void* userdata);

FMOD_RESULT FMOD_OS_Thread_GetCurrentID(FMOD_UINT_NATIVE* id);

FMOD_RESULT FMOD_OS_Thread_Destroy(void* handle);

FMOD_RESULT FMOD_OS_Thread_Create(const char* name, FMOD_THREAD_CALLBACK func, void* param, FMOD_THREAD_PRIORITY priority, void* stack, int32_t stacksize, void** handle);

#endif
