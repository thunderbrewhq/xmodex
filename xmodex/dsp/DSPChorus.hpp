#ifndef XMODEX_DSP_DSP_CHORUS_HPP
#define XMODEX_DSP_DSP_CHORUS_HPP

#include "xmodex/dsp/DSPFilter.hpp"

namespace FMOD {

constexpr int32_t DSP_CHORUS_COSTABBITS = 13;
constexpr int32_t DSP_CHORUS_COSTABSIZE = (1 << DSP_CHORUS_COSTABBITS);
constexpr int32_t DSP_CHORUS_TABLERANGE = (DSP_CHORUS_COSTABSIZE * 4);
constexpr int32_t DSP_CHORUS_TABLEMASK = (DSP_CHORUS_TABLERANGE - 1);
constexpr float DSP_CHORUS_MAXBUFFERLENGTHMS = 200.0f;

class DSPChorus : public DSPFilter {
    private:
        float mDepth;
        float mDryMix;
        float mWetMix1;
        float mWetMix2;
        float mWetMix3;
        float mFeedback;
        float mDelay;
        float mRateHz;
        int16_t* mChorusBuffer;
        int16_t* mChorusBufferMemory;
        uint32_t mChorusBufferLength;
        uint32_t mChorusBufferLengthBytes;
        uint32_t mChorusBufferPosition;
        float mChorusPosition[3];
        float mChorusTick;
        float mChorusSpeed;
        int32_t mOutputRate;
        float mCosTab[8192];

    private:
        FMOD_RESULT createInternal();
        FMOD_RESULT releaseInternal();
        FMOD_RESULT resetInternal();
        FMOD_RESULT readInternal(float* inbuffer, float* outbuffer, uint32_t length, int32_t inchannels, int32_t outchannels);
        FMOD_RESULT setParameterInternal(int32_t index, float value);
        FMOD_RESULT getParameterInternal(int32_t index, float* value, char* valuestr);

        inline float cosine(float x) {
            int32_t y;

            x *= DSP_CHORUS_TABLERANGE;
            y = static_cast<int32_t>(x);
            if (y < 0) {
                y = -y;
            }

            y &= DSP_CHORUS_TABLEMASK;
            switch (y >> DSP_CHORUS_COSTABBITS) {
            case 0:
                return this->mCosTab[y];
            case 1:
                return -this->mCosTab[(DSP_CHORUS_COSTABSIZE - 1) - (y - (DSP_CHORUS_COSTABSIZE * 1))];
            case 2:
                return -this->mCosTab[(y - (DSP_CHORUS_COSTABSIZE * 2))];
            case 3:
                return this->mCosTab[(DSP_CHORUS_COSTABSIZE - 1) - (y - (DSP_CHORUS_COSTABSIZE * 3))];
            }

            return 0.0f;
        }

        inline float sine(float x) {
            return this->cosine(x - 0.25f);
        }

    public:
        static FMOD_RESULT F_CALLBACK createCallback(FMOD_DSP_STATE* dsp);
        static FMOD_RESULT F_CALLBACK getParameterCallback(FMOD_DSP_STATE* dsp, int32_t index, float* value, char* valuestr);
        static FMOD_RESULT F_CALLBACK readCallback(FMOD_DSP_STATE* dsp, float* inbuffer, float* outbuffer, uint32_t length, int32_t inchannels, int32_t outchannels);
        static FMOD_RESULT F_CALLBACK releaseCallback(FMOD_DSP_STATE* dsp);
        static FMOD_RESULT F_CALLBACK resetCallback(FMOD_DSP_STATE* dsp);
        static FMOD_RESULT F_CALLBACK setParameterCallback(FMOD_DSP_STATE* dsp, int32_t index, float value);
        static FMOD_DSP_DESCRIPTION_EX* getDescriptionEx();
};

} // namespace FMOD

#endif
