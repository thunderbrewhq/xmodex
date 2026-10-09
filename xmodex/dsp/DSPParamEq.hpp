#ifndef XMODEX_DSP_DSP_PARAM_EQ_HPP
#define XMODEX_DSP_DSP_PARAM_EQ_HPP

#include "xmodex/dsp/DSPConnection.hpp"
#include "xmodex/dsp/DSPFilter.hpp"

namespace FMOD {

class DSPParamEq : public DSPFilter {
    private:
        float mCenter;
        float mBandwidth;
        float mGain;
        float mCenterUpdate;
        float mBandwidthUpdate;
        float mGainUpdate;
        float mFilterIn[DSP_MAXLEVELS_MAX][2];
        float mFilterOut[DSP_MAXLEVELS_MAX][2];
        float mCoefficient_a0;
        float mCoefficient_a1;
        float mCoefficient_a2;
        float mCoefficient_b0;
        float mCoefficient_b1;
        float mCoefficient_b2;

    private:
        FMOD_RESULT createInternal();
        FMOD_RESULT resetInternal();
        FMOD_RESULT readInternal(float* inbuffer, float* outbuffer, uint32_t length, int32_t inchannels, int32_t outchannels);
        FMOD_RESULT setParameterInternal(int32_t index, float value);
        FMOD_RESULT getParameterInternal(int32_t index, float* value, char* valuestr);
        FMOD_RESULT updateCoefficients(float center, float bandwidth, float gain);

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
