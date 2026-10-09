#ifndef XMODEX_ASYNC_HPP
#define XMODEX_ASYNC_HPP

#include "xmodex/LinkedListNode.hpp"
#include "xmodex/Thread.hpp"
#include "xmodex/os/CriticalSection.hpp"

namespace FMOD {

class SystemI;
class SoundI;

typedef FMOD_RESULT(F_CALLBACK* FMOD_ASYNC_CALLBACK)();

class AsyncThread : public LinkedListNode {
        friend class SystemI;
        friend class Stream;
        friend class SoundI;
        friend class ChannelStream;

    private:
        Thread mThread;
        bool mThreadActive;
        LinkedListNode mHead;
        FMOD_OS_CRITICALSECTION* mCrit;
        bool mOwned;
        bool mBusy;
        bool mDone;
        LinkedListNode mCallbackHead;

    public:
        AsyncThread();

        FMOD_RESULT init(bool owned);
        FMOD_RESULT reallyRelease();
        FMOD_RESULT threadFunc();
        FMOD_RESULT release();

        static FMOD_RESULT shutDown();
        static FMOD_RESULT getAsyncThread(SoundI* sound);
        static FMOD_RESULT update();

        static FMOD_OS_CRITICALSECTION* gAsyncCrit;
        static LinkedListNode gAsyncHead;
};

} // namespace FMOD

#endif
