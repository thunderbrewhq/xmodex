#ifndef XMODEX_THREAD_HPP
#define XMODEX_THREAD_HPP

#include "fmod.hpp"
#include "xmodex/String.hpp"
#include "xmodex/Types.hpp"
#include "xmodex/os/Semaphore.hpp"

namespace FMOD {

class Thread {
    public:
        enum PRIORITY {
            PRIORITY_VERYLOW = -2,
            PRIORITY_LOW = -1,
            PRIORITY_NORMAL = 0,
            PRIORITY_HIGH = 1,
            PRIORITY_VERYHIGH = 2,
            PRIORITY_CRITICAL = 3
        };

    private:
        char mName[FMOD_STRING_MAXNAMELEN];
        void* mHandle;
        bool mRunning;
        void* mUserData;
        void* mStack;
        FMOD_OS_SEMAPHORE* mSema;
        FMOD_OS_SEMAPHORE* mEndSema;
        void (*mUserCallback)(void* userdata);
        int32_t mPeriod;

    public:
        Thread();
        virtual ~Thread();
        virtual FMOD_RESULT threadFunc();

        FMOD_RESULT initThread(const char* name, void (*func)(void* userdata), void* userdata, PRIORITY priority, void* stack, int32_t stacksize, bool usesemaphore, int32_t sleepperiod);
        FMOD_RESULT closeThread();
        FMOD_RESULT getCurrentThreadID(FMOD_UINT_NATIVE* id);
        FMOD_RESULT wakeupThread(bool frominterrupt = false);

        static uintptr_t F_CALLBACK callback(void* data);
};

}; // namespace FMOD

#endif
