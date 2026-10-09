#ifndef XMODEX_DSP_A_SFX_DSP_HPP
#define XMODEX_DSP_A_SFX_DSP_HPP

#include <cstdint>

#define kNumLateReverbDelays 8
#define kNumMatrixStages 3
#define kNumEarly_EarlyLateDelayTaps 1
#define kNumLate_EarlyLateDelayTaps 8
#define kNumEarlyLateDelayTaps (kNumEarly_EarlyLateDelayTaps + kNumLate_EarlyLateDelayTaps)
#define kNumEarlyDelayTaps 7
#define kNumAllpassDelays 2

#define LOWEST_DELAY_LINE_LENGTH_SEC (0.061f)
#define DELAY_LINE_RATIO (1.32f)
#define LOWEST_DELAY_LINE_LENGTH_B_SEC (0.0015f)
#define DELAY_LINE_RATIO_B (1.47f)

#define kAllpassDelayLenSec0 (0.0059f)
#define kAllpassDelayLenSec1 (0.0077f)

#define kEarlyLateNextLengthSec (0.0187f)
#define kEarlyLateDelayRatio (1.29f)

#define kEarlyFirstDelaySec (0.005f)
#define kEarlyDelayRatio (1.59f)

#define REVERB_ALLPASS_ALLOCATION_ERR 4500
#define REVERB_EARLYLATE_ALLOCATION_ERR 4501
#define REVERB_EARLY_ALLOCATION_ERR 4502
#define REVERB_LATE_ALLOCATION_ERR 4503
#define REVERB_PARAMETER_ERROR 4504
#define REVERB_INBUFF_ALLOCATION_ERR 4505

struct coeff2ndorder {
        float a0;
        float a1;
        float a2;
        float b1;
        float b2;
};

class ASfxDsp {
    public:
        float* mInBuff;
        int32_t mNumAllocatedInBuffSamples;
        float mRoomHF;
        bool mRoomHFChanged;
        float mLatchRoomHF;
        float mPrevRoomHF;
        float mOldRoomHF;
        float mzRoomHF0;
        float mzRoomHF1;
        coeff2ndorder mRoomLFcoeffs;
        float mRoomLF;
        float mzRoomLF0;
        float mzRoomLF1;
        float mDecayHF[8];
        bool mDecayHFChanged[8];
        float mLatchDecayHF[8];
        float mLatchOneMinusDecayHF[8];
        float mPrevDecayHF[8];
        float mOldDecayHF[8];
        float mzDecayHF[8];
        float mMatrix[24];
        float mFeedback[8];
        float mLatchFeedback[8];
        float mOldFeedback[8];
        float mPrevFeedback[8];
        float mHadamard;
        bool mHadamardChanged;
        float mLatchHadamard;
        float mPrevHadamard;
        float mOldHadamard;
        float mERgain;
        float mLRgain;
        bool mLRgainChanged;
        float mLatchLRgain;
        float mPrevLRgain;
        float mOldLRgain;
        bool mERgainChanged;
        float mLatchERgain;
        float mPrevERgain;
        float mOldERgain;
        float mDiffusionScale;
        bool mDiffusionScaleChanged;
        float mLatchDiffusionScale;
        float mPrevDiffusionScale;
        float mOldDiffusionScale;
        float mAllpassGain;
        int32_t mNumLateReverbDelays;
        float** mLateDelays;
        float mLateDelayLenSec[8];
        int32_t mLateDelayLenSamples[8];
        int32_t mOldLateDelayLenSamples[8];
        int32_t mLateDelayLenBSamples[8];
        int32_t mLateDelaySamplesAllocated[8];
        bool mLateChanged[8];
        int32_t mLateMask[8];
        int32_t mLateWritePointer[8];
        int32_t mLateReadPointer[8];
        int32_t mOldLateReadPointer[8];
        int32_t mLateReadPointerB[8];
        float* mEarlyLateDelay;
        float* mEarlyLateDelayMemory;
        float mEarlyLateSec[9];
        int32_t mEarlyLateSamples[9];
        int32_t mOldEarlyLateSamples[9];
        bool mEarlyLateChanged[9];
        int32_t mEarlyLateSamplesAllocated;
        int32_t mEarlyLateMask;
        int32_t mEarlyLateWritePointer;
        int32_t mEarlyLateReadPointer[9];
        int32_t mOldEarlyLateReadPointer[9];
        float* mEarlyDelay;
        float mEarlyDelayLenSec[7];
        int32_t mEarlyDelayLenSamples[7];
        int32_t mEarlyDelaySamplesAllocated;
        int32_t mEarlyMask;
        int32_t mEarlyWritePointer;
        int32_t mEarlyReadPointer[7];
        float** mAllpassDelays;
        float mAllpassDelayLenSec[2];
        int32_t mAllpassDelayLenSamples[2];
        int32_t mAllpassSamplesAllocated[2];
        int32_t mAllpassMask[2];
        int32_t mAllpassWritePointer[2];
        int32_t mAllpassReadPointer[2];
        int32_t mNumMatrixStages;

    public:
        int32_t AllocateAllpassDelays(int32_t numDelays, float* delaySec, float sampleRate);
        int32_t AllocateEarlyDelay(float delaySec, float sampleRate);
        int32_t AllocateEarlyLateDelay(float* delaySec, float sampleRate);
        int32_t AllocateLateDelays(int32_t numDelays, float* delaySec, float sampleRate);
        void BlockProcessInput(uint32_t sampframes, int32_t channels, float* inAudio, float rate);
        void ClearBuffers();
        void ClearInBuff();
        void ClearReverbInternalBuffers();
        void DeallocateAllpassDelays();
        void DeallocateEarlyDelay();
        void DeallocateEarlyLateDelay();
        void DeallocateLateDelays();
        void DoDSPProcessing(float* inAudio, float* outAudio, int32_t channels, uint32_t l_sampframes, float rate, float drymix);
        void SetAllpassDelays(float rate);
        void SetEarlyDelay(float nextLengthSec, float delayRatio, float rate);
        void SetLateDelays(float nextLengthSec, float delayRatio, float nextLengthBSec, float delayRatioB, float rate);
        void SetLate_EarlyLateDelayTaps(float refPlusRevDelaySec, float nextLengthSec, float delayRatio, float rate);
        bool UpdateBufferSize(int32_t newMaxBlockSize);
        void ZeroWritePointers();
        void close();
        int32_t init(float rate);

        static int32_t NextPowerOf2(int32_t delaySamples);
};

#endif
