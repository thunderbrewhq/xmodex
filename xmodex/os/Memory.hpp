#ifndef XMODEX_OS_MEMORY_HPP
#define XMODEX_OS_MEMORY_HPP

#include <cstdint>

void* FMOD_OS_Memory_Alloc(int32_t size);

void* FMOD_OS_Memory_Realloc(void* ptr, int32_t size);

void FMOD_OS_Memory_Free(void* ptr);

#endif
