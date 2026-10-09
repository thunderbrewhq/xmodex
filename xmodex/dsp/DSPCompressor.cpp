#include "xmodex/dsp/DSPCompressor.hpp"
#include "xmodex/Math.hpp"
#include "xmodex/String.hpp"
#include "xmodex/SystemI.hpp"
#include <cstdio>

namespace FMOD {

FMOD_DSP_DESCRIPTION_EX dspcompressor;

FMOD_DSP_PARAMETERDESC dspcompressor_param[4] = {
    { -60.0f, 0.0f, 0.0f, "Threshold", "dB", "Compressor threshold [-60dB, 0dB]  Default = 0dB." },
    { 10.0f, 200.0f, 50.0f, "Attack", "ms", "Compressor attack time. [10ms,200ms]  Default = 50ms." },
    { 20.0f, 1000.0f, 50.0f, "Release", "ms", "Compressor release time. [10ms,1000ms]  Default = 50ms." },
    { 0.0f, 30.0f, 0.0f, "Make up gain", "dB", "Compressor make up gain [0dB, 30dB]  Default = 0dB." }
};

FMOD_RESULT DSPCompressor::createInternal() {
    Plugin::init();

    for (int32_t i = 0; i < this->mDescription.numparameters; i++) {
        this->setParameter(i, this->mDescription.paramdesc[i].defaultval);
    }

    for (int32_t i = 0; i < DSP_MAXLEVELS_MAX; i++) {
        this->mMaxChannelIn[i] = 0.0f;
    }

    this->mGain = 1.0f;

    return FMOD_OK;
}

FMOD_RESULT DSPCompressor::readInternal(float* inbuffer, float* outbuffer, uint32_t length, int32_t inchannels, int32_t outchannels) {
    float in_abs;
    float x;
    float holdconstant = this->mHoldConstant;
    float threshold_lin = this->mThreshold_lin;
    float attack_constant = this->mAttack_constant;
    float release_constant = this->mRelease_constant;
    float gainmakeup_lin = this->mGainMakeup_lin;

    for (uint32_t sample = 0; sample < length; sample++) {
        uint32_t base = sample * inchannels;
        float* sample_in = &(inbuffer[base]);
        float* sample_out = &(outbuffer[base]);
        float in_max = 0.0f;
        float over;

        for (int32_t channel = 0; channel < inchannels; channel++) {
            float* m;

            x = sample_in[channel];
            m = &(this->mMaxChannelIn[channel]);

            in_abs = (x < 0.0f) ? -x : x;

            *m -= holdconstant;

            if (in_abs > *m) {
                *m = in_abs;
            }

            if (*m > in_max) {
                in_max = *m;
            }
        }

        over = in_max / threshold_lin;

        if (over > 1.0f) {
            this->mGain = over + attack_constant * (this->mGain - over);
        } else {
            this->mGain = over + release_constant * (this->mGain - over);
        }

        if (this->mGain > 1.0f) {
            for (int32_t channel = 0; channel < inchannels; channel++) {
                sample_out[channel] = sample_in[channel] / this->mGain * gainmakeup_lin;
            }
        } else {
            for (int32_t channel = 0; channel < inchannels; channel++) {
                sample_out[channel] = gainmakeup_lin * sample_in[channel];
            }
        }
    }

    return FMOD_OK;
}

FMOD_RESULT DSPCompressor::setParameterInternal(int32_t index, float value) {
    FMOD_RESULT result;
    int32_t outputrate;

    result = this->mSystem->getSoftwareFormat(&outputrate, nullptr, nullptr, nullptr, nullptr, nullptr);
    if (result != FMOD_OK) {
        return result;
    }

    this->mHoldConstant = 10.0f / static_cast<float>(outputrate);

    switch (index) {
    case FMOD_DSP_COMPRESSOR_THRESHOLD: {
        this->mThreshold_dB = value;
        this->mThreshold_lin = FMOD_POW(10.0f, this->mThreshold_dB / 20.0f);
        break;
    }
    case FMOD_DSP_COMPRESSOR_ATTACK: {
        this->mAttack_ms = value;
        this->mAttack_constant = FMOD_EXP(-1000.0f / (this->mAttack_ms * outputrate));
        break;
    }
    case FMOD_DSP_COMPRESSOR_RELEASE: {
        this->mRelease_ms = value;
        this->mRelease_constant = FMOD_EXP(-1000.0f / (this->mRelease_ms * outputrate));
        break;
    }
    case FMOD_DSP_COMPRESSOR_GAINMAKEUP: {
        this->mGainMakeup_dB = value;
        this->mGainMakeup_lin = FMOD_POW(10.0f, this->mGainMakeup_dB / 20.0f);
        break;
    }
    }

    return FMOD_OK;
}

FMOD_RESULT DSPCompressor::getParameterInternal(int32_t index, float* value, char* valuestr) {
    switch (index) {
    case FMOD_DSP_COMPRESSOR_THRESHOLD: {
        *value = this->mThreshold_dB;
        sprintf(valuestr, "%.02f", this->mThreshold_dB);
        break;
    }
    case FMOD_DSP_COMPRESSOR_ATTACK: {
        *value = this->mAttack_ms;
        sprintf(valuestr, "%.02f", this->mAttack_ms);
        break;
    }
    case FMOD_DSP_COMPRESSOR_RELEASE: {
        *value = this->mRelease_ms;
        sprintf(valuestr, "%.02f", this->mRelease_ms);
        break;
    }
    case FMOD_DSP_COMPRESSOR_GAINMAKEUP: {
        *value = this->mGainMakeup_dB;
        sprintf(valuestr, "%.02f", this->mGainMakeup_dB);
        break;
    }
    }

    return FMOD_OK;
}

FMOD_RESULT F_CALLBACK DSPCompressor::createCallback(FMOD_DSP_STATE* dsp) {
    auto compressor = static_cast<DSPCompressor*>(dsp);
    return compressor->createInternal();
}

FMOD_RESULT F_CALLBACK DSPCompressor::readCallback(FMOD_DSP_STATE* dsp, float* inbuffer, float* outbuffer, uint32_t length, int32_t inchannels, int32_t outchannels) {
    auto compressor = static_cast<DSPCompressor*>(dsp);
    return compressor->readInternal(inbuffer, outbuffer, length, inchannels, outchannels);
}

FMOD_RESULT F_CALLBACK DSPCompressor::setParameterCallback(FMOD_DSP_STATE* dsp, int32_t index, float value) {
    auto compressor = static_cast<DSPCompressor*>(dsp);
    return compressor->setParameterInternal(index, value);
}

FMOD_RESULT F_CALLBACK DSPCompressor::getParameterCallback(FMOD_DSP_STATE* dsp, int32_t index, float* value, char* valuestr) {
    auto compressor = static_cast<DSPCompressor*>(dsp);
    return compressor->getParameterInternal(index, value, valuestr);
}

FMOD_DSP_DESCRIPTION_EX* DSPCompressor::getDescriptionEx() {
    FMOD_memset(&dspcompressor, 0, sizeof(FMOD_DSP_DESCRIPTION_EX));

    FMOD_strcpy(dspcompressor.name, "FMOD Compressor");
    dspcompressor.version = 0x10100;

    dspcompressor.create = DSPCompressor::createCallback;
    dspcompressor.read = DSPCompressor::readCallback;

    dspcompressor.numparameters = (sizeof(dspcompressor_param) / sizeof(dspcompressor_param[0]));
    dspcompressor.paramdesc = dspcompressor_param;
    dspcompressor.setparameter = DSPCompressor::setParameterCallback;
    dspcompressor.getparameter = DSPCompressor::getParameterCallback;

    dspcompressor.mType = FMOD_DSP_TYPE_COMPRESSOR;
    dspcompressor.mCategory = FMOD_DSP_CATEGORY_FILTER;
    dspcompressor.mSize = sizeof(DSPCompressor);

    return &dspcompressor;
}

} // namespace FMOD
