#ifndef XMODEX_DSP_DSP_COMPRESSOR_HPP
#define XMODEX_DSP_DSP_COMPRESSOR_HPP

#include "xmodex/dsp/DSPConnection.hpp"
#include "xmodex/dsp/DSPFilter.hpp"

namespace FMOD {

class DSPCompressor : public DSPFilter {
    private:
        float mHoldConstant;
        float mAttack_ms;
        float mAttack_constant;
        float mRelease_ms;
        float mRelease_constant;
        float mThreshold_dB;
        float mThreshold_lin;
        float mGainMakeup_dB;
        float mGainMakeup_lin;
        float mGain;
        float mMaxChannelIn[DSP_MAXLEVELS_MAX];

    private:
        FMOD_RESULT createInternal();
        FMOD_RESULT getParameterInternal(int32_t index, float* value, char* valuestr);
        FMOD_RESULT readInternal(float* inbuffer, float* outbuffer, uint32_t length, int32_t inchannels, int32_t outchannels);
        FMOD_RESULT setParameterInternal(int32_t index, float value);

    public:
        static FMOD_RESULT F_CALLBACK createCallback(FMOD_DSP_STATE* dsp);
        static FMOD_RESULT F_CALLBACK readCallback(FMOD_DSP_STATE* dsp, float* inbuffer, float* outbuffer, uint32_t length, int32_t inchannels, int32_t outchannels);
        static FMOD_RESULT F_CALLBACK setParameterCallback(FMOD_DSP_STATE* dsp, int32_t index, float value);
        static FMOD_RESULT F_CALLBACK getParameterCallback(FMOD_DSP_STATE* dsp, int32_t index, float* value, char* valuestr);
        static FMOD_DSP_DESCRIPTION_EX* getDescriptionEx();
};

} // namespace FMOD

#endif
