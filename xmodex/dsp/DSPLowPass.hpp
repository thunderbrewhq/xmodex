#ifndef XMODEX_DSP_DSP_LOW_PASS_HPP
#define XMODEX_DSP_DSP_LOW_PASS_HPP

#include "xmodex/dsp/DSPFilter.hpp"

namespace FMOD {

constexpr int32_t LOWPASS_FILTER_SECTIONS = 2;

struct LOWPASS_BIQUAD {
        float a0;
        float a1;
        float a2;
        float b0;
        float b1;
        float b2;
};

class DSPLowPass : public DSPFilter {
    private:
        float mResonance;
        float mResonanceUpdate;
        float mCutoffHz;
        float mCutoffHzUpdate;
        float mGainFactor[2];
        float mHistory[16][4];
        float mCoefficients[9];
        LOWPASS_BIQUAD mProtoCoef[2];

    private:
        inline float filter(float input, int32_t channel) {
            float* history1;
            float* history2;
            float* coef_ptr;
            float output;
            float new_hist;
            static float dc = static_cast<float>(1E-20);

            input += dc;
            dc = -dc;

            coef_ptr = this->mCoefficients;

            history1 = this->mHistory[channel];
            history2 = history1 + 1;

            output = input * (*coef_ptr++);

            for (int32_t i = 0; i < LOWPASS_FILTER_SECTIONS; i++) {
                output = output - history1[0] * coef_ptr[0];
                new_hist = output - history2[0] * coef_ptr[1];

                output = new_hist + history1[0] * coef_ptr[2];
                output = output + history2[0] * coef_ptr[3];

                coef_ptr += 4;
                *history2++ = *history1;
                *history1++ = new_hist;
                history1++;
                history2++;
            }

            return output;
        }

        FMOD_RESULT bilinear(float a0, float a1, float a2, float b0, float b1, float b2, float* k, float fs, float* coef);
        FMOD_RESULT szxform(float* a0, float* a1, float* a2, float* b0, float* b1, float* b2, float fc, float fs, float* k, float* coef);
        FMOD_RESULT prewarp(float* a0, float* a1, float* a2, float fc, float fs);
        FMOD_RESULT updateState(float resonance, float cutoff);

        FMOD_RESULT createInternal();
        FMOD_RESULT readInternal(float* inbuffer, float* outbuffer, uint32_t length, int32_t inchannels, int32_t outchannels);
        FMOD_RESULT setParameterInternal(int32_t index, float value);
        FMOD_RESULT getParameterInternal(int32_t index, float* value, char* valuestr);

    public:
        static FMOD_RESULT F_CALLBACK createCallback(FMOD_DSP_STATE* dsp);
        static FMOD_RESULT F_CALLBACK readCallback(FMOD_DSP_STATE* dsp, float* inbuffer, float* outbuffer, uint32_t length, int32_t inchannels, int32_t outchannels);
        static FMOD_RESULT F_CALLBACK setParameterCallback(FMOD_DSP_STATE* dsp, int32_t index, float value);
        static FMOD_RESULT F_CALLBACK getParameterCallback(FMOD_DSP_STATE* dsp, int32_t index, float* value, char* valuestr);
        static FMOD_DSP_DESCRIPTION_EX* getDescriptionEx();
};

} // namespace FMOD

#endif
