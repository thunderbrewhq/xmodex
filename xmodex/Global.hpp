#ifndef XMODEX_GLOBAL_HPP
#define XMODEX_GLOBAL_HPP

#include "xmodex/MemPool.hpp"
#include "xmodex/Types.hpp"

namespace FMOD {

class SystemI;

class Global {
    public:
        SystemI* gSystemHead;
        MemPool* gSystemPool;
        int32_t gSystemInitCount;
#if defined(XMODEX_SYSTEM_MAC) || defined(XMODEX_SYSTEM_LINUX)
        uint32_t gStartTimeSeconds;
#endif
        FMOD_UINT64P gDSPClock;
        uint32_t gDSPClockTimeStamp;
        int32_t gFileBusy;

    public:
        Global();

        void init();
};

extern Global* gGlobal;

} // namespace FMOD

#endif
