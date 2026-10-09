#include "xmodex/dsp/ASfxDsp.hpp"
#include "xmodex/Math.hpp"
#include "xmodex/Memory.hpp"
#include "xmodex/String.hpp"
#include "xmodex/dsp/3DL2.hpp"

int32_t ASfxDsp::AllocateAllpassDelays(int32_t numDelays, float* delaySec, float sampleRate) {
    int32_t delaySamples;

    this->DeallocateAllpassDelays();

    for (int32_t delayLine = 0; delayLine < kNumAllpassDelays; delayLine++) {
        delaySamples = static_cast<int32_t>(delaySec[delayLine] * sampleRate) + 1;
        delaySamples = NextPowerOf2(delaySamples);
        this->mAllpassDelays[delayLine] = static_cast<float*>(FMOD_Memory_Calloc(delaySamples * sizeof(float)));
        if (!this->mAllpassDelays[delayLine]) {
            return REVERB_ALLPASS_ALLOCATION_ERR;
        }

        this->mAllpassSamplesAllocated[delayLine] = delaySamples;
        this->mAllpassMask[delayLine] = delaySamples - 1;
        this->mAllpassWritePointer[delayLine] = 0;
    }

    return 0;
}

int32_t ASfxDsp::AllocateEarlyDelay(float delaySec, float sampleRate) {
    int32_t delaySamples = static_cast<int32_t>(delaySec * sampleRate) + 1;

    delaySamples = NextPowerOf2(delaySamples);

    this->DeallocateEarlyDelay();

    this->mEarlyDelay = static_cast<float*>(FMOD_Memory_Calloc(delaySamples * sizeof(float)));
    if (!this->mEarlyDelay) {
        return REVERB_EARLY_ALLOCATION_ERR;
    }

    this->mEarlyDelaySamplesAllocated = delaySamples;
    this->mEarlyMask = delaySamples - 1;

    return 0;
}

int32_t ASfxDsp::AllocateEarlyLateDelay(float* delaySec, float sampleRate) {
    float maxDelaySec = I3DL2LISTENER_MAXREFLECTIONSDELAY + I3DL2LISTENER_MAXREVERBDELAY + delaySec[kNumEarlyLateDelayTaps - 1] - delaySec[kNumEarly_EarlyLateDelayTaps];

    int32_t delaySamples = static_cast<int32_t>(maxDelaySec * sampleRate) + 1;

    delaySamples = NextPowerOf2(delaySamples);

    this->DeallocateEarlyLateDelay();

    this->mEarlyLateDelay = static_cast<float*>(FMOD_Memory_Calloc(delaySamples * sizeof(float)));
    if (!this->mEarlyLateDelay) {
        return REVERB_EARLYLATE_ALLOCATION_ERR;
    }

    this->mEarlyLateSamplesAllocated = delaySamples;
    this->mEarlyLateMask = delaySamples - 1;

    return 0;
}

int32_t ASfxDsp::AllocateLateDelays(int32_t numDelays, float* delaySec, float sampleRate) {
    int32_t delaySamples;

    this->DeallocateLateDelays();

    if (!this->mLateDelays) {
        this->mLateDelays = static_cast<float**>(FMOD_Memory_Calloc(kNumLateReverbDelays * sizeof(float*)));
        if (!this->mLateDelays) {
            return REVERB_LATE_ALLOCATION_ERR;
        }
    }

    for (int32_t delayLine = 0; delayLine < kNumLateReverbDelays; delayLine++) {
        delaySamples = static_cast<int32_t>(delaySec[delayLine] * sampleRate) + 1;

        delaySamples = NextPowerOf2(delaySamples);

        this->mLateDelays[delayLine] = static_cast<float*>(FMOD_Memory_Calloc(delaySamples * sizeof(float)));
        if (!this->mLateDelays[delayLine]) {
            return REVERB_LATE_ALLOCATION_ERR;
        }

        this->mLateDelaySamplesAllocated[delayLine] = delaySamples;
        this->mLateMask[delayLine] = delaySamples - 1;
        this->mLateWritePointer[delayLine] = 0;
    }

    return 0;
}

void ASfxDsp::BlockProcessInput(uint32_t sampframes, int32_t channels, float* inAudio, float rate) {
    if (channels == 1) {
        memmove(this->mInBuff, inAudio, sampframes * sizeof(float));
    } else {
        for (int32_t i = 0; i < sampframes; i++) {
            this->mInBuff[i] = 0.0f;

            for (int32_t ch = 0; ch < channels; ++ch) {
                this->mInBuff[i] += inAudio[i * channels + ch];
            }
        }
    }
}

