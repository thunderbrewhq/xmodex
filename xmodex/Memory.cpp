#include "xmodex/Memory.hpp"
#include "fmod.hpp"
#include "xmodex/Global.hpp"
#include "xmodex/SystemI.hpp"
#include "xmodex/os/Memory.hpp"

namespace FMOD {

void* F_CALLBACK Memory_DefaultMalloc(int32_t size, FMOD_MEMORY_TYPE type) {
    return FMOD_OS_Memory_Alloc(size);
}

void* F_CALLBACK Memory_DefaultRealloc(void* data, int32_t size, FMOD_MEMORY_TYPE type) {
    return FMOD_OS_Memory_Realloc(data, size);
}

void F_CALLBACK Memory_DefaultFree(void* ptr, FMOD_MEMORY_TYPE type) {
    FMOD_OS_Memory_Free(ptr);
}

} // namespace FMOD

FMOD_RESULT F_API FMOD_Memory_Initialize(void* poolmem, int32_t poollen, FMOD_MEMORY_ALLOCCALLBACK useralloc, FMOD_MEMORY_REALLOCCALLBACK userrealloc, FMOD_MEMORY_FREECALLBACK userfree) {
    if (!FMOD::gGlobal->gSystemHead->isEmpty()) {
        return FMOD_ERR_INITIALIZED;
    }

    if (poollen % FMOD_MEMORY_DEFAULTBLOCKSIZE) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (poollen && poolmem) {
        FMOD_RESULT result;

        if (useralloc || userrealloc || userfree) {
            return FMOD_ERR_INVALID_PARAM;
        }
        if (poollen < FMOD_MEMORY_DEFAULTBLOCKSIZE) {
            return FMOD_ERR_INVALID_PARAM;
        }

        result = FMOD::gGlobal->gSystemPool->init(poolmem, poollen, FMOD_MEMORY_DEFAULTBLOCKSIZE);
        if (result == FMOD_OK) {
            FMOD::gGlobal->gSystemPool->setCallbacks(nullptr, nullptr, nullptr);
        }

        return result;
    } else {
        if (poolmem || poollen) {
            return FMOD_ERR_INVALID_PARAM;
        }

        if (useralloc && userrealloc && userfree) {
            FMOD::gGlobal->gSystemPool->setCallbacks(useralloc, userrealloc, userfree);
        } else if (!useralloc && !userrealloc && !userfree) {
            FMOD::gGlobal->gSystemPool->setCallbacks(FMOD::Memory_DefaultMalloc, FMOD::Memory_DefaultRealloc, FMOD::Memory_DefaultFree);
        } else {
            return FMOD_ERR_INVALID_PARAM;
        }
    }

    return FMOD_OK;
}

extern "C" {

void* FMOD_Memory_allocC(int32_t len, const char* file, const int32_t line) {
    return FMOD::gGlobal->gSystemPool->alloc(len, file, line, FMOD_MEMORY_NORMAL, false);
}

void* FMOD_Memory_callocC(int32_t len, const char* file, const int32_t line) {
    return FMOD::gGlobal->gSystemPool->calloc(len, file, line, FMOD_MEMORY_NORMAL);
}

void* FMOD_Memory_reallocC(void* ptr, int32_t len, const char* file, const int32_t line) {
    return FMOD::gGlobal->gSystemPool->realloc(ptr, len, file, line, FMOD_MEMORY_NORMAL);
}

void FMOD_Memory_freeC(void* ptr, const char* file, const int32_t line) {
    return FMOD::gGlobal->gSystemPool->free(ptr, file, line, FMOD_MEMORY_NORMAL);
}
}
