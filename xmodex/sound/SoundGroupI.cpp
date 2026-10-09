#include "xmodex/sound/SoundGroupI.hpp"
#include "xmodex/Memory.hpp"
#include "xmodex/SystemI.hpp"

namespace FMOD {

SoundGroupI::SoundGroupI() {
    this->mFadeSpeed = 0.0f;
    this->mMaxAudible = -1;
    this->mVolume = 1.0f;
    this->mName = nullptr;
}

FMOD_RESULT SoundGroupI::release() {
    if (this == this->mSystem->mSoundGroup) {
        return FMOD_ERR_INVALID_HANDLE;
    }

    if (this->mSystem->mSoundGroup) {
        auto sound = static_cast<SoundI*>(this->mSoundHead.getNext());
        while (sound != &this->mSoundHead) {
            sound->setSoundGroup(this->mSystem->mSoundGroup);
            sound = static_cast<SoundI*>(sound->getNext());
        }
    }

    return this->releaseInternal();
}

FMOD_RESULT SoundGroupI::releaseInternal() {
    if (this->mName) {
        FMOD_Memory_Free(this->mName);
    }

    this->removeNode();
    FMOD_Memory_Free(this);

    return FMOD_OK;
}

FMOD_RESULT SoundGroupI::setMaxAudible(int32_t maxaudible) {
    if (maxaudible < -1) {
        return FMOD_ERR_INVALID_PARAM;
    }

    this->mMaxAudible = maxaudible;

    return FMOD_OK;
}

FMOD_RESULT SoundGroupI::setMaxAudibleBehavior(FMOD_SOUNDGROUP_BEHAVIOR behavior) {
    if (behavior >= FMOD_SOUNDGROUP_BEHAVIOR_MAX) {
        return FMOD_ERR_INVALID_PARAM;
    }

    this->mMaxAudibleBehavior = behavior;

    return FMOD_OK;
}

FMOD_RESULT SoundGroupI::setMuteFadeSpeed(float speed) {
    if (speed < 0.0f) {
        return FMOD_ERR_INVALID_PARAM;
    }

    this->mFadeSpeed = speed;

    return FMOD_OK;
}

FMOD_RESULT SoundGroupI::getNumPlaying(int32_t* numplaying) {
    if (!numplaying) {
        return FMOD_ERR_INVALID_PARAM;
    }

    auto current = static_cast<SoundI*>(this->mSoundHead.getNext());
    *numplaying = 0;

    while (current != &this->mSoundHead) {
        *numplaying += current->mNumAudible;
        current = static_cast<SoundI*>(current->getNext());
    }

    return FMOD_OK;
}

FMOD_RESULT SoundGroupI::validate(SoundGroup* soundgroup, SoundGroupI** soundgroupi) {
    auto sg = reinterpret_cast<SoundGroupI*>(soundgroup);

    if (!soundgroup) {
        return FMOD_ERR_INVALID_HANDLE;
    }

    if (!soundgroupi) {
        return FMOD_ERR_INVALID_PARAM;
    }

    *soundgroupi = sg;

    return FMOD_OK;
}

} // namespace FMOD