void ASfxDsp::ClearBuffers() {
    this->ClearInBuff();
    this->ClearReverbInternalBuffers();
}

void ASfxDsp::ClearInBuff() {
    if (this->mInBuff) {
        for (int32_t i = 0; i < this->mNumAllocatedInBuffSamples; i++) {
            this->mInBuff[i] = 0.0f;
        }
    }
}

void ASfxDsp::ClearReverbInternalBuffers() {
    int32_t delayLine;

    if (this->mLateDelays) {
        for (delayLine = 0; delayLine < kNumLateReverbDelays; delayLine++) {
            if (this->mLateDelays[delayLine]) {
                for (int32_t samp = 0; samp < this->mLateDelaySamplesAllocated[delayLine]; samp++) {
                    this->mLateDelays[delayLine][samp] = 0.0f;
                }
            }
        }
    }

    for (int32_t samp = 0; samp < this->mEarlyLateSamplesAllocated; samp++) {
        this->mEarlyLateDelay[samp] = 0.0f;
    }

    for (int32_t samp = 0; samp < this->mEarlyDelaySamplesAllocated; samp++) {
        this->mEarlyDelay[samp] = 0.0f;
    }

    for (delayLine = 0; delayLine < kNumAllpassDelays; delayLine++) {
        for (int32_t samp = 0; samp < this->mAllpassSamplesAllocated[delayLine]; samp++) {
            this->mAllpassDelays[delayLine][samp] = 0.0f;
        }
    }

    FMOD_memset(this->mzDecayHF, 0, kNumLateReverbDelays * sizeof(float));
    FMOD_memset(this->mMatrix, 0, kNumMatrixStages * kNumLateReverbDelays * sizeof(float));
    this->mzRoomHF0 = 0.0f;
    this->mzRoomHF1 = 0.0f;
    this->mzRoomLF0 = 0.0f;
    this->mzRoomLF1 = 0.0f;
}

void ASfxDsp::DeallocateAllpassDelays() {
    if (this->mAllpassDelays) {
        for (int32_t delayLine = 0; delayLine < kNumAllpassDelays; delayLine++) {
            if (this->mAllpassDelays[delayLine]) {
                FMOD_Memory_Free(this->mAllpassDelays[delayLine]);
            }

            this->mAllpassDelays[delayLine] = nullptr;
        }

        FMOD_Memory_Free(this->mAllpassDelays);
    }

    this->mAllpassDelays = nullptr;
}

void ASfxDsp::DeallocateEarlyDelay() {
    if (this->mEarlyDelay) {
        FMOD_Memory_Free(this->mEarlyDelay);
    }

    this->mEarlyDelay = nullptr;
}

void ASfxDsp::DeallocateEarlyLateDelay() {
    if (this->mEarlyLateDelay) {
        FMOD_Memory_Free(this->mEarlyLateDelay);
    }

    this->mEarlyLateDelay = nullptr;
}

void ASfxDsp::DeallocateLateDelays() {
    if (this->mLateDelays) {
        for (int32_t delayLine = 0; delayLine < kNumLateReverbDelays; delayLine++) {
            if (this->mLateDelays[delayLine]) {
                FMOD_Memory_Free(this->mLateDelays[delayLine]);
            }

            this->mLateDelays[delayLine] = nullptr;
        }

        FMOD_Memory_Free(this->mLateDelays);
    }

    this->mLateDelays = nullptr;
}

