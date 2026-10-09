#ifndef XMODEX_SOUND_GROUP_I_HPP
#define XMODEX_SOUND_GROUP_I_HPP

#include "fmod.hpp"
#include "xmodex/LinkedListNode.hpp"

namespace FMOD {

class SystemI;

class SoundGroupI : public LinkedListNode {
    public:
        SystemI* mSystem;
        void* mUserData;
        LinkedListNode mSoundHead;
        char* mName;
        int32_t mMaxAudible;
        FMOD_SOUNDGROUP_BEHAVIOR mMaxAudibleBehavior;
        int32_t mPlayCount;
        int32_t mNumAudible;
        float mFadeSpeed;
        float mVolume;

    public:
        SoundGroupI();

        FMOD_RESULT releaseInternal();
        FMOD_RESULT setMaxAudible(int32_t maxaudible);
        FMOD_RESULT setMaxAudibleBehavior(FMOD_SOUNDGROUP_BEHAVIOR behavior);
        FMOD_RESULT setMuteFadeSpeed(float speed);
        FMOD_RESULT getNumPlaying(int32_t* numplaying);
        FMOD_RESULT release();

        static FMOD_RESULT validate(SoundGroup* soundgroup, SoundGroupI** soundgroupi);
};

} // namespace FMOD

#endif
