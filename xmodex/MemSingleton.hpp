#ifndef XMODEX_MEM_SINGLETON_HPP
#define XMODEX_MEM_SINGLETON_HPP

#include <cstdint>

namespace FMOD {

class MemSingleton {
    private:
        void* mBuffer;
        int32_t mRefCount;

    public:
        MemSingleton()
            : mBuffer(0)
            , mRefCount(0) {}

        ~MemSingleton() {
            this->free();
        }

        void* alloc(int32_t len, const char* file = "", int32_t line = 0);

        void free(const char* file = "", int32_t line = 0);

        void* getData() {
            return this->mBuffer;
        }
};

} // namespace FMOD

#endif