void ASfxDsp::DoDSPProcessing(float* inAudio, float* outAudio, int32_t channels, uint32_t l_sampframes, float rate, float drymix) {
    float eruOut[kNumEarlyDelayTaps + 1];
    float val;
    float oldVal;
    uint32_t sampframes = static_cast<uint32_t>(l_sampframes);
    float invSampframes = 1.0f / static_cast<float>(sampframes);

    constexpr float VERY_SMALL_BUT_NOT_DENORMAL_VALUE = 1.0e-15f;

    for (int32_t ch = 0; ch < channels; ch++) {
        for (uint32_t i = 0; i < sampframes; i += 2) {
            inAudio[ch + channels * i] += VERY_SMALL_BUT_NOT_DENORMAL_VALUE;
        }
        for (uint32_t i = 1; i < sampframes; i += 2) {
            inAudio[ch + channels * i] -= VERY_SMALL_BUT_NOT_DENORMAL_VALUE;
        }
    }

    this->BlockProcessInput(sampframes, channels, inAudio, rate);

    for (int32_t delayTap = 0; delayTap < kNumEarlyLateDelayTaps; delayTap++) {
        this->mEarlyLateReadPointer[delayTap] = (this->mEarlyLateSamples[delayTap] + this->mEarlyLateWritePointer - 1) & this->mEarlyLateMask;

        if (this->mEarlyLateSamples[delayTap] == this->mOldEarlyLateSamples[delayTap]) {
            this->mEarlyLateChanged[delayTap] = false;
        } else {
            this->mEarlyLateChanged[delayTap] = true;
            this->mOldEarlyLateReadPointer[delayTap] = (this->mOldEarlyLateSamples[delayTap] + this->mEarlyLateWritePointer - 1) & this->mEarlyLateMask;
            this->mOldEarlyLateSamples[delayTap] = this->mEarlyLateSamples[delayTap];
        }
    }

    for (int32_t delayLine = 0; delayLine < kNumLateReverbDelays; delayLine++) {
        this->mLateReadPointer[delayLine] = (this->mLateDelayLenSamples[delayLine] + this->mLateWritePointer[delayLine] - 1) & this->mLateMask[delayLine];
        this->mLateReadPointerB[delayLine] = (this->mLateDelayLenBSamples[delayLine] + this->mLateWritePointer[delayLine] - 1) & this->mLateMask[delayLine];
        if (this->mLateDelayLenSamples[delayLine] == this->mOldLateDelayLenSamples[delayLine]) {
            this->mLateChanged[delayLine] = false;
        } else {
            this->mLateChanged[delayLine] = true;
            this->mOldLateReadPointer[delayLine] = (this->mOldLateDelayLenSamples[delayLine] + this->mLateWritePointer[delayLine] - 1) & this->mLateMask[delayLine];
            this->mOldLateDelayLenSamples[delayLine] = this->mLateDelayLenSamples[delayLine];
        }

        this->mLatchFeedback[delayLine] = this->mFeedback[delayLine];

        if (this->mLatchFeedback[delayLine] == this->mOldFeedback[delayLine]) {
            this->mPrevFeedback[delayLine] = this->mLatchFeedback[delayLine];
        } else {
            if (!this->mLateChanged[delayLine]) {
                this->mLateChanged[delayLine] = true;
                this->mOldLateReadPointer[delayLine] = (this->mOldLateDelayLenSamples[delayLine] + this->mLateWritePointer[delayLine] - 1) & this->mLateMask[delayLine];
            }

            this->mPrevFeedback[delayLine] = this->mOldFeedback[delayLine];
            this->mOldFeedback[delayLine] = this->mLatchFeedback[delayLine];
        }

        this->mLatchDecayHF[delayLine] = this->mDecayHF[delayLine];
        this->mLatchOneMinusDecayHF[delayLine] = 1.0f - this->mLatchDecayHF[delayLine];

        if (this->mLatchDecayHF[delayLine] == this->mOldDecayHF[delayLine]) {
            this->mDecayHFChanged[delayLine] = false;
        } else {
            this->mDecayHFChanged[delayLine] = true;
            this->mPrevDecayHF[delayLine] = this->mOldDecayHF[delayLine];
            this->mOldDecayHF[delayLine] = this->mLatchDecayHF[delayLine];
        }
    }

    this->mLatchLRgain = this->mLRgain;

    if (this->mLatchLRgain == this->mOldLRgain) {
        this->mLRgainChanged = false;
    } else {
        this->mLRgainChanged = true;
        this->mPrevLRgain = this->mOldLRgain;
        this->mOldLRgain = this->mLatchLRgain;
    }

    this->mLatchERgain = this->mERgain;

    if (this->mLatchERgain == this->mOldERgain) {
        this->mERgainChanged = false;
    } else {
        this->mERgainChanged = true;
        this->mPrevERgain = this->mOldERgain;
        this->mOldERgain = this->mLatchERgain;
    }

    this->mLatchRoomHF = this->mRoomHF;

    if (this->mLatchRoomHF == this->mOldRoomHF) {
        this->mRoomHFChanged = false;
    } else {
        this->mRoomHFChanged = true;
        this->mPrevRoomHF = this->mOldRoomHF;
        this->mOldRoomHF = this->mLatchRoomHF;
    }

    this->mLatchHadamard = this->mHadamard;

    if (this->mLatchHadamard == this->mOldHadamard) {
        this->mHadamardChanged = false;
    } else {
        this->mHadamardChanged = true;
        this->mPrevHadamard = this->mOldHadamard;
        this->mOldHadamard = this->mLatchHadamard;
    }

    this->mLatchDiffusionScale = this->mDiffusionScale;

    if (this->mLatchDiffusionScale == this->mOldDiffusionScale) {
        this->mDiffusionScaleChanged = false;
    } else {
        this->mDiffusionScaleChanged = true;
        this->mPrevDiffusionScale = this->mOldDiffusionScale;
        this->mOldDiffusionScale = this->mLatchDiffusionScale;
    }

    for (uint32_t i = 0; i < sampframes; i++) {
        uint32_t sampframesmi = sampframes - i;
        val = static_cast<float>(this->mInBuff[i]);

        float roomHF;
        float hadamard;
        float diffusionscale;

        if (this->mRoomHFChanged) {
            roomHF = ((this->mPrevRoomHF * sampframesmi) + (this->mLatchRoomHF * i)) * invSampframes;
        } else {
            roomHF = this->mLatchRoomHF;
        }

        if (this->mHadamardChanged) {
            hadamard = ((this->mPrevHadamard * sampframesmi) + (this->mLatchHadamard * i)) * invSampframes;
        } else {
            hadamard = this->mLatchHadamard;
        }

        if (this->mDiffusionScaleChanged) {
            diffusionscale = ((this->mPrevDiffusionScale * sampframesmi) + (this->mLatchDiffusionScale * i)) * invSampframes;
        } else {
            diffusionscale = this->mLatchDiffusionScale;
        }

        float oneMinusRoomHF = 1.0f - roomHF;
        this->mzRoomHF0 = val * roomHF + this->mzRoomHF0 * oneMinusRoomHF;
        this->mzRoomHF1 = this->mzRoomHF0 * roomHF + this->mzRoomHF1 * oneMinusRoomHF;

        float fout;
        if (this->mRoomLF != 0.0f) {
            fout = this->mzRoomHF1 * this->mRoomLFcoeffs.a0 + this->mzRoomLF0;
            this->mzRoomLF0 = this->mzRoomHF1 * this->mRoomLFcoeffs.a1 + this->mzRoomLF1 + fout * this->mRoomLFcoeffs.b1;
            this->mzRoomLF1 = this->mzRoomHF1 * this->mRoomLFcoeffs.a2 + fout * this->mRoomLFcoeffs.b2;
        } else {
            fout = this->mzRoomHF1;
        }

        this->mEarlyLateWritePointer = --this->mEarlyLateWritePointer & this->mEarlyLateMask;
        this->mEarlyLateDelay[this->mEarlyLateWritePointer] = fout;

        this->mEarlyLateReadPointer[0] = --this->mEarlyLateReadPointer[0] & this->mEarlyLateMask;
        float input;
        float output;
        float ERgain;

        if (this->mERgainChanged) {
            ERgain = ((this->mPrevERgain * sampframesmi) + (this->mLatchERgain * i)) * invSampframes;
        } else {
            ERgain = this->mLatchERgain;
        }

        input = this->mEarlyLateDelay[this->mEarlyLateReadPointer[0]] * ERgain;

        if (this->mEarlyLateChanged[0]) {
            this->mOldEarlyLateReadPointer[0] = --this->mOldEarlyLateReadPointer[0] & this->mEarlyLateMask;
            float oldInput = this->mEarlyLateDelay[this->mOldEarlyLateReadPointer[0]] * ERgain;
            input = ((oldInput * sampframesmi) + (input * i)) * invSampframes;
        }

        for (int32_t delayLine = 0; delayLine < kNumAllpassDelays; delayLine++) {
            this->mAllpassWritePointer[delayLine] = --this->mAllpassWritePointer[delayLine] & this->mAllpassMask[delayLine];

            this->mAllpassReadPointer[delayLine] = --this->mAllpassReadPointer[delayLine] & this->mAllpassMask[delayLine];
            output = this->mAllpassDelays[delayLine][this->mAllpassReadPointer[delayLine]] - input * this->mAllpassGain;

            this->mAllpassDelays[delayLine][this->mAllpassWritePointer[delayLine]] =
                input + output * this->mAllpassGain;
            input = output;
        }

        eruOut[0] = output;

        this->mEarlyWritePointer = --this->mEarlyWritePointer & this->mEarlyMask;
        this->mEarlyDelay[this->mEarlyWritePointer] = output;

        for (int32_t delayTap = 0; delayTap < kNumEarlyDelayTaps; delayTap++) {
            this->mEarlyReadPointer[delayTap] = --this->mEarlyReadPointer[delayTap] & this->mEarlyMask;
            eruOut[delayTap + 1] = this->mEarlyDelay[this->mEarlyReadPointer[delayTap]];
        }

        for (int32_t delayLine = 0; delayLine < this->mNumLateReverbDelays; delayLine++) {
            int32_t delayTap = kNumEarly_EarlyLateDelayTaps + delayLine;

            this->mEarlyLateReadPointer[delayTap] = --this->mEarlyLateReadPointer[delayTap] & this->mEarlyLateMask;
            val = this->mEarlyLateDelay[this->mEarlyLateReadPointer[delayTap]];

            if (this->mEarlyLateChanged[delayTap]) {
                this->mOldEarlyLateReadPointer[delayTap] = --this->mOldEarlyLateReadPointer[delayTap] & this->mEarlyLateMask;
                oldVal = this->mEarlyLateDelay[this->mOldEarlyLateReadPointer[delayTap]];
                val = ((oldVal * sampframesmi) + (val * i)) * invSampframes;
            }

            if (this->mLRgainChanged) {
                float LRgain = ((this->mPrevLRgain * sampframesmi) + (this->mLatchLRgain * i)) * invSampframes;
                val *= LRgain;
            } else {
                val *= this->mLatchLRgain;
            }

            this->mLateReadPointer[delayLine] = --this->mLateReadPointer[delayLine] & this->mLateMask[delayLine];
            if (this->mLateChanged[delayLine]) {
                this->mOldLateReadPointer[delayLine] = --this->mOldLateReadPointer[delayLine] & this->mLateMask[delayLine];
                oldVal = this->mPrevFeedback[delayLine] * this->mLateDelays[delayLine][this->mOldLateReadPointer[delayLine]];
                float thisVal = this->mLatchFeedback[delayLine] * this->mLateDelays[delayLine][this->mLateReadPointer[delayLine]];
                val += (((oldVal * sampframesmi) + (thisVal * i)) * invSampframes);
            } else {
                val += (this->mLatchFeedback[delayLine] * this->mLateDelays[delayLine][this->mLateReadPointer[delayLine]]);
            }

            val *= diffusionscale;

            if (this->mDecayHFChanged[delayLine]) {
                float decayHF = ((this->mPrevDecayHF[delayLine] * sampframesmi) + (this->mLatchDecayHF[delayLine] * i)) * invSampframes;
                float oneMinusDecayHF = 1.0f - decayHF;
                this->mzDecayHF[delayLine] = val * decayHF + this->mzDecayHF[delayLine] * oneMinusDecayHF;
            } else {
                this->mzDecayHF[delayLine] = val * this->mLatchDecayHF[delayLine] + this->mzDecayHF[delayLine] * this->mLatchOneMinusDecayHF[delayLine];
            }

            this->mLateWritePointer[delayLine] = --this->mLateWritePointer[delayLine] & this->mLateMask[delayLine];
        }

        this->mMatrix[0 * kNumLateReverbDelays + 0] = this->mzDecayHF[0] + this->mzDecayHF[1] * hadamard;
        this->mMatrix[0 * kNumLateReverbDelays + 1] = this->mzDecayHF[1] - this->mzDecayHF[0] * hadamard;
        this->mMatrix[0 * kNumLateReverbDelays + 2] = this->mzDecayHF[2] + this->mzDecayHF[3] * hadamard;
        this->mMatrix[0 * kNumLateReverbDelays + 3] = this->mzDecayHF[3] - this->mzDecayHF[2] * hadamard;
        this->mMatrix[0 * kNumLateReverbDelays + 4] = this->mzDecayHF[4] + this->mzDecayHF[5] * hadamard;
        this->mMatrix[0 * kNumLateReverbDelays + 5] = this->mzDecayHF[5] - this->mzDecayHF[4] * hadamard;
        this->mMatrix[0 * kNumLateReverbDelays + 6] = this->mzDecayHF[6] + this->mzDecayHF[7] * hadamard;
        this->mMatrix[0 * kNumLateReverbDelays + 7] = this->mzDecayHF[7] - this->mzDecayHF[6] * hadamard;

        this->mMatrix[1 * kNumLateReverbDelays + 0] = this->mMatrix[0 * kNumLateReverbDelays + 0] + this->mMatrix[0 * kNumLateReverbDelays + 2] * hadamard;
        this->mMatrix[1 * kNumLateReverbDelays + 1] = this->mMatrix[0 * kNumLateReverbDelays + 1] + this->mMatrix[0 * kNumLateReverbDelays + 3] * hadamard;
        this->mMatrix[1 * kNumLateReverbDelays + 2] = this->mMatrix[0 * kNumLateReverbDelays + 2] - this->mMatrix[0 * kNumLateReverbDelays + 0] * hadamard;
        this->mMatrix[1 * kNumLateReverbDelays + 3] = this->mMatrix[0 * kNumLateReverbDelays + 3] - this->mMatrix[0 * kNumLateReverbDelays + 1] * hadamard;
        this->mMatrix[1 * kNumLateReverbDelays + 4] = this->mMatrix[0 * kNumLateReverbDelays + 4] + this->mMatrix[0 * kNumLateReverbDelays + 6] * hadamard;
        this->mMatrix[1 * kNumLateReverbDelays + 5] = this->mMatrix[0 * kNumLateReverbDelays + 5] + this->mMatrix[0 * kNumLateReverbDelays + 7] * hadamard;
        this->mMatrix[1 * kNumLateReverbDelays + 6] = this->mMatrix[0 * kNumLateReverbDelays + 6] - this->mMatrix[0 * kNumLateReverbDelays + 4] * hadamard;
        this->mMatrix[1 * kNumLateReverbDelays + 7] = this->mMatrix[0 * kNumLateReverbDelays + 7] - this->mMatrix[0 * kNumLateReverbDelays + 5] * hadamard;

        this->mMatrix[2 * kNumLateReverbDelays + 0] = this->mMatrix[1 * kNumLateReverbDelays + 0] + this->mMatrix[1 * kNumLateReverbDelays + 4] * hadamard;
        this->mMatrix[2 * kNumLateReverbDelays + 1] = this->mMatrix[1 * kNumLateReverbDelays + 1] + this->mMatrix[1 * kNumLateReverbDelays + 5] * hadamard;
        this->mMatrix[2 * kNumLateReverbDelays + 2] = this->mMatrix[1 * kNumLateReverbDelays + 2] + this->mMatrix[1 * kNumLateReverbDelays + 6] * hadamard;
        this->mMatrix[2 * kNumLateReverbDelays + 3] = this->mMatrix[1 * kNumLateReverbDelays + 3] + this->mMatrix[1 * kNumLateReverbDelays + 7] * hadamard;
        this->mMatrix[2 * kNumLateReverbDelays + 4] = this->mMatrix[1 * kNumLateReverbDelays + 4] - this->mMatrix[1 * kNumLateReverbDelays + 0] * hadamard;
        this->mMatrix[2 * kNumLateReverbDelays + 5] = this->mMatrix[1 * kNumLateReverbDelays + 5] - this->mMatrix[1 * kNumLateReverbDelays + 1] * hadamard;
        this->mMatrix[2 * kNumLateReverbDelays + 6] = this->mMatrix[1 * kNumLateReverbDelays + 6] - this->mMatrix[1 * kNumLateReverbDelays + 2] * hadamard;
        this->mMatrix[2 * kNumLateReverbDelays + 7] = this->mMatrix[1 * kNumLateReverbDelays + 7] - this->mMatrix[1 * kNumLateReverbDelays + 3] * hadamard;

        this->mLateDelays[0][this->mLateWritePointer[0]] = this->mMatrix[2 * kNumLateReverbDelays + 1];
        this->mLateDelays[1][this->mLateWritePointer[1]] = this->mMatrix[2 * kNumLateReverbDelays + 2];
        this->mLateDelays[2][this->mLateWritePointer[2]] = this->mMatrix[2 * kNumLateReverbDelays + 3];
        this->mLateDelays[3][this->mLateWritePointer[3]] = this->mMatrix[2 * kNumLateReverbDelays + 4];
        this->mLateDelays[4][this->mLateWritePointer[4]] = this->mMatrix[2 * kNumLateReverbDelays + 5];
        this->mLateDelays[5][this->mLateWritePointer[5]] = this->mMatrix[2 * kNumLateReverbDelays + 6];
        this->mLateDelays[6][this->mLateWritePointer[6]] = this->mMatrix[2 * kNumLateReverbDelays + 7];
        this->mLateDelays[7][this->mLateWritePointer[7]] = this->mMatrix[2 * kNumLateReverbDelays + 0];

        for (int32_t ch = 0; ch < channels; ch++) {
            float lruSum = 0.0f;
            float eruSum = 0.0f;
            int32_t nextDelay = ch;

            for (int32_t delay = 0; delay < (kNumLateReverbDelays / channels); delay++) {
                this->mLateReadPointerB[nextDelay] = --this->mLateReadPointerB[nextDelay] & this->mLateMask[nextDelay];
                lruSum += this->mLateDelays[nextDelay][this->mLateReadPointerB[nextDelay]];

                eruSum += eruOut[nextDelay];
                nextDelay += channels;
            }

            if (drymix == 0.0f) {
                outAudio[i * channels + ch] = eruSum + lruSum;
            } else {
                outAudio[i * channels + ch] = (inAudio[i * channels + ch] * drymix) + eruSum + lruSum;
            }
        }
    }
}

