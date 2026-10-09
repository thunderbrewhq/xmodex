#ifndef XMODEX_OS_LIBRARY_HPP
#define XMODEX_OS_LIBRARY_HPP

#include "fmod.hpp"

struct FMOD_OS_LIBRARY;

FMOD_RESULT FMOD_OS_Library_Load(const char* dllname, FMOD_OS_LIBRARY** handle);

FMOD_RESULT FMOD_OS_Library_GetProcAddress(FMOD_OS_LIBRARY* handle, const char* procname, void** address);

FMOD_RESULT FMOD_OS_Library_Free(FMOD_OS_LIBRARY* handle);

#endif
