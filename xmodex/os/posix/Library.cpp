#include "xmodex/os/Library.hpp"
#include "fmod.hpp"
#include <dlfcn.h>

FMOD_RESULT FMOD_OS_Library_Load(const char* dllname, FMOD_OS_LIBRARY** handle) {
    if (!dllname || !handle) {
        return FMOD_ERR_INVALID_PARAM;
    }

    *handle = reinterpret_cast<FMOD_OS_LIBRARY*>(dlopen(dllname, RTLD_LAZY));
    if (!*handle) {
        return FMOD_ERR_FILE_NOTFOUND;
    }
    return FMOD_OK;
}

FMOD_RESULT FMOD_OS_Library_GetProcAddress(FMOD_OS_LIBRARY* handle, const char* procname, void** address) {
    *address = dlsym(handle, procname);
    if (!*address) {
        return FMOD_ERR_INVALID_PARAM;
    }

    return FMOD_OK;
}

FMOD_RESULT FMOD_OS_Library_Free(FMOD_OS_LIBRARY* handle) {
    if (!handle) {
        return FMOD_ERR_INVALID_PARAM;
    }

    dlclose(handle);
    return FMOD_OK;
}
