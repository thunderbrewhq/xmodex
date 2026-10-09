#include "xmodex/dsp/DSPResampler.hpp"
#include "xmodex/Math.hpp"
#include "xmodex/Memory.hpp"
#include "xmodex/SystemI.hpp"
#include "xmodex/dsp/DSPCodecADPCM.hpp"
#include "xmodex/dsp/DSPCodecMPEG.hpp"
#include "xmodex/dsp/DSPCodecRaw.hpp"
#include "xmodex/dsp/resampler/Cubic.hpp"
#include "xmodex/dsp/resampler/Linear.hpp"
#include "xmodex/dsp/resampler/NoInterp.hpp"
#include "xmodex/dsp/resampler/Spline.hpp"

namespace FMOD {

DSPResampler::DSPResampler() {
    this->mResampleBufferMemory = nullptr;
    this->mResampleBuffer = nullptr;
    this->mTargetFrequency = 0;
    this->mSpeed.mLo = 0;
    this->mSpeed.mHi = 0;
    this->mResampleBufferPos = 0;
    this->mResamplePosition.mLo = 0;
    this->mResamplePosition.mHi = 0;
    this->mPosition.mLo = 0;
    this->mPosition.mHi = 0;
    this->mOverflowLength = 4;
    this->mResampleFinishPos = static_cast<uint32_t>(-1);
    this->mFill = 2;
}

FMOD_RESULT DSPResampler::release(bool freethis) {
    DSPI::release(false);

    if (this->mResampleBufferMemory) {
        FMOD_Memory_Free(this->mResampleBufferMemory);
        this->mResampleBufferMemory = nullptr;
    }

    if (freethis) {
        FMOD_Memory_Free(this);
    }

    return FMOD_OK;
}

FMOD_RESULT DSPResampler::setFrequency(float frequency) {
    this->mFrequency = frequency;
    this->mSpeed.mValue = static_cast<FMOD_SINT64>((frequency / this->mTargetFrequency) * 4294967296.0f);

    return FMOD_OK;
}

FMOD_RESULT DSPResampler::setPosition(uint32_t position, bool processinputs) {
    FMOD_RESULT result;

    result = DSPI::setPosition(position, processinputs);
    if (result != FMOD_OK) {
        return result;
    }

    this->mResampleBufferPos = 0;
    this->mResampleFinishPos = static_cast<uint32_t>(-1);
    this->mFill = 2;
    this->mResamplePosition.mValue = 0;
    this->mPosition.mHi = position;
    this->mPosition.mLo = 0;

    return FMOD_OK;
}

FMOD_RESULT DSPResampler::alloc(FMOD_DSP_DESCRIPTION_EX* description) {
    FMOD_RESULT result;
    int32_t channels;
    uint32_t bytespersample;

    result = DSPI::alloc(description);
    if (result != FMOD_OK) {
        return result;
    }

    this->mSystem->getSoftwareFormat(&this->mTargetFrequency, nullptr, nullptr, nullptr, nullptr, nullptr);

    if (description->mResamplerBlockLength) {
        this->mResampleBlockLength = description->mResamplerBlockLength;
        channels = description->channels;
    } else {
        result = this->mSystem->getDSPBufferSize(&this->mResampleBlockLength, nullptr);
        if (result != FMOD_OK) {
            return result;
        }
        channels = this->mSystem->mMaxInputChannels;
    }

    this->mResampleBufferLength = this->mResampleBlockLength * 2;

    if (this->mDescription.mFormat == FMOD_SOUND_FORMAT_NONE) {
        this->mDescription.mFormat = FMOD_SOUND_FORMAT_PCMFLOAT;
    }

    SoundI::getBytesFromSamples(1, &bytespersample, channels, this->mDescription.mFormat);

    if (this->mDescription.mCategory == FMOD_DSP_CATEGORY_DSPCODECMPEG) {
        auto dspcodecmpeg = static_cast<DSPCodecMPEG*>(this);
        this->mResampleBuffer = reinterpret_cast<char*>(FMOD_ALIGNPOINTER(dspcodecmpeg->mResampleBufferMemory, 16));
    } else if (this->mDescription.mCategory == FMOD_DSP_CATEGORY_DSPCODECADPCM) {
        auto dspcodecadpcm = static_cast<DSPCodecADPCM*>(this);
        this->mResampleBuffer = reinterpret_cast<char*>(FMOD_ALIGNPOINTER(dspcodecadpcm->mResampleBufferMemory, 16));
    } else if (this->mDescription.mCategory == FMOD_DSP_CATEGORY_DSPCODECRAW) {
        auto dspcodecraw = static_cast<DSPCodecRaw*>(this);
        this->mResampleBuffer = reinterpret_cast<char*>(FMOD_ALIGNPOINTER(dspcodecraw->mResampleBufferMemory, 16));
    } else {
        this->mResampleBufferMemory = FMOD_Memory_Calloc(((this->mResampleBufferLength + (this->mOverflowLength * 4)) * bytespersample) + 16);
        if (!this->mResampleBufferMemory) {
            return FMOD_ERR_MEMORY;
        }
        this->mResampleBuffer = reinterpret_cast<char*>(FMOD_ALIGNPOINTER(this->mResampleBufferMemory, 16));
    }

    this->mResampleBuffer = reinterpret_cast<char*>(this->mResampleBuffer) + (this->mOverflowLength * bytespersample);
    this->mResampleBuffer = reinterpret_cast<char*>(FMOD_ALIGNPOINTER(this->mResampleBuffer, 16));

    this->mResampleBufferPos = 0;
    this->mResampleFinishPos = static_cast<uint32_t>(-1);
    this->mFill = 2;
    this->mPosition.mValue = 0;
    this->mResamplePosition.mValue = 0;

    return FMOD_OK;
}

FMOD_RESULT DSPResampler::execute(float* inbuffer, float** outbuffer, uint32_t* length, int32_t inchannels, int32_t* outchannels, FMOD_SPEAKERMODE speakermode) {
    FMOD_RESULT result = FMOD_OK;
    uint32_t bytespersample;
    int32_t len;
    int32_t readoffset;
    FMOD_SINT64P speed;
    float* buffer;

    if (this->mFlags & FMOD_DSP_FLAG_FINISHED) {
        return FMOD_OK;
    }

    this->mFlags &= ~FMOD_DSP_FLAG_IDLE;

    SoundI::getBytesFromSamples(1, &bytespersample, this->mDescription.channels, this->mDescription.mFormat);

    if (this->mFlags & FMOD_DSP_FLAG_VISITED) {
        this->mFlags &= ~FMOD_DSP_FLAG_IDLE;

        *outbuffer = this->mBuffer;
        *outchannels = this->mDescription.channels;
        return FMOD_OK;
    }

    len = *length;
    readoffset = 0;
    buffer = this->mBuffer ? this->mBuffer : inbuffer;

    speed.mValue = this->mSpeed.mValue;

    do {
        uint32_t rlength;
        FMOD_RESAMPLER_END endflag = FMOD_RESAMPLER_END_MIXBUFFER;

        while (this->mFill) {
            uint32_t readlen;
            uint32_t oldresamplebufferpos;
            void* resamplebuff;

            oldresamplebufferpos = this->mResampleBufferPos;
            readlen = this->mResampleBlockLength;
            resamplebuff = reinterpret_cast<char*>(this->mResampleBuffer) + (this->mResampleBufferPos * bytespersample);

            if (static_cast<int32_t>(this->mResampleFinishPos) == -1) {
                instance = reinterpret_cast<FMOD_DSP*>(this);

                result = this->mDescription.read(static_cast<FMOD_DSP_STATE*>(this), 0, reinterpret_cast<float*>(resamplebuff), readlen, this->mDescription.channels, this->mDescription.channels);
            } else {
                FMOD_memset(resamplebuff, 0, readlen * this->mDescription.channels * sizeof(int16_t));
                result = FMOD_OK;
            }

            this->mResampleBufferPos += readlen;
            if (this->mResampleBufferPos >= this->mResampleBufferLength) {
                this->mResampleBufferPos = 0;
            }

            if (static_cast<int32_t>(this->mResampleFinishPos) == -1 && result != FMOD_OK) {
                this->mResampleFinishPos = this->mResampleBufferPos;
                if (!this->mResampleFinishPos) {
                    this->mResampleFinishPos = this->mResampleBufferLength;
                }
            }

            if (!oldresamplebufferpos) {
                for (uint32_t i = 0; i < this->mOverflowLength * 2 * bytespersample; i++) {
                    (reinterpret_cast<uint8_t*>(this->mResampleBuffer))[(this->mResampleBufferLength * bytespersample) + i] = (reinterpret_cast<uint8_t*>(this->mResampleBuffer))[i];
                }
            }

            this->mFill--;
        }

        rlength = len;

        if (speed.mValue > 0x100) {
            FMOD_UINT64P mixesleft;
            FMOD_UINT64P fracleft;
            int32_t nextpos;
            bool endofsound = false;

            nextpos = this->mResamplePosition.mHi - this->mOverflowLength;
            nextpos /= static_cast<int32_t>(this->mResampleBlockLength);
            nextpos++;
            nextpos *= this->mResampleBlockLength;
            nextpos += this->mOverflowLength;

            mixesleft.mHi = nextpos;
            mixesleft.mLo = 0;
            mixesleft.mValue -= this->mResamplePosition.mValue;

            if (this->mResampleFinishPos != static_cast<uint32_t>(-1)) {
                FMOD_UINT64P mixeslefttofinish;

                mixeslefttofinish.mHi = this->mResampleFinishPos;
                mixeslefttofinish.mLo = 0;
                mixeslefttofinish.mValue -= this->mResamplePosition.mValue;

                if (mixeslefttofinish.mValue <= mixesleft.mValue) {
                    mixesleft.mValue = mixeslefttofinish.mValue;
                    endofsound = true;
                }
            }

            fracleft.mValue = mixesleft.mValue % speed.mValue;
            mixesleft.mValue /= speed.mValue;

            if (fracleft.mValue) {
                mixesleft.mValue++;
            }

            if (mixesleft.mValue <= rlength) {
                if (endofsound) {
                    endflag = FMOD_RESAMPLER_END_SOUND;
                } else {
                    endflag = FMOD_RESAMPLER_END_RESAMPLEBUFFER;
                }
                rlength = mixesleft.mLo;
            }
        }

        if (speed.mHi == 1 && speed.mLo == 0) {
            FMOD_Resampler_NoInterp(buffer + (readoffset * this->mDescription.channels), rlength, this->mResampleBuffer, this->mDescription.mFormat, &this->mResamplePosition, &speed, this->mDescription.channels);
        } else {
            switch (this->mSystem->mResampleMethod) {
            case FMOD_DSP_RESAMPLER_NOINTERP: {
                FMOD_Resampler_NoInterp(buffer + (readoffset * this->mDescription.channels), rlength, this->mResampleBuffer, this->mDescription.mFormat, &this->mResamplePosition, &speed, this->mDescription.channels);
                break;
            }
            case FMOD_DSP_RESAMPLER_LINEAR: {
                FMOD_Resampler_Linear(buffer + (readoffset * this->mDescription.channels), rlength, this->mResampleBuffer, this->mDescription.mFormat, &this->mResamplePosition, &speed, this->mDescription.channels);
                break;
            }
            case FMOD_DSP_RESAMPLER_CUBIC: {
                FMOD_Resampler_Cubic(buffer + (readoffset * this->mDescription.channels), rlength, this->mResampleBuffer, this->mDescription.mFormat, &this->mResamplePosition, &speed, this->mDescription.channels);
                break;
            }
            case FMOD_DSP_RESAMPLER_SPLINE: {
                FMOD_Resampler_Spline(buffer + (readoffset * this->mDescription.channels), rlength, this->mResampleBuffer, this->mDescription.mFormat, &this->mResamplePosition, &speed, this->mDescription.channels);
                break;
            }
            default: {
                FMOD_Resampler_Linear(buffer + (readoffset * this->mDescription.channels), rlength, this->mResampleBuffer, this->mDescription.mFormat, &this->mResamplePosition, &speed, this->mDescription.channels);
                break;
            }
            }
        }

        if (this->mResamplePosition.mHi >= (this->mResampleBufferLength + this->mOverflowLength)) {
            this->mResamplePosition.mHi -= this->mResampleBufferLength;
        }

        len -= rlength;
        readoffset += rlength;

        if (endflag == FMOD_RESAMPLER_END_SOUND) {
            FMOD_memset(buffer + (readoffset * this->mDescription.channels), 0, len * this->mDescription.channels * sizeof(float));
            this->mFlags |= FMOD_DSP_FLAG_FINISHED;
            break;
        } else if (endflag == FMOD_RESAMPLER_END_RESAMPLEBUFFER) {
            this->mFill++;
        }
    } while (len > 0);

    {
        uint32_t endpoint;

        if (this->mLength < this->mLoopStart + this->mLoopLength) {
            this->mLoopLength = this->mLength - this->mLoopStart;
        }

        if (this->mMode & FMOD_LOOP_NORMAL && this->mLoopCount) {
            endpoint = this->mLoopStart + this->mLoopLength - 1;
        } else {
            endpoint = this->mLength - 1;
        }

        this->mPosition.mValue += speed.mValue * *length;

        if (this->mPosition.mHi > endpoint) {
            if ((this->mMode & FMOD_LOOP_NORMAL && this->mLoopCount) || this->mLength == static_cast<uint32_t>(-1)) {
                this->mPosition.mHi -= this->mLoopLength;
            } else {
                this->mPosition.mHi = this->mLength;
            }
        }
    }

    *outbuffer = buffer;
    *outchannels = this->mDescription.channels;

    return FMOD_OK;
}

} // namespace FMOD
