#ifndef XMODEX_CHANNEL_CHANNEL_I_HPP
#define XMODEX_CHANNEL_CHANNEL_I_HPP

#include "fmod.hpp"
#include "xmodex/LinkedListNode.hpp"

#define CHANNELI_FLAG_PAUSED 0x00000001
#define CHANNELI_FLAG_MUTED 0x00000002
#define CHANNELI_FLAG_JUSTWENTVIRTUAL 0x00000004
#define CHANNELI_FLAG_REALMUTE 0x00000008
#define CHANNELI_FLAG_MOVED 0x00000010
#define CHANNELI_FLAG_USEDADDDSP 0x00000020 // invented name
#define CHANNELI_FLAG_FORCEVIRTUAL 0x00000040
#define CHANNELI_FLAG_USEDINPUTMIX 0x00000080
#define CHANNELI_FLAG_PLAYINGPAUSED 0x00000100
#define CHANNELI_FLAG_USEDPAUSEDELAY 0x00000200
#define CHANNELI_FLAG_ENDDELAY 0x00000400
#define CHANNELI_FLAG_MUSICOWNED 0x00000800

#define CHANNELI_STOPFLAG_REFSTAMP 0x00000001
#define CHANNELI_STOPFLAG_UPDATELIST 0x00000002
#define CHANNELI_STOPFLAG_RESETCALLBACKS 0x00000004
#define CHANNELI_STOPFLAG_CALLENDCALLBACK 0x00000008
#define CHANNELI_STOPFLAG_RESETCHANNELGROUP 0x00000010
#define CHANNELI_STOPFLAG_PROCESSENDDELAY 0x00000020
#define CHANNELI_STOPFLAG_UPDATESYNCPOINTS 0x00000040
#define CHANNELI_STOPFLAG_DONTFREELEVELS 0x00000080

namespace FMOD {

class SyncPoint;
class ChannelReal;
class ChannelGroupI;
class SystemI;
class DSPI;
class SoundI;
class ReverbI;

enum FMOD_CHANNEL_PANMODE {
    FMOD_CHANNEL_PANMODE_PAN,
    FMOD_CHANNEL_PANMODE_SPEAKERMIX,
    FMOD_CHANNEL_PANMODE_SPEAKERLEVELS,
};

typedef uint32_t FMOD_CHANNEL_FLAG;

const int32_t FMOD_CHANNEL_MAXINPUTCHANNELS = 16;
const int32_t FMOD_CHANNEL_MAXREALSUBCHANNELS = 16;
const int32_t FMOD_CHANNEL_DEFAULTPRIORITY = 128;

struct FMOD_CHANNEL_INFO {
        float* mLevels;
        uint32_t mPCM;
        uint32_t mLoopStart;
        uint32_t mLoopEnd;
        ChannelReal* mRealChannel;
        SoundI* mSound;
        DSPI* mDSP;
        int32_t mLoopCount;
        bool mMute;
        bool mPaused;
        uint32_t mDelayStart;
        uint32_t mDelayEnd;
        FMOD_REVERB_CHANNELPROPERTIES mReverbProperties;
        DSPI* mAddDSP;
        FMOD_MODE mMode;
};

class ChannelI : public LinkedListNode {
    public:
        SortedLinkedListNode mSortedListNode;
        int32_t mIndex;
        void* mUserData;
        uint32_t mHandleOriginal;
        SystemI* mSystem;
        uint32_t mNumRealChannels;
        ChannelReal* mRealChannel[FMOD_CHANNEL_MAXREALSUBCHANNELS];
        uint32_t mHandleCurrent;
        FMOD_CHANNEL_FLAG mFlags;
        FMOD_CHANNEL_PANMODE mLastPanMode;
        int32_t mPriority;
        uint32_t mListPosition;
        SyncPoint* mSyncPointCurrent;
        ChannelGroupI* mChannelGroup;
        LinkedListNode mChannelGroupNode;
        float mFadeVolume;
        float mFadeTarget;
        float mVolume;
        float mFrequency;
        float mPan;
        float mSpeakerFL;
        float mSpeakerFR;
        float mSpeakerC;
        float mSpeakerLFE;
        float mSpeakerBL;
        float mSpeakerBR;
        float mSpeakerSL;
        float mSpeakerSR;
        float mInputMix[FMOD_CHANNEL_MAXINPUTCHANNELS];
        float* mLevels;
        float mDirectOcclusionVolume;
        float mReverbDryVolume;
        float mVolume3D;
        float mPitch3D;
        FMOD_VECTOR mPosition3D;
        FMOD_VECTOR mVelocity3D;
        float mDistance;
        float mMinDistance;
        float mMaxDistance;
        float mConeVolume3D;
        float mConeInsideAngle;
        float mConeOutsideAngle;
        float mConeOutsideVolume;
        FMOD_VECTOR mConeOrientation;
        float mDirectOcclusion;
        float mReverbOcclusion;
        float mDirectOcclusionTarget;
        float mReverbOcclusionTarget;
        FMOD_VECTOR* mRolloffPoint;
        int32_t mNumRolloffPoints;
        float mSpread;
        float m3DPanLevel;
        float m3DDopplerLevel;
        FMOD_CHANNEL_CALLBACK mCallback[FMOD_CHANNEL_CALLBACKTYPE_MAX];
        int32_t mCallbackCommand[FMOD_CHANNEL_CALLBACKTYPE_MAX];

    public:
        ChannelI();
        ChannelI(int32_t index, SystemI* system);

