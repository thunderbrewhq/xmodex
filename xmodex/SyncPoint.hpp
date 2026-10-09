#ifndef XMODEX_SYNC_POINT_HPP
#define XMODEX_SYNC_POINT_HPP

#include "xmodex/LinkedListNode.hpp"
#include "xmodex/String.hpp"

namespace FMOD {

class SyncPoint : public SortedLinkedListNode {
    public:
        uint32_t mOffset;
        char mName[FMOD_STRING_MAXNAMELEN];
        uint32_t mIndex;
};

} // namespace FMOD

#endif
