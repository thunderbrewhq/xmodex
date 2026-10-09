#ifndef XMODEX_OUTPUT_OUTPUT_POLLED_HPP
#define XMODEX_OUTPUT_OUTPUT_POLLED_HPP

#include "xmodex/Thread.hpp"
#include "xmodex/output/Output.hpp"

namespace FMOD {

class OutputPolled : public Output, public Thread {
    public:
        int32_t mCursorBlock;
        bool mPolledFromMainThread;
        FMOD_OS_SEMAPHORE* mFinishedSema;

    public:
        OutputPolled();

        FMOD_RESULT start();
        FMOD_RESULT stop();
        FMOD_RESULT threadFunc();
};

} // namespace FMOD

#endif
