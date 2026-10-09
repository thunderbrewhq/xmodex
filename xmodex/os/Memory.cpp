#include "xmodex/os/Memory.hpp"
#include <cstdlib>

void* FMOD_OS_Memory_Alloc(int32_t size) {
    return malloc(size);
}

void* FMOD_OS_Memory_Realloc(void* ptr, int32_t size) {
    return realloc(ptr, size);
}

void FMOD_OS_Memory_Free(void* ptr) {
    free(ptr);
}
