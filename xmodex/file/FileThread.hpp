#ifndef XMODEX_FILE_FILE_THREAD_HPP
#define XMODEX_FILE_FILE_THREAD_HPP

#include "xmodex/LinkedListNode.hpp"
#include "xmodex/Thread.hpp"
#include "xmodex/os/CriticalSection.hpp"

namespace FMOD {

class FileThread : public LinkedListNode {
    public:
        Thread mThread;
        bool mThreadActive;
        LinkedListNode mFileListHead;
        LinkedListNode* mFileListCurrent;
        LinkedListNode* mFileListNext;
        FMOD_OS_CRITICALSECTION* mFileListCrit;
        int32_t mDeviceType;
        bool mOwned;

    public:
        FileThread();
        FMOD_RESULT threadFunc();
        FMOD_RESULT init(int32_t devicetype, bool owned);
        FMOD_RESULT release();
};

} // namespace FMOD

#endif
