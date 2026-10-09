#include "xmodex/output/OutputPolled.hpp"
#include "xmodex/SystemI.hpp"
#include "xmodex/Thread.hpp"

namespace FMOD {

OutputPolled::OutputPolled() {
    this->mCursorBlock = 0;
}

FMOD_RESULT OutputPolled::start() {
    FMOD_RESULT result;

    if (this->mSystem->mFlags & FMOD_INIT_SYNCMIXERWITHUPDATE) {
        this->mPolledFromMainThread = true;
    }

    if (this->mPolledFromMainThread) {
        result = this->initThread("FMOD mixer thread", nullptr, nullptr, PRIORITY_CRITICAL, nullptr, 32768, true, 0);
        if (result != FMOD_OK) {
            return result;
        }

        result = FMOD_OS_Semaphore_Create(&this->mFinishedSema);
        if (result != FMOD_OK) {
            return result;
        }
    } else {
        uint32_t blocksize;
        int32_t rate;
        float ms;

        result = this->mSystem->getDSPBufferSize(&blocksize, nullptr);
        if (result != FMOD_OK) {
            return result;
        }

        result = this->mSystem->getSoftwareFormat(&rate, nullptr, nullptr, nullptr, nullptr, nullptr);
        if (result != FMOD_OK) {
            return result;
        }

        ms = static_cast<float>(blocksize) * 1000.0f / static_cast<float>(rate);
        if (ms < 20.0f) {
            ms /= 3.0f;
            if (ms < 1.0f) {
                ms = 1.0f;
            }
        } else {
            ms = 10.0f;
        }

        result = this->initThread("FMOD mixer thread", nullptr, nullptr, PRIORITY_CRITICAL, nullptr, 32768, false, static_cast<int32_t>(ms));
        if (result != FMOD_OK) {
            return result;
        }
    }

    return FMOD_OK;
}

FMOD_RESULT OutputPolled::stop() {
    FMOD_RESULT result;

    result = this->closeThread();
    if (result != FMOD_OK) {
        return result;
    }

    if (this->mFinishedSema) {
        result = FMOD_OS_Semaphore_Free(this->mFinishedSema);
        if (result != FMOD_OK) {
            return result;
        }
    }

    return FMOD_OK;
}

FMOD_RESULT OutputPolled::threadFunc() {
    FMOD_RESULT result;
    FMOD_SOUND_FORMAT outputformat;
    int32_t numblocks;
    int32_t outputchannels;
    uint32_t blocksize;
    uint32_t pcm;

    result = this->mSystem->getDSPBufferSize(&blocksize, &numblocks);
    if (result != FMOD_OK) {
        return result;
    }

    numblocks += this->mMixAheadBlocks;

    result = this->mSystem->getSoftwareFormat(nullptr, &outputformat, &outputchannels, nullptr, nullptr, nullptr);
    if (result != FMOD_OK) {
        return result;
    }

    if (this->mDescription.getposition) {
        result = this->mDescription.getposition(this, &pcm);
        if (result != FMOD_OK) {
            return result;
        }
    }

    this->mSystem->mDSPTimeStamp.stampIn();

    pcm /= blocksize;
    pcm %= numblocks;

    while (this->mCursorBlock != static_cast<int32_t>(pcm)) {
        auto block = this->mCursorBlock;
        uint32_t len1 = 0;
        uint32_t len2 = 0;
        uint32_t lenbytes = 0;
        uint32_t numsamples;
        uint32_t offbytes = 0;
        void* ptr1 = nullptr;
        void* ptr2 = nullptr;

        block -= this->mMixAheadBlocks;
        if (block < 0) {
            block += numblocks;
        }

        result = SoundI::getBytesFromSamples(blocksize, &lenbytes, outputchannels, outputformat);
        if (result != FMOD_OK) {
            return result;
        }
        result = SoundI::getBytesFromSamples(block * blocksize, &offbytes, outputchannels, outputformat);
        if (result != FMOD_OK) {
            return result;
        }

        if (this->mDescription.lock) {
            result = this->mDescription.lock(this, offbytes, lenbytes, &ptr1, &ptr2, &len1, &len2);
            if (result != FMOD_OK) {
                return result;
            }
        }

        result = SoundI::getSamplesFromBytes(len1, &numsamples, outputchannels, outputformat);
        if (result != FMOD_OK) {
            return result;
        }

        result = this->mix(ptr1, numsamples);
        if (result != FMOD_OK) {
            return result;
        }

        if (this->mDescription.unlock) {
            result = this->mDescription.unlock(this, ptr1, ptr2, len1, len2);
            if (result != FMOD_OK) {
                return result;
            }
        }

        this->mCursorBlock++;
        if (this->mCursorBlock >= numblocks) {
            this->mCursorBlock = 0;
        }
    }

    if (this->mFinishedSema) {
        FMOD_OS_Semaphore_Signal(this->mFinishedSema);
    }

    this->mSystem->mDSPTimeStamp.stampOut(95);

    return FMOD_OK;
}

} // namespace FMOD
