#ifndef XMODEX_OUTPUT_WIN_OUTPUT_TIMER_HPP
#define XMODEX_OUTPUT_WIN_OUTPUT_TIMER_HPP

#include "xmodex/output/Output.hpp"
#include <windows.h>

namespace FMOD {

class OutputTimer : public Output {
    protected:
        float mMixerTimerPeriod;
        uint32_t mMixerTimerID;
        bool mPlaying;
        bool mThreadElevated;
        FMOD_OS_CRITICALSECTION* mMixerCrit;
        int32_t mFillBlock;

    private:
        FMOD_RESULT timerFunc();
        static void F_CALLBACK timerFuncCallback(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2);

    public:
        FMOD_RESULT stop();
        FMOD_RESULT start();
};

} // namespace FMOD

#endif
