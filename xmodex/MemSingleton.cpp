#include "xmodex/MemSingleton.hpp"
#include "fmod.hpp"
#include "xmodex/Global.hpp"

namespace FMOD {

void* MemSingleton::alloc(int32_t len, const char* file, int32_t line) {
    if (!this->mRefCount) {
        this->mBuffer = FMOD::gGlobal->gSystemPool->alloc(len, file, line, FMOD_MEMORY_NORMAL, false);
    }

    this->mRefCount++;
    return this->mBuffer;
}

void MemSingleton::free(const char* file, int32_t line) {
    if (this->mRefCount) {
        this->mRefCount--;
    }
    if (!this->mRefCount && this->mBuffer) {
        FMOD::gGlobal->gSystemPool->free(this->mBuffer, file, line, FMOD_MEMORY_NORMAL);
        this->mBuffer = nullptr;
    }
    return;
}

} // namespace FMOD
