#include "xmodex/SpeakerLevelsPool.hpp"
#include "xmodex/Memory.hpp"
#include "xmodex/SystemI.hpp"

namespace FMOD {

SpeakerLevelsPool::SpeakerLevelsPool() {
    this->mLevelsPool = nullptr;
}

FMOD_RESULT SpeakerLevelsPool::alloc(float** levels) {
    int32_t freepos = this->mSystem->mNumChannels;
    int32_t numoutputchannels = this->mSystem->mMaxOutputChannels;

    if (!this->mLevelsPool) {
        this->mLevelsPool = static_cast<LevelsInfo*>(FMOD_Memory_Calloc(sizeof(LevelsInfo) * this->mSystem->mNumChannels));
        if (!this->mLevelsPool) {
            return FMOD_ERR_MEMORY;
        }
    }

    if (this->mSystem->mSpeakerMode == FMOD_SPEAKERMODE_PROLOGIC) {
        numoutputchannels = 6;
    }

    for (int32_t i = 0; i < this->mSystem->mNumChannels; i++) {
        if (!this->mLevelsPool[i].inuse && this->mLevelsPool[i].levelsmemory) {
            FMOD_memset(this->mLevelsPool[i].levelsmemory, 0, sizeof(float) * numoutputchannels * this->mSystem->mMaxInputChannels);

            this->mLevelsPool[i].inuse = true;
            *levels = this->mLevelsPool[i].levelsmemory;

            return FMOD_OK;
        }

        if (!this->mLevelsPool[i].levelsmemory) {
            if (i < freepos) {
                freepos = i;
            }
        }
    }

    this->mLevelsPool[freepos].levelsmemory = static_cast<float*>(FMOD_Memory_Calloc(sizeof(float) * numoutputchannels * this->mSystem->mMaxInputChannels));
    if (!this->mLevelsPool[freepos].levelsmemory) {
        return FMOD_ERR_MEMORY;
    }

    this->mLevelsPool[freepos].inuse = true;
    *levels = this->mLevelsPool[freepos].levelsmemory;

    return FMOD_OK;
}

FMOD_RESULT SpeakerLevelsPool::free(float* levels) {
    if (this->mLevelsPool) {
        for (int32_t i = 0; i < this->mSystem->mNumChannels; i++) {
            if (this->mLevelsPool[i].levelsmemory == levels) {
                this->mLevelsPool[i].inuse = false;
                break;
            }
        }
    }

    return FMOD_OK;
}

FMOD_RESULT SpeakerLevelsPool::release() {
    if (this->mLevelsPool) {
        for (int32_t i = 0; i < this->mSystem->mNumChannels; i++) {
            if (this->mLevelsPool[i].levelsmemory) {
                FMOD_Memory_Free(this->mLevelsPool[i].levelsmemory);
                this->mLevelsPool[i].levelsmemory = NULL;
            }
        }

        FMOD_Memory_Free(this->mLevelsPool);
        this->mLevelsPool = nullptr;
    }

    return FMOD_OK;
}

} // namespace FMOD
