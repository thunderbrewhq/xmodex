#ifndef XMODEX_CHANNEL_CHANNEL_GROUP_I_HPP
#define XMODEX_CHANNEL_CHANNEL_GROUP_I_HPP

#include "fmod.hpp"
#include "xmodex/LinkedListNode.hpp"

namespace FMOD {

class SystemI;
class DSPI;

class ChannelGroupI : public LinkedListNode {
    public:
        SystemI* mSystem;
        DSPI* mDSPHead;
        DSPI* mDSPMixTarget;
        void* mUserData;
        ChannelGroupI* mParent;
        ChannelGroupI* mGroupHead;
        LinkedListNode mChannelHead;
        int32_t mNumChannels;
        char* mName;
        float mVolume;
        float mRealVolume;
        float mDirectOcclusion;
        float mReverbOcclusion;
        float mRealDirectOcclusionVolume;
        float mRealReverbOcclusionVolume;
        float mPitch;
        float mRealPitch;
        bool mMute;
        bool mPaused;

    public:
        ChannelGroupI();

        FMOD_RESULT setVolumeInternal();
        FMOD_RESULT setPitchInternal();
        FMOD_RESULT setPaused(bool paused, bool setpausedflag);
        FMOD_RESULT setMute(bool mute, bool setmuteflag);
        FMOD_RESULT getDSPHead(DSPI** dsp);
        FMOD_RESULT getNumChannels(int32_t* numchannels);
        FMOD_RESULT setVolume(float volume);
        FMOD_RESULT addGroup(ChannelGroupI* group);
        FMOD_RESULT releaseInternal();
        FMOD_RESULT release();

        static FMOD_RESULT validate(ChannelGroup* channelgroup, ChannelGroupI** channelgroupi);
};

} // namespace FMOD

#endif
