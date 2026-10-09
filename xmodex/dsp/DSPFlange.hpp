#ifndef XMODEX_DSP_DSP_FLANGE_HPP
#define XMODEX_DSP_DSP_FLANGE_HPP

#include "xmodex/dsp/DSPFilter.hpp"

namespace FMOD {

constexpr int32_t DSP_FLANGE_COSTABBITS = 13;
constexpr int32_t DSP_FLANGE_COSTABSIZE = (1 << DSP_FLANGE_COSTABBITS);
constexpr int32_t DSP_FLANGE_TABLERANGE = (DSP_FLANGE_COSTABSIZE * 4);
constexpr int32_t DSP_FLANGE_TABLEMASK = (DSP_FLANGE_TABLERANGE - 1);
constexpr float DSP_FLANGE_MAXBUFFERLENGTHMS = 40.0f;

class DSPFlange : public DSPFilter {
    private:
        float mDepth;
        float mDryMix;
        float mWetMix;
        float mRateHz;
        float* mFlangeBuffer;
        uint32_t mFlangeBufferLength;
        uint32_t mFlangeBufferLengthBytes;
        uint32_t mFlangeBufferPosition;
        float mFlangePosition;
        float mFlangeTick;
        float mFlangeSpeed;
        int32_t mOutputRate;
        float mCosTab[DSP_FLANGE_COSTABSIZE];

    private:
        inline float cosine(float x) {
            int32_t y;

            x *= DSP_FLANGE_TABLERANGE;
            y = static_cast<int32_t>(x);
            if (y < 0) {
                y = -y;
            }

            y &= DSP_FLANGE_TABLEMASK;
            switch (y >> DSP_FLANGE_COSTABBITS) {
            case 0:
                return this->mCosTab[y];
            case 1:
                return -this->mCosTab[(DSP_FLANGE_COSTABSIZE - 1) - (y - (DSP_FLANGE_COSTABSIZE * 1))];
            case 2:
                return -this->mCosTab[(y - (DSP_FLANGE_COSTABSIZE * 2))];
            case 3:
                return this->mCosTab[(DSP_FLANGE_COSTABSIZE - 1) - (y - (DSP_FLANGE_COSTABSIZE * 3))];
            }

            return 0.0f;
        }

        inline float sine(float x) {
            return this->cosine(x - 0.25f);
        }

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
