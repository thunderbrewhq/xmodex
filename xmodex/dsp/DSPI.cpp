#include "xmodex/dsp/DSPI.hpp"
#include "xmodex/LocalCriticalSection.hpp"
#include "xmodex/Math.hpp"
#include "xmodex/Memory.hpp"
#include "xmodex/SystemI.hpp"
#include "xmodex/channel/ChannelI.hpp"
#include "xmodex/dsp/DSPConnection.hpp"
#include "xmodex/os/SIMD.hpp"

namespace FMOD {

DSPI::DSPI() {
    this->mWantsToFinish = &this->mWantsToFinishMem;
    this->mBuffer = nullptr;
    this->mOutputBuffer = nullptr;
    this->mFlags = 0;
    this->mWantsToFinishMem = 0;
    this->mDefaultVolume = 1.0f;
    this->mNumInputs = 0;
    this->mDefaultFrequency = 44100.0f;
    this->mNumOutputs = 0;
    this->mTreeLevel = -1;
    this->mDefaultPan = 0.0f;
    this->mDefaultPriority = FMOD_CHANNEL_DEFAULTPRIORITY;
}

FMOD_RESULT DSPI::resetVisited() {
    auto current = this->mInputHead.getNext();
    while (current != &this->mInputHead) {
        auto connection = static_cast<DSPConnection*>(current->getData());
        if (!connection) {
            break;
        }

        connection->mInputUnit->resetVisited();
        current = current->getNext();
    }

    this->mFlags &= ~FMOD_DSP_FLAG_VISITED;

    if (*this->mWantsToFinish) {
        this->mFlags |= FMOD_DSP_FLAG_FINISHED;
        *this->mWantsToFinish = 0;
    }

    return FMOD_OK;
}

FMOD_RESULT DSPI::updateTreeLevel(int32_t level) {
    if (this->mNumOutputs > 1 && level < this->mTreeLevel) {
        return FMOD_OK;
    }

    this->mTreeLevel = level;

    if (!this->mOutputBuffer || reinterpret_cast<FMOD_UINT_NATIVE>(this->mBuffer) != FMOD_ALIGNPOINTER(this->mOutputBuffer, 16) && level > 0) {
        if (!this->mSystem->mDSPMixBuff[level - 1]) {
            this->mSystem->mDSPMixBuff[level - 1] = static_cast<float*>(FMOD_Memory_Calloc((this->mSystem->mDSPBlockSize * (this->mSystem->mMaxOutputChannels < this->mSystem->mMaxInputChannels ? this->mSystem->mMaxInputChannels : this->mSystem->mMaxOutputChannels) * sizeof(float)) + 16));
            if (!this->mSystem->mDSPMixBuff[level - 1]) {
                return FMOD_ERR_MEMORY;
            }
        }
    }

    auto current = this->mInputHead.getNext();
    while (current != &this->mInputHead) {
        auto connection = static_cast<DSPConnection*>(current->getData());
        connection->mInputUnit->updateTreeLevel(this->mTreeLevel + 1);
        current = current->getNext();
    }

    if (!this->mOutputBuffer || reinterpret_cast<FMOD_UINT_NATIVE>(this->mBuffer) != FMOD_ALIGNPOINTER(this->mOutputBuffer, 16)) {
        this->mBuffer = reinterpret_cast<float*>(FMOD_ALIGNPOINTER(this->mSystem->mDSPMixBuff[this->mTreeLevel], 16));
    }

    return FMOD_OK;
}

FMOD_RESULT DSPI::getNumInputs(int32_t* numinputs) {
    if (!numinputs) {
        return FMOD_ERR_INVALID_PARAM;
    }

    this->mSystem->flushDSPConnectionRequests(true);

    FMOD_OS_CriticalSection_Enter(this->mSystem->mDSPConnectionCrit);
    *numinputs = this->mNumInputs;
    FMOD_OS_CriticalSection_Leave(this->mSystem->mDSPConnectionCrit);

    return FMOD_OK;
}

FMOD_RESULT DSPI::getNumOutputs(int32_t* numoutputs) {
    if (!numoutputs) {
        return FMOD_ERR_INVALID_PARAM;
    }

    this->mSystem->flushDSPConnectionRequests(true);

    FMOD_OS_CriticalSection_Enter(this->mSystem->mDSPConnectionCrit);
    *numoutputs = this->mNumOutputs;
    FMOD_OS_CriticalSection_Leave(this->mSystem->mDSPConnectionCrit);

    return FMOD_OK;
}

FMOD_RESULT DSPI::setUserData(void* userdata) {
    this->mDescription.userdata = userdata;
    return FMOD_OK;
}

FMOD_RESULT DSPI::getUserData(void** userdata) {
    if (!userdata) {
        return FMOD_ERR_INVALID_PARAM;
    }

    *userdata = this->mDescription.userdata;
    return FMOD_OK;
}

FMOD_RESULT DSPI::setFinished(bool finished, bool force) {
    if (!finished) {
        *this->mWantsToFinish = 0;
        this->mFlags &= ~FMOD_DSP_FLAG_FINISHED;
    }

    if (force) {
        FMOD_OS_CriticalSection_Enter(this->mSystem->mDSPCrit);

        *this->mWantsToFinish = 0;

        if (finished) {
            this->mFlags |= FMOD_DSP_FLAG_FINISHED;
        } else {
            this->mFlags &= ~FMOD_DSP_FLAG_FINISHED;
        }

        FMOD_OS_CriticalSection_Leave(this->mSystem->mDSPCrit);
    } else {
        *this->mWantsToFinish = finished;
    }

    return FMOD_OK;
}

FMOD_RESULT DSPI::getInput(int32_t index, DSPConnection** input, DSPI** inputdsp) {
    LocalCriticalSection crit(this->mSystem->mDSPConnectionCrit);
    LinkedListNode* current;
    DSPConnection* connection;
    int32_t count;

    this->mSystem->flushDSPConnectionRequests(true);

    crit.enter();

    if (index >= this->mNumInputs) {
        return FMOD_ERR_INVALID_PARAM;
    }

    current = this->mInputHead.getNext();
    if (current == &this->mInputHead) {
        return FMOD_ERR_INTERNAL;
    }

    count = 0;
    while (count < index) {
        current = current->getNext();
        count++;
    }

    connection = static_cast<DSPConnection*>(current->getData());

    if (input) {
        *input = connection;
    }
    if (inputdsp) {
        *inputdsp = connection->mInputUnit;
    }

    crit.leave();

    return FMOD_OK;
}

FMOD_RESULT DSPI::getOutput(int32_t index, DSPConnection** output, DSPI** outputdsp) {
    LocalCriticalSection crit(this->mSystem->mDSPConnectionCrit);
    int32_t count;
    LinkedListNode* current;
    DSPConnection* connection;

    this->mSystem->flushDSPConnectionRequests(true);

    crit.enter();

    if (index >= this->mNumOutputs) {
        return FMOD_ERR_INVALID_PARAM;
    }

    current = this->mOutputHead.getNext();
    if (current == &this->mOutputHead) {
        return FMOD_ERR_INTERNAL;
    }

    count = 0;
    while (count < index) {
        current = current->getNext();
        count++;
    }
    connection = static_cast<DSPConnection*>(current->getData());

    if (output) {
        *output = connection;
    }
    if (outputdsp) {
        *outputdsp = connection->mOutputUnit;
    }

    crit.leave();

    return FMOD_OK;
}

FMOD_RESULT DSPI::disconnectFromInternal(DSPI* target) {
    FMOD_RESULT result;
    auto dspcrit = this->mSystem->mDSPCrit;
    auto dspconnectioncrit = this->mSystem->mDSPConnectionCrit;

    if (!target) {
        int32_t numinputs = 0;
        int32_t numoutputs = 0;

        this->getNumInputs(&numinputs);

        while (numinputs) {
            DSPI* input;

            result = this->getInput(0, nullptr, &input);
            if (result != FMOD_OK) {
                return result;
            }

            this->disconnectFromInternal(input);
            this->getNumInputs(&numinputs);
        }

        this->getNumOutputs(&numoutputs);
        while (numoutputs) {
            DSPI* output;

            result = this->getOutput(0, nullptr, &output);
            if (result != FMOD_OK) {
                return result;
            }

            output->disconnectFromInternal(this);

            this->getNumOutputs(&numoutputs);
        }

        return FMOD_OK;
    } else {
        int32_t count;
        DSPConnection* connection = nullptr;

        FMOD_OS_CriticalSection_Enter(dspconnectioncrit);
        FMOD_OS_CriticalSection_Enter(dspcrit);

        for (count = 0; count < this->mNumInputs; count++) {
            result = this->getInput(count, &connection, nullptr);
            if (result != FMOD_OK) {
                FMOD_OS_CriticalSection_Leave(dspconnectioncrit);
                FMOD_OS_CriticalSection_Leave(dspcrit);
                return result;
            }

            if (connection->mInputUnit == target) {
                break;
            }
        }

        if (count == this->mNumInputs) {
            FMOD_OS_CriticalSection_Leave(dspcrit);
            FMOD_OS_CriticalSection_Leave(dspconnectioncrit);
            return FMOD_ERR_DSP_NOTFOUND;
        }

        connection->mInputNode.removeNode();
        this->mNumInputs--;

        if (this->mOutputBuffer && this->mNumOutputs <= 1) {
            FMOD_Memory_Free(this->mOutputBuffer);
            this->mOutputBuffer = 0;
        }

        connection->mOutputNode.removeNode();
        target->mNumOutputs--;

        if (*this->mWantsToFinish) {
            this->mFlags |= FMOD_DSP_FLAG_FINISHED;
            *this->mWantsToFinish = 0;
        }
        if (*target->mWantsToFinish) {
            target->mFlags |= FMOD_DSP_FLAG_FINISHED;
            *target->mWantsToFinish = 0;
        }

        result = this->mSystem->mDSPConnectionPool.free(connection);
        if (result != FMOD_OK) {
            FMOD_OS_CriticalSection_Leave(dspconnectioncrit);
            FMOD_OS_CriticalSection_Leave(dspcrit);
            return result;
        }

        FMOD_OS_CriticalSection_Leave(dspconnectioncrit);
        FMOD_OS_CriticalSection_Leave(dspcrit);
    }

    return FMOD_OK;
}

FMOD_RESULT DSPI::disconnectFrom(DSPI* dsp) {
    return this->disconnectFromQueued(dsp);
}

FMOD_RESULT DSPI::disconnectFromQueued(DSPI* target) {
    FMOD_OS_CriticalSection_Enter(this->mSystem->mDSPRequestCrit);

    DSPConnectionRequest* request;

    if (this->mSystem->mConnectionRequestFreeHead.isEmpty()) {
        this->mSystem->flushDSPConnectionRequests(true);
    }

    request = static_cast<DSPConnectionRequest*>(this->mSystem->mConnectionRequestFreeHead.getNext());
    request->removeNode();
    request->addBefore(&this->mSystem->mConnectionRequestUsedHead);

    request->mThis = this;
    request->mTarget = target;
    request->mRequest = DSPCONNECTION_REQUEST_DISCONNECTFROM;

    FMOD_OS_CriticalSection_Leave(this->mSystem->mDSPRequestCrit);

    return FMOD_OK;
}

FMOD_RESULT DSPI::disconnectAll(bool inputs, bool outputs) {
    return this->disconnectAllInternal(inputs, outputs);
}

FMOD_RESULT DSPI::getInput(int32_t index, DSPI** inputdsp) {
    return this->getInput(index, nullptr, inputdsp);
}

FMOD_RESULT DSPI::getOutput(int32_t index, DSPI** outputdsp) {
    return this->getOutput(index, nullptr, outputdsp);
}

FMOD_RESULT DSPI::doesUnitExist(DSPI* target) {
    FMOD_RESULT result;
    int32_t numinputs;

    if (this == target) {
        return FMOD_OK;
    }

    result = this->getNumInputs(&numinputs);
    if (result != FMOD_OK) {
        return FMOD_ERR_INVALID_PARAM;
    }

    for (int32_t i = 0; i < numinputs; i++) {
        DSPConnection* connection;

        result = this->getInput(i, &connection, nullptr);
        if (result != FMOD_OK) {
            return result;
        }

        result = connection->mInputUnit->doesUnitExist(target);
        if (result == FMOD_OK) {
            return result;
        }
    }

    return FMOD_ERR_INVALID_PARAM;
}

FMOD_RESULT DSPI::addInputInternal(
    DSPI* target,
    bool checkcircular,
    DSPConnection* connection,
    DSPConnection** connection_out) {
    FMOD_RESULT result;
    LocalCriticalSection crit(this->mSystem->mDSPCrit);
    LocalCriticalSection crit2(this->mSystem->mDSPConnectionCrit);
    bool allocated = false;

    if (!target) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if ((this->mDescription.mCategory == FMOD_DSP_CATEGORY_RESAMPLER && target->mNumOutputs) || target->mDescription.mCategory == FMOD_DSP_CATEGORY_SOUNDCARD) {
        return FMOD_ERR_DSP_CONNECTION;
    }

    if (checkcircular) {
        if (target->doesUnitExist(this) == FMOD_OK) {
            return FMOD_ERR_DSP_CONNECTION;
        }
    }

    crit.enter();
    crit2.enter();

    if (!connection) {
        result = this->mSystem->mDSPConnectionPool.alloc(&connection);
        if (result != FMOD_OK) {
            return result;
        }

        allocated = true;
    }

    connection->mInputNode.addBefore(&this->mInputHead);
    this->mNumInputs++;

    connection->mOutputNode.addBefore(&target->mOutputHead);
    target->mNumOutputs++;

    connection->mInputUnit = target;
    connection->mOutputUnit = this;

    if (allocated) {
        connection->reset();
    }

    if (this->mTreeLevel >= 0) {
        target->updateTreeLevel(this->mTreeLevel + 1);
    }

    if (!this->mOutputBuffer) {
        this->mBuffer = reinterpret_cast<float*>(FMOD_ALIGNPOINTER(this->mSystem->mDSPMixBuff[this->mTreeLevel], 16));
    }

    if (target->mNumOutputs > 1) {
        if (!target->mOutputBuffer) {
            target->mOutputBuffer = static_cast<float*>(FMOD_Memory_Calloc((this->mSystem->mDSPBlockSize * (this->mSystem->mMaxOutputChannels < this->mSystem->mMaxInputChannels ? this->mSystem->mMaxInputChannels : this->mSystem->mMaxOutputChannels) * sizeof(float)) + 15));
            if (!target->mOutputBuffer) {
                return FMOD_ERR_MEMORY;
            }
        }
        target->mBuffer = reinterpret_cast<float*>(FMOD_ALIGNPOINTER(target->mOutputBuffer, 16));
    }

    crit.leave();
    crit2.leave();

    if (connection_out) {
        *connection_out = connection;
    }

    return FMOD_OK;
}

FMOD_RESULT DSPI::addInputQueued(
    DSPI* target,
    bool checkcircular,
    DSPConnection* connection_old,
    DSPConnection** connection_out) {
    FMOD_RESULT result;
    DSPConnection* connection;
    DSPConnectionRequest* request;

    if (!target) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if ((this->mDescription.mCategory == FMOD_DSP_CATEGORY_RESAMPLER && target->mNumOutputs) || target->mDescription.mCategory == FMOD_DSP_CATEGORY_SOUNDCARD) {
        return FMOD_ERR_DSP_CONNECTION;
    }

    if (checkcircular) {
        if (target->doesUnitExist(this) == FMOD_OK) {
            return FMOD_ERR_DSP_CONNECTION;
        }
    }

    result = this->mSystem->mDSPConnectionPool.alloc(&connection);
    if (result != FMOD_OK) {
        return result;
    }

    if (connection_old) {
        connection->copy(connection_old);
    } else {
        connection->reset();
    }

    FMOD_OS_CriticalSection_Enter(this->mSystem->mDSPRequestCrit);
    if (this->mSystem->mConnectionRequestFreeHead.isEmpty()) {
        this->mSystem->flushDSPConnectionRequests(true);
    }

    request = static_cast<DSPConnectionRequest*>(this->mSystem->mConnectionRequestFreeHead.getNext());
    request->removeNode();
    request->addBefore(&this->mSystem->mConnectionRequestUsedHead);

    request->mThis = this;
    request->mTarget = target;
    request->mConnection = connection;
    request->mRequest = DSPCONNECTION_REQUEST_ADDINPUT;
    FMOD_OS_CriticalSection_Leave(this->mSystem->mDSPRequestCrit);

    if (connection_out) {
        *connection_out = connection;
    }

    return FMOD_OK;
}

FMOD_RESULT DSPI::disconnectAllInternal(bool inputs, bool outputs) {
    FMOD_RESULT result;

    if (inputs) {
        int32_t numinputs;

        result = this->getNumInputs(&numinputs);
        if (result != FMOD_OK) {
            return result;
        }

        for (int32_t i = 0; i < numinputs; i++) {
            DSPI* input;

            result = this->getInput(0, nullptr, &input);
            if (result != FMOD_OK) {
                return result;
            }

            result = this->disconnectFromInternal(input);
            if (result != FMOD_OK) {
                return result;
            }
        }
    }

    if (outputs) {
        int32_t numoutputs;

        result = this->getNumOutputs(&numoutputs);
        if (result != FMOD_OK) {
            return result;
        }

        for (int32_t i = 0; i < numoutputs; i++) {
            DSPI* output;

            result = this->getOutput(0, nullptr, &output);
            if (result != FMOD_OK) {
                return result;
            }

            result = output->disconnectFromInternal(this);
            if (result != FMOD_OK) {
                return result;
            }
        }
    }

    return FMOD_OK;
}

FMOD_RESULT DSPI::disconnectAllQueued(bool inputs, bool outputs) {
    DSPConnectionRequest* request;

    if (!inputs && !outputs) {
        return FMOD_OK;
    }

    FMOD_OS_CriticalSection_Enter(this->mSystem->mDSPRequestCrit);

    if (this->mSystem->mConnectionRequestFreeHead.isEmpty()) {
        this->mSystem->flushDSPConnectionRequests(true);
    }

    request = static_cast<DSPConnectionRequest*>(this->mSystem->mConnectionRequestFreeHead.getNext());
    request->removeNode();
    request->addBefore(&this->mSystem->mConnectionRequestUsedHead);

    request->mThis = this;
    request->mTarget = 0;

    if (inputs && outputs) {
        request->mRequest = DSPCONNECTION_REQUEST_DISCONNECTALL;
    } else if (inputs) {
        request->mRequest = DSPCONNECTION_REQUEST_DISCONNECTALLINPUTS;
    } else {
        request->mRequest = DSPCONNECTION_REQUEST_DISCONNECTALLOUTPUTS;
    }

    FMOD_OS_CriticalSection_Leave(this->mSystem->mDSPRequestCrit);

    return FMOD_OK;
}

FMOD_RESULT DSPI::execute(
    float* inbuffer,
    float** outbuffer,
    uint32_t* length,
    int32_t inchannels,
    int32_t* outchannels,
    FMOD_SPEAKERMODE speakermode) {
    return FMOD_ERR_INTERNAL;
}

FMOD_RESULT DSPI::execute(
    void* inbuffer,
    void** outbuffer,
    uint32_t* length,
    int32_t inchannels,
    int32_t* outchannels,
    FMOD_SPEAKERMODE speakermode) {
    return this->execute(
        reinterpret_cast<float*>(inbuffer),
        reinterpret_cast<float**>(outbuffer),
        length,
        inchannels,
        outchannels,
        speakermode);
}

FMOD_RESULT DSPI::alloc(FMOD_DSP_DESCRIPTION_EX* description) {
    FMOD_RESULT result;
    int32_t outputchannels;
    int32_t rate;

    if (!description) {
        return FMOD_ERR_INVALID_PARAM;
    }
    if (description->channels < 0) {
        return FMOD_ERR_INVALID_PARAM;
    }

    result = this->mSystem->getSoftwareFormat(&rate, nullptr, &outputchannels, nullptr, nullptr, nullptr);
    if (result != FMOD_OK) {
        return result;
    }

    switch (description->mCategory) {
    case FMOD_DSP_CATEGORY_FILTER: {
        if (description->channels > outputchannels) {
            return FMOD_ERR_TOOMANYCHANNELS;
        }
        break;
    }
    case FMOD_DSP_CATEGORY_DSPCODECMPEG:
    case FMOD_DSP_CATEGORY_DSPCODECRAW:
    case FMOD_DSP_CATEGORY_DSPCODECXMA:
    case FMOD_DSP_CATEGORY_DSPCODECADPCM: {
        if (!description->channels) {
            return FMOD_ERR_INVALID_PARAM;
        }
        break;
    }
    case FMOD_DSP_CATEGORY_RESAMPLER: {
        if (description->channels) {
            return FMOD_ERR_INVALID_PARAM;
        }
        break;
    }
    case FMOD_DSP_CATEGORY_WAVETABLE: {
        break;
    }
    case FMOD_DSP_CATEGORY_SOUNDCARD: {
        break;
    }
    default: {
        return FMOD_ERR_INVALID_PARAM;
    }
    }

    FMOD_memcpy(&this->mDescription, description, sizeof(FMOD_DSP_DESCRIPTION_EX));

    this->mFlags &= ~FMOD_DSP_FLAG_ACTIVE;
    this->mFlags &= ~FMOD_DSP_FLAG_USEDADDDSP;

    return FMOD_OK;
}

FMOD_RESULT DSPI::setPosition(uint32_t position, bool processinputs) {
    if (processinputs) {
        FMOD_RESULT result;
        int32_t numinputs;

        numinputs = 0;

        result = this->getNumInputs(&numinputs);
        if (result != FMOD_OK) {
            return result;
        }

        for (int32_t i = 0; i < numinputs; i++) {
            DSPI* input = nullptr;

            this->getInput(i, nullptr, &input);

            input->setPosition(position, true);
        }
    }

    if (this->mDescription.setposition) {
        this->instance = reinterpret_cast<FMOD_DSP*>(this);

        return this->mDescription.setposition(static_cast<FMOD_DSP_STATE*>(this), position);
    }

    return FMOD_OK;
}

FMOD_RESULT DSPI::getSystemObject(SystemI** system) {
    if (!system) {
        return FMOD_ERR_INVALID_PARAM;
    }

    *system = this->mSystem;
    return FMOD_OK;
}

FMOD_RESULT DSPI::addInput(DSPI* target) {
    FMOD_RESULT result;
    DSPConnection* connection;
    DSPConnectionRequest* request;

    if (!this->mSystem) {
        return FMOD_ERR_UNINITIALIZED;
    }

    if (!target) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if ((this->mDescription.mCategory == FMOD_DSP_CATEGORY_RESAMPLER && target->mNumOutputs) || target->mDescription.mCategory == FMOD_DSP_CATEGORY_SOUNDCARD) {
        return FMOD_ERR_DSP_CONNECTION;
    }

    if (target->doesUnitExist(this) == FMOD_OK) {
        return FMOD_ERR_DSP_CONNECTION;
    }

    result = this->mSystem->mDSPConnectionPool.alloc(&connection);
    if (result != FMOD_OK) {
        return result;
    }

    connection->reset();

    FMOD_OS_CriticalSection_Enter(this->mSystem->mDSPRequestCrit);
    if (this->mSystem->mConnectionRequestFreeHead.isEmpty()) {
        this->mSystem->flushDSPConnectionRequests(true);
    }

    request = static_cast<DSPConnectionRequest*>(this->mSystem->mConnectionRequestFreeHead.getNext());
    request->removeNode();
    request->addBefore(&this->mSystem->mConnectionRequestUsedHead);

    request->mThis = this;
    request->mTarget = target;
    request->mConnection = connection;
    request->mRequest = DSPCONNECTION_REQUEST_ADDINPUT;
    FMOD_OS_CriticalSection_Leave(this->mSystem->mDSPRequestCrit);

    return FMOD_OK;
}

FMOD_RESULT DSPI::release(bool freethis) {
    FMOD_RESULT result;

    if (this->mSystem) {
        result = this->mSystem->stopDSP(this);
        if (result != FMOD_OK) {
            return result;
        }
    }

    result = this->remove();
    if (result != FMOD_OK) {
        result = this->disconnectFromQueued(nullptr);
        if (result != FMOD_OK) {
            return result;
        }
    }

    this->mSystem->flushDSPConnectionRequests(true);

    if (this->mOutputBuffer) {
        FMOD_Memory_Free(this->mOutputBuffer);
        this->mOutputBuffer = nullptr;
    }

    this->removeNode();

    if (this->mDescription.release) {
        this->instance = reinterpret_cast<FMOD_DSP*>(this);
        this->mDescription.release(static_cast<FMOD_DSP_STATE*>(this));
    }

    if (freethis) {
        FMOD_Memory_Free(this);
    }

    return FMOD_OK;
}

FMOD_RESULT DSPI::remove() {
    FMOD_RESULT result;
    int32_t numinputs;
    int32_t numoutputs;

    this->getNumInputs(&numinputs);

    result = this->getNumOutputs(&numoutputs);
    if (result != FMOD_OK) {
        return result;
    }

    if (numinputs == 1 && numoutputs == 1) {
        DSPI* input;
        DSPI* output;

        this->mFlags &= ~FMOD_DSP_FLAG_ACTIVE;

        result = this->getInput(0, nullptr, &input);
        if (result != FMOD_OK) {
            return result;
        }

        result = this->getOutput(0, nullptr, &output);
        if (result != FMOD_OK) {
            return result;
        }

        result = this->disconnectFromQueued(nullptr);
        if (result != FMOD_OK) {
            return result;
        }

        result = output->addInput(input);
        if (result != FMOD_OK) {
            return result;
        }
    } else {
        this->mFlags &= ~FMOD_DSP_FLAG_USEDADDDSP;

        return this->disconnectFromQueued(nullptr);
    }

    this->mFlags &= ~FMOD_DSP_FLAG_USEDADDDSP;

    return FMOD_OK;
}

FMOD_RESULT DSPI::reset() {
    if (!this->mDescription.reset) {
        return FMOD_ERR_UNSUPPORTED;
    }

    this->instance = reinterpret_cast<FMOD_DSP*>(this);

    return this->mDescription.reset(static_cast<FMOD_DSP_STATE*>(this));
}

FMOD_RESULT DSPI::setParameter(int32_t index, float value) {
    FMOD_RESULT result;

    if (!this->mDescription.setparameter) {
        return FMOD_ERR_UNSUPPORTED;
    }

    if (index < 0 || index > this->mDescription.numparameters) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (value < this->mDescription.paramdesc[index].min) {
        value = this->mDescription.paramdesc[index].min;
    }
    if (value > this->mDescription.paramdesc[index].max) {
        value = this->mDescription.paramdesc[index].max;
    }

    this->instance = reinterpret_cast<FMOD_DSP*>(this);

    result = this->mDescription.setparameter(static_cast<FMOD_DSP_STATE*>(this), index, value);
    if (result != FMOD_OK) {
        return result;
    }

    return FMOD_OK;
}

FMOD_RESULT DSPI::getParameter(
    int32_t index,
    float* value,
    char* valuestr,
    int32_t valuestrlen) {
    FMOD_RESULT result;
    float v;
    char s[32];

    if (!this->mDescription.getparameter) {
        return FMOD_ERR_UNSUPPORTED;
    }

    if (index < 0 || index > this->mDescription.numparameters) {
        return FMOD_ERR_INVALID_PARAM;
    }

    this->instance = reinterpret_cast<FMOD_DSP*>(this);

    result = this->mDescription.getparameter(static_cast<FMOD_DSP_STATE*>(this), index, &v, s);
    if (result != FMOD_OK) {
        return result;
    }

    if (value) {
        *value = v;
    }

    if (valuestr) {
        FMOD_strncpy(valuestr, s, valuestrlen > 16 ? 16 : valuestrlen);
    }

    return FMOD_OK;
}

FMOD_RESULT DSPI::getNumParameters(int32_t* numparams) {
    if (!numparams) {
        return FMOD_ERR_INVALID_PARAM;
    }

    *numparams = this->mDescription.numparameters;
    return FMOD_OK;
}

FMOD_RESULT DSPI::getParameterInfo(
    int32_t index,
    char* name,
    char* label,
    char* description,
    int32_t descriptionlen,
    float* min,
    float* max) {

    if (index < 0 || index >= this->mDescription.numparameters) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (name) {
        FMOD_strcpy(name, this->mDescription.paramdesc[index].name);
    }

    if (description && descriptionlen) {
        if (this->mDescription.paramdesc[index].description) {
            FMOD_strncpy(description, this->mDescription.paramdesc[index].description, descriptionlen);
        } else {
            *description = 0;
        }
    }
    if (label) {
        FMOD_strcpy(label, this->mDescription.paramdesc[index].label);
    }
    if (min) {
        *min = this->mDescription.paramdesc[index].min;
    }
    if (max) {
        *max = this->mDescription.paramdesc[index].max;
    }

    return FMOD_OK;
}

FMOD_RESULT DSPI::showConfigDialog(void* hwnd, bool show) {
    if (!this->mDescription.config) {
        return FMOD_ERR_UNSUPPORTED;
    }

    this->instance = reinterpret_cast<FMOD_DSP*>(this);

    return this->mDescription.config(static_cast<FMOD_DSP_STATE*>(this), hwnd, show);
}

FMOD_RESULT DSPI::getInfo(
    char* name,
    uint32_t* version,
    int32_t* channels,
    int32_t* configwidth,
    int32_t* configheight) {

    if (name) {
        FMOD_strncpy(name, this->mDescription.name, 32);
    }

    if (version) {
        *version = this->mDescription.version;
    }

    if (channels) {
        *channels = this->mDescription.channels;
    }

    if (configwidth) {
        *configwidth = this->mDescription.configwidth;
    }

    if (configheight) {
        *configheight = this->mDescription.configheight;
    }

    return FMOD_OK;
}

FMOD_RESULT DSPI::getType(FMOD_DSP_TYPE* type) {
    if (type) {
        *type = this->mDescription.mType;
    }

    return FMOD_OK;
}

FMOD_RESULT DSPI::setDefaults(
    float frequency,
    float volume,
    float pan,
    int32_t priority) {

    if (volume > 1.0f) {
        volume = 1.0f;
    }
    if (volume < 0.0f) {
        volume = 0.0f;
    }
    if (pan < -1.0f) {
        pan = -1.0f;
    }
    if (pan > 1.0f) {
        pan = 1.0f;
    }
    if (priority < 0) {
        priority = 0;
    }
    if (priority > 256) {
        priority = 256;
    }

    this->mDefaultFrequency = frequency;
    this->mDefaultVolume = volume;
    this->mDefaultPan = pan;
    this->mDefaultPriority = priority;

    return FMOD_OK;
}

FMOD_RESULT DSPI::getDefaults(
    float* frequency,
    float* volume,
    float* pan,
    int32_t* priority) {

    if (frequency) {
        *frequency = this->mDefaultFrequency;
    }
    if (volume) {
        *volume = this->mDefaultVolume;
    }
    if (pan) {
        *pan = this->mDefaultPan;
    }
    if (priority) {
        *priority = this->mDefaultPriority;
    }

    return FMOD_OK;
}

FMOD_RESULT DSPI::setTargetFrequency(int32_t frequency) {
    this->mTargetFrequency = frequency;

    return FMOD_OK;
}

FMOD_RESULT DSPI::getTargetFrequency(int32_t* frequency) {
    if (!frequency) {
        return FMOD_ERR_INVALID_PARAM;
    }

    *frequency = this->mTargetFrequency;
    return FMOD_OK;
}

FMOD_RESULT DSPI::validate(DSP* dsp, DSPI** dspi) {
    if (!dspi) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (!dsp) {
        return FMOD_ERR_INVALID_HANDLE;
    }

    *dspi = reinterpret_cast<DSPI*>(dsp);

    return FMOD_OK;
}

FMOD_RESULT DSPI::calculateSpeakerLevels(
    float frontleft,
    float frontright,
    float center,
    float lfe,
    float backleft,
    float backright,
    float sideleft,
    float sideright,
    FMOD_SPEAKERMODE speakermode,
    int32_t channels,
    FMOD_SPEAKERMAPTYPE speakermap,
    float* outlevels,
    int32_t* numinputlevels) {
    *numinputlevels = 0;

    switch (speakermode) {
    case FMOD_SPEAKERMODE_RAW: {
        switch (channels) {
        case 1: {
            break;
        }
        case 2: {
            break;
        }
        case 4: {
            break;
        }
        case 6: {
            break;
        }
        default: {
            break;
        }
        }
        break;
    }
    case FMOD_SPEAKERMODE_MONO: {
        switch (channels) {
        case 1: {
            float levels[1][1] = {
                { FMOD_SQRT(frontleft * frontleft + center * center * 0.5f + backleft * backleft * 0.25f) },
            };

            FMOD_memcpy(outlevels, levels, sizeof(levels));
            *numinputlevels = 1;
            break;
        }
        case 2: {
            float levels[1][2] = {
                {
                    FMOD_SQRT(frontleft * frontleft + center * center * 0.5f + backleft * backleft * 0.25f),
                    0,
                },
            };

            FMOD_memcpy(outlevels, levels, sizeof(levels));
            *numinputlevels = 1;
            break;
        }
        case 4: {
            float levels[1][4] = {
                { frontleft, 0, backleft * 0.5f, 0 },
            };

            FMOD_memcpy(outlevels, levels, sizeof(levels));
            *numinputlevels = 4;
            break;
        }
        case 6: {
            float levels[1][6] = {
                { frontleft, frontright, center, lfe, backleft, backright }
            };

            FMOD_memcpy(outlevels, levels, sizeof(levels));
            *numinputlevels = 6;
            break;
        }
        default: {
            break;
        }
        }
        break;
    }
    case FMOD_SPEAKERMODE_STEREO: {
        if (speakermap == FMOD_SPEAKERMAPTYPE_ALLMONO) {
            int32_t count;

            FMOD_memset(outlevels, 0, 2 * channels * sizeof(float));

            for (count = 0; count < channels; count++) {
                outlevels[(channels * FMOD_SPEAKER_FRONT_LEFT) + count] = center * .707f;
                outlevels[(channels * FMOD_SPEAKER_FRONT_RIGHT) + count] = center * .707f;
            }

            *numinputlevels = channels;
            break;
        } else if (speakermap == FMOD_SPEAKERMAPTYPE_ALLSTEREO || channels > 8) {
            int32_t count;

            FMOD_memset(outlevels, 0, 2 * channels * sizeof(float));

            for (count = 0; count < channels; count++) {
                if (count & 1) {
                    outlevels[(channels * FMOD_SPEAKER_FRONT_RIGHT) + count] = frontright;
                } else {
                    outlevels[(channels * FMOD_SPEAKER_FRONT_LEFT) + count] = frontleft;
                }
            }

            *numinputlevels = channels;
            break;
        }

        switch (channels) {
        case 1: {
            float levels[2][1] = {
                { FMOD_SQRT(frontleft * frontleft + center * center * 0.5f + backleft * backleft * 0.25f) },
                { FMOD_SQRT(frontright * frontright + center * center * 0.5f + backright * backright * 0.25f) }
            };

            FMOD_memcpy(outlevels, levels, sizeof(levels));
            *numinputlevels = 1;
            break;
        }
        case 2: {
            float levels[2][2] = {
                { FMOD_SQRT(frontleft * frontleft + center * center * 0.5f + backleft * backleft * 0.25f),
                  0 },
                { 0,
                  FMOD_SQRT(frontright * frontright + center * center * 0.5f + backright * backright * 0.25f) }
            };

            FMOD_memcpy(outlevels, levels, sizeof(levels));
            *numinputlevels = 2;
            break;
        }
        case 4: {
            float levels[2][4] = {
                { frontleft, 0, backleft * 0.5f, 0 },
                { 0, frontright, 0, backright * 0.5f },
            };
            FMOD_memcpy(outlevels, levels, sizeof(levels));
            *numinputlevels = 4;
            break;
        }
        case 6: {
            float levels[2][6] = {
                { frontleft, 0, center * 0.5f, lfe * 0.5f, backleft * 0.5f, 0 },
                { 0, frontright, center * 0.5f, lfe * 0.5f, 0, backright * 0.5f },
            };
            FMOD_memcpy(outlevels, levels, sizeof(levels));

            *numinputlevels = 6;
            break;
        }
        default: {
            break;
        }
        }
        break;
    }
    case FMOD_SPEAKERMODE_QUAD: {
        if (speakermap == FMOD_SPEAKERMAPTYPE_ALLMONO) {
            int32_t count;

            FMOD_memset(outlevels, 0, 4 * channels * sizeof(float));

            for (count = 0; count < channels; count++) {
                outlevels[(channels * FMOD_SPEAKER_FRONT_LEFT) + count] = center * .707f;
                outlevels[(channels * FMOD_SPEAKER_FRONT_RIGHT) + count] = center * .707f;
            }

            *numinputlevels = channels;
            break;
        } else if (speakermap == FMOD_SPEAKERMAPTYPE_ALLSTEREO || channels > 8) {
            int32_t count;

            FMOD_memset(outlevels, 0, 4 * channels * sizeof(float));

            for (count = 0; count < channels; count++) {
                if (count & 1) {
                    outlevels[(channels * FMOD_SPEAKER_FRONT_RIGHT) + count] = frontright;
                } else {
                    outlevels[(channels * FMOD_SPEAKER_FRONT_LEFT) + count] = frontleft;
                }
            }

            *numinputlevels = channels;
            break;
        }

        switch (channels) {
        case 1: {
            float levels[4][1] = {
                { frontleft + center },
                { frontright + center },
                { backleft },
                { backright }
            };

            FMOD_memcpy(outlevels, levels, sizeof(levels));
            *numinputlevels = 1;
            break;
        }
        case 2: {
            float levels[4][2] = {
                { frontleft, 0 },
                { 0, frontright },
                { backleft, 0 },
                { 0, backright }
            };

            FMOD_memcpy(outlevels, levels, sizeof(levels));
            *numinputlevels = 2;
            break;
        }
        case 4: {
            float levels[4][4] = {
                { frontleft, 0, 0, 0 },
                { 0, frontright, 0, 0 },
                { 0, 0, backleft * 0.5f, 0 },
                { 0, 0, 0, backright * 0.5f }
            };
            FMOD_memcpy(outlevels, levels, sizeof(levels));
            *numinputlevels = 4;
            break;
        }
        case 6: {
            float levels[4][6] = {
                { frontleft, 0, center * 0.5f, lfe * 0.25f, 0, 0 },
                { 0, frontright, center * 0.5f, lfe * 0.25f, 0, 0 },
                { 0, 0, 0, lfe * 0.25f, backleft, 0 },
                { 0, 0, 0, lfe * 0.25f, 0, backright },
            };
            FMOD_memcpy(outlevels, levels, sizeof(levels));

            *numinputlevels = 6;
            break;
        }
        default: {
            break;
        }
        }
        break;
    }
    case FMOD_SPEAKERMODE_SURROUND: {
        if (speakermap == FMOD_SPEAKERMAPTYPE_ALLMONO) {
            int32_t count;

            FMOD_memset(outlevels, 0, 5 * channels * sizeof(float));

            for (count = 0; count < channels; count++) {
                outlevels[(channels * FMOD_SPEAKER_FRONT_CENTER) + count] = center;
            }

            *numinputlevels = channels;
            break;
        } else if (speakermap == FMOD_SPEAKERMAPTYPE_ALLSTEREO || channels > 8) {
            int32_t count;

            FMOD_memset(outlevels, 0, 5 * channels * sizeof(float));

            for (count = 0; count < channels; count++) {
                if (count & 1) {
                    outlevels[(channels * FMOD_SPEAKER_FRONT_RIGHT) + count] = frontright;
                } else {
                    outlevels[(channels * FMOD_SPEAKER_FRONT_LEFT) + count] = frontleft;
                }
            }

            *numinputlevels = channels;
            break;
        }

        switch (channels) {
        case 1: {
            float levels[5][1] = {
                { frontleft },
                { frontright },
                { center },
                { backleft },
                { backright }
            };

            FMOD_memcpy(outlevels, levels, sizeof(levels));
            *numinputlevels = 1;
            break;
        }
        case 2: {
            float levels[5][2] = {
                { frontleft, 0 },
                { 0, frontright },
                { center * 0.5f, center * 0.5f },
                { backleft, 0 },
                { 0, backright },
            };

            FMOD_memcpy(outlevels, levels, sizeof(levels));
            *numinputlevels = 2;
            break;
        }
        case 4: {
            float levels[5][4] = {
                { frontleft, 0, 0, 0 },
                { 0, frontright, 0, 0 },
                { center * 0.5f, center * 0.5f, 0, 0 },
                { 0, 0, backleft, 0 },
                { 0, 0, 0, backright }
            };
            FMOD_memcpy(outlevels, levels, sizeof(levels));
            *numinputlevels = 4;
            break;
        }
        case 6: {
            float levels[5][6] = {
                { frontleft, 0, 0, 0, 0, 0 },
                { 0, frontright, 0, 0, 0, 0 },
                { 0, 0, center, 0, 0, 0 },
                { 0, 0, 0, 0, backleft, 0 },
                { 0, 0, 0, 0, 0, backright }
            };
            FMOD_memcpy(outlevels, levels, sizeof(levels));

            *numinputlevels = 6;
            break;
        }
        default: {
            break;
        }
        }
        break;
    }
    case FMOD_SPEAKERMODE_5POINT1: {
        if (speakermap == FMOD_SPEAKERMAPTYPE_ALLMONO) {
            int32_t count;

            FMOD_memset(outlevels, 0, 6 * channels * sizeof(float));

            for (count = 0; count < channels; count++) {
                outlevels[(channels * FMOD_SPEAKER_FRONT_CENTER) + count] = center;
            }

            *numinputlevels = channels;
            break;
        } else if (speakermap == FMOD_SPEAKERMAPTYPE_ALLSTEREO || channels > 8) {
            int32_t count;

            FMOD_memset(outlevels, 0, 6 * channels * sizeof(float));

            for (count = 0; count < channels; count++) {
                if (count & 1) {
                    outlevels[(channels * FMOD_SPEAKER_FRONT_RIGHT) + count] = frontright;
                } else {
                    outlevels[(channels * FMOD_SPEAKER_FRONT_LEFT) + count] = frontleft;
                }
            }

            *numinputlevels = channels;
            break;
        }

        switch (channels) {
        case 1: {
            float levels[6][1] = {
                { frontleft },
                { frontright },
                { center },
                { lfe },
                { backleft },
                { backright }
            };

            FMOD_memcpy(outlevels, levels, sizeof(levels));
            *numinputlevels = 1;
            break;
        }
        case 2: {
            float levels[6][2] = {
                { frontleft, 0 },
                { 0, frontright },
                { center * 0.5f, center * 0.5f },
                { lfe * 0.5f, lfe * 0.5f },
                { backleft, 0 },
                { 0, backright },
            };

            FMOD_memcpy(outlevels, levels, sizeof(levels));
            *numinputlevels = 2;
            break;
        }
        case 4: {
            float levels[6][4] = {
                { frontleft, 0, 0, 0 },
                { 0, frontright, 0, 0 },
                { center * 0.5f, center * 0.5f, 0, 0 },
                { lfe * 0.5f, lfe * 0.5f, 0, 0 },
                { 0, 0, backleft, 0 },
                { 0, 0, 0, backright }
            };
            FMOD_memcpy(outlevels, levels, sizeof(levels));
            *numinputlevels = 4;
            break;
        }
        case 6: {
            float levels[6][6] = {
                { frontleft, 0, 0, 0, 0, 0 },
                { 0, frontright, 0, 0, 0, 0 },
                { 0, 0, center, 0, 0, 0 },
                { 0, 0, 0, lfe, 0, 0 },
                { 0, 0, 0, 0, backleft, 0 },
                { 0, 0, 0, 0, 0, backright }
            };
            FMOD_memcpy(outlevels, levels, sizeof(levels));

            *numinputlevels = 6;
            break;
        }
        default: {
            break;
        }
        }
        break;
    }
    case FMOD_SPEAKERMODE_7POINT1: {
        if (speakermap == FMOD_SPEAKERMAPTYPE_ALLMONO) {
            int32_t count;

            FMOD_memset(outlevels, 0, 8 * channels * sizeof(float));

            for (count = 0; count < channels; count++) {
                outlevels[(channels * FMOD_SPEAKER_FRONT_CENTER) + count] = center;
            }

            *numinputlevels = channels;
            break;
        } else if (speakermap == FMOD_SPEAKERMAPTYPE_ALLSTEREO || channels > 8) {
            int32_t count;

            FMOD_memset(outlevels, 0, 8 * channels * sizeof(float));

            for (count = 0; count < channels; count++) {
                if (count & 1) {
                    outlevels[(channels * FMOD_SPEAKER_FRONT_RIGHT) + count] = frontright;
                } else {
                    outlevels[(channels * FMOD_SPEAKER_FRONT_LEFT) + count] = frontleft;
                }
            }

            *numinputlevels = channels;
            break;
        }

        switch (channels) {
        case 1: {
            float levels[8][1] = {
                { frontleft },
                { frontright },
                { center },
                { lfe },
                { backleft },
                { backright },
                { sideleft },
                { sideright }
            };

            FMOD_memcpy(outlevels, levels, sizeof(levels));
            *numinputlevels = 1;
            break;
        }
        case 2: {
            float levels[8][2] = {
                { frontleft, 0 },
                { 0, frontright },
                { center * 0.5f, center * 0.5f },
                { lfe * 0.5f, lfe * 0.5f },
                { backleft, 0 },
                { 0, backright },
                { sideleft, 0 },
                { 0, sideright }
            };

            FMOD_memcpy(outlevels, levels, sizeof(levels));
            *numinputlevels = 2;
            break;
        }
        case 4: {
            float levels[8][4] = {

                { frontleft, 0, 0, 0 },
                { 0, frontright, 0, 0 },
                { center * 0.5f, center * 0.5f, 0, 0 },
                { lfe * 0.5f, lfe * 0.5f, 0, 0 },
                { 0, 0, backleft * 0.5f, 0 },
                { 0, 0, 0, backright * 0.5f },
                { 0, 0, sideleft * 0.5f, 0 },
                { 0, 0, 0, sideright * 0.5f },
            };

            FMOD_memcpy(outlevels, levels, sizeof(levels));
            *numinputlevels = 4;
            break;
        }
        case 6: {
            float levels[8][6] = {
                { frontleft, 0, 0, 0, 0, 0 },
                { 0, frontright, 0, 0, 0, 0 },
                { 0, 0, center, 0, 0, 0 },
                { 0, 0, 0, lfe, 0, 0 },
                { 0, 0, 0, 0, backleft, 0 },
                { 0, 0, 0, 0, 0, backright },
                { sideleft * 0.5f, 0, 0, 0, sideleft * 0.5f, 0 },
                { 0, sideright * 0.5f, 0, 0, 0, sideleft * 0.5f },
            };

            FMOD_memcpy(outlevels, levels, sizeof(levels));

            *numinputlevels = 6;
            break;
        }
        case 8: {
            float levels[8][8] = {
                { frontleft, 0, 0, 0, 0, 0, 0, 0 },
                { 0, frontright, 0, 0, 0, 0, 0, 0 },
                { 0, 0, center, 0, 0, 0, 0, 0 },
                { 0, 0, 0, lfe, 0, 0, 0, 0 },
                { 0, 0, 0, 0, backleft, 0, 0, 0 },
                { 0, 0, 0, 0, 0, backright, 0, 0 },
                { 0, 0, 0, 0, 0, 0, sideleft, 0 },
                { 0, 0, 0, 0, 0, 0, 0, sideright },
            };

            FMOD_memcpy(outlevels, levels, sizeof(levels));
            *numinputlevels = 8;
            break;
        }
        default: {
            break;
        }
        }
        break;
    }
    case FMOD_SPEAKERMODE_PROLOGIC: {
        if (speakermap == FMOD_SPEAKERMAPTYPE_ALLMONO) {
            int32_t count;

            FMOD_memset(outlevels, 0, 2 * channels * sizeof(float));

            for (count = 0; count < channels; count++) {
                outlevels[(channels * FMOD_SPEAKER_FRONT_LEFT) + count] = center * .707f;
                outlevels[(channels * FMOD_SPEAKER_FRONT_RIGHT) + count] = center * .707f;
            }

            *numinputlevels = channels;
            break;
        } else if (speakermap == FMOD_SPEAKERMAPTYPE_ALLSTEREO || channels > 8) {
            int32_t count;

            FMOD_memset(outlevels, 0, 2 * channels * sizeof(float));

            for (count = 0; count < channels; count++) {
                if (count & 1) {
                    outlevels[(channels * FMOD_SPEAKER_FRONT_RIGHT) + count] = frontright;
                } else {
                    outlevels[(channels * FMOD_SPEAKER_FRONT_LEFT) + count] = frontleft;
                }
            }

            *numinputlevels = channels;
            break;
        }

        float fl2 = frontleft * frontleft;
        float fr2 = frontright * frontright;
        float c2 = center * center;
        float bl2 = backleft * backleft;
        float br2 = backright * backright;

        switch (channels) {
        case 1: {
            float levels[2] = {
                fl2 + (c2 * 0.707f) + (-0.872f * bl2) + (-0.49f * br2),
                fr2 + (c2 * 0.707f) + (0.49f * bl2) + (0.872f * br2)
            };

            FMOD_memcpy(outlevels, levels, sizeof(levels));
            *numinputlevels = 1;
            break;
        }
        case 2: {
            float levels[2][2] = {
                { fl2 + (c2 * 0.707f) + (-0.872f * bl2) + (-0.49f * br2), fl2 + (c2 * 0.707f) + (-0.872f * bl2) + (-0.49f * br2) },
                { fr2 + (c2 * 0.707f) + (0.49f * bl2) + (0.872f * br2), fl2 + (c2 * 0.707f) + (0.49f * bl2) + (0.872f * br2) }
            };

            FMOD_memcpy(outlevels, levels, sizeof(levels));
            *numinputlevels = 2;
            break;
        }
        case 4: {
            float levels[2][4] = {
                { fl2, 0, 0.5f * bl2, 0 },
                { 0, fr2, 0, 0.5f * br2 }
            };

            FMOD_memcpy(outlevels, levels, sizeof(levels));
            *numinputlevels = 4;
            break;
        }
        case 6: {
            float levels[2][6] = {
                { fl2, 0, c2 * 0.5f, lfe * 0.5f, bl2, 0 },
                { 0, fr2, c2 * 0.5f, lfe * 0.5f, 0, br2 },
            };

            FMOD_memcpy(outlevels, levels, sizeof(levels));

            *numinputlevels = 6;
            break;
        }
        default: {
            break;
        }
        }
        break;
    }
    default: {
        break;
    }
    }

    return FMOD_OK;
}

FMOD_RESULT DSPI::convert(
    void* outbuffer,
    void* inbuffer,
    FMOD_SOUND_FORMAT outformat,
    FMOD_SOUND_FORMAT informat,
    uint32_t length,
    int32_t destchannelstep,
    int32_t srcchannelstep,
    float volume) {

    bool supportssimd;

    // TODO:
    // supportssimd = FMOD_OS_SupportsSIMD();

    supportssimd = false;

    if (outformat == FMOD_SOUND_FORMAT_PCMFLOAT) {
        auto destptr = reinterpret_cast<float*>(outbuffer);

        switch (informat) {
        case FMOD_SOUND_FORMAT_PCM8: {
            auto srcptr = reinterpret_cast<int8_t*>(inbuffer);
            uint32_t len;

            volume /= static_cast<float>(1 << 7);

            len = length >> 3;
            while (len) {
                destptr[0 * destchannelstep] = static_cast<float>(srcptr[0 * srcchannelstep]) * volume;
                destptr[1 * destchannelstep] = static_cast<float>(srcptr[1 * srcchannelstep]) * volume;
                destptr[2 * destchannelstep] = static_cast<float>(srcptr[2 * srcchannelstep]) * volume;
                destptr[3 * destchannelstep] = static_cast<float>(srcptr[3 * srcchannelstep]) * volume;
                destptr[4 * destchannelstep] = static_cast<float>(srcptr[4 * srcchannelstep]) * volume;
                destptr[5 * destchannelstep] = static_cast<float>(srcptr[5 * srcchannelstep]) * volume;
                destptr[6 * destchannelstep] = static_cast<float>(srcptr[6 * srcchannelstep]) * volume;
                destptr[7 * destchannelstep] = static_cast<float>(srcptr[7 * srcchannelstep]) * volume;
                destptr += (8 * destchannelstep);
                srcptr += (8 * srcchannelstep);
                len--;
            }
            len = length & 7;
            while (len) {
                destptr[0] = static_cast<float>(srcptr[0 * srcchannelstep]) * volume;
                destptr += destchannelstep;
                srcptr += srcchannelstep;
                len--;
            }
            break;
        }
        case FMOD_SOUND_FORMAT_PCM16: {
            auto srcptr = reinterpret_cast<int16_t*>(inbuffer);
            uint32_t len;

            volume /= static_cast<float>(1 << 15);

            len = length >> 3;
            while (len) {
                destptr[0 * destchannelstep] = static_cast<float>(srcptr[0 * srcchannelstep]) * volume;
                destptr[1 * destchannelstep] = static_cast<float>(srcptr[1 * srcchannelstep]) * volume;
                destptr[2 * destchannelstep] = static_cast<float>(srcptr[2 * srcchannelstep]) * volume;
                destptr[3 * destchannelstep] = static_cast<float>(srcptr[3 * srcchannelstep]) * volume;
                destptr[4 * destchannelstep] = static_cast<float>(srcptr[4 * srcchannelstep]) * volume;
                destptr[5 * destchannelstep] = static_cast<float>(srcptr[5 * srcchannelstep]) * volume;
                destptr[6 * destchannelstep] = static_cast<float>(srcptr[6 * srcchannelstep]) * volume;
                destptr[7 * destchannelstep] = static_cast<float>(srcptr[7 * srcchannelstep]) * volume;
                destptr += (8 * destchannelstep);
                srcptr += (8 * srcchannelstep);
                len--;
            }
            len = length & 7;
            while (len) {
                destptr[0] = static_cast<float>(srcptr[0]) * volume;
                destptr += destchannelstep;
                srcptr += srcchannelstep;
                len--;
            }
            break;
        }
        case FMOD_SOUND_FORMAT_PCM24: {
            auto srcptr = reinterpret_cast<FMOD_INT24*>(inbuffer);
            uint32_t count;

            volume /= static_cast<float>(1 << 23);

            for (count = 0; count < length; count++) {
                int32_t val;

                val = (static_cast<uint32_t>(srcptr->val[0 * srcchannelstep]) << 8);
                val |= (static_cast<uint32_t>(srcptr->val[1 * srcchannelstep]) << 16);
                val |= (static_cast<uint32_t>(srcptr->val[2 * srcchannelstep]) << 24);
                val >>= 8;

                destptr[0] = static_cast<float>(val) * volume;
                srcptr += srcchannelstep;
                destptr += destchannelstep;
            }
            break;
        }
        case FMOD_SOUND_FORMAT_PCM32: {
            auto srcptr = reinterpret_cast<int32_t*>(inbuffer);
            uint32_t len;

            volume /= static_cast<float>(1U << 31);

            len = length >> 2;
            while (len) {
                destptr[0 * destchannelstep] = static_cast<float>(srcptr[0 * srcchannelstep]) * volume;
                destptr[1 * destchannelstep] = static_cast<float>(srcptr[1 * srcchannelstep]) * volume;
                destptr[2 * destchannelstep] = static_cast<float>(srcptr[2 * srcchannelstep]) * volume;
                destptr[3 * destchannelstep] = static_cast<float>(srcptr[3 * srcchannelstep]) * volume;
                destptr += (4 * destchannelstep);
                srcptr += (4 * srcchannelstep);
                len--;
            }
            len = length & 3;
            while (len) {
                destptr[0] = static_cast<float>(srcptr[0]) * volume;
                destptr += destchannelstep;
                srcptr += srcchannelstep;
                len--;
            }
            break;
        }
        case FMOD_SOUND_FORMAT_PCMFLOAT: {
            auto srcptr = reinterpret_cast<float*>(inbuffer);
            uint32_t len;

            len = length >> 2;
            while (len) {
                destptr[0 * destchannelstep] = srcptr[0 * srcchannelstep] * volume;
                destptr[1 * destchannelstep] = srcptr[1 * srcchannelstep] * volume;
                destptr[2 * destchannelstep] = srcptr[2 * srcchannelstep] * volume;
                destptr[3 * destchannelstep] = srcptr[3 * srcchannelstep] * volume;
                destptr += (4 * destchannelstep);
                srcptr += (4 * srcchannelstep);
                len--;
            }

            len = length & 3;
            while (len) {
                destptr[0] = srcptr[0] * volume;
                destptr += destchannelstep;
                srcptr += srcchannelstep;
                len--;
            }
            break;
        }
        default: {
            break;
        }
        }
    } else {
        auto srcptr = reinterpret_cast<float*>(inbuffer);

        if (informat != FMOD_SOUND_FORMAT_PCMFLOAT) {
            return FMOD_ERR_DSP_FORMAT;
        }

        switch (outformat) {
        case FMOD_SOUND_FORMAT_PCM8: {
            auto destptr = reinterpret_cast<int8_t*>(outbuffer);
            uint32_t len;

            volume *= static_cast<float>(1 << 7);

            len = length >> 2;
            while (len) {
                int32_t val[4];

                val[0] = static_cast<int32_t>(srcptr[0 * srcchannelstep] * volume);
                val[1] = static_cast<int32_t>(srcptr[1 * srcchannelstep] * volume);
                val[2] = static_cast<int32_t>(srcptr[2 * srcchannelstep] * volume);
                val[3] = static_cast<int32_t>(srcptr[3 * srcchannelstep] * volume);

                destptr[0 * destchannelstep] = val[0] < -128 ? -128 : val[0] > 127 ? 127
                                                                                   : static_cast<int8_t>(val[0]);
                destptr[1 * destchannelstep] = val[1] < -128 ? -128 : val[1] > 127 ? 127
                                                                                   : static_cast<int8_t>(val[1]);
                destptr[2 * destchannelstep] = val[2] < -128 ? -128 : val[2] > 127 ? 127
                                                                                   : static_cast<int8_t>(val[2]);
                destptr[3 * destchannelstep] = val[3] < -128 ? -128 : val[3] > 127 ? 127
                                                                                   : static_cast<int8_t>(val[3]);

                srcptr += (4 * srcchannelstep);
                destptr += (4 * destchannelstep);
                len--;
            }

            len = length & 3;
            while (len) {
                int32_t val;

                val = static_cast<int32_t>(srcptr[0] * volume);

                destptr[0] = val < -128 ? -128 : val > 127 ? 127
                                                           : static_cast<int8_t>(val);

                srcptr += srcchannelstep;
                destptr += destchannelstep;
                len--;
            }
            break;
        }
        case FMOD_SOUND_FORMAT_PCM16: {
            volume *= static_cast<float>(1 << 15);

            if (supportssimd) {
                // TODO:
                //
                // FMOD_DSP_Convert_FloatToPCM16(reinterpret_cast<int16_t*>(outbuffer), reinterpret_cast<float*>(inbuffer), length, destchannelstep, srcchannelstep, volume);
            } else {
                auto destptr = reinterpret_cast<int16_t*>(outbuffer);
                uint32_t len;

                len = length >> 2;
                while (len) {
                    int32_t val[4];

                    val[0] = static_cast<int32_t>(srcptr[0 * srcchannelstep] * volume);
                    val[1] = static_cast<int32_t>(srcptr[1 * srcchannelstep] * volume);
                    val[2] = static_cast<int32_t>(srcptr[2 * srcchannelstep] * volume);
                    val[3] = static_cast<int32_t>(srcptr[3 * srcchannelstep] * volume);

                    destptr[0 * destchannelstep] = val[0] < -32768 ? -32768 : val[0] > 32767 ? 32767
                                                                                             : static_cast<int16_t>(val[0]);
                    destptr[1 * destchannelstep] = val[1] < -32768 ? -32768 : val[1] > 32767 ? 32767
                                                                                             : static_cast<int16_t>(val[1]);
                    destptr[2 * destchannelstep] = val[2] < -32768 ? -32768 : val[2] > 32767 ? 32767
                                                                                             : static_cast<int16_t>(val[2]);
                    destptr[3 * destchannelstep] = val[3] < -32768 ? -32768 : val[3] > 32767 ? 32767
                                                                                             : static_cast<int16_t>(val[3]);

                    srcptr += (4 * srcchannelstep);
                    destptr += (4 * destchannelstep);
                    len--;
                }

                len = length & 3;
                while (len) {
                    int32_t val;

                    val = static_cast<int32_t>(srcptr[0] * volume);

                    destptr[0] = val < -32768 ? -32768 : val > 32767 ? 32767
                                                                     : static_cast<int16_t>(val);

                    srcptr += srcchannelstep;
                    destptr += destchannelstep;
                    len--;
                }
            }
            break;
        }
        case FMOD_SOUND_FORMAT_PCM24: {
            auto destptr = reinterpret_cast<FMOD_INT24*>(outbuffer);
            uint32_t count;

            volume *= static_cast<float>(1 << 23);

            for (count = 0; count < length; count++) {
                int32_t val;

                val = static_cast<int32_t>(srcptr[0] * volume);
                val = val < -8388608 ? -8388608 : val > 8388607 ? 8388607
                                                                : val;

                destptr->val[0] = (val >> 0) & 0xFF;
                destptr->val[1] = (val >> 8) & 0xFF;
                destptr->val[2] = (val >> 16) & 0xFF;
                destptr += destchannelstep;
                srcptr += srcchannelstep;
            }
            break;
        }
        case FMOD_SOUND_FORMAT_PCM32: {
            auto destptr = reinterpret_cast<int32_t*>(outbuffer);
            uint32_t count;

            volume *= static_cast<float>(1U << 31);

            for (count = 0; count < length; count++) {
                float val = srcptr[0] * volume;
                destptr[0] = val < -2147483648.0f ? -static_cast<int32_t>(2147483647) : val > 2147483647.0f ? 2147483647
                                                                                                            : static_cast<int32_t>(val);

                destptr += destchannelstep;
                srcptr += srcchannelstep;
            }
            break;
        }
        case FMOD_SOUND_FORMAT_PCMFLOAT: {
            auto destptr = reinterpret_cast<float*>(outbuffer);
            uint32_t count;

            for (count = 0; count < length; count++) {
                float val = srcptr[0] * volume;
                destptr[0] = val < -1.0f ? -1.0f : val > 1.0f ? 1.0f
                                                              : val;

                destptr += destchannelstep;
                srcptr += srcchannelstep;
            }
            break;
        }
        default: {
            break;
        }
        }
    }

    return FMOD_OK;
}

} // namespace FMOD
