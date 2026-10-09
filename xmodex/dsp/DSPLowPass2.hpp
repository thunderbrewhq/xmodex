#ifndef XMODEX_DSP_DSP_LOW_PASS_2_HPP
#define XMODEX_DSP_DSP_LOW_PASS_2_HPP

#include "xmodex/dsp/DSPConnection.hpp"
#include "xmodex/dsp/DSPFilter.hpp"

namespace FMOD {

class DSPLowPass2 : public DSPFilter {
    private:
        float mResonance;
        float mResonanceUpdate;
        float mCutoffHz;
        float mCutoffHzUpdate;
        float mFilter_Y[DSP_MAXLEVELS_MAX][2];
        float mFilter_A0;
        float mFilter_B0;
        float mFilter_B1;

    private:
        FMOD_RESULT createInternal();
        FMOD_RESULT resetInternal();
        FMOD_RESULT readInternal(float* inbuffer, float* outbuffer, uint32_t length, int32_t inchannels, int32_t outchannels);
        FMOD_RESULT setParameterInternal(int32_t index, float value);
        FMOD_RESULT getParameterInternal(int32_t index, float* value, char* valuestr);
        FMOD_RESULT updateCoefficients(float resonance, float cutoff);

    public:
        static FMOD_RESULT F_CALLBACK createCallback(FMOD_DSP_STATE* dsp);
        static FMOD_RESULT F_CALLBACK readCallback(FMOD_DSP_STATE* dsp, float* inbuffer, float* outbuffer, uint32_t length, int32_t inchannels, int32_t outchannels);
        static FMOD_RESULT F_CALLBACK resetCallback(FMOD_DSP_STATE* dsp);
        static FMOD_RESULT F_CALLBACK setParameterCallback(FMOD_DSP_STATE* dsp, int32_t index, float value);
        static FMOD_RESULT F_CALLBACK getParameterCallback(FMOD_DSP_STATE* dsp, int32_t index, float* value, char* valuestr);
        static FMOD_DSP_DESCRIPTION_EX* getDescriptionEx();
};

} // namespace FMOD

#endif
