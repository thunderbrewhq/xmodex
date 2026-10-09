#include "xmodex/dsp/DSPOscillator.hpp"
#include "xmodex/Math.hpp"
#include "xmodex/String.hpp"
#include "xmodex/SystemI.hpp"
#include <cstdio>

namespace FMOD {

FMOD_DSP_DESCRIPTION_EX dsposcillator;

FMOD_DSP_PARAMETERDESC dsposcillator_param[2] = {
    { 0.0f, 5.0f, 0.0f, "Oscillator type", "", "Select a waveform type" },
    { 1.0f, 22000.0f, 220.0f, "Frequency", "hz", "Playback frequency of tone, for example music note A above middle C is 440.0." }
};

FMOD_RESULT DSPOscillator::createInternal() {
    Plugin::init();

    for (int32_t i = 0; i < this->mDescription.numparameters; i++) {
        FMOD_RESULT result;

        result = this->setParameter(i, this->mDescription.paramdesc[i].defaultval);
        if (result != FMOD_OK) {
            return result;
        }
    }

    return FMOD_OK;
}

FMOD_RESULT DSPOscillator::releaseInternal() {
    return FMOD_OK;
}

FMOD_RESULT DSPOscillator::readInternal(float* inbuffer, float* outbuffer, uint32_t length, int32_t inchannels, int32_t outchannels) {
    switch (this->mType) {
    // Sine
    case 0: {
        for (int32_t i = 0; i < length; i++) {
            outbuffer[i] = FMOD_SIN(this->mPosition);

            this->mPosition += (this->mRate * (FMOD_PI * 2));
            if (this->mPosition >= (FMOD_PI * 2.0f)) {
                this->mPosition -= (FMOD_PI * 2.0f);
            }
        }

        break;
    }
    // Square
    case 1: {
        for (int32_t i = 0; i < length; i++) {
            outbuffer[i] = 1.0f * static_cast<float>(this->mDirection);
            this->mPosition += this->mRate;
            if (this->mPosition >= 1.0f) {
                this->mPosition -= 1.0f;
                this->mDirection = -this->mDirection;
            }
        }
        break;
    }
    // Saw up
    case 2: {
        for (int32_t i = 0; i < length; i++) {
            outbuffer[i] = (this->mPosition * 2.0f) - 1.0f;
            this->mPosition += this->mRate;
            if (this->mPosition >= 1.0f) {
                this->mPosition -= 1.0f;
            }
        }
        break;
    }
    // Saw down
    case 3: {
        for (int32_t i = 0; i < length; i++) {
            outbuffer[i] = (this->mPosition * -2.0f) + 1.0f;

            this->mPosition += this->mRate;
            if (this->mPosition >= 1.0f) {
                this->mPosition -= 1.0f;
            }
        }
        break;
    }
    // Triangle
    case 4: {
        for (int32_t i = 0; i < length; i++) {
            outbuffer[i] = this->mPosition;

            this->mPosition += (this->mRate * static_cast<float>(this->mDirection) * 2.0f);
            if (this->mPosition > 1.0f || this->mPosition < -1.0f) {
                this->mDirection = -this->mDirection;
                this->mPosition += (this->mRate * static_cast<float>(this->mDirection) * 2.0f);
            }
        }
        break;
    }
    // Noise
    case 5: {
        for (int32_t i = 0; i < length; i++) {
            outbuffer[i] = FMOD_Random_Value(1.0f);
        }
        break;
    }
    }

    return FMOD_OK;
}

FMOD_RESULT DSPOscillator::setParameterInternal(int32_t index, float value) {
    FMOD_RESULT result;
    int32_t outputrate;

    switch (index) {
    case FMOD_DSP_OSCILLATOR_TYPE: {
        this->mType = static_cast<int32_t>(value);
        break;
    }
    case FMOD_DSP_OSCILLATOR_RATE: {
        this->mRateHz = value;
        break;
    }
    }

    this->mDirection = 1;
    result = this->mSystem->getSoftwareFormat(&outputrate, nullptr, nullptr, nullptr, nullptr, nullptr);
    if (result != FMOD_OK) {
        return result;
    }

    this->mRate = this->mRateHz / static_cast<float>(outputrate);

    return FMOD_OK;
}

FMOD_RESULT DSPOscillator::getParameterInternal(int32_t index, float* value, char* valuestr) {
    switch (index) {
    case FMOD_DSP_OSCILLATOR_TYPE: {
        *value = static_cast<float>(this->mType);
        switch (this->mType) {
        case 0: {
            FMOD_strcpy(valuestr, "sine");
            break;
        }
        case 1: {
            FMOD_strcpy(valuestr, "square");
            break;
        }
        case 2: {
            FMOD_strcpy(valuestr, "saw up");
            break;
        }
        case 3: {
            FMOD_strcpy(valuestr, "saw down");
            break;
        }
        case 4: {
            FMOD_strcpy(valuestr, "triangle");
            break;
        }
        case 5: {
            FMOD_strcpy(valuestr, "noise");
            break;
        }
        }
        break;
    }
    case FMOD_DSP_OSCILLATOR_RATE: {
        *value = this->mRateHz;
        sprintf(valuestr, "%.02f", this->mRateHz);
        break;
    }
    }

    return FMOD_OK;
}

FMOD_RESULT F_CALLBACK DSPOscillator::createCallback(FMOD_DSP_STATE* dsp) {
    auto osc = static_cast<DSPOscillator*>(dsp);
    return osc->createInternal();
}

FMOD_RESULT F_CALLBACK DSPOscillator::releaseCallback(FMOD_DSP_STATE* dsp) {
    auto osc = static_cast<DSPOscillator*>(dsp);
    return osc->releaseInternal();
}

FMOD_RESULT F_CALLBACK DSPOscillator::readCallback(FMOD_DSP_STATE* dsp, float* inbuffer, float* outbuffer, uint32_t length, int32_t inchannels, int32_t outchannels) {
    auto osc = static_cast<DSPOscillator*>(dsp);
    return osc->readInternal(inbuffer, outbuffer, length, inchannels, outchannels);
}

FMOD_RESULT F_CALLBACK DSPOscillator::setParameterCallback(FMOD_DSP_STATE* dsp, int32_t index, float value) {
    auto osc = static_cast<DSPOscillator*>(dsp);
    return osc->setParameterInternal(index, value);
}

FMOD_RESULT F_CALLBACK DSPOscillator::getParameterCallback(FMOD_DSP_STATE* dsp, int32_t index, float* value, char* valuestr) {
    auto osc = static_cast<DSPOscillator*>(dsp);
    return osc->getParameterInternal(index, value, valuestr);
}

FMOD_DSP_DESCRIPTION_EX* DSPOscillator::getDescriptionEx() {
    FMOD_memset(&dsposcillator, 0, sizeof(FMOD_DSP_DESCRIPTION_EX));

    FMOD_strcpy(dsposcillator.name, "FMOD Oscillator");
    dsposcillator.version = 0x10100;
    dsposcillator.channels = 1;

    dsposcillator.create = DSPOscillator::createCallback;
    dsposcillator.release = DSPOscillator::releaseCallback;

    dsposcillator.read = DSPOscillator::readCallback;

    dsposcillator.numparameters = (sizeof(dsposcillator_param) / sizeof(dsposcillator_param[0]));
    dsposcillator.paramdesc = dsposcillator_param;
    dsposcillator.setparameter = DSPOscillator::setParameterCallback;
    dsposcillator.getparameter = DSPOscillator::getParameterCallback;

    dsposcillator.mType = FMOD_DSP_TYPE_OSCILLATOR;
    dsposcillator.mSize = sizeof(DSPOscillator);
    dsposcillator.mCategory = FMOD_DSP_CATEGORY_FILTER;

    return &dsposcillator;
}

} // namespace FMOD
