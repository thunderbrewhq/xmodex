#include "xmodex/dsp/DSPEcho.hpp"
#include "xmodex/Memory.hpp"
#include "xmodex/String.hpp"
#include "xmodex/SystemI.hpp"
#include <cstdio>

namespace FMOD {

FMOD_DSP_DESCRIPTION_EX dspecho_desc;

FMOD_DSP_PARAMETERDESC dspecho_param[5] = {
    { 1.0f, 5000.0f, 500.0f, "Delay", "ms", "Echo delay in ms.  10  to 5000.  Default = 500." },
    { 0.0f, 1.0f, 0.5f, "Decay", "%", "Echo decay per delay.  0 to 1.  1.0 = No decay, 0.0 = total decay.  Default = 0.5." },
    { 0.0f, 16.0f, 0.0f, "Max channels", "channels", "Maximum channels supported.  0 to 16.  0 = same as fmod's default output polyphony, 1 = mono, 2 = stereo etc.  Default = 0.  It is suggested to leave at 0!" },
    { 0.0f, 1.0f, 1.0f, "Drymix", "%", "Volume of original signal to pass to output.  0.0 to 1.0. Default = 1.0." },
    { 0.0f, 1.0f, 1.0f, "Wetmix", "%", "Volume of echo delay signal to pass to output.  0.0 to 1.0. Default = 1.0." },
};

FMOD_RESULT DSPEcho::createInternal() {
    Plugin::init();

    this->mChannels = 0;

    for (int32_t i = 0; i < this->mDescription.numparameters; i++) {
        FMOD_RESULT result;

        result = this->setParameter(i, this->mDescription.paramdesc[i].defaultval);
        if (result != FMOD_OK) {
            return result;
        }
    }

    return FMOD_OK;
}

FMOD_RESULT DSPEcho::releaseInternal() {
    if (this->mEchoBuffer) {
        FMOD_Memory_Free(this->mEchoBuffer);
        this->mEchoBuffer = nullptr;
    }

    return FMOD_OK;
}

FMOD_RESULT DSPEcho::resetInternal() {
    this->mEchoPosition = 0;

    if (this->mEchoBuffer) {
        FMOD_memset(this->mEchoBuffer, 0, this->mEchoBufferLengthBytes);
    }

    return FMOD_OK;
}

FMOD_RESULT DSPEcho::readInternal(float* inbuffer, float* outbuffer, uint32_t length, int32_t inchannels, int32_t outchannels) {
    if (inchannels > this->mChannels || !this->mEchoBuffer) {
        FMOD_memcpy(outbuffer, inbuffer, length * outchannels * sizeof(float));
        return FMOD_OK;
    }

    if (inchannels == 1) {
        uint32_t len;
        float* out;
        float* in;

        in = inbuffer;
        out = outbuffer;

        len = length;
        while (len) {
            uint32_t len2 = len;
            float* echobuff;

            echobuff = this->mEchoBuffer + (this->mEchoPosition * 1);

            if (this->mEchoPosition + len > this->mEchoLength) {
                len2 = this->mEchoLength - this->mEchoPosition;
            }

            for (uint32_t i = 0; i < len2; i++) {
                float in0 = in[0];

                out[0] = (in0 * this->mDryMix) + (echobuff[0] * this->mWetMix);
                echobuff[0] = (in0) + (echobuff[0] * this->mDecayRatio);

                in++;
                out++;
                echobuff++;
            }

            this->mEchoPosition += len2;
            if (this->mEchoPosition >= this->mEchoLength) {
                this->mEchoPosition = 0;
            }
            len -= len2;
        }
    } else if (inchannels == 2) {
        uint32_t len;
        float* out;
        float* in;

        in = inbuffer;
        out = outbuffer;

        len = length;
        while (len) {
            uint32_t len2 = len;
            float* echobuff;

            echobuff = this->mEchoBuffer + (this->mEchoPosition * 2);

            if (this->mEchoPosition + len > this->mEchoLength) {
                len2 = this->mEchoLength - this->mEchoPosition;
            }

            for (uint32_t i = 0; i < len2; i++) {
                float in0 = in[0];
                float in1 = in[1];

                out[0] = (in0 * this->mDryMix) + (echobuff[0] * this->mWetMix);
                out[1] = (in1 * this->mDryMix) + (echobuff[1] * this->mWetMix);
                echobuff[0] = in0 + (echobuff[0] * this->mDecayRatio);
                echobuff[1] = in1 + (echobuff[1] * this->mDecayRatio);

                in += 2;
                out += 2;
                echobuff += 2;
            }

            this->mEchoPosition += len2;
            if (this->mEchoPosition >= this->mEchoLength) {
                this->mEchoPosition = 0;
            }
            len -= len2;
        }
    } else {
        uint32_t len;
        float* out;
        float* in;

        in = inbuffer;
        out = outbuffer;

        len = length;
        while (len) {
            uint32_t len2 = len;
            float* echobuff;

            echobuff = this->mEchoBuffer + (this->mEchoPosition * inchannels);

            if (this->mEchoPosition + len > this->mEchoLength) {
                len2 = this->mEchoLength - this->mEchoPosition;
            }

            for (uint32_t i = 0; i < len2; i++) {
                for (int32_t j = 0; j < inchannels; j++) {
                    float in0 = in[j];

                    out[j] = (in0 * this->mDryMix) + (echobuff[j] * this->mWetMix);
                    echobuff[j] = (in0) + (echobuff[j] * this->mDecayRatio);
                }

                in += inchannels;
                out += inchannels;
                echobuff += inchannels;
            }

            this->mEchoPosition += len2;
            if (this->mEchoPosition >= this->mEchoLength) {
                this->mEchoPosition = 0;
            }
            len -= len2;
        }
    }

    return FMOD_OK;
}

FMOD_RESULT DSPEcho::setParameterInternal(int32_t index, float value) {
    float olddelay = this->mDelay;
    int32_t oldchannels = this->mChannels;

    this->mSystem->getSoftwareFormat(&this->mOutputRate, nullptr, &this->mChannels, nullptr, nullptr, nullptr);

    this->mSystem->lockDSP();

    switch (index) {
    case FMOD_DSP_ECHO_DELAY: {
        this->mDelay = value;
        break;
    }
    case FMOD_DSP_ECHO_DECAYRATIO: {
        this->mDecayRatio = value;
        break;
    }
    case FMOD_DSP_ECHO_MAXCHANNELS: {
        this->mMaxChannels = static_cast<int32_t>(value);
        if (this->mMaxChannels) {
            this->mChannels = this->mMaxChannels;
        }
        break;
    }
    case FMOD_DSP_ECHO_DRYMIX: {
        this->mDryMix = value;
        break;
    }
    case FMOD_DSP_ECHO_WETMIX: {
        this->mWetMix = value;
        break;
    }
    }

    if (this->mDelay != olddelay || this->mChannels != oldchannels || !this->mEchoBuffer) {
        this->mEchoLength = static_cast<int32_t>(static_cast<float>(this->mOutputRate) * this->mDelay) / 1000;

        if (this->mEchoBuffer) {
            FMOD_Memory_Free(this->mEchoBuffer);
        }

        this->mEchoBufferLengthBytes = this->mEchoLength;
        this->mEchoBufferLengthBytes *= this->mChannels;
        this->mEchoBufferLengthBytes *= sizeof(float);

        this->mEchoBuffer = static_cast<float*>(FMOD_Memory_Calloc(this->mEchoBufferLengthBytes));
        if (!this->mEchoBuffer) {
            this->mSystem->unlockDSP();
            return FMOD_ERR_MEMORY;
        }

        this->resetInternal();
    }

    this->mSystem->unlockDSP();

    return FMOD_OK;
}

FMOD_RESULT DSPEcho::getParameterInternal(int32_t index, float* value, char* valuestr) {
    switch (index) {
    case FMOD_DSP_ECHO_DELAY: {
        *value = this->mDelay;
        sprintf(valuestr, "%.02f", this->mDelay);
        break;
    }
    case FMOD_DSP_ECHO_DECAYRATIO: {
        *value = this->mDecayRatio;
        sprintf(valuestr, "%.1f", this->mDecayRatio * 100.0f);
        break;
    }
    case FMOD_DSP_ECHO_MAXCHANNELS: {
        *value = static_cast<float>(this->mMaxChannels);
        sprintf(valuestr, "%d", this->mMaxChannels);
        break;
    }
    case FMOD_DSP_ECHO_DRYMIX: {
        *value = this->mDryMix;
        sprintf(valuestr, "%.1f", this->mDryMix * 100.0f);
        break;
    }
    case FMOD_DSP_ECHO_WETMIX: {
        *value = this->mWetMix;
        sprintf(valuestr, "%.1f", this->mWetMix * 100.0f);
        break;
    }
    }

    return FMOD_OK;
}

FMOD_RESULT F_CALLBACK DSPEcho::createCallback(FMOD_DSP_STATE* dsp) {
    auto echo = static_cast<DSPEcho*>(dsp);
    return echo->createInternal();
}

FMOD_RESULT F_CALLBACK DSPEcho::releaseCallback(FMOD_DSP_STATE* dsp) {
    auto echo = static_cast<DSPEcho*>(dsp);
    return echo->releaseInternal();
}

FMOD_RESULT F_CALLBACK DSPEcho::resetCallback(FMOD_DSP_STATE* dsp) {
    auto echo = static_cast<DSPEcho*>(dsp);
    return echo->resetInternal();
}

FMOD_RESULT F_CALLBACK DSPEcho::readCallback(FMOD_DSP_STATE* dsp, float* inbuffer, float* outbuffer, uint32_t length, int32_t inchannels, int32_t outchannels) {
    auto echo = static_cast<DSPEcho*>(dsp);
    return echo->readInternal(inbuffer, outbuffer, length, inchannels, outchannels);
}

FMOD_RESULT F_CALLBACK DSPEcho::setParameterCallback(FMOD_DSP_STATE* dsp, int32_t index, float value) {
    auto echo = static_cast<DSPEcho*>(dsp);
    return echo->setParameterInternal(index, value);
}

FMOD_RESULT F_CALLBACK DSPEcho::getParameterCallback(FMOD_DSP_STATE* dsp, int32_t index, float* value, char* valuestr) {
    auto echo = static_cast<DSPEcho*>(dsp);
    return echo->getParameterInternal(index, value, valuestr);
}

FMOD_DSP_DESCRIPTION_EX* DSPEcho::getDescriptionEx() {
    FMOD_memset(&dspecho_desc, 0, sizeof(FMOD_DSP_DESCRIPTION_EX));

    FMOD_strcpy(dspecho_desc.name, "FMOD Echo");
    dspecho_desc.version = 0x10100;

    dspecho_desc.create = DSPEcho::createCallback;
    dspecho_desc.release = DSPEcho::releaseCallback;
    dspecho_desc.reset = DSPEcho::resetCallback;
    dspecho_desc.read = DSPEcho::readCallback;

    dspecho_desc.numparameters = sizeof(dspecho_param) / sizeof(dspecho_param[0]);
    dspecho_desc.paramdesc = dspecho_param;
    dspecho_desc.setparameter = DSPEcho::setParameterCallback;
    dspecho_desc.getparameter = DSPEcho::getParameterCallback;

    dspecho_desc.mType = FMOD_DSP_TYPE_ECHO;
    dspecho_desc.mCategory = FMOD_DSP_CATEGORY_FILTER;
    dspecho_desc.mSize = sizeof(DSPEcho);

    return &dspecho_desc;
}

} // namespace FMOD
