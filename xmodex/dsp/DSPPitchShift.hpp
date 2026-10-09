#ifndef XMODEX_DSP_DSP_PITCH_SHIFT_HPP
#define XMODEX_DSP_DSP_PITCH_SHIFT_HPP

#include "xmodex/dsp/DSPFilter.hpp"
#include "xmodex/dsp/DSPPitchShiftSMB.hpp"

namespace FMOD {

class DSPPitchShift : public DSPFilter {
    private:
        float mCosTab[8192];
        float mWindow[4096];
        float mPitch;
        int32_t mFFTSize;
        int32_t mOverlap;
        int32_t mMaxChannels;
        DSPPitchShiftSMB* mPitchShift;
        int32_t mOutputRate;
        int32_t mChannels;
        int32_t mFFTFrameBits;

    private:
        FMOD_RESULT createInternal();
        FMOD_RESULT releaseInternal();
        FMOD_RESULT resetInternal();
        FMOD_RESULT readInternal(float* inbuffer, float* outbuffer, uint32_t length, int32_t inchannels, int32_t outchannels);
        FMOD_RESULT setParameterInternal(int32_t index, float value);
        FMOD_RESULT getParameterInternal(int32_t index, float* value, char* valuestr);

    public:
        static FMOD_RESULT F_CALLBACK createCallback(FMOD_DSP_STATE* dsp);
        static FMOD_RESULT F_CALLBACK releaseCallback(FMOD_DSP_STATE* dsp);
        static FMOD_RESULT F_CALLBACK resetCallback(FMOD_DSP_STATE* dsp);
        static FMOD_RESULT F_CALLBACK readCallback(FMOD_DSP_STATE* dsp, float* inbuffer, float* outbuffer, uint32_t length, int32_t inchannels, int32_t outchannels);
        static FMOD_RESULT F_CALLBACK setParameterCallback(FMOD_DSP_STATE* dsp, int32_t index, float value);
        static FMOD_RESULT F_CALLBACK getParameterCallback(FMOD_DSP_STATE* dsp, int32_t index, float* value, char* valuestr);
        static FMOD_DSP_DESCRIPTION_EX* getDescriptionEx();
};

} // namespace FMOD

#endif