void ASfxDsp::SetAllpassDelays(float rate) {
    this->mAllpassDelayLenSec[0] = kAllpassDelayLenSec0;
    this->mAllpassDelayLenSec[1] = kAllpassDelayLenSec1;

    for (int32_t delayLine = 0; delayLine < kNumAllpassDelays; delayLine++) {
        this->mAllpassDelayLenSamples[delayLine] = static_cast<int32_t>(this->mAllpassDelayLenSec[delayLine] * rate);
        this->mAllpassReadPointer[delayLine] = this->mAllpassDelayLenSamples[delayLine] - 1;
    }
}

void ASfxDsp::SetEarlyDelay(float nextLengthSec, float delayRatio, float rate) {
    for (int32_t delayLine = 0; delayLine < kNumEarlyDelayTaps; delayLine++) {
        this->mEarlyDelayLenSec[delayLine] = nextLengthSec;
        this->mEarlyDelayLenSamples[delayLine] = static_cast<int32_t>(nextLengthSec * rate);
        nextLengthSec *= delayRatio;

        this->mEarlyReadPointer[delayLine] = this->mEarlyDelayLenSamples[delayLine] - 1;
    }
}

void ASfxDsp::SetLateDelays(float nextLengthSec, float delayRatio, float nextLengthBSec, float delayRatioB, float rate) {
    float nextLengthSamples;
    float nextLengthBSamples = nextLengthBSec * rate;

    for (int32_t delayLine = 0; delayLine < kNumLateReverbDelays; delayLine++) {
        this->mLateDelayLenSec[delayLine] = nextLengthSec;
        nextLengthSamples = nextLengthSec * rate;
        this->mLateDelayLenSamples[delayLine] = static_cast<int32_t>(nextLengthSamples + 0.5);
        nextLengthSec *= delayRatio;
        this->mLateReadPointer[delayLine] = (this->mLateDelayLenSamples[delayLine] + this->mLateWritePointer[delayLine] - 1) & this->mLateMask[delayLine];

        this->mLateDelayLenBSamples[delayLine] = static_cast<int32_t>(nextLengthBSamples + 0.5);
        nextLengthBSamples *= delayRatioB;
        this->mLateReadPointerB[delayLine] = (this->mLateDelayLenBSamples[delayLine] + this->mLateWritePointer[delayLine] - 1) & this->mLateMask[delayLine];
    }
}

