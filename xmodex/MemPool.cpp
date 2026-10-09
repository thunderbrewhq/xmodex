#include "xmodex/MemPool.hpp"
#include "xmodex/Math.hpp"
#include "xmodex/Memory.hpp"
#include "xmodex/String.hpp"
#include "xmodex/os/Thread.hpp"

#define BYTESTOBLOCKS(_bytes) (((_bytes) + (this->mBlockSize - 1)) / this->mBlockSize)

#define ALIGNMENT 256

namespace FMOD {

int32_t MemPool::findFreeBlocks(int32_t offset, int32_t to, int32_t numblocks) {
    int32_t found = 0;
    int32_t one_ls_bitoffset;
    int32_t byteoffset;
    int32_t bitoffset;

    byteoffset = offset / 8;
    bitoffset = offset & 7;
    one_ls_bitoffset = (1 << bitoffset);

    while (found < numblocks && (byteoffset * 8) + bitoffset < to &&
           (byteoffset * 8) + bitoffset < this->mSizeBlocks) {
        if (this->mBitmap[byteoffset] & one_ls_bitoffset ||
            (!bitoffset && !(byteoffset & 3) &&
             *(reinterpret_cast<uint32_t*>(&this->mBitmap[byteoffset])) == 0xFFFFFFFF)) {
            found = 0;
        } else {
            found++;
        }

        if (!bitoffset && !(byteoffset & 3) &&
            *(reinterpret_cast<uint32_t*>(&this->mBitmap[byteoffset])) == 0xFFFFFFFF) {
            byteoffset += 4;
        } else {
            bitoffset++;

            one_ls_bitoffset *= 2;
            if (bitoffset >= 8) {
                bitoffset = 0;
                one_ls_bitoffset = 1;
                byteoffset++;
            }
        }
    }

    offset = (byteoffset * 8) + bitoffset;

    if (found == numblocks) {
        offset -= numblocks;
    } else {
        offset = -1;
    }

    return offset;
}

MemPool::MemPool() {
    this->mAlloc = Memory_DefaultMalloc;
    this->mRealloc = Memory_DefaultRealloc;
    this->mFree = Memory_DefaultFree;
    this->mCrit = nullptr;

    for (int32_t i = 0; i < FMOD_MEMORY_MAXTHREADS; i++) {
        this->mCurrentAllocated[i] = 0;
        this->mCurrentAllocatedThreadID[i] = 0;
    }
}

FMOD_RESULT MemPool::init(void* poolmem, int32_t poolsize, int32_t blocksize) {
    void* oldpoolmem = poolmem;
    int32_t bitmapsize;

    if (!poolmem || !poolsize) {
        return FMOD_ERR_MEMORY;
    }

    this->close();

    poolmem = reinterpret_cast<uint8_t*>(FMOD_ALIGNPOINTER(poolmem, ALIGNMENT));
    poolsize -= static_cast<int32_t>(reinterpret_cast<FMOD_SINT_NATIVE>(poolmem) - reinterpret_cast<FMOD_SINT_NATIVE>(oldpoolmem));
    poolsize &= ~(static_cast<uint32_t>(blocksize) - 1);

    this->mBlockSize = blocksize;

    bitmapsize = BYTESTOBLOCKS((poolsize + 7) / 8);
    bitmapsize += (blocksize - 1);
    bitmapsize &= ~(blocksize - 1);

    this->mSizeBlocks = BYTESTOBLOCKS(poolsize - bitmapsize);
    this->mSizeBytes = this->mSizeBlocks * this->mBlockSize;
    this->mBitmap = reinterpret_cast<uint8_t*>(poolmem);
    this->mData = this->mBitmap + bitmapsize;

    this->set(0, 0, this->mSizeBlocks);

    FMOD_memset(this->mData, 0, this->mSizeBytes);

    this->mNumBlocks = 0;
    this->mMaxBlocks = 0;
    this->mMaxAllocated = 0;
    this->mActualMaxBytes = 0;
    this->mWastage = 0;
    this->mFirstFreeBlock = 0;

    for (int32_t i = 0; i < FMOD_MEMORY_MAXTHREADS; i++) {
        this->mCurrentAllocated[i] = 0;
        this->mCurrentAllocatedThreadID[i] = 0;
    }

    return FMOD_OS_CriticalSection_Create(&this->mCrit, true);
}

void MemPool::close() {
    if (this->mCustomPool && this->mBitmap) {
        FMOD_Memory_Free(this->mBitmap);
    }

    this->mBitmap = nullptr;
    this->mData = nullptr;
    this->mSizeBytes = 0;
    this->mSizeBlocks = 0;
    this->mNumBlocks = 0;
    this->mMaxBlocks = 0;
    this->mMaxAllocated = 0;

    for (int32_t i = 0; i < FMOD_MEMORY_MAXTHREADS; i++) {
        this->mCurrentAllocated[i] = 0;
        this->mCurrentAllocatedThreadID[i] = 0;
    }

    this->mAlloc = Memory_DefaultMalloc;
    this->mRealloc = Memory_DefaultRealloc;
    this->mFree = Memory_DefaultFree;
    this->mCustomPool = false;

    if (this->mCrit) {
        FMOD_OS_CriticalSection_Free(this->mCrit);
        this->mCrit = nullptr;
    }
}

void* MemPool::alloc(int32_t len, const char* file, int32_t line, FMOD_MEMORY_TYPE type, bool clear) {
    MemBlockHeader* block = nullptr;
    int32_t numblocks = 0;
    int32_t reallen = len;

    if (!this->mCrit) {
        FMOD_RESULT result = FMOD_OS_CriticalSection_Create(&this->mCrit, true);
        if (result != FMOD_OK) {
            return nullptr;
        }
    }

    FMOD_OS_CriticalSection_Enter(this->mCrit);

    if (!this->mCustomPool) {
        reallen += sizeof(MemBlockHeader);
    }

    if (this->mAlloc) {
        block = reinterpret_cast<MemBlockHeader*>(this->mAlloc(reallen, type));
    } else {
        int32_t offset;

        numblocks = BYTESTOBLOCKS(reallen);

        offset = this->findFreeBlocks(this->mFirstFreeBlock, this->mSizeBlocks, numblocks);
        if (offset >= 0) {
            this->set(offset, 1, numblocks);

            if (this->mCustomPool) {
                block = reinterpret_cast<MemBlockHeader*>(FMOD_Memory_Alloc(sizeof(MemBlockHeader)));
            } else {
                block = reinterpret_cast<MemBlockHeader*>(this->mData + (offset * this->mBlockSize));
            }

            block->mBlockOffset = offset;
        }
    }

    if (!block) {
        FMOD_OS_CriticalSection_Leave(this->mCrit);
        return nullptr;
    }

    block->mSize = len;
    block->mNumBlocks = numblocks;
    block->mThreadID = this->getCurrentThreadID(0);

    this->mCurrentAllocated[0] += block->mSize;
    this->mCurrentAllocated[block->mThreadID] += block->mSize;
    if (this->mCurrentAllocated[0] > this->mMaxAllocated) {
        this->mMaxAllocated = this->mCurrentAllocated[0];
    }

    this->mNumBlocks += block->mNumBlocks;
    if (this->mNumBlocks > this->mMaxBlocks) {
        this->mMaxBlocks = this->mNumBlocks;
        this->mActualMaxBytes = this->mMaxBlocks * this->mBlockSize;
        this->mWastage = this->mActualMaxBytes - this->mMaxAllocated;
    }

    if (!this->mCustomPool) {
        block++;

        if (clear && block) {
            FMOD_memset(block, 0, len);
        }
    }

    FMOD_OS_CriticalSection_Leave(this->mCrit);
    return block;
}

void* MemPool::calloc(int32_t len, const char* file, int32_t line, FMOD_MEMORY_TYPE type) {
    return this->alloc(len, file, line, type, true);
}

void* MemPool::realloc(void* ptr, int32_t len, const char* file, int32_t line, FMOD_MEMORY_TYPE type) {
    MemBlockHeader* block = reinterpret_cast<MemBlockHeader*>(ptr);
    int32_t reallen = len;
    int32_t numblocks = 0;

    if (!ptr) {
        return MemPool::alloc(len, file, line, 0, false);
    }

    if (!this->mCrit) {
        FMOD_RESULT result = FMOD_OS_CriticalSection_Create(&this->mCrit, true);
        if (result != FMOD_OK) {
            return nullptr;
        }
    }

    FMOD_OS_CriticalSection_Enter(this->mCrit);

    if (!this->mCustomPool) {
        reallen += sizeof(MemBlockHeader);
        block--;
    }

    this->mCurrentAllocated[0] -= block->mSize;
    this->mCurrentAllocated[block->mThreadID] -= block->mSize;
    this->mNumBlocks -= block->mNumBlocks;

    if (this->mRealloc) {
        block = reinterpret_cast<MemBlockHeader*>(this->mRealloc(block, reallen, type));
    } else {
        int32_t offset;

        numblocks = BYTESTOBLOCKS(reallen);

        this->set(block->mBlockOffset, 0, block->mNumBlocks);

        offset = this->findFreeBlocks(block->mBlockOffset,
                                      block->mBlockOffset + numblocks, numblocks);
        if (offset >= 0) {
            this->set(offset, 1, numblocks);

            block = reinterpret_cast<MemBlockHeader*>(this->mData + (offset * this->mBlockSize));
            block->mBlockOffset = offset;
        } else {
            offset = this->findFreeBlocks(this->mFirstFreeBlock, this->mSizeBlocks, numblocks);
            if (offset >= 0) {
                MemBlockHeader* newblock;

                this->set(offset, 1, numblocks);

                if (this->mCustomPool) {
                    newblock = block;
                } else {
                    newblock = reinterpret_cast<MemBlockHeader*>(this->mData + (offset * this->mBlockSize));
                }

                newblock->mBlockOffset = offset;

                if (!this->mCustomPool) {
                    FMOD_memmove(newblock + 1, block + 1, block->mSize);
                }

                block = newblock;
            } else {
                block = nullptr;
            }
        }
    }

    if (!block) {
        FMOD_OS_CriticalSection_Leave(this->mCrit);
        return nullptr;
    }

    block->mSize = len;
    block->mNumBlocks = numblocks;
    block->mThreadID = this->getCurrentThreadID(0);

    this->mCurrentAllocated[0] += block->mSize;
    this->mCurrentAllocated[block->mThreadID] += block->mSize;
    if (this->mCurrentAllocated[0] > this->mMaxAllocated) {
        this->mMaxAllocated = this->mCurrentAllocated[0];
    }

    this->mNumBlocks += block->mNumBlocks;
    if (this->mNumBlocks > this->mMaxBlocks) {
        this->mMaxBlocks = this->mNumBlocks;
        this->mActualMaxBytes = this->mMaxBlocks * this->mBlockSize;
        this->mWastage = this->mActualMaxBytes - this->mMaxAllocated;
    }

    if (!this->mCustomPool) {
        block++;
    }

    FMOD_OS_CriticalSection_Leave(this->mCrit);

    return block;
}

void MemPool::free(void* ptr, const char* file, int32_t line, FMOD_MEMORY_TYPE type) {
    MemBlockHeader* block = reinterpret_cast<MemBlockHeader*>(ptr);

    if (!this->mCrit) {
        FMOD_RESULT result = FMOD_OS_CriticalSection_Create(&this->mCrit, true);
        if (result != FMOD_OK) {
            return;
        }
    }

    FMOD_OS_CriticalSection_Enter(this->mCrit);

    if (!this->mCustomPool) {
        block--;
    }

    this->mCurrentAllocated[0] -= block->mSize;
    this->mCurrentAllocated[block->mThreadID] -= block->mSize;
    this->mNumBlocks -= block->mNumBlocks;
    block->mThreadID = 0;

    if (this->mFree) {
        this->mFree(block, type);
    } else {
        this->set(block->mBlockOffset, 0, block->mNumBlocks);
    }

    FMOD_OS_CriticalSection_Leave(this->mCrit);

    if (this->mCustomPool) {
        FMOD_Memory_Free(block);
    }
}

void MemPool::clearThreadID(FMOD_UINT_NATIVE id) {
    for (int32_t i = 1; i < FMOD_MEMORY_MAXTHREADS; i++) {
        if (id == this->mCurrentAllocatedThreadID[i]) {
            this->mCurrentAllocated[i] = 0;
            this->mCurrentAllocatedThreadID[i] = 0;
            break;
        }
    }
}

uint32_t MemPool::getCurrentThreadID(FMOD_UINT_NATIVE threadid) {
    uint32_t count;
    FMOD_UINT_NATIVE id = threadid;

    if (!threadid) {
        FMOD_OS_Thread_GetCurrentID(&threadid);
    }

    for (count = 1; count < FMOD_MEMORY_MAXTHREADS; count++) {
        if (id == this->mCurrentAllocatedThreadID[count]) {
            break;
        } else if (!this->mCurrentAllocatedThreadID[count]) {
            this->mCurrentAllocatedThreadID[count] = id;
            this->mCurrentAllocated[count] = 0;
            break;
        }
    }

    return count;
}

void MemPool::set(int32_t blockoffset, int32_t value, int32_t numblocks) {
    int32_t byteoffset;
    int32_t bitoffset;
    int32_t count;
    int32_t blocksleft;

    blocksleft = numblocks;
    byteoffset = blockoffset / 8;
    bitoffset = blockoffset & 7;

    count = 0;
    if (blockoffset & 31) {
        count = 32 - (blockoffset & 31);
        if (count > numblocks) {
            count = numblocks;
        }
    }

    while (count) {
        if (value) {
            this->mBitmap[byteoffset] |= (1 << bitoffset);
        } else {
            this->mBitmap[byteoffset] &= ~(1 << bitoffset);
        }

        bitoffset++;
        if (bitoffset >= 8) {
            bitoffset = 0;
            byteoffset++;
        }
        count--;
        blocksleft--;
    }

    count = blocksleft / 8;
    if (count) {
        FMOD_memset(&this->mBitmap[byteoffset], value ? 0xFF : 0, count);

        byteoffset += count;
        blocksleft -= count * 8;
    }

    count = blocksleft & 31;
    while (count) {
        if (value) {
            this->mBitmap[byteoffset] |= (1 << bitoffset);
        } else {
            this->mBitmap[byteoffset] &= ~(1 << bitoffset);
        }

        bitoffset++;
        if (bitoffset >= 8) {
            bitoffset = 0;
            byteoffset++;
        }
        count--;
    }

    if (value) {
        if (blockoffset == this->mFirstFreeBlock) {
            this->mFirstFreeBlock = blockoffset + numblocks;
        }

        byteoffset = this->mFirstFreeBlock / 8;
        bitoffset = this->mFirstFreeBlock & 7;
        if (this->mBitmap[byteoffset] & (1 << bitoffset)) {
            this->mFirstFreeBlock = this->findFreeBlocks(this->mFirstFreeBlock, this->mSizeBlocks, 1);
        }
    } else {
        if (blockoffset < this->mFirstFreeBlock) {
            this->mFirstFreeBlock = blockoffset;
        }
    }
}

FMOD_RESULT MemPool::setCallbacks(FMOD_MEMORY_ALLOCCALLBACK useralloc, FMOD_MEMORY_REALLOCCALLBACK userrealloc, FMOD_MEMORY_FREECALLBACK userfree) {
    this->mAlloc = useralloc;
    this->mFree = userfree;
    this->mRealloc = userrealloc;
    return FMOD_OK;
}

} // namespace FMOD
