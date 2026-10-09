#ifndef XMODEX_DSP_DSP_WAVE_TABLE_HPP
#define XMODEX_DSP_DSP_WAVE_TABLE_HPP

#include "xmodex/dsp/DSPI.hpp"

namespace FMOD {

class ChannelReal;
class SoundI;

enum DSPWAVETABLE_SPEEDDIR {
    DSPWAVETABLE_SPEEDDIR_FORWARDS,
    DSPWAVETABLE_SPEEDDIR_BACKWARDS
};

class DSPWaveTable : public DSPI {
    public:
        FMOD_UINT64P mPosition;
        FMOD_SINT64P mSpeed;
        DSPWAVETABLE_SPEEDDIR mDirection;
        float mFrequency;
        ChannelReal* mChannel;
        SoundI* mSound;

    private:
        FMOD_RESULT setPositionInternal(uint32_t position);
        FMOD_RESULT setParameterInternal(int32_t index, float value);
        FMOD_RESULT getParameterInternal(int32_t index, float* value, char* valuestr);

    public:
        DSPWaveTable();

        FMOD_RESULT alloc(FMOD_DSP_DESCRIPTION_EX* description);
        FMOD_RESULT addInput(DSPI* target);
        FMOD_RESULT execute(float* inbuffer, float** outbuffer, uint32_t* length, int32_t inchannels, int32_t* outchannels, FMOD_SPEAKERMODE speakermode);
        FMOD_RESULT setFrequency(float frequency);

        static FMOD_RESULT F_CALLBACK setPositionCallback(FMOD_DSP_STATE* dsp, uint32_t position);
        static FMOD_RESULT F_CALLBACK setParameterCallback(FMOD_DSP_STATE* dsp, int32_t index, float value);
        static FMOD_RESULT F_CALLBACK getParameterCallback(FMOD_DSP_STATE* dsp, int32_t index, float* value, char* valuestr);
};

} // namespace FMOD

#endif
