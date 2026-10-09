#ifndef XMODEX_OS_FILE_HPP
#define XMODEX_OS_FILE_HPP

#include "fmod.hpp"

FMOD_RESULT FMOD_OS_File_Cancel(void* handle);

FMOD_RESULT FMOD_OS_File_Close(void* handle);

FMOD_RESULT FMOD_OS_File_Open(const char* name, const char* mode, int32_t unicode, uint32_t* filesize, void** handle);

FMOD_RESULT FMOD_OS_File_Read(void* handle, void* buf, uint32_t count, uint32_t* read);

FMOD_RESULT FMOD_OS_File_Seek(void* handle, uint32_t offset);

#endif
