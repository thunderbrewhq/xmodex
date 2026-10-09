#include "xmodex/LocalMemoryCounter.hpp"
#include "xmodex/Global.hpp"
#include "xmodex/MemPool.hpp"

namespace FMOD {

LocalMemoryCounter::LocalMemoryCounter() {
    auto pool = gGlobal->gSystemPool;
    this->mUsageAtStart = pool->mCurrentAllocated[pool->getCurrentThreadID(0)];
}

int32_t LocalMemoryCounter::getMemoryUsed() {
    auto pool = gGlobal->gSystemPool;
    return pool->mCurrentAllocated[pool->getCurrentThreadID(0)] - this->mUsageAtStart;
}

} // namespace FMOD
