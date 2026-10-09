#include "xmodex/dsp/DSPResamplerMultiInput.hpp"
#include "xmodex/SystemI.hpp"
#include "xmodex/dsp/resampler/Cubic.hpp"
#include "xmodex/dsp/resampler/Linear.hpp"
#include "xmodex/dsp/resampler/NoInterp.hpp"
#include "xmodex/dsp/resampler/Spline.hpp"

namespace FMOD {

DSPResamplerMultiInput::DSPResamplerMultiInput() {
}

FMOD_RESULT DSPResamplerMultiInput::execute(float* inbuffer, float** outbuffer, uint32_t* length, int32_t inchannels, int32_t* outchannels, FMOD_SPEAKERMODE speakermode) {
    FMOD_RESULT result = FMOD_OK;
    int32_t channels = 0;
    float* readbuffer;

    if (this->mFlags & FMOD_DSP_FLAG_FINISHED) {
        return FMOD_OK;
    }

    this->mFlags &= ~FMOD_DSP_FLAG_IDLE;

    if (this->mFlags & FMOD_DSP_FLAG_VISITED) {
        readbuffer = this->mBuffer;
    } else {
        int32_t inlength = *length;
        int32_t readoffset = 0;
        int32_t tempbuffindex = this->mSystem->mDSPTempBuffIndex;
        FMOD_SINT64P speed;

        readbuffer = this->mBuffer;
        speed.mValue = this->mSpeed.mValue;

        if (!this->mBuffer) {
            return FMOD_ERR_INTERNAL;
        }

        do {
            uint32_t rlength;
            FMOD_RESAMPLER_END endflag = FMOD_RESAMPLER_END_MIXBUFFER;

            while (this->mFill) {
                auto oldresamplebufferpos = this->mResampleBufferPos;
                auto resamplebuff = static_cast<float*>(this->mResampleBuffer) + (this->mResampleBufferPos * this->mResampleBufferChannels);
                auto readlen = this->mResampleBlockLength;
                float* outbuff;
                int32_t outch;

                this->resetVisited();

                this->mSystem->mDSPTempBuffIndex = tempbuffindex;

                result = DSPFilter::execute(resamplebuff, &outbuff, &readlen, inchannels, &outch, speakermode);
                if (result != FMOD_OK) {
                    outbuff = resamplebuff;
                    FMOD_memset(outbuff, 0, readlen * this->mResampleBufferChannels * sizeof(float));
                    this->mResampleFinishPos = this->mResampleBufferPos;
                }

                this->mResampleBufferChannels = outch;

                if (outbuff != resamplebuff) {
                    FMOD_memcpy(resamplebuff, outbuff, readlen * this->mResampleBufferChannels * sizeof(float));
                }

                this->mResampleBufferPos += readlen;
                if (this->mResampleBufferPos >= this->mResampleBufferLength) {
                    this->mResampleBufferPos = 0;
                }

                if (!oldresamplebufferpos) {
                    for (int32_t i = 0; i < this->mResampleBufferChannels * this->mOverflowLength * 2; i++) {
                        (static_cast<float*>(this->mResampleBuffer))[(this->mResampleBufferLength * this->mResampleBufferChannels) + i] = (static_cast<float*>(this->mResampleBuffer))[i];
                    }
                }

                this->mFill--;
            }

            rlength = inlength;

            if (speed.mValue > 0x100) {
                FMOD_UINT64P mixesleft;
                FMOD_UINT64P fracleft;
                int32_t nextpos;

                nextpos = this->mResamplePosition.mHi - this->mOverflowLength;
                nextpos /= static_cast<int32_t>(this->mResampleBlockLength);
                nextpos++;
                nextpos *= this->mResampleBlockLength;
                nextpos += this->mOverflowLength;

                mixesleft.mHi = nextpos;
                mixesleft.mLo = 0;
                mixesleft.mValue -= this->mResamplePosition.mValue;

                fracleft.mValue = mixesleft.mValue % speed.mValue;
                mixesleft.mValue /= speed.mValue;

                if (fracleft.mValue) {
                    mixesleft.mValue++;
                }

                if (mixesleft.mValue <= rlength) {
                    endflag = FMOD_RESAMPLER_END_RESAMPLEBUFFER;
                    rlength = mixesleft.mLo;
                }
            }

            if (speed.mHi == 1 && speed.mLo == 0) {
                FMOD_memcpy(readbuffer + (readoffset * this->mResampleBufferChannels), static_cast<float*>(this->mResampleBuffer) + (this->mResamplePosition.mHi * this->mResampleBufferChannels), rlength * sizeof(float) * this->mResampleBufferChannels);
                this->mResamplePosition.mValue += speed.mValue * rlength;
            } else {
                switch (this->mSystem->mResampleMethod) {
                case FMOD_DSP_RESAMPLER_NOINTERP: {
                    FMOD_Resampler_NoInterp(
                        readbuffer + (readoffset * this->mResampleBufferChannels),
                        rlength,
                        this->mResampleBuffer,
                        FMOD_SOUND_FORMAT_PCMFLOAT,
                        &this->mResamplePosition,
                        &speed,
                        this->mResampleBufferChannels);
                    break;
                }
                case FMOD_DSP_RESAMPLER_LINEAR: {
                    FMOD_Resampler_Linear(
                        readbuffer + (readoffset * this->mResampleBufferChannels),
                        rlength,
                        this->mResampleBuffer,
                        FMOD_SOUND_FORMAT_PCMFLOAT,
                        &this->mResamplePosition,
                        &speed,
                        this->mResampleBufferChannels);
                    break;
                }
                case FMOD_DSP_RESAMPLER_CUBIC: {
                    FMOD_Resampler_Cubic(
                        readbuffer + (readoffset * this->mResampleBufferChannels),
                        rlength,
                        this->mResampleBuffer,
                        FMOD_SOUND_FORMAT_PCMFLOAT,
                        &this->mResamplePosition,
                        &speed,
                        this->mResampleBufferChannels);
                    break;
                }
                case FMOD_DSP_RESAMPLER_SPLINE: {
                    FMOD_Resampler_Spline(
                        readbuffer + (readoffset * this->mResampleBufferChannels),
                        rlength,
                        this->mResampleBuffer,
                        FMOD_SOUND_FORMAT_PCMFLOAT,
                        &this->mResamplePosition,
                        &speed,
                        this->mResampleBufferChannels);
                    break;
                }
                default: {
                    FMOD_Resampler_Linear(
                        readbuffer + (readoffset * this->mResampleBufferChannels),
                        rlength, this->mResampleBuffer,
                        FMOD_SOUND_FORMAT_PCMFLOAT,
                        &this->mResamplePosition,
                        &speed,
                        this->mResampleBufferChannels);
                    break;
                }
                }
            }

            if (this->mResamplePosition.mHi >= (this->mResampleBufferLength + this->mOverflowLength)) {
                this->mResamplePosition.mHi -= this->mResampleBufferLength;
            }

            inlength -= rlength;
            readoffset += rlength;

            if (endflag == FMOD_RESAMPLER_END_SOUND) {
                this->mFlags |= FMOD_DSP_FLAG_FINISHED;
                break;
            } else if (endflag == FMOD_RESAMPLER_END_RESAMPLEBUFFER) {
                this->mFill++;
            }
        } while (inlength > 0);

        channels = this->mResampleBufferChannels;
    }

    *outbuffer = readbuffer;
    *outchannels = channels;

    return result;
}

FMOD_RESULT DSPResamplerMultiInput::addInput(DSPI* target) {
    FMOD_RESULT result;

    result = DSPI::addInput(target);
    if (result != FMOD_OK) {
        return result;
    }

    this->mResampleBufferPos = 0;
    this->mFill = 2;
    this->mPosition.mValue = 0;
    this->mResamplePosition.mValue = 0;

    return FMOD_OK;
}

} // namespace FMOD
