#include "xmodex/os/Library.hpp"
#include <windows.h>

FMOD_RESULT FMOD_OS_Library_Load(const char* dllname, FMOD_OS_LIBRARY** handle) {
    if (!dllname || !handle) {
        return FMOD_ERR_INVALID_PARAM;
    }

    *handle = reinterpret_cast<FMOD_OS_LIBRARY*>(LoadLibrary(dllname));
    if (!*handle) {
        auto err = GetLastError();
        return FMOD_ERR_FILE_NOTFOUND;
    }

    return FMOD_OK;
}

FMOD_RESULT FMOD_OS_Library_GetProcAddress(FMOD_OS_LIBRARY* handle, const char* procname, void** address) {
    if (!handle || !address) {
        return FMOD_ERR_INVALID_PARAM;
    }

    *address = reinterpret_cast<void*>(GetProcAddress(reinterpret_cast<HMODULE>(handle), procname));
    if (!*address) {
        return FMOD_ERR_FORMAT;
    }

    return FMOD_OK;
}

FMOD_RESULT FMOD_OS_Library_Free(FMOD_OS_LIBRARY* handle) {
    if (!handle) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (!FreeLibrary(reinterpret_cast<HMODULE>(handle))) {
        return FMOD_ERR_INVALID_PARAM;
    }

    return FMOD_OK;
}
