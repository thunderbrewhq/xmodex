#ifndef XMODEX_DSP_DSP_CONNECTION_POOL_HPP
#define XMODEX_DSP_DSP_CONNECTION_POOL_HPP

#include "fmod.hpp"
#include "xmodex/LinkedListNode.hpp"

#define DSP_MAX_CONNECTION_BLOCKS 32

namespace FMOD {

class DSPConnection;
class SystemI;

class DSPConnectionPool {
    private:
        SystemI* mSystem;
        DSPConnection* mConnection[DSP_MAX_CONNECTION_BLOCKS];
        DSPConnection* mConnectionMemory[DSP_MAX_CONNECTION_BLOCKS];
        LinkedListNode* mNodeMemory[DSP_MAX_CONNECTION_BLOCKS];
        int32_t mNumInputLevels;
        int32_t mNumOutputLevels;
        int32_t mNumConnections;
        LinkedListNode mUsedListHead;
        LinkedListNode mFreeListHead;
        float* mLevelData[DSP_MAX_CONNECTION_BLOCKS];
        float* mLevelDataMemory[DSP_MAX_CONNECTION_BLOCKS];

    public:
        FMOD_RESULT close();
        FMOD_RESULT alloc(DSPConnection** connection);
        FMOD_RESULT free(DSPConnection* connection);
        FMOD_RESULT init(SystemI* system, int32_t numconnections, int32_t numoutputlevels, int32_t numinputlevels);
};

} // namespace FMOD

#endif