void ASfxDsp::SetLate_EarlyLateDelayTaps(float refPlusRevDelaySec, float nextLengthSec, float delayRatio, float rate) {
    this->mEarlyLateSec[kNumEarly_EarlyLateDelayTaps] = refPlusRevDelaySec;
    auto earlyLateSamples = static_cast<int32_t>(refPlusRevDelaySec * rate);
    if (!earlyLateSamples) {
        earlyLateSamples++;
    }
    this->mEarlyLateSamples[kNumEarly_EarlyLateDelayTaps] = earlyLateSamples;

    for (int32_t delayTap = kNumEarly_EarlyLateDelayTaps + 1; delayTap < kNumEarlyLateDelayTaps; delayTap++) {
        this->mEarlyLateSec[delayTap] = refPlusRevDelaySec + nextLengthSec;
        this->mEarlyLateSamples[delayTap] = static_cast<int32_t>(this->mEarlyLateSec[delayTap] * rate);
        nextLengthSec *= delayRatio;
    }
}

bool ASfxDsp::UpdateBufferSize(int32_t newMaxBlockSize) {
    if (newMaxBlockSize == this->mNumAllocatedInBuffSamples) {
        return false;
    }

    this->mNumAllocatedInBuffSamples = newMaxBlockSize;

    if (this->mInBuff) {
        FMOD_Memory_Free(this->mInBuff);
    }

    this->mInBuff = static_cast<float*>(FMOD_Memory_Alloc(newMaxBlockSize * sizeof(float)));

    return true;
}