        FMOD_RESULT addDSP(DSPI* dsp);
        FMOD_RESULT addDSPChain(DSPI* dsp);
        FMOD_RESULT alloc(DSPI* dsp, bool reset);
        FMOD_RESULT alloc(SoundI* sound, bool reset);
        FMOD_RESULT calcVolumeAndPitchFor3D(int32_t delta);
        FMOD_RESULT calculate3DReverbGain(ReverbI* reverb, FMOD_VECTOR* channelpos, float* gain);
        FMOD_RESULT get3DAttributes(FMOD_VECTOR* pos, FMOD_VECTOR* vel);
        FMOD_RESULT get3DMinMaxDistance(float* mindistance, float* maxdistance);
        FMOD_RESULT get3DPanLevel(float* level);
        FMOD_RESULT getAudibility(float* audibility);
        FMOD_RESULT getChannelInfo(FMOD_CHANNEL_INFO* info);
        FMOD_RESULT getCurrentDSP(DSPI** dsp);
        FMOD_RESULT getCurrentSound(SoundI** sound);
        FMOD_RESULT getDelay(uint32_t* startdelay, uint32_t* enddelay);
        FMOD_RESULT getFadeVolume(float* fadevolume);
        FMOD_RESULT getFrequency(float* frequency);
        FMOD_RESULT getLoopCount(int32_t* loopcount);
        FMOD_RESULT getLoopPoints(uint32_t* loopstart, FMOD_TIMEUNIT loopstarttype, uint32_t* loopend, FMOD_TIMEUNIT loopendtype);
        FMOD_RESULT getMode(FMOD_MODE* mode);
        FMOD_RESULT getMute(bool* mute);
        FMOD_RESULT getPaused(bool* paused);
        FMOD_RESULT getPosition(uint32_t* position, FMOD_TIMEUNIT postype);
        FMOD_RESULT getPriority(int32_t* priority);
        FMOD_RESULT getRealChannel(ChannelReal** realchan, int32_t* subchannels);
        FMOD_RESULT getReverbProperties(FMOD_REVERB_CHANNELPROPERTIES* prop);
        FMOD_RESULT getSpeakerLevels(FMOD_SPEAKER speaker, float* levels, int32_t numlevels);
        FMOD_RESULT getVolume(float* volume);
        FMOD_RESULT init();
        FMOD_RESULT isPlaying(bool* isplaying);
        FMOD_RESULT isVirtual(bool* isvirtual);
        FMOD_RESULT play(DSPI* dsp, bool paused, bool reset, bool startmuted);
        FMOD_RESULT play(SoundI* sound, bool paused, bool reset, bool startmuted);
        FMOD_RESULT referenceStamp(bool newstamp);
        FMOD_RESULT returnToFreeList();
        FMOD_RESULT set3DAttributes(const FMOD_VECTOR* pos, const FMOD_VECTOR* vel);
        FMOD_RESULT set3DConeOrientation(FMOD_VECTOR* orientation);
        FMOD_RESULT set3DConeSettings(float insideconeangle, float outsideconeangle, float outsidevolume);
        FMOD_RESULT setCallback(FMOD_CHANNEL_CALLBACKTYPE type, FMOD_CHANNEL_CALLBACK callback, int32_t command);
        FMOD_RESULT setChannelGroup(ChannelGroupI* channelgroup);
        FMOD_RESULT setChannelGroupInternal(ChannelGroupI* channelgroup, bool resetattributes);
        FMOD_RESULT setChannelInfo(FMOD_CHANNEL_INFO* info);
        FMOD_RESULT setDefaults();
        FMOD_RESULT setDelay(uint32_t startdelay, uint32_t enddelay);
        FMOD_RESULT setFrequency(float frequency);
        FMOD_RESULT setLoopCount(int32_t loopcount);
        FMOD_RESULT setLoopPoints(uint32_t loopstart, FMOD_TIMEUNIT loopstarttype, uint32_t loopend, FMOD_TIMEUNIT loopendtype);
        FMOD_RESULT setMute(bool mute);
        FMOD_RESULT setPan(float pan, bool calldriver);
        FMOD_RESULT setPaused(bool paused);
        FMOD_RESULT setPosition(uint32_t position, FMOD_TIMEUNIT postype);
        FMOD_RESULT setReverbProperties(const FMOD_REVERB_CHANNELPROPERTIES* prop);
        FMOD_RESULT setSpeakerLevels(FMOD_SPEAKER speaker, float* levels, int32_t numlevels, bool calldriver);
        FMOD_RESULT setSpeakerMix(float frontleft, float frontright, float center, float lfe, float backleft, float backright, float sideleft, float sideright, bool calldriver);
        FMOD_RESULT setVolume(float volume, bool forceupdatepos);
        FMOD_RESULT start();
        FMOD_RESULT stopEx(bool refstamp, bool updatelist, bool resetcallbacks, bool stoprealchannels, bool callendcallback, bool resetchannelgroup, bool processenddelay);
        FMOD_RESULT updateSyncPoints(bool seeking);
        FMOD_RESULT stop();
        FMOD_RESULT forceVirtual(bool force);
        FMOD_RESULT updatePosition();
        FMOD_RESULT setPriority(int32_t priority);
        FMOD_RESULT set3DOcclusionInternal(float direct, float reverb, bool resettarget);
        FMOD_RESULT update(int32_t delta);
        FMOD_RESULT set3DMinMaxDistance(float mindistance, float maxdistance);
        FMOD_RESULT set3DPanLevel(float level);
        FMOD_RESULT setMode(FMOD_MODE mode);
        FMOD_RESULT setUserData(void* userdata);
        FMOD_RESULT getUserData(void** userdata);

        static FMOD_RESULT validate(Channel* channel, ChannelI** channeli);
};

} // namespace FMOD

#endif
