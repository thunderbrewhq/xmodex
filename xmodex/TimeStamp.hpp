#ifndef XMODEX_TIME_STAMP_HPP
#define XMODEX_TIME_STAMP_HPP

#include "fmod.hpp"
#include <cstdint>

namespace FMOD {

class TimeStamp {
    public:
        uint32_t mIn;
        uint32_t mOut;
        uint32_t mPausedIn;
        uint32_t mPausedOut;
        uint32_t mTotalIn;
        uint32_t mTotalOut;
        float mPercent;
        float mAvTotal;
        float mAvCurrent;
        float mCPUUsage;
        bool mPaused;
        uint32_t mPausedTotal;
        int32_t mPausedRefCount;
        bool mTiming;

    public:
        TimeStamp();
        FMOD_RESULT stampIn();
        FMOD_RESULT stampOut(int32_t damppercentage);
        FMOD_RESULT setPaused(bool paused);
};

} // namespace FMOD

#endif
