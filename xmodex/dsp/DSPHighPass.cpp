#include "xmodex/dsp/DSPHighPass.hpp"
#include "xmodex/Math.hpp"
#include "xmodex/String.hpp"
#include "xmodex/SystemI.hpp"
#include <cstdio>

namespace FMOD {

FMOD_DSP_DESCRIPTION_EX dsphighpass;

FMOD_DSP_PARAMETERDESC dsphighpass_param[2] = {
    { 1.0f, 22000.0f, 5000.0f, "Cutoff freq", "hz", "Highpass cutoff frequency in hz.   10.0 to 22000.0.  Default = 5000.0." },
    { 1.0f, 10.0f, 1.0f, "Resonance", "", "Highpass resonance Q value. 1.0 to 10.0.  Default = 1.0." }
};

FMOD_RESULT DSPHighPass::createInternal() {
    Plugin::init();

    for (int32_t i = 0; i < mDescription.numparameters; i++) {
        FMOD_RESULT result;

        result = this->setParameter(i, this->mDescription.paramdesc[i].defaultval);
        if (result != FMOD_OK) {
            return result;
        }
    }

    this->resetInternal();

    this->mResonance = this->mResonanceUpdate;
    this->mCutoffHz = this->mCutoffHzUpdate;

    this->updateCoefficients(this->mResonance, this->mCutoffHz);

    return FMOD_OK;
}

FMOD_RESULT DSPHighPass::readInternal(float* inbuffer, float* outbuffer, uint32_t length, int32_t inchannels, int32_t outchannels) {
    static float dc = static_cast<float>(1E-20);

    if (this->mResonance != this->mResonanceUpdate ||
        this->mCutoffHz != this->mCutoffHzUpdate) {
        float cutoffdelta = this->mCutoffHz - this->mCutoffHzUpdate;

        this->mResonance = this->mResonanceUpdate;
        this->mCutoffHz = this->mCutoffHzUpdate;

        this->updateCoefficients(this->mResonance, this->mCutoffHz);

        if (FMOD_FABS(cutoffdelta) >= 1000.0f) {
            this->resetInternal();
        }
    }

    if (this->mCutoffHz == 1.0f) {
        FMOD_memcpy(outbuffer, inbuffer, sizeof(float) * length * inchannels);
        return FMOD_OK;
    }

    if (this->mCutoffHz == 22000.0f) {
        FMOD_memset(outbuffer, 0, sizeof(float) * length * inchannels);

        for (int32_t i = 0; i < inchannels; i++) {
            this->mIn1[i] = this->mIn2[i] = 0.0f;
            this->mOut1[i] = this->mOut2[i] = 0.0f;
        }

        return FMOD_OK;
    }

    if (inchannels == 1) {
        for (int32_t i = 0; i < length; i++) {
            float in0;
            float out0;

            in0 = inbuffer[i] + dc;

            out0 = (this->mCoefficient_B0 * in0 + this->mCoefficient_B1 * this->mIn1[0] + this->mCoefficient_B2 * this->mIn2[0] - this->mCoefficient_A1 * this->mOut1[0] - this->mCoefficient_A2 * this->mOut2[0]) / this->mCoefficient_A0;

            this->mIn2[0] = this->mIn1[0];
            this->mIn1[0] = in0;

            this->mOut2[0] = this->mOut1[0];
            this->mOut1[0] = out0;

            outbuffer[i] = out0;

            dc = -dc;
        }
    } else if (inchannels == 2) {
        for (int32_t i = 0; i < length; i++) {
            float in0l;
            float out0l;
            float in0r;
            float out0r;

            in0l = inbuffer[i * 2 + 0] + dc;
            in0r = inbuffer[i * 2 + 1] + dc;

            out0l = (this->mCoefficient_B0 * in0l + this->mCoefficient_B1 * this->mIn1[0] + this->mCoefficient_B2 * this->mIn2[0] - this->mCoefficient_A1 * this->mOut1[0] - this->mCoefficient_A2 * this->mOut2[0]) / this->mCoefficient_A0;
            out0r = (this->mCoefficient_B0 * in0r + this->mCoefficient_B1 * this->mIn1[1] + this->mCoefficient_B2 * this->mIn2[1] - this->mCoefficient_A1 * this->mOut1[1] - this->mCoefficient_A2 * this->mOut2[1]) / this->mCoefficient_A0;

            this->mIn2[0] = this->mIn1[0];
            this->mIn1[0] = in0l;
            this->mIn2[1] = this->mIn1[1];
            this->mIn1[1] = in0r;

            this->mOut2[0] = this->mOut1[0];
            this->mOut1[0] = out0l;
            this->mOut2[1] = this->mOut1[1];
            this->mOut1[1] = out0r;

            outbuffer[i * 2 + 0] = out0l;
            outbuffer[i * 2 + 1] = out0r;

            dc = -dc;
        }
    } else {
        for (int32_t i = 0; i < length; i++) {
            for (int32_t j = 0; j < inchannels; j++) {
                float in0;
                float out0;

                in0 = inbuffer[(i * inchannels) + j] + dc;

                out0 = (this->mCoefficient_B0 * in0 + this->mCoefficient_B1 * this->mIn1[j] + this->mCoefficient_B2 * this->mIn2[j] - this->mCoefficient_A1 * this->mOut1[j] - this->mCoefficient_A2 * this->mOut2[j]) / this->mCoefficient_A0;

                this->mIn2[j] = this->mIn1[j];
                this->mIn1[j] = in0;

                this->mOut2[j] = this->mOut1[j];
                this->mOut1[j] = out0;

                outbuffer[(i * inchannels) + j] = out0;
            }

            dc = -dc;
        }
    }

    return FMOD_OK;
}

FMOD_RESULT DSPHighPass::resetInternal() {
    for (int32_t i = 0; i < DSP_MAXLEVELS_MAX; i++) {
        this->mIn1[i] = this->mIn2[i] = 0;
        this->mOut1[i] = this->mOut2[i] = 0;
    }

    return FMOD_OK;
}

FMOD_RESULT DSPHighPass::setParameterInternal(int32_t index, float value) {
    int32_t outputrate;

    this->mSystem->getSoftwareFormat(&outputrate, nullptr, nullptr, nullptr, nullptr, nullptr);

    switch (index) {
    case FMOD_DSP_HIGHPASS_CUTOFF: {
        this->mCutoffHzUpdate = value;
        if (this->mCutoffHzUpdate >= (static_cast<float>(outputrate) / 2.0f) - 10.0f) {
            this->mCutoffHzUpdate = (static_cast<float>(outputrate) / 2.0f) - 10.0f;
        }
        break;
    }
    case FMOD_DSP_HIGHPASS_RESONANCE: {
        this->mResonanceUpdate = value;
        break;
    }
    }

    return FMOD_OK;
}

FMOD_RESULT DSPHighPass::getParameterInternal(int32_t index, float* value, char* valuestr) {
    switch (index) {
    case FMOD_DSP_HIGHPASS_CUTOFF: {
        *value = this->mCutoffHzUpdate;
        sprintf(valuestr, "%.02f", this->mCutoffHzUpdate);
        break;
    }
    case FMOD_DSP_HIGHPASS_RESONANCE: {
        *value = this->mResonanceUpdate;
        sprintf(valuestr, "%.02f", this->mResonanceUpdate);
        break;
    }
    }

    return FMOD_OK;
}

FMOD_RESULT DSPHighPass::updateCoefficients(float resonance, float cutoff) {
    int32_t outputrate;
    float w;

    this->mSystem->getSoftwareFormat(&outputrate, nullptr, nullptr, nullptr, nullptr, nullptr);

    w = FMOD_PI2 * static_cast<float>(cutoff) / outputrate;

    this->mCoefficient_B0 = (1.0f + FMOD_COS(w)) / 2.0f;
    this->mCoefficient_B1 = -(1.0f + FMOD_COS(w));
    this->mCoefficient_B2 = (1.0f + FMOD_COS(w)) / 2.0f;
    this->mCoefficient_A0 = 1.0f + FMOD_SIN(w) / (2 * resonance);
    this->mCoefficient_A1 = -2.0f * FMOD_COS(w);
    this->mCoefficient_A2 = 1.0f - FMOD_SIN(w) / (2 * resonance);

    return FMOD_OK;
}

FMOD_RESULT F_CALLBACK DSPHighPass::createCallback(FMOD_DSP_STATE* dsp) {
    auto highpass = static_cast<DSPHighPass*>(dsp);
    return highpass->createInternal();
}

FMOD_RESULT F_CALLBACK DSPHighPass::resetCallback(FMOD_DSP_STATE* dsp) {
    auto highpass = static_cast<DSPHighPass*>(dsp);
    return highpass->resetInternal();
}

FMOD_RESULT F_CALLBACK DSPHighPass::readCallback(FMOD_DSP_STATE* dsp, float* inbuffer, float* outbuffer, uint32_t length, int32_t inchannels, int32_t outchannels) {
    auto highpass = static_cast<DSPHighPass*>(dsp);
    return highpass->readInternal(inbuffer, outbuffer, length, inchannels, outchannels);
}

FMOD_RESULT F_CALLBACK DSPHighPass::setParameterCallback(FMOD_DSP_STATE* dsp, int32_t index, float value) {
    auto highpass = static_cast<DSPHighPass*>(dsp);
    return highpass->setParameterInternal(index, value);
}

FMOD_RESULT F_CALLBACK DSPHighPass::getParameterCallback(FMOD_DSP_STATE* dsp, int32_t index, float* value, char* valuestr) {
    auto highpass = static_cast<DSPHighPass*>(dsp);
    return highpass->getParameterInternal(index, value, valuestr);
}

FMOD_DSP_DESCRIPTION_EX* DSPHighPass::getDescriptionEx() {
    FMOD_memset(&dsphighpass, 0, sizeof(FMOD_DSP_DESCRIPTION_EX));

    FMOD_strcpy(dsphighpass.name, "FMOD Highpass");
    dsphighpass.version = 0x10100;

    dsphighpass.create = DSPHighPass::createCallback;
    dsphighpass.reset = DSPHighPass::resetCallback;
    dsphighpass.read = DSPHighPass::readCallback;

    dsphighpass.numparameters = sizeof(dsphighpass_param) / sizeof(dsphighpass_param[0]);
    dsphighpass.paramdesc = dsphighpass_param;
    dsphighpass.setparameter = DSPHighPass::setParameterCallback;
    dsphighpass.getparameter = DSPHighPass::getParameterCallback;

    dsphighpass.mType = FMOD_DSP_TYPE_HIGHPASS;
    dsphighpass.mCategory = FMOD_DSP_CATEGORY_FILTER;
    dsphighpass.mSize = sizeof(DSPHighPass);

    return &dsphighpass;
}

} // namespace FMOD
