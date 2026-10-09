#include "xmodex/dsp/DSPNormalize.hpp"
#include "xmodex/Math.hpp"
#include "xmodex/String.hpp"
#include "xmodex/SystemI.hpp"
#include <cstdio>

namespace FMOD {

FMOD_DSP_DESCRIPTION_EX dspnormalize;

FMOD_DSP_PARAMETERDESC dspnormalize_param[3] = {
    { 0.0f, 20000.0f, 5000.0f, "Fade in time", "seconds", "Time to ramp the silence to full in ms.  0.0 to 20000.0. Default = 5000.0." },
    { 0.0f, 1.0f, 0.1f, "Lowest volume", "", "Lower volume range threshold to ignore.  0.0 to 1.0.  Default = 0.1.  Raise higher to stop amplification of very quiet signals." },
    { 0.0f, 100000.0f, 20.0f, "Maximum amp", "x", "Maximum amplification allowed.  1.0 to 100000.0.  Default = 20.0.  1.0 = no amplifaction, higher values allow more boost." },
};

FMOD_RESULT DSPNormalize::createInternal() {
    FMOD_RESULT result;

    Plugin::init();

    this->mTargetMaximum = 1.0f;
    this->mMaximum = 1.0f;

    result = this->mSystem->getSoftwareFormat(&this->mOutputRate, nullptr, nullptr, nullptr, nullptr, nullptr);
    if (result != FMOD_OK) {
        return result;
    }

    for (int32_t i = 0; i < this->mDescription.numparameters; i++) {
        result = this->setParameter(i, this->mDescription.paramdesc[i].defaultval);
        if (result != FMOD_OK) {
            return result;
        }
    }

    return FMOD_OK;
}

FMOD_RESULT DSPNormalize::releaseInternal() {
    return FMOD_OK;
}

FMOD_RESULT DSPNormalize::resetInternal() {
    this->mTargetMaximum = 1.0f;
    this->mMaximum = 1.0f;
    return FMOD_OK;
}

FMOD_RESULT DSPNormalize::readInternal(float* inbuffer, float* outbuffer, uint32_t length, int32_t inchannels, int32_t outchannels) {
    auto attackspeed = this->mAttackSpeed;
    auto threshold = this->mThreshold;
    auto maxamp = this->mMaxAmp;

    for (uint32_t i = 0; i < length; i++) {
        float scale;

        this->mMaximum -= attackspeed;
        if (this->mMaximum < threshold) {
            this->mMaximum = threshold;
        }

        for (uint32_t j = 0; j < static_cast<uint32_t>(inchannels); j++) {
            if (FMOD_FABS(inbuffer[(i * inchannels) + j]) > this->mMaximum) {
                this->mMaximum = FMOD_FABS(inbuffer[(i * inchannels) + j]);
            }
        }

        scale = 1.0f / this->mMaximum;

        if (scale > maxamp) {
            scale = maxamp;
        }

        for (int32_t j = 0; j < static_cast<uint32_t>(inchannels); j++) {
            outbuffer[(i * inchannels) + j] = inbuffer[(i * inchannels) + j] * scale;
        }
    }

    return FMOD_OK;
}

FMOD_RESULT DSPNormalize::setParameterInternal(int32_t index, float value) {
    switch (index) {
    case FMOD_DSP_NORMALIZE_FADETIME: {
        this->mFadeTime = value;
        break;
    }
    case FMOD_DSP_NORMALIZE_THRESHHOLD: {
        this->mThreshold = value;
        break;
    }
    case FMOD_DSP_NORMALIZE_MAXAMP: {
        this->mMaxAmp = value;
        break;
    }
    }

    if (this->mFadeTime) {
        this->mAttackSpeed = 1.0f / (this->mFadeTime * static_cast<float>(this->mOutputRate) / 1000.0f);
    } else {
        this->mAttackSpeed = 1.0f;
    }

    return FMOD_OK;
}

FMOD_RESULT DSPNormalize::getParameterInternal(int32_t index, float* value, char* valuestr) {
    switch (index) {
    case FMOD_DSP_NORMALIZE_FADETIME: {
        *value = this->mFadeTime;
        sprintf(valuestr, "%.02f", this->mFadeTime);
        break;
    }
    case FMOD_DSP_NORMALIZE_THRESHHOLD: {
        *value = this->mThreshold;
        sprintf(valuestr, "%.02f", this->mThreshold);
        break;
    }
    case FMOD_DSP_NORMALIZE_MAXAMP: {
        *value = this->mMaxAmp;
        sprintf(valuestr, "%.02f", this->mMaxAmp);
        break;
    }
    }

    return FMOD_OK;
}

FMOD_RESULT F_CALLBACK DSPNormalize::createCallback(FMOD_DSP_STATE* dsp) {
    auto normalize = static_cast<DSPNormalize*>(dsp);
    return normalize->createInternal();
}

FMOD_RESULT F_CALLBACK DSPNormalize::releaseCallback(FMOD_DSP_STATE* dsp) {
    auto normalize = static_cast<DSPNormalize*>(dsp);
    return normalize->releaseInternal();
}

FMOD_RESULT F_CALLBACK DSPNormalize::resetCallback(FMOD_DSP_STATE* dsp) {
    auto normalize = static_cast<DSPNormalize*>(dsp);
    return normalize->resetInternal();
}

FMOD_RESULT F_CALLBACK DSPNormalize::readCallback(FMOD_DSP_STATE* dsp, float* inbuffer, float* outbuffer, uint32_t length, int32_t inchannels, int32_t outchannels) {
    auto normalize = static_cast<DSPNormalize*>(dsp);
    return normalize->readInternal(inbuffer, outbuffer, length, inchannels, outchannels);
}

FMOD_RESULT F_CALLBACK DSPNormalize::setParameterCallback(FMOD_DSP_STATE* dsp, int32_t index, float value) {
    auto normalize = static_cast<DSPNormalize*>(dsp);
    return normalize->setParameterInternal(index, value);
}

FMOD_RESULT F_CALLBACK DSPNormalize::getParameterCallback(FMOD_DSP_STATE* dsp, int32_t index, float* value, char* valuestr) {
    auto normalize = static_cast<DSPNormalize*>(dsp);
    return normalize->getParameterInternal(index, value, valuestr);
}

FMOD_DSP_DESCRIPTION_EX* DSPNormalize::getDescriptionEx() {
    FMOD_memset(&dspnormalize, 0, sizeof(FMOD_DSP_DESCRIPTION_EX));

    FMOD_strcpy(dspnormalize.name, "FMOD Normalize");
    dspnormalize.version = 0x10100;

    dspnormalize.create = DSPNormalize::createCallback;
    dspnormalize.release = DSPNormalize::releaseCallback;
    dspnormalize.reset = DSPNormalize::resetCallback;

    dspnormalize.read = DSPNormalize::readCallback;

    dspnormalize.numparameters = sizeof(dspnormalize_param) / sizeof(dspnormalize_param[0]);
    dspnormalize.paramdesc = dspnormalize_param;
    dspnormalize.setparameter = DSPNormalize::setParameterCallback;
    dspnormalize.getparameter = DSPNormalize::getParameterCallback;

    dspnormalize.mType = FMOD_DSP_TYPE_NORMALIZE;
    dspnormalize.mCategory = FMOD_DSP_CATEGORY_FILTER;
    dspnormalize.mSize = sizeof(DSPNormalize);

    return &dspnormalize;
}

} // namespace FMOD
