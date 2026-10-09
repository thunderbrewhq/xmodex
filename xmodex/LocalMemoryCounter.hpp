#ifndef XMODEX_LOCAL_MEMORY_COUNTER_HPP
#define XMODEX_LOCAL_MEMORY_COUNTER_HPP

#include <cstdint>

namespace FMOD {

class LocalMemoryCounter {
    private:
        int32_t mUsageAtStart;

    public:
        LocalMemoryCounter();

        int32_t getMemoryUsed();
};

} // namespace FMOD

#endif