void ASfxDsp::ZeroWritePointers() {
    int32_t delayLine;

    this->mEarlyWritePointer = 0;
    this->mEarlyLateWritePointer = 0;

    for (delayLine = 0; delayLine < kNumAllpassDelays; delayLine++) {
        this->mAllpassWritePointer[delayLine] = 0;
    }

    for (delayLine = 0; delayLine < kNumLateReverbDelays; delayLine++) {
        this->mLateWritePointer[delayLine] = 0;
    }
}

void ASfxDsp::close() {
    if (this->mInBuff) {
        FMOD_Memory_Free(this->mInBuff);
    }

    this->mInBuff = nullptr;

    this->DeallocateLateDelays();
    this->DeallocateEarlyLateDelay();
    this->DeallocateAllpassDelays();
    this->DeallocateEarlyDelay();
}

int32_t ASfxDsp::init(float rate) {
    int32_t delayLine;
    int32_t error = 0;

    this->mLateDelays = nullptr;
    this->mEarlyLateDelay = nullptr;
    this->mEarlyDelay = nullptr;
    this->mAllpassDelays = nullptr;

    this->ZeroWritePointers();

    this->mEarlyLateSec[0] = 0.040f;
    this->mEarlyLateSamples[0] = static_cast<int32_t>(this->mEarlyLateSec[0] * rate);
    this->SetLate_EarlyLateDelayTaps(0.060f, kEarlyLateNextLengthSec, kEarlyLateDelayRatio, rate);

    for (delayLine = 0; delayLine < kNumEarlyLateDelayTaps; delayLine++) {
        this->mOldEarlyLateSamples[delayLine] = this->mEarlyLateSamples[delayLine];
    }
    for (delayLine = 0; delayLine < kNumLateReverbDelays; delayLine++) {
        this->mLateMask[delayLine] = 0;
    }

    this->SetEarlyDelay(kEarlyFirstDelaySec, kEarlyDelayRatio, rate);
    this->SetAllpassDelays(rate);
    this->SetLateDelays(LOWEST_DELAY_LINE_LENGTH_SEC, DELAY_LINE_RATIO, LOWEST_DELAY_LINE_LENGTH_B_SEC, DELAY_LINE_RATIO_B, rate);

    for (delayLine = 0; delayLine < kNumLateReverbDelays; delayLine++) {
        this->mOldLateDelayLenSamples[delayLine] = this->mLateDelayLenSamples[delayLine];
        this->mLateChanged[delayLine] = false;
        this->mDecayHFChanged[delayLine] = false;
    }

    error = this->AllocateLateDelays(kNumLateReverbDelays, this->mLateDelayLenSec, rate);
    if (error) {
        return error;
    }
    error = this->AllocateEarlyLateDelay(this->mEarlyLateSec, rate);
    if (error) {
        return error;
    }
    error = this->AllocateAllpassDelays(kNumAllpassDelays, this->mAllpassDelayLenSec, rate);
    if (error) {
        return error;
    }
    error = this->AllocateEarlyDelay(this->mEarlyDelayLenSec[kNumEarlyDelayTaps - 1], rate);
    if (error) {
        return error;
    }

    this->mNumAllocatedInBuffSamples = 0;
    this->mInBuff = 0;

    for (delayLine = 0; delayLine < kNumLateReverbDelays; delayLine++) {
        this->mOldFeedback[delayLine] = this->mPrevFeedback[delayLine] = this->mLatchFeedback[delayLine] = this->mFeedback[delayLine] = 0.32f;
        this->mLatchDecayHF[delayLine] = this->mPrevDecayHF[delayLine] = this->mOldDecayHF[delayLine] = this->mDecayHF[delayLine] = 0.40f;
        this->mLatchOneMinusDecayHF[delayLine] = 1.0f - this->mDecayHF[delayLine];
    }

    this->mOldHadamard = this->mPrevHadamard = this->mLatchHadamard = this->mHadamard = 1.0f;
    this->mOldDiffusionScale = this->mPrevDiffusionScale = this->mLatchDiffusionScale = this->mDiffusionScale = 1.0f;
    this->mOldRoomHF = this->mPrevRoomHF = this->mLatchRoomHF = this->mRoomHF = 0.98f;
    this->mRoomHFChanged = false;

    FMOD_memset(&this->mRoomLFcoeffs, 0, sizeof(coeff2ndorder));
    this->mRoomLFcoeffs.a0 = 1.0f;
    this->mRoomLF = 0.0f;

    this->mDiffusionScale = 0.25f;
    this->mAllpassGain = 0.63f;
    this->mPrevLRgain = this->mOldLRgain = this->mLatchLRgain = this->mLRgain = 0.0f;
    this->mLRgainChanged = false;
    this->mPrevERgain = this->mOldERgain = this->mLatchERgain = this->mERgain = 0.0f;
    this->mERgainChanged = false;
    this->ClearReverbInternalBuffers();

    return 0;
}

int32_t ASfxDsp::NextPowerOf2(int32_t delaySamples) {
    return 1 << (static_cast<int32_t>(FMOD_LOG(static_cast<float>(delaySamples)) / FMOD_LOG(2.0f)) + 1);
}
