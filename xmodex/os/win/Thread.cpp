#include "xmodex/os/Thread.hpp"
#include "xmodex/os/Library.hpp"
#include <windows.h>

FMOD_RESULT FMOD_OS_Thread_GetCurrentID(FMOD_UINT_NATIVE* id) {
    *id = static_cast<FMOD_UINT_NATIVE>(GetCurrentThreadId());
    return FMOD_OK;
}

FMOD_RESULT FMOD_OS_Thread_Destroy(void* handle) {
    CloseHandle(handle);
    return FMOD_OK;
}

FMOD_RESULT FMOD_OS_Thread_Create(const char* name, FMOD_THREAD_CALLBACK func, void* param, FMOD_THREAD_PRIORITY priority, void* stack, int32_t stacksize, void** handle) {
    DWORD threadid;
    FMOD_RESULT result;

    if (!handle) {
        return FMOD_ERR_INVALID_PARAM;
    }

    auto thread = CreateThread(
        nullptr,
        0,
        reinterpret_cast<LPTHREAD_START_ROUTINE>(func),
        param,
        0,
        &threadid);
    *handle = thread;

    switch (priority) {
    case FMOD_THREAD_PRIORITY_NORMAL: {
        SetThreadPriority(thread, THREAD_PRIORITY_NORMAL);
        break;
    }
    case FMOD_THREAD_PRIORITY_HIGH: {
        SetThreadPriority(thread, THREAD_PRIORITY_ABOVE_NORMAL);
        break;
    }
    case FMOD_THREAD_PRIORITY_VERYHIGH: {
        SetThreadPriority(thread, THREAD_PRIORITY_HIGHEST);
        break;
    }
    case FMOD_THREAD_PRIORITY_CRITICAL: {
        SetThreadPriority(thread, THREAD_PRIORITY_TIME_CRITICAL);

        HANDLE(WINAPI * SetThreadTask)
        (LPCTSTR, LPDWORD);
        FMOD_OS_LIBRARY* avrtHandle = nullptr;
        DWORD taskIndex = 0;
        result = FMOD_OS_Library_Load("avrt.dll", &avrtHandle);
        if (result == FMOD_OK) {
            result = FMOD_OS_Library_GetProcAddress(
                avrtHandle,
                "AvSetMmThreadCharacteristicsA",
                reinterpret_cast<void**>(&SetThreadTask));
            if (result != FMOD_OK) {
                return result;
            }
            SetThreadTask("Pro Audio", &taskIndex);
            result = FMOD_OS_Library_Free(avrtHandle);
            if (result != FMOD_OK) {
                return result;
            }
        }
        break;
    }
    case FMOD_THREAD_PRIORITY_VERYLOW: {
        SetThreadPriority(thread, THREAD_PRIORITY_LOWEST);
        break;
    }
    case FMOD_THREAD_PRIORITY_LOW: {
        SetThreadPriority(thread, THREAD_PRIORITY_BELOW_NORMAL);
        break;
    }
    }
    return FMOD_OK;
}
