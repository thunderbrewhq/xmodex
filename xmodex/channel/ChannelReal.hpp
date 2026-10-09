#ifndef XMODEX_CHANNEL_CHANNEL_REAL_HPP
#define XMODEX_CHANNEL_CHANNEL_REAL_HPP

#include "fmod.hpp"
#include <cstdint>

#define CHANNELREAL_FLAG_ALLOCATED 0x00000010
#define CHANNELREAL_FLAG_PAUSED 0x00000020
#define CHANNELREAL_FLAG_PLAYING 0x00000040
#define CHANNELREAL_FLAG_STOPPED 0x00000080
#define CHANNELREAL_FLAG_IN_USE 0x00000100
#define CHANNELREAL_FLAG_PAUSEDFORSETPOS 0x00000200
#define CHANNELREAL_FLAG_ENDDELAY 0x00000400
#define CHANNELREAL_FLAG_NOREVERB 0x00000800
#define CHANNELREAL_FLAG_RESERVED 0x00001000
#define CHANNELREAL_FLAG_HASPLAYED 0x00002000
#define CHANNELREAL_FLAG_MAX 0xFFFFFFFF

namespace FMOD {

class SystemI;
class ChannelPool;
class ChannelI;
class Output;
class SoundI;
class DSPI;
class ChannelGroupI;

enum CHANNELREAL_TYPE {
    CHANNELREAL_TYPE_HW3D,
    CHANNELREAL_TYPE_HW2D,
    CHANNELREAL_TYPE_SW,
    CHANNELREAL_TYPE_MAX
};

enum CHANNELREAL_PLAYDIR {
    CHANNELREAL_PLAYDIR_FORWARDS,
    CHANNELREAL_PLAYDIR_BACKWARDS,
    CHANNELREAL_PLAYDIR_MAX = 0xFFFFFFFF
};

typedef uint32_t CHANNELREAL_FLAG;

class ChannelReal {
    public:
        SystemI* mSystem;
        int32_t mNumSubChannels;
        ChannelReal* mSubChannel[16];
        int32_t mSubChannelIndex;
        ChannelPool* mPool;
        ChannelI* mParent;
        Output* mOutput;
        SoundI* mSound;
        DSPI* mDSP;
        FMOD_MODE mMode;
        CHANNELREAL_FLAG mFlags;
        int32_t mIndex;
        uint32_t mPosition;
        int32_t mDirection;
        int32_t mLoopCount;
        uint32_t mLoopStart;
        uint32_t mLoopLength;
        uint32_t mLength;
        uint32_t mDelayStart;
        uint32_t mDelayEnd;
        float mMaxFrequency;
        float mMinFrequency;

    public:
        ChannelReal();
        virtual ~ChannelReal();

        // virtual ChannelReal methods
        virtual FMOD_RESULT init(
            int32_t index,
            SystemI* system,
            Output* output,
            DSPI* dspmixtarget);
        virtual FMOD_RESULT set2DFreqVolumePanFor3D();
        virtual bool isStream();
        virtual FMOD_RESULT moveChannelGroup(ChannelGroupI* oldchannelgroup, ChannelGroupI* newchannelgroup);
        virtual FMOD_RESULT close();
        virtual FMOD_RESULT alloc(DSPI* dsp);
        virtual FMOD_RESULT alloc();
        virtual FMOD_RESULT start();
        virtual FMOD_RESULT update(int32_t delta);
        virtual FMOD_RESULT updateStream();
        virtual FMOD_RESULT stop(bool ignoreenddelay, bool resetstate, bool force);
        virtual FMOD_RESULT stop(bool ignoreenddelay, bool resetstate);
        virtual FMOD_RESULT setPaused(bool paused);
        virtual FMOD_RESULT getPaused(bool* paused);
        virtual FMOD_RESULT setVolume(float volume);
        virtual FMOD_RESULT setFrequency(float frequency);
        virtual FMOD_RESULT setPan(float pan, float fbpan);
        virtual FMOD_RESULT setDelay(uint32_t startdelay, uint32_t enddelay);
        virtual FMOD_RESULT setSpeakerMix(
            float frontleft,
            float frontright,
            float center,
            float lfe,
            float backleft,
            float backright,
            float sideleft,
            float sideright);
        virtual FMOD_RESULT setSpeakerLevels(int32_t speaker, float* levels, int32_t numlevels);
        virtual FMOD_RESULT updateSpeakerLevels(float volume);
        virtual FMOD_RESULT setPosition(uint32_t position, FMOD_TIMEUNIT postype);
        virtual FMOD_RESULT getPosition(uint32_t* position, FMOD_TIMEUNIT postype);
        virtual FMOD_RESULT setLoopPoints(uint32_t loopstart, uint32_t looplength);
        virtual FMOD_RESULT setLoopCount(int32_t loopcount);
        virtual FMOD_RESULT set3DAttributes();
        virtual FMOD_RESULT set3DMinMaxDistance();
        virtual FMOD_RESULT set3DOcclusion(float directOcclusion, float reverbOcclusion);
        virtual FMOD_RESULT setReverbProperties(const FMOD_REVERB_CHANNELPROPERTIES* prop);
        virtual FMOD_RESULT getReverbProperties(FMOD_REVERB_CHANNELPROPERTIES* prop);
        virtual FMOD_RESULT isPlaying(bool* isplaying, bool includethreadlatency);
        virtual FMOD_RESULT isVirtual(bool* isvirtual);
        virtual FMOD_RESULT getSpectrum(
            float* spectrumarray,
            int32_t numentries,
            int32_t channeloffset,
            FMOD_DSP_FFT_WINDOW windowtype);
        virtual FMOD_RESULT getWaveData(float* wavearray, int32_t numvalues, int32_t channeloffset);
        virtual FMOD_RESULT getDSPHead(DSPI** dsp);
        virtual FMOD_RESULT setMode(FMOD_MODE mode);
};

} // namespace FMOD

#endif
