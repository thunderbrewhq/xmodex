#ifndef XMODEX_MEMORY_HPP
#define XMODEX_MEMORY_HPP

#include "xmodex/Global.hpp"
#include <new>

#define FMOD_MEMORY_DEFAULTBLOCKSIZE 64

#define FMOD_Memory_Alloc(_len) FMOD::gGlobal->gSystemPool->alloc((_len), __FILE__, __LINE__, 0, false)
#define FMOD_Memory_Calloc(_len) FMOD::gGlobal->gSystemPool->calloc((_len), __FILE__, __LINE__, 0)
#define FMOD_Memory_ReAlloc(_ptr, _len) FMOD::gGlobal->gSystemPool->realloc((_ptr), _len, __FILE__, __LINE__, 0)
#define FMOD_Memory_Free(_ptr) FMOD::gGlobal->gSystemPool->free((_ptr), __FILE__, __LINE__, 0)

#define FMOD_Memory_AllocType(_len, _type) FMOD::gGlobal->gSystemPool->alloc((_len), __FILE__, __LINE__, _type, false)
#define FMOD_Memory_CallocType(_len, _type) FMOD::gGlobal->gSystemPool->calloc((_len), __FILE__, __LINE__, _type)
#define FMOD_Memory_ReAllocType(_ptr, _len, _type) FMOD::gGlobal->gSystemPool->realloc((_ptr), _len, __FILE__, __LINE__, _type)
#define FMOD_Memory_FreeType(_ptr, _type) FMOD::gGlobal->gSystemPool->free((_ptr), __FILE__, __LINE__, _type)

template <typename T>
inline T* FMOD_Object_Alloc_(const char* file, int32_t line) {
    auto mem = FMOD::gGlobal->gSystemPool->alloc(sizeof(T), file, line, 0, false);
    if (!mem) {
        return nullptr;
    }
    return new (mem) T();
}

template <typename T>
inline T* FMOD_Object_Calloc_(const char* file, int32_t line) {
    auto mem = FMOD::gGlobal->gSystemPool->calloc(sizeof(T), file, line, 0);
    if (!mem) {
        return nullptr;
    }
    return new (mem) T();
}

template <typename T>
inline T* FMOD_Object_AllocSize_(int32_t len, const char* file, int32_t line) {
    if (len < sizeof(T)) {
        len = sizeof(T);
    }

    auto mem = FMOD::gGlobal->gSystemPool->alloc(len, file, line, 0, false);
    if (!mem) {
        return nullptr;
    }
    return new (mem) T();
}

template <typename T>
inline T* FMOD_Object_CallocSize_(int32_t len, const char* file, int32_t line) {
    if (len < sizeof(T)) {
        len = sizeof(T);
    }

    auto mem = FMOD::gGlobal->gSystemPool->calloc(len, file, line, 0);
    if (!mem) {
        return nullptr;
    }
    return new (mem) T();
}

#define FMOD_Object_Alloc(_type) FMOD_Object_Alloc_<_type>(__FILE__, __LINE__)
#define FMOD_Object_Calloc(_type) FMOD_Object_Calloc_<_type>(__FILE__, __LINE__)

#define FMOD_Object_AllocSize(_type, _len) FMOD_Object_AllocSize_<_type>(_len, __FILE__, __LINE__)
#define FMOD_Object_CallocSize(_type, _len) FMOD_Object_CallocSize_<_type>(_len, __FILE__, __LINE__)

namespace FMOD {

void* F_CALLBACK Memory_DefaultMalloc(int32_t size, FMOD_MEMORY_TYPE type);

void* F_CALLBACK Memory_DefaultRealloc(void* data, int32_t size, FMOD_MEMORY_TYPE type);

void F_CALLBACK Memory_DefaultFree(void* ptr, FMOD_MEMORY_TYPE type);

} // namespace FMOD

#endif
