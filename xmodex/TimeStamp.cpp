#include "xmodex/TimeStamp.hpp"
#include "xmodex/os/Time.hpp"

namespace FMOD {

TimeStamp::TimeStamp() {
    this->mIn = 0;
    this->mOut = 0;
    this->mTotalIn = 0;
    this->mTotalOut = 0;
    this->mPaused = false;
    this->mPausedIn = 0;
    this->mPausedOut = 0;
    this->mPausedTotal = 0;
    this->mPausedRefCount = 0;
    this->mTiming = false;
}

FMOD_RESULT TimeStamp::stampIn() {
    FMOD_OS_Time_GetNs(&this->mIn);

    this->mTiming = true;

    return FMOD_OK;
}

FMOD_RESULT TimeStamp::stampOut(int32_t damppercentage) {
    uint32_t val;
    float total;
    float smoothedtotal;
    float dampratio = static_cast<float>(damppercentage) / 100;

    FMOD_OS_Time_GetNs(&val);

    this->mOut = val;
    this->mTotalOut = val;

    if (this->mTotalOut < this->mTotalIn) {
        total = 0;
    } else {
        total = static_cast<float>(this->mTotalOut - this->mTotalIn);
    }

    this->mAvTotal = static_cast<float>(this->mAvTotal * dampratio);
    this->mAvTotal += total;
    smoothedtotal = static_cast<float>(this->mAvTotal * (1.0f - dampratio));

    this->mPercent *= dampratio;
    if (this->mOut > this->mIn) {
        uint32_t delta = this->mOut - this->mIn - this->mPausedTotal;
        this->mPercent += ((static_cast<float>(delta) / smoothedtotal) * 100.0f);
    }

    this->mCPUUsage = this->mPercent * (1.0f - dampratio);
    this->mTotalIn = val;
    this->mPausedTotal = 0;
    this->mPausedRefCount = 0;
    this->mTiming = false;

    return FMOD_OK;
}

FMOD_RESULT TimeStamp::setPaused(bool paused) {
    if (!this->mTiming) {
        return FMOD_OK;
    }

    if (paused) {
        if (!this->mPausedRefCount) {
            FMOD_OS_Time_GetNs(&this->mPausedIn);
        }
        this->mPausedRefCount++;
    } else if (!paused) {
        this->mPausedRefCount--;
        if (this->mPausedRefCount < 0) {
            // XXX: ?
            // this->mPausedRefCount = this->mPausedRefCount;
        }

        if (!this->mPausedRefCount) {
            FMOD_OS_Time_GetNs(&this->mPausedOut);

            if (this->mPausedOut > this->mPausedIn) {
                this->mPausedTotal += (this->mPausedOut - this->mPausedIn);
            }
        }
    }

    this->mPaused = paused;

    return FMOD_OK;
}

} // namespace FMOD
