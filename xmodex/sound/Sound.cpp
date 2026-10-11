#include "fmod.hpp"
#include "xmodex/sound/SoundI.hpp"

namespace FMOD {

Sound::Sound() {
}

FMOD_RESULT F_API Sound::readyForRelease(bool* ready) {
    FMOD_RESULT result;
    SoundI* soundi;

    *ready = true;

    result = SoundI::validate(this, &soundi);
    if (result != FMOD_OK) {
        return result;
    }

    *ready = soundi->readyForRelease();

    return FMOD_OK;
}

FMOD_RESULT F_API Sound::release() {
    FMOD_RESULT result;
    SoundI* soundi;

    result = SoundI::validate(this, &soundi);
    if (result != FMOD_OK) {
        return result;
    } else {
        return soundi->release(true);
    }
}

FMOD_RESULT F_API Sound::lock(uint32_t offset, uint32_t length, void** ptr1, void** ptr2, uint32_t* len1, uint32_t* len2) {
    FMOD_RESULT result;
    SoundI* soundi;

    result = SoundI::validate(this, &soundi);
    if (result != FMOD_OK) {
        return result;
    } else {
        if (soundi->mOpenState != FMOD_OPENSTATE_READY) {
            return FMOD_ERR_NOTREADY;
        }
        return soundi->lock(offset, length, ptr1, ptr2, len1, len2);
    }
}

FMOD_RESULT F_API Sound::unlock(void* ptr1, void* ptr2, uint32_t len1, uint32_t len2) {
    FMOD_RESULT result;
    SoundI* soundi;

    result = SoundI::validate(this, &soundi);
    if (result != FMOD_OK) {
        return result;
    } else {
        if (soundi->mOpenState != FMOD_OPENSTATE_READY) {
            return FMOD_ERR_NOTREADY;
        }
        return soundi->unlock(ptr1, ptr2, len1, len2);
    }
}

FMOD_RESULT F_API Sound::getName(char* name, int32_t namelen) {
    FMOD_RESULT result;
    SoundI* soundi;

    result = SoundI::validate(this, &soundi);
    if (result != FMOD_OK) {
        return result;
    } else {
        if (soundi->mOpenState != FMOD_OPENSTATE_READY) {
            return FMOD_ERR_NOTREADY;
        }
        return soundi->getName(name, namelen);
    }
}

FMOD_RESULT F_API Sound::getLength(uint32_t* length, FMOD_TIMEUNIT lengthtype) {
    FMOD_RESULT result;
    SoundI* soundi;

    result = SoundI::validate(this, &soundi);
    if (result != FMOD_OK) {
        return result;
    } else {
        if (soundi->mOpenState != FMOD_OPENSTATE_READY) {
            return FMOD_ERR_NOTREADY;
        }
        return soundi->getLength(length, lengthtype);
    }
}

FMOD_RESULT F_API Sound::setSoundGroup(SoundGroup* soundgroup) {
    FMOD_RESULT result;
    SoundI* soundi;

    result = SoundI::validate(this, &soundi);
    if (result != FMOD_OK) {
        return result;
    } else {
        if (soundi->mOpenState != FMOD_OPENSTATE_READY) {
            return FMOD_ERR_NOTREADY;
        }
        return soundi->setSoundGroup((SoundGroupI*)soundgroup);
    }
}

FMOD_RESULT F_API Sound::getUserData(void** _userdata) {
    FMOD_RESULT result;
    SoundI* soundi;

    result = SoundI::validate(this, &soundi);
    if (result != FMOD_OK) {
        return result;
    } else {
        return soundi->getUserData(_userdata);
    }
}

FMOD_RESULT F_API Sound::set3DMinMaxDistance(float min, float max) {
    FMOD_RESULT result;
    SoundI* soundi;

    result = SoundI::validate(this, &soundi);
    if (result != FMOD_OK) {
        return result;
    } else {
        if (soundi->mOpenState != FMOD_OPENSTATE_READY) {
            return FMOD_ERR_NOTREADY;
        }

        return soundi->set3DMinMaxDistance(min, max);
    }
}

FMOD_RESULT F_API Sound::setLoopCount(int32_t loopcount) {
    FMOD_RESULT result;
    SoundI* soundi;

    result = SoundI::validate(this, &soundi);
    if (result != FMOD_OK) {
        return result;
    } else {
        if (soundi->mOpenState != FMOD_OPENSTATE_READY) {
            return FMOD_ERR_NOTREADY;
        }
        return soundi->setLoopCount(loopcount);
    }
}

FMOD_RESULT F_API Sound::getLoopCount(int32_t* loopcount) {
    FMOD_RESULT result;
    SoundI* soundi;

    result = SoundI::validate(this, &soundi);
    if (result != FMOD_OK) {
        return result;
    } else {
        if (soundi->mOpenState != FMOD_OPENSTATE_READY) {
            return FMOD_ERR_NOTREADY;
        }
        return soundi->getLoopCount(loopcount);
    }
}

} // namespace FMOD
