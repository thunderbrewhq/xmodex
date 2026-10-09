#include "xmodex/dsp/DSPFlange.hpp"
#include "xmodex/Math.hpp"
#include "xmodex/Memory.hpp"
#include "xmodex/String.hpp"
#include "xmodex/SystemI.hpp"
#include <cstdio>

namespace FMOD {

FMOD_DSP_DESCRIPTION_EX dspflange;

FMOD_DSP_PARAMETERDESC dspflange_param[4] = {
    { 0.0f, 1.0f, 0.45f, "Drymix", "%", "Volume of original signal to pass to output.  0.0 to 1.0. Default = 0.45." },
    { 0.0f, 1.0f, 0.55f, "Wetmix", "%", "Volume of flange signal to pass to output.  0.0 to 1.0. Default = 0.55." },
    { 0.01f, 1.0f, 1.00f, "Depth", "", "Flange depth.  0.01 to 1.0.  Default = 1.0." },
    { 0.0f, 20.0f, 0.10f, "Rate", "hz", "Flange speed in hz.  0.0 to 20.0.  Default = 0.1." }
};

FMOD_RESULT DSPFlange::createInternal() {
    FMOD_RESULT result;
    int32_t channels;

    Plugin::init();

    for (int32_t i = 0; i < DSP_FLANGE_COSTABSIZE; i++) {
        this->mCosTab[i] = FMOD_COS(FMOD_PI_2 * static_cast<float>(i) / static_cast<float>(DSP_FLANGE_COSTABSIZE));
    }

    result = this->mSystem->getSoftwareFormat(&this->mOutputRate, nullptr, &channels, nullptr, nullptr, nullptr);
    if (result != FMOD_OK) {
        return result;
    }

    this->mFlangeBufferLengthBytes = static_cast<int32_t>(static_cast<float>(this->mOutputRate) * DSP_FLANGE_MAXBUFFERLENGTHMS) / 1000;
    this->mFlangeBufferLengthBytes *= channels;

    this->mFlangeBufferLengthBytes *= sizeof(float);
    this->mFlangeBufferLengthBytes += 1024;

    this->mFlangeBuffer = static_cast<float*>(FMOD_Memory_Calloc(this->mFlangeBufferLengthBytes));

    if (!this->mFlangeBuffer) {
        return FMOD_ERR_MEMORY;
    }

    this->mFlangeTick = 0.0f;

    for (int32_t i = 0; i < this->mDescription.numparameters; i++) {
        result = this->setParameter(i, this->mDescription.paramdesc[i].defaultval);
        if (result != FMOD_OK) {
            return result;
        }
    }

    return FMOD_OK;
}

FMOD_RESULT DSPFlange::releaseInternal() {
    if (this->mFlangeBuffer) {
        FMOD_Memory_Free(this->mFlangeBuffer);
        this->mFlangeBuffer = nullptr;
    }

    return FMOD_OK;
}

FMOD_RESULT DSPFlange::resetInternal() {
    this->mFlangeBufferPosition = 0;

    if (this->mFlangeBuffer) {
        FMOD_memset(this->mFlangeBuffer, 0, this->mFlangeBufferLengthBytes);
    }

    return FMOD_OK;
}

FMOD_RESULT DSPFlange::readInternal(float* inbuffer, float* outbuffer, uint32_t length, int32_t inchannels, int32_t outchannels) {
    float halfdepth = this->mDepth * 0.5f;

    for (int32_t i = 0; i < length; i++) {
        uint32_t p1;
        uint32_t p2;
        float frac;

        p1 = (this->mFlangeBufferPosition + static_cast<uint32_t>(this->mFlangePosition)) % this->mFlangeBufferLength;
        p2 = p1 + 1;

        frac = this->mFlangePosition - static_cast<float>(static_cast<int32_t>(this->mFlangePosition));

        for (int32_t j = 0; j < inchannels; j++) {
            int32_t offset = (i * inchannels) + j;
            float val;

            val = inbuffer[offset] * this->mDryMix;

            val += ((this->mFlangeBuffer[(p1 * inchannels) + j] * (1.0f - frac)) + (this->mFlangeBuffer[(p2 * inchannels) + j] * frac)) * this->mWetMix;

            this->mFlangeBuffer[(this->mFlangeBufferPosition * inchannels) + j] = inbuffer[offset];
            outbuffer[offset] = val;
        }

        if (!this->mFlangeBufferPosition) {
            for (int32_t j = 0; j < inchannels; j++) {
                this->mFlangeBuffer[(this->mFlangeBufferLength * inchannels) + j] = this->mFlangeBuffer[j];
            }
        }

        this->mFlangeBufferPosition++;
        if (this->mFlangeBufferPosition >= this->mFlangeBufferLength) {
            this->mFlangeBufferPosition = 0;
        }

        this->mFlangePosition = (1.0f + this->sine(this->mFlangeTick + 0.00f)) * halfdepth;

        this->mFlangePosition = this->mFlangePosition * (this->mFlangeBufferLength - 1);
        this->mFlangeTick += this->mFlangeSpeed;
    }

    return FMOD_OK;
}

FMOD_RESULT DSPFlange::setParameterInternal(int32_t index, float value) {
    float olddepth;

    olddepth = this->mDepth;

    this->mSystem->lockDSP();

    switch (index) {
    case FMOD_DSP_FLANGE_DRYMIX: {
        this->mDryMix = value;
        break;
    }
    case FMOD_DSP_FLANGE_WETMIX: {
        this->mWetMix = value;
        break;
    }
    case FMOD_DSP_FLANGE_DEPTH: {
        this->mDepth = value;
        break;
    }
    case FMOD_DSP_FLANGE_RATE: {
        this->mRateHz = value;
        break;
    }
    }

    if (this->mDepth != olddepth) {
        float delay = this->mDepth * 10.0f;

        this->mFlangeBufferLength = static_cast<uint32_t>(static_cast<float>(this->mOutputRate) * delay / 1000.0f);
        if (this->mFlangeBufferLength < 4) {
            this->mFlangeBufferLength = 4;
        }

        this->resetInternal();
    }

    this->mFlangeSpeed = this->mRateHz / static_cast<float>(this->mOutputRate);

    this->mSystem->unlockDSP();

    return FMOD_OK;
}

FMOD_RESULT DSPFlange::getParameterInternal(int32_t index, float* value, char* valuestr) {
    switch (index) {
    case FMOD_DSP_FLANGE_DRYMIX: {
        *value = this->mDryMix;
        sprintf(valuestr, "%.1f", this->mDryMix * 100.0f);
        break;
    }
    case FMOD_DSP_FLANGE_WETMIX: {
        *value = this->mWetMix;
        sprintf(valuestr, "%.1f", this->mWetMix * 100.0f);
        break;
    }
    case FMOD_DSP_FLANGE_DEPTH: {
        *value = this->mDepth;
        sprintf(valuestr, "%.02f", this->mDepth);
        break;
    }
    case FMOD_DSP_FLANGE_RATE: {
        *value = this->mRateHz;
        sprintf(valuestr, "%.02f", this->mRateHz);
        break;
    }
    }

    return FMOD_OK;
}

FMOD_RESULT F_CALLBACK DSPFlange::createCallback(FMOD_DSP_STATE* dsp) {
    auto flange = static_cast<DSPFlange*>(dsp);
    return flange->createInternal();
}

FMOD_RESULT F_CALLBACK DSPFlange::releaseCallback(FMOD_DSP_STATE* dsp) {
    auto flange = static_cast<DSPFlange*>(dsp);
    return flange->releaseInternal();
}

FMOD_RESULT F_CALLBACK DSPFlange::resetCallback(FMOD_DSP_STATE* dsp) {
    auto flange = static_cast<DSPFlange*>(dsp);
    return flange->resetInternal();
}

FMOD_RESULT F_CALLBACK DSPFlange::readCallback(FMOD_DSP_STATE* dsp, float* inbuffer, float* outbuffer, uint32_t length, int32_t inchannels, int32_t outchannels) {
    auto flange = static_cast<DSPFlange*>(dsp);
    return flange->readInternal(inbuffer, outbuffer, length, inchannels, outchannels);
}

FMOD_RESULT F_CALLBACK DSPFlange::setParameterCallback(FMOD_DSP_STATE* dsp, int32_t index, float value) {
    auto flange = static_cast<DSPFlange*>(dsp);
    return flange->setParameterInternal(index, value);
}

FMOD_RESULT F_CALLBACK DSPFlange::getParameterCallback(FMOD_DSP_STATE* dsp, int32_t index, float* value, char* valuestr) {
    auto flange = static_cast<DSPFlange*>(dsp);
    return flange->getParameterInternal(index, value, valuestr);
}

FMOD_DSP_DESCRIPTION_EX* DSPFlange::getDescriptionEx() {
    FMOD_memset(&dspflange, 0, sizeof(FMOD_DSP_DESCRIPTION_EX));

    FMOD_strcpy(dspflange.name, "FMOD Flange");
    dspflange.version = 0x10100;

    dspflange.create = DSPFlange::createCallback;
    dspflange.release = DSPFlange::releaseCallback;
    dspflange.reset = DSPFlange::resetCallback;
    dspflange.read = DSPFlange::readCallback;

    dspflange.numparameters = (sizeof(dspflange_param) / sizeof(dspflange_param[0]));
    dspflange.paramdesc = dspflange_param;
    dspflange.setparameter = DSPFlange::setParameterCallback;
    dspflange.getparameter = DSPFlange::getParameterCallback;

    dspflange.mType = FMOD_DSP_TYPE_FLANGE;
    dspflange.mCategory = FMOD_DSP_CATEGORY_FILTER;
    dspflange.mSize = sizeof(DSPFlange);

    return &dspflange;
}

} // namespace FMOD
