#ifndef XMODEX_ASYNC_DATA_HPP
#define XMODEX_ASYNC_DATA_HPP

#include "fmod.hpp"
#include "xmodex/LinkedListNode.hpp"
#include "xmodex/String.hpp"

namespace FMOD {

class AsyncThread;

class AsyncData {
    public:
        char mName[FMOD_STRING_MAXNAMELEN];
        AsyncThread* mThread;
        LinkedListNode mNode;
        void* mNameData;
        FMOD_CREATESOUNDEXINFO mExInfo;
        bool mExInfoExists;
        FMOD_RESULT mResult;
};

} // namespace FMOD

#endif
