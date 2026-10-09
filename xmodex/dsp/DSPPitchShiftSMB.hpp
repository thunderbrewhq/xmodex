#ifndef XMODEX_DSP_DSP_PITCH_SHIFT_SMB_HPP
#define XMODEX_DSP_DSP_PITCH_SHIFT_SMB_HPP

#include "xmodex/Math.hpp"
#include <cstdint>

namespace FMOD {

constexpr int32_t DSP_PITCHSHIFT_COSTABBITS = 13;
constexpr int32_t DSP_PITCHSHIFT_COSTABSIZE = (1 << DSP_PITCHSHIFT_COSTABBITS);
constexpr int32_t DSP_PITCHSHIFT_TABLERANGE = (DSP_PITCHSHIFT_COSTABSIZE * 4);
constexpr int32_t DSP_PITCHSHIFT_TABLEMASK = (DSP_PITCHSHIFT_TABLERANGE - 1);

constexpr int32_t MAX_FRAME_LENGTH = 4096;
constexpr int32_t MAX_FRAME_LENGTH_HALF_BITS = 6;

class DSPPitchShiftSMB {
    public:
        float mInFIFO[4096];
        float mOutFIFO[4096];
        float mLastPhase[2052];
        float mSumPhase[2052];
        float mOutputAccum[8192];
        float* mWindow;
        int32_t mRover;
        float mPitchRatioFromLast;
        int32_t mFFTFrameSize;
        int32_t mFFTFrameBits;
        float* mCosTab;
        bool mResetPhaseFlag;

    private:
        inline float cosine(float x) {
            int32_t y;

            x *= DSP_PITCHSHIFT_TABLERANGE;
            y = static_cast<int32_t>(x);
            if (y < 0) {
                y = -y;
            }

            y &= DSP_PITCHSHIFT_TABLEMASK;
            switch (y >> DSP_PITCHSHIFT_COSTABBITS) {
            case 0:
                return this->mCosTab[y];
            case 1:
                return -this->mCosTab[(DSP_PITCHSHIFT_COSTABSIZE - 1) - (y - (DSP_PITCHSHIFT_COSTABSIZE * 1))];
            case 2:
                return -this->mCosTab[(y - (DSP_PITCHSHIFT_COSTABSIZE * 2))];
            case 3:
                return this->mCosTab[(DSP_PITCHSHIFT_COSTABSIZE - 1) - (y - (DSP_PITCHSHIFT_COSTABSIZE * 3))];
            }

            return 0.0f;
        }

        inline float sine(float x) {
            return this->cosine(x - 0.25f);
        }

        inline static float smbAtan2(float x, float y) {
            float signx;
            if (x > 0.0f) {
                signx = 1.0f;
            } else {
                signx = -1.0f;
            }

            if (x == 0.0f) {
                return 0.0f;
            }
            if (y == 0.0f) {
                return signx * FMOD_PI / 2.0f;
            }

            return static_cast<float>(atan2(x, y));
        }

    public:
        void smbInit();
        void smbFft(float* fftBuffer, int32_t sign);
        void smbPitchShift(float pitchShift, int32_t numSampsToProcess, int32_t osamp, float sampleRate, float* indata, float* outdata, int32_t channel, int32_t numchannels);
        void setResetPhaseFlag();
};

} // namespace FMOD

#endif
