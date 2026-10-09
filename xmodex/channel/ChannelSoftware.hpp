#ifndef XMODEX_CHANNEL_CHANNEL_SOFTWARE_HPP
#define XMODEX_CHANNEL_CHANNEL_SOFTWARE_HPP

#include "xmodex/ReverbI.hpp"
#include "xmodex/channel/ChannelRealManual3D.hpp"
#include "xmodex/dsp/DSPFilter.hpp"
#include "xmodex/dsp/DSPWaveTable.hpp"

namespace FMOD {

class DSPResampler;
class DSPCodec;

class ChannelSoftware : public ChannelRealManual3D {
    private:
        DSPI* mDSPHead;
        DSPFilter mDSPHeadMemory;
        DSPWaveTable* mDSPWaveTable;
        DSPWaveTable mDSPWaveTableMemory;
        DSPResampler* mDSPResampler;
        DSPI* mDSPLowPass;
        DSPI* mDSPReverb;
        DSPCodec* mDSPCodec;
        DSPCodec* mDSPCodecStopped;
        DSPConnection* mDSPConnection;

    public:
        ChannelSoftware();

        FMOD_RESULT addToReverbs(DSPI* dsptarget);
        FMOD_RESULT setupDSPCodec(DSPI* dsp);
        FMOD_RESULT updateDirectMix(float volume);
        FMOD_RESULT updateReverbMix(ReverbI* reverb, float volume);

        // virtual ChannelReal methods
        FMOD_RESULT init(
            int32_t index,
            SystemI* system,
            Output* output,
            DSPI* dspmixtarget);
        FMOD_RESULT moveChannelGroup(ChannelGroupI* oldchannelgroup, ChannelGroupI* newchannelgroup);
        FMOD_RESULT alloc(DSPI* dsp);
        FMOD_RESULT alloc();
        FMOD_RESULT start();
        FMOD_RESULT stop(bool a2, bool a3, bool a4);
        FMOD_RESULT stop(bool a2, bool a3);
        FMOD_RESULT setPaused(bool paused);
        FMOD_RESULT setVolume(float volume);
        FMOD_RESULT setFrequency(float frequency);
        FMOD_RESULT setPan(float lrpan, float fbpan);
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
        FMOD_RESULT set3DOcclusion(float directOcclusion, float reverbOcclusion);
        FMOD_RESULT setReverbProperties(const FMOD_REVERB_CHANNELPROPERTIES* prop);
        FMOD_RESULT getReverbProperties(FMOD_REVERB_CHANNELPROPERTIES* prop);
        FMOD_RESULT isPlaying(bool* isplaying, bool includethreadlatency);
        FMOD_RESULT getSpectrum(
            float* spectrumarray,
            int32_t numvalues,
            int32_t channeloffset,
            FMOD_DSP_FFT_WINDOW windowtype);
        FMOD_RESULT getWaveData(
            float* wavearray,
            int32_t numvalues,
            int32_t channeloffset);
        FMOD_RESULT getDSPHead(DSPI** dsp);
        FMOD_RESULT setMode(FMOD_MODE mode);
};

} // namespace FMOD

#endif
