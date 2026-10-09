#ifndef XMODEX_MEM_POOL_HPP
#define XMODEX_MEM_POOL_HPP

#include "fmod.hpp"
#include "xmodex/Types.hpp"
#include "xmodex/os/CriticalSection.hpp"
#include <cstdint>

#define FMOD_MEMORY_MAXTHREADS 32

namespace FMOD {

struct MemBlockHeader {
        int32_t mSize;
        int32_t mNumBlocks;
        int32_t mBlockOffset;
        uint32_t mThreadID;
};

class MemPool {
        friend class LocalMemoryCounter;

    private:
        uint8_t* mBitmap;
        uint8_t* mData;
        bool mCustomPool;
        int32_t mSizeBytes;
        int32_t mSizeBlocks;
        int32_t mNumBlocks;
        int32_t mMaxBlocks;
        uint32_t mCurrentAllocated[33];
        uint32_t mCurrentAllocatedThreadID[32];
        uint32_t mMaxAllocated;
        int32_t mActualMaxBytes;
        int32_t mWastage;
        int32_t mFirstFreeBlock;
        FMOD_MEMORY_ALLOCCALLBACK mAlloc;
        FMOD_MEMORY_REALLOCCALLBACK mRealloc;
        FMOD_MEMORY_FREECALLBACK mFree;
        FMOD_OS_CRITICALSECTION* mCrit;
        int32_t mBlockSize;

    private:
        int32_t findFreeBlocks(int32_t offset, int32_t to, int32_t numblocks);

    public:
        MemPool();
        FMOD_RESULT init(void* poolmem, int32_t poolsize, int32_t blocksize);
        void close();
        void* alloc(int32_t len, const char* file, int32_t line, FMOD_MEMORY_TYPE type, bool clear);
        void* calloc(int32_t len, const char* file, int32_t line, FMOD_MEMORY_TYPE type);
        void* realloc(void* ptr, int32_t len, const char* file, int32_t line, FMOD_MEMORY_TYPE type);
        void free(void* ptr, const char* file, int32_t line, FMOD_MEMORY_TYPE type);
        void clearThreadID(FMOD_UINT_NATIVE id);
        uint32_t getCurrentThreadID(FMOD_UINT_NATIVE threadid);
        void set(int32_t blockoffset, int32_t value, int32_t numblocks);
        FMOD_RESULT setCallbacks(FMOD_MEMORY_ALLOCCALLBACK useralloc, FMOD_MEMORY_REALLOCCALLBACK userrealloc, FMOD_MEMORY_FREECALLBACK userfree);
};

} // namespace FMOD

#endif
