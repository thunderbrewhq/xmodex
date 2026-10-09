#include "xmodex/dsp/DSPWaveTable.hpp"
#include "xmodex/SystemI.hpp"
#include "xmodex/channel/ChannelReal.hpp"
#include "xmodex/dsp/DSPResampler.hpp"
#include "xmodex/dsp/resampler/Cubic.hpp"
#include "xmodex/dsp/resampler/Linear.hpp"
#include "xmodex/dsp/resampler/NoInterp.hpp"
#include "xmodex/dsp/resampler/Spline.hpp"
#include "xmodex/sound/SampleSoftware.hpp"

namespace FMOD {

DSPWaveTable::DSPWaveTable() {
}

FMOD_RESULT DSPWaveTable::setPositionInternal(uint32_t position) {
    if (!this->mSound) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (position >= this->mSound->mLength) {
        position = this->mSound->mLength;
    }

    this->mPosition.mHi = position;
    this->mPosition.mLo = 0;

    return FMOD_OK;
}

FMOD_RESULT DSPWaveTable::setParameterInternal(int32_t index, float value) {
    return FMOD_ERR_INVALID_PARAM;
}

FMOD_RESULT DSPWaveTable::getParameterInternal(int32_t index, float* value, char* valuestr) {
    return FMOD_ERR_INVALID_PARAM;
}

FMOD_RESULT DSPWaveTable::alloc(FMOD_DSP_DESCRIPTION_EX* description) {
    FMOD_RESULT result;

    result = DSPI::alloc(description);
    if (result != FMOD_OK) {
        return result;
    }

    this->mFrequency = 0;

    this->mSystem->getSoftwareFormat(&this->mTargetFrequency, nullptr, nullptr, nullptr, nullptr, nullptr);

    this->mDirection = DSPWAVETABLE_SPEEDDIR_FORWARDS;

    return FMOD_OK;
}

FMOD_RESULT DSPWaveTable::addInput(DSPI* target) {
    return DSPI::addInput(target);
}

FMOD_RESULT DSPWaveTable::execute(float* inbuffer, float** outbuffer, uint32_t* length, int32_t inchannels, int32_t* outchannels, FMOD_SPEAKERMODE speakermode) {
    *outbuffer = inbuffer;

    this->mFlags &= ~FMOD_DSP_FLAG_IDLE;

    if (this->mFlags & FMOD_DSP_FLAG_VISITED) {
        *outbuffer = this->mBuffer;
    } else {
        void* readbuffer;
        uint32_t outlength = *length;
        uint32_t readoffset = 0;

        auto samplesoftware = static_cast<SampleSoftware*>(this->mSound);
        if (!samplesoftware) {
            return FMOD_ERR_INVALID_PARAM;
        }

        if (this->mBuffer) {
            *outbuffer = this->mBuffer;
        }

        readbuffer = samplesoftware->mBuffer;
        if (!readbuffer) {
            FMOD_memset(*outbuffer, 0, *length * sizeof(float) * this->mSound->mChannels);
            this->mFlags |= FMOD_DSP_FLAG_FINISHED;
            *outchannels = this->mSound->mChannels;
            return FMOD_OK;
        }

        do {
            uint32_t rlength;
            FMOD_SINT64P speed;
            FMOD_RESAMPLER_END endflag = FMOD_RESAMPLER_END_MIXBUFFER;

            speed = this->mSpeed;
            if (this->mDirection == DSPWAVETABLE_SPEEDDIR_BACKWARDS) {
                speed.mValue = -speed.mValue;
            }

            rlength = outlength;

            if (this->mSpeed.mValue > 0x100) {
                FMOD_UINT64P mixesleft;
                FMOD_UINT64P fracleft;

                if (this->mDirection == DSPWAVETABLE_SPEEDDIR_BACKWARDS) {
                    mixesleft.mHi = this->mPosition.mHi;
                    if (mixesleft.mHi >= this->mChannel->mLoopStart) {
                        mixesleft.mHi -= this->mChannel->mLoopStart;
                    }
                    mixesleft.mLo = 0;
                } else {
                    uint32_t endpoint;

                    if (this->mChannel->mMode & (FMOD_LOOP_NORMAL | FMOD_LOOP_BIDI) && this->mChannel->mLoopCount) {
                        endpoint = this->mChannel->mLoopStart + this->mChannel->mLoopLength;
                    } else {
                        endpoint = this->mSound->mLength - 1;
                    }

                    if (endpoint > this->mPosition.mHi) {
                        mixesleft.mHi = endpoint;
                        mixesleft.mLo = 0;
                        mixesleft.mValue -= this->mPosition.mValue;
                    } else {
                        mixesleft.mValue = 0;
                    }
                }

                fracleft.mValue = mixesleft.mValue % this->mSpeed.mValue;
                mixesleft.mValue /= this->mSpeed.mValue;

                if (fracleft.mValue) {
                    mixesleft.mValue++;
                }

                if (mixesleft.mLo <= outlength) {
                    endflag = FMOD_RESAMPLER_END_SOUND;
                    rlength = mixesleft.mLo;
                }
            }

            if (this->mSpeed.mHi == 1 && speed.mLo == 0) {
                FMOD_Resampler_NoInterp(*outbuffer + (readoffset * this->mSound->mChannels), rlength, readbuffer, this->mSound->mFormat, &this->mPosition, &speed, this->mSound->mChannels);
            } else {
                switch (this->mSystem->mResampleMethod) {
                case FMOD_DSP_RESAMPLER_NOINTERP: {
                    FMOD_Resampler_NoInterp(*outbuffer + (readoffset * this->mSound->mChannels), rlength, readbuffer, this->mSound->mFormat, &this->mPosition, &speed, this->mSound->mChannels);
                    break;
                }
                case FMOD_DSP_RESAMPLER_LINEAR: {
                    FMOD_Resampler_Linear(*outbuffer + (readoffset * this->mSound->mChannels), rlength, readbuffer, this->mSound->mFormat, &this->mPosition, &speed, this->mSound->mChannels);
                    break;
                }
                case FMOD_DSP_RESAMPLER_CUBIC: {
                    FMOD_Resampler_Cubic(*outbuffer + (readoffset * this->mSound->mChannels), rlength, readbuffer, this->mSound->mFormat, &this->mPosition, &speed, this->mSound->mChannels);
                    break;
                }
                case FMOD_DSP_RESAMPLER_SPLINE: {
                    FMOD_Resampler_Spline(*outbuffer + (readoffset * this->mSound->mChannels), rlength, readbuffer, this->mSound->mFormat, &this->mPosition, &speed, this->mSound->mChannels);
                    break;
                }
                default: {
                    FMOD_Resampler_Linear(*outbuffer + (readoffset * this->mSound->mChannels), rlength, readbuffer, this->mSound->mFormat, &this->mPosition, &speed, this->mSound->mChannels);
                    break;
                }
                }
            }

            outlength -= rlength;
            readoffset += rlength;

            if (endflag == FMOD_RESAMPLER_END_SOUND) {
                if (this->mChannel->mMode & FMOD_LOOP_BIDI && this->mChannel->mLoopCount) {
                    if (static_cast<int32_t>(this->mPosition.mHi) < 0) {
                        this->mPosition.mHi = 0;
                    }

                    if (this->mDirection == DSPWAVETABLE_SPEEDDIR_FORWARDS) {
                        this->mDirection = DSPWAVETABLE_SPEEDDIR_BACKWARDS;
                    } else {
                        this->mDirection = DSPWAVETABLE_SPEEDDIR_FORWARDS;
                    }
                } else if (this->mChannel->mMode & FMOD_LOOP_NORMAL && this->mChannel->mLoopCount) {
                    if (this->mDirection == DSPWAVETABLE_SPEEDDIR_BACKWARDS) {
                        this->mPosition.mHi += this->mChannel->mLoopLength;

                        while (this->mPosition.mHi < this->mChannel->mLoopStart) {
                            this->mPosition.mHi += this->mChannel->mLoopLength;
                        }
                    } else {
                        this->mPosition.mHi -= this->mChannel->mLoopLength;

                        while (this->mPosition.mHi >= this->mChannel->mLoopStart + this->mChannel->mLoopLength) {
                            this->mPosition.mHi -= this->mChannel->mLoopLength;
                        }

                        if (this->mChannel->mLoopCount > 0) {
                            this->mChannel->mLoopCount--;
                        }
                    }
                } else {
                    this->mPosition.mHi = this->mSound->mLength;
                    this->mPosition.mLo = 0;

                    FMOD_memset(*outbuffer + (readoffset * this->mSound->mChannels), 0, outlength * this->mSound->mChannels * sizeof(float));

                    this->mFlags |= FMOD_DSP_FLAG_FINISHED;
                    break;
                }
            }
        } while (outlength);
    }

    *outchannels = this->mSound->mChannels;

    return FMOD_OK;
}

FMOD_RESULT DSPWaveTable::setFrequency(float frequency) {
    if (frequency < 0) {
        this->mDirection = DSPWAVETABLE_SPEEDDIR_BACKWARDS;
        frequency = -frequency;
    } else {
        auto samplesoftware = static_cast<SampleSoftware*>(this->mSound);
        if (samplesoftware && !(this->mChannel->mMode & FMOD_LOOP_BIDI)) {
            this->mDirection = DSPWAVETABLE_SPEEDDIR_FORWARDS;
        }
    }

    this->mFrequency = frequency;
    this->mSpeed.mValue = static_cast<FMOD_SINT64>(this->mFrequency / static_cast<float>(this->mTargetFrequency) * 4294967296.0f);

    return FMOD_OK;
}

FMOD_RESULT F_CALLBACK DSPWaveTable::setPositionCallback(FMOD_DSP_STATE* dsp, uint32_t position) {
    auto wavetable = static_cast<DSPWaveTable*>(dsp);
    return wavetable->setPositionInternal(position);
}

FMOD_RESULT F_CALLBACK DSPWaveTable::setParameterCallback(FMOD_DSP_STATE* dsp, int32_t index, float value) {
    auto wavetable = static_cast<DSPWaveTable*>(dsp);
    return wavetable->setParameterInternal(index, value);
}

FMOD_RESULT F_CALLBACK DSPWaveTable::getParameterCallback(FMOD_DSP_STATE* dsp, int32_t index, float* value, char* valuestr) {
    auto wavetable = static_cast<DSPWaveTable*>(dsp);
    return wavetable->getParameterInternal(index, value, valuestr);
}

} // namespace FMOD
