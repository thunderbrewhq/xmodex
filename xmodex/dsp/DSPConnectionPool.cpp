#include "xmodex/dsp/DSPConnectionPool.hpp"
#include "xmodex/LocalCriticalSection.hpp"
#include "xmodex/Math.hpp"
#include "xmodex/Memory.hpp"
#include "xmodex/SystemI.hpp"
#include "xmodex/dsp/DSPConnection.hpp"

namespace FMOD {

FMOD_RESULT DSPConnectionPool::close() {
    for (int32_t i = 0; i < DSP_MAX_CONNECTION_BLOCKS; i++) {
        if (this->mConnectionMemory[i]) {
            FMOD_Memory_Free(this->mConnectionMemory[i]);
            this->mConnectionMemory[i] = nullptr;
        }
        this->mConnection[i] = nullptr;

        if (this->mLevelDataMemory[i]) {
            FMOD_Memory_Free(this->mLevelDataMemory[i]);
            this->mLevelDataMemory[i] = nullptr;
        }
        this->mLevelData[i] = nullptr;

        if (this->mNodeMemory[i]) {
            FMOD_Memory_Free(this->mNodeMemory[i]);
            this->mNodeMemory[i] = nullptr;
        }
    }

    return FMOD_OK;
}

FMOD_RESULT DSPConnectionPool::alloc(DSPConnection** connection) {
    DSPConnection* newconnection;
    uint32_t alignment = 0;

    if (!this->mSystem) {
        return FMOD_ERR_UNINITIALIZED;
    }

    LocalCriticalSection crit(this->mSystem->mDSPConnectionCrit);

    if (!connection) {
        return FMOD_ERR_INVALID_PARAM;
    }

    crit.enter();

    if (this->mFreeListHead.isEmpty()) {
        int32_t newblock;
        float* leveldata = nullptr;

        for (newblock = 0; newblock < DSP_MAX_CONNECTION_BLOCKS; newblock++) {
            if (!this->mConnectionMemory[newblock]) {
                break;
            }
        }
        if (newblock >= DSP_MAX_CONNECTION_BLOCKS) {
            return FMOD_ERR_INTERNAL;
        }

        this->mConnectionMemory[newblock] = static_cast<DSPConnection*>(FMOD_Memory_Calloc(this->mNumConnections * sizeof(DSPConnection) + 16));
        if (!mConnectionMemory[newblock]) {
            return FMOD_ERR_MEMORY;
        }
        this->mConnection[newblock] = reinterpret_cast<DSPConnection*>(FMOD_ALIGNPOINTER(this->mConnectionMemory[newblock], 16));

        this->mNodeMemory[newblock] = static_cast<LinkedListNode*>(FMOD_Memory_Calloc(this->mNumConnections * sizeof(LinkedListNode)));
        if (!this->mNodeMemory[newblock]) {
            return FMOD_ERR_MEMORY;
        }

        this->mLevelDataMemory[newblock] = static_cast<float*>(FMOD_Memory_Calloc(
            this->mNumConnections * (this->mNumOutputLevels < 2 ? 2 : this->mNumOutputLevels) * (this->mNumInputLevels < this->mNumOutputLevels ? this->mNumOutputLevels : this->mNumInputLevels) * sizeof(float) * 3 + alignment));
        if (!this->mLevelDataMemory[newblock]) {
            return FMOD_ERR_MEMORY;
        }

        this->mLevelData[newblock] = this->mLevelDataMemory[newblock];

        leveldata = this->mLevelData[newblock];

        for (int32_t i = 0; i < this->mNumConnections; i++) {
            auto conn = &this->mConnection[newblock][i];
            new (conn) DSPConnection;

            conn->init(leveldata, this->mNumOutputLevels, this->mNumInputLevels);
            conn->mNode = &this->mNodeMemory[newblock][i];
            conn->mNode->setData(conn);
            conn->mNode->addAfter(&mFreeListHead);
        }
    }

    newconnection = static_cast<DSPConnection*>(this->mFreeListHead.getNext()->getData());

    newconnection->mInputNode.setData(newconnection);
    newconnection->mOutputNode.setData(newconnection);

    newconnection->mNode->removeNode();
    newconnection->mNode->addAfter(&mUsedListHead);

    crit.leave();

    *connection = newconnection;

    return FMOD_OK;
}

FMOD_RESULT DSPConnectionPool::free(DSPConnection* connection) {
    LocalCriticalSection crit(this->mSystem->mDSPConnectionCrit);

    if (!connection) {
        return FMOD_ERR_INVALID_PARAM;
    }

    crit.enter();

    connection->mInputNode.removeNode();
    connection->mOutputNode.removeNode();

    connection->mInputUnit = 0;
    connection->mOutputUnit = 0;

    connection->mNode->removeNode();
    connection->mNode->addAfter(&this->mFreeListHead);
    connection->mNode->setData(connection);

    crit.leave();

    return FMOD_OK;
}

FMOD_RESULT DSPConnectionPool::init(SystemI* system, int32_t numconnections, int32_t numoutputlevels, int32_t numinputlevels) {
    float* leveldata;
    uint32_t alignment = 0;

    if (numconnections < 0) {
        return FMOD_ERR_INVALID_PARAM;
    }

    for (int32_t i = 0; i < DSP_MAX_CONNECTION_BLOCKS; i++) {
        this->mConnection[i] = nullptr;
        this->mLevelData[i] = nullptr;
    }

    numconnections += 128;
    numconnections /= 128;
    numconnections *= 128;

    this->mNumConnections = numconnections;
    this->mConnectionMemory[0] = static_cast<DSPConnection*>(FMOD_Memory_Calloc(this->mNumConnections * sizeof(DSPConnection) + 16));
    if (!this->mConnectionMemory[0]) {
        return FMOD_ERR_MEMORY;
    }
    this->mConnection[0] = reinterpret_cast<DSPConnection*>(FMOD_ALIGNPOINTER(this->mConnectionMemory[0], 16));

    this->mNodeMemory[0] = static_cast<LinkedListNode*>(FMOD_Memory_Calloc(this->mNumConnections * sizeof(LinkedListNode)));
    if (!this->mNodeMemory[0]) {
        return FMOD_ERR_MEMORY;
    }

    this->mNumOutputLevels = numoutputlevels;
    this->mNumInputLevels = numinputlevels;

    this->mLevelDataMemory[0] = static_cast<float*>(FMOD_Memory_Calloc(
        this->mNumConnections * (this->mNumOutputLevels < 2 ? 2 : this->mNumOutputLevels) * (this->mNumInputLevels < this->mNumOutputLevels ? this->mNumOutputLevels : this->mNumInputLevels) * sizeof(float) * 3 + alignment));
    if (!this->mLevelDataMemory[0]) {
        return FMOD_ERR_MEMORY;
    }

    this->mLevelData[0] = this->mLevelDataMemory[0];
    leveldata = this->mLevelData[0];

    this->mFreeListHead.initNode();
    for (int32_t i = 0; i < this->mNumConnections; i++) {
        auto conn = &this->mConnection[0][i];
        new (conn) DSPConnection;

        conn->init(leveldata, numoutputlevels, numinputlevels);
        conn->mNode = &this->mNodeMemory[0][i];
        conn->mNode->setData(conn);
        conn->mNode->addAfter(&this->mFreeListHead);
    }

    this->mSystem = system;

    return FMOD_OK;
}

} // namespace FMOD
