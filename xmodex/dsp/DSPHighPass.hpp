#ifndef XMODEX_DSP_DSP_HIGH_PASS_HPP
#define XMODEX_DSP_DSP_HIGH_PASS_HPP

#include "xmodex/dsp/DSPConnection.hpp"
#include "xmodex/dsp/DSPFilter.hpp"

namespace FMOD {

class DSPHighPass : public DSPFilter {
    private:
        float mCutoffHz;
        float mCutoffHzUpdate;
        float mResonance;
        float mResonanceUpdate;
        float mIn1[DSP_MAXLEVELS_MAX];
        float mIn2[DSP_MAXLEVELS_MAX];
        float mOut1[DSP_MAXLEVELS_MAX];
        float mOut2[DSP_MAXLEVELS_MAX];
        float mCoefficient_A0;
        float mCoefficient_A1;
        float mCoefficient_A2;
        float mCoefficient_B0;
        float mCoefficient_B1;
        float mCoefficient_B2;

    private:
        FMOD_RESULT createInternal();
        FMOD_RESULT getParameterInternal(int32_t index, float* value, char* valuestr);
        FMOD_RESULT readInternal(float* inbuffer, float* outbuffer, uint32_t length, int32_t inchannels, int32_t outchannels);
        FMOD_RESULT resetInternal();
        FMOD_RESULT setParameterInternal(int32_t index, float value);
        FMOD_RESULT updateCoefficients(float resonance, float cutoff);

    public:
        static FMOD_RESULT F_CALLBACK createCallback(FMOD_DSP_STATE* dsp);
        static FMOD_RESULT F_CALLBACK resetCallback(FMOD_DSP_STATE* dsp);
        static FMOD_RESULT F_CALLBACK readCallback(FMOD_DSP_STATE* dsp, float* inbuffer, float* outbuffer, uint32_t length, int32_t inchannels, int32_t outchannels);
        static FMOD_RESULT F_CALLBACK setParameterCallback(FMOD_DSP_STATE* dsp, int32_t index, float value);
        static FMOD_RESULT F_CALLBACK getParameterCallback(FMOD_DSP_STATE* dsp, int32_t index, float* value, char* valuestr);
        static FMOD_DSP_DESCRIPTION_EX* getDescriptionEx();
};

} // namespace FMOD

#endif
