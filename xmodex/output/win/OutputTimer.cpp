#include "xmodex/output/win/OutputTimer.hpp"
#include "xmodex/SystemI.hpp"
#include <windows.h>

namespace FMOD {

FMOD_RESULT OutputTimer::timerFunc() {
    FMOD_RESULT result = FMOD_OK;
    uint32_t playPosition = 0;
    uint32_t blockSize = 0;
    int32_t numBlocks = 0;
    FMOD_SOUND_FORMAT outputFormat = FMOD_SOUND_FORMAT_NONE;
    int32_t outputChannels = 0;

    if (!this->mPlaying) {
        return FMOD_OK;
    }

    this->mSystem->mDSPTimeStamp.stampIn();

    result = this->mSystem->getDSPBufferSize(&blockSize, &numBlocks);
    if (result != FMOD_OK) {
        return result;
    }

    result = this->mSystem->getSoftwareFormat(
        nullptr,
        &outputFormat,
        &outputChannels,
        nullptr,
        nullptr,
        nullptr);
    if (result != FMOD_OK) {
        return result;
    }

    if (this->mDescription.getposition) {
        result = this->mDescription.getposition(this, &playPosition);
        if (result != FMOD_OK) {
            return result;
        }
    }

    playPosition /= blockSize;
    playPosition %= numBlocks;

    while (this->mFillBlock != static_cast<int32_t>(playPosition)) {
        void* ptr1 = nullptr;
        void* ptr2 = nullptr;
        uint32_t len1 = 0;
        uint32_t len2 = 0;
        uint32_t writePositionBytes = 0;
        uint32_t blockSizeBytes = 0;
        uint32_t samplesLocked = 0;

        result = SoundI::getBytesFromSamples(blockSize, &blockSizeBytes, outputChannels, outputFormat);
        if (result != FMOD_OK) {
            return result;
        }
        result = SoundI::getBytesFromSamples(this->mFillBlock * blockSize, &writePositionBytes, outputChannels, outputFormat);
        if (result != FMOD_OK) {
            return result;
        }

        if (this->mDescription.lock) {
            result = this->mDescription.lock(this, writePositionBytes, blockSizeBytes, &ptr1, &ptr2, &len1, &len2);
            if (result != FMOD_OK) {
                return result;
            }
        }

        result = SoundI::getSamplesFromBytes(len1, &samplesLocked, outputChannels, outputFormat);
        if (result != FMOD_OK) {
            return result;
        }

        result = this->mix(ptr1, samplesLocked);
        if (result != FMOD_OK) {
            return result;
        }

        if (this->mDescription.unlock) {
            result = this->mDescription.unlock(this, ptr1, ptr2, len1, len2);
            if (result != FMOD_OK) {
                return result;
            }
        }

        this->mFillBlock++;
        if (this->mFillBlock >= numBlocks) {
            this->mFillBlock = 0;
        }
    }

    this->mSystem->mDSPTimeStamp.stampOut(95);

    return FMOD_OK;
}

void F_CALLBACK OutputTimer::timerFuncCallback(uint32_t uTimerID, uint32_t uMsg, uintptr_t dwUser, uintptr_t dw1, uintptr_t dw2) {
    auto outputTimer = reinterpret_cast<OutputTimer*>(dwUser);

    FMOD_OS_CriticalSection_Enter(outputTimer->mMixerCrit);
    outputTimer->timerFunc();
    FMOD_OS_CriticalSection_Leave(outputTimer->mMixerCrit);
}

FMOD_RESULT OutputTimer::stop() {
    FMOD_RESULT fResult = FMOD_OK;
    HRESULT hResult = S_OK;

    timeKillEvent(this->mMixerTimerID);
    this->mMixerTimerID = 0;

    this->mPlaying = false;

    FMOD_OS_CriticalSection_Enter(this->mMixerCrit);

    FMOD_OS_CriticalSection_Leave(this->mMixerCrit);
    FMOD_OS_CriticalSection_Free(this->mMixerCrit);

    this->mMixerCrit = nullptr;

    return FMOD_OK;
}

FMOD_RESULT OutputTimer::start() {
    FMOD_RESULT result = FMOD_OK;
    uint32_t blockSize = 0;
    int32_t sampleRate = 0;

    result = this->mSystem->getDSPBufferSize(&blockSize, nullptr);
    if (result != FMOD_OK) {
        return result;
    }

    result = this->mSystem->getSoftwareFormat(&sampleRate, nullptr, nullptr, nullptr, nullptr, nullptr);
    if (result != FMOD_OK) {
        return result;
    }

    this->mMixerTimerPeriod = static_cast<float>(blockSize) * 1000.0f / static_cast<float>(sampleRate);
    if (this->mMixerTimerPeriod < 20) {
        this->mMixerTimerPeriod /= 3;
        if (this->mMixerTimerPeriod < 1) {
            this->mMixerTimerPeriod = 1;
        }
    } else {
        this->mMixerTimerPeriod = 10;
    }

    this->mPlaying = true;

    result = FMOD_OS_CriticalSection_Create(&this->mMixerCrit);
    if (result != FMOD_OK) {
        return result;
    }

    this->mMixerTimerID = timeSetEvent(
        static_cast<UINT>(this->mMixerTimerPeriod),
        0,
        static_cast<LPTIMECALLBACK>(timerFuncCallback),
        reinterpret_cast<DWORD_PTR>(this),
        TIME_PERIODIC);
    if (!this->mMixerTimerID) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    return FMOD_OK;
}

} // namespace FMOD
