#ifndef XMODEX_CHANNEL_CHANNEL_STREAM_HPP
#define XMODEX_CHANNEL_CHANNEL_STREAM_HPP

#include "xmodex/LinkedListNode.hpp"
#include "xmodex/channel/ChannelReal.hpp"

namespace FMOD {

class ChannelStream : public ChannelReal, public LinkedListNode {
    public:
        volatile bool mFinished;
        uint32_t mLastPCM;
        uint32_t mDecodeOffset;
        uint32_t mSamplesPlayed;
        uint32_t mSamplesWritten;
        LinkedListNode mStreamNode;

    public:
        ChannelStream();

        // virtual ChannelReal methods
        FMOD_RESULT set2DFreqVolumePanFor3D();
        bool isStream();
        FMOD_RESULT moveChannelGroup(ChannelGroupI* oldchannelgroup, ChannelGroupI* newchannelgroup);
        FMOD_RESULT alloc();
        FMOD_RESULT start();
        FMOD_RESULT update(int32_t delta);
        FMOD_RESULT updateStream();
        FMOD_RESULT stop(bool ignoreenddelay, bool resetstate, bool force);
        FMOD_RESULT stop(bool ignoreenddelay, bool resetstate);
        FMOD_RESULT setPaused(bool paused);
        FMOD_RESULT setVolume(float volume);
        FMOD_RESULT setFrequency(float frequency);
        FMOD_RESULT setPan(float pan, float fbpan);
        FMOD_RESULT setDelay(uint32_t startdelay, uint32_t enddelay);
        FMOD_RESULT setSpeakerMix(
            float frontleft,
            float frontright,
            float center,
            float lfe,
            float backleft,
            float backright,
            float sideleft,
            float sideright);
        FMOD_RESULT setSpeakerLevels(int32_t speaker, float* levels, int32_t numlevels);
        FMOD_RESULT setPosition(uint32_t position, FMOD_TIMEUNIT postype);
        FMOD_RESULT getPosition(uint32_t* position, FMOD_TIMEUNIT postype);
        FMOD_RESULT setLoopPoints(uint32_t loopstart, uint32_t looplength);
        FMOD_RESULT setLoopCount(int32_t loopcount);
        FMOD_RESULT set3DAttributes();
        FMOD_RESULT set3DMinMaxDistance();
        FMOD_RESULT set3DOcclusion(float directOcclusion, float reverbOcclusion);
        FMOD_RESULT setReverbProperties(const FMOD_REVERB_CHANNELPROPERTIES* prop);
        FMOD_RESULT getReverbProperties(FMOD_REVERB_CHANNELPROPERTIES* prop);
        FMOD_RESULT isPlaying(bool* isplaying, bool includethreadlatency);
        FMOD_RESULT getSpectrum(
            float* spectrumarray,
            int32_t numentries,
            int32_t channeloffset,
            FMOD_DSP_FFT_WINDOW windowtype);
        FMOD_RESULT getWaveData(float* wavearray, int32_t numvalues, int32_t channeloffset);
        FMOD_RESULT getDSPHead(DSPI** dsp);
        FMOD_RESULT setMode(FMOD_MODE mode);
};

} // namespace FMOD

#endif
