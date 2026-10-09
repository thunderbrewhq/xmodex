#include "fmod.hpp"
#include "xmodex/sound/SoundGroupI.hpp"

namespace FMOD {

SoundGroup::SoundGroup() {
}

FMOD_RESULT F_API SoundGroup::release() {
    FMOD_RESULT result;
    SoundGroupI* soundgroupi;

    result = SoundGroupI::validate(this, &soundgroupi);
    if (result != FMOD_OK) {
        return result;
    } else {
        return soundgroupi->release();
    }
}

FMOD_RESULT F_API SoundGroup::setMaxAudible(int32_t maxaudible) {
    FMOD_RESULT result;
    SoundGroupI* soundgroupi;

    result = SoundGroupI::validate(this, &soundgroupi);
    if (result != FMOD_OK) {
        return result;
    } else {
        return soundgroupi->setMaxAudible(maxaudible);
    }
}

FMOD_RESULT F_API SoundGroup::setMaxAudibleBehavior(FMOD_SOUNDGROUP_BEHAVIOR behavior) {
    FMOD_RESULT result;
    SoundGroupI* soundgroupi;

    result = SoundGroupI::validate(this, &soundgroupi);
    if (result != FMOD_OK) {
        return result;
    } else {
        return soundgroupi->setMaxAudibleBehavior(behavior);
    }
}

FMOD_RESULT F_API SoundGroup::setMuteFadeSpeed(float speed) {
    FMOD_RESULT result;
    SoundGroupI* soundgroupi;

    result = SoundGroupI::validate(this, &soundgroupi);
    if (result != FMOD_OK) {
        return result;
    } else {
        return soundgroupi->setMuteFadeSpeed(speed);
    }
}

} // namespace FMOD
