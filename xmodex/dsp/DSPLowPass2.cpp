#include "xmodex/dsp/DSPLowPass2.hpp"
#include "xmodex/Math.hpp"
#include "xmodex/String.hpp"
#include "xmodex/SystemI.hpp"
#include <cstdio>

namespace FMOD {

FMOD_DSP_DESCRIPTION_EX dsplowpass2;

FMOD_DSP_PARAMETERDESC dsplowpass2_param[2] = {
    { 1.0f, 22000.0f, 5000.0f, "Cutoff freq", "hz", "Lowpass cutoff frequency in hz.  1.0 to 22000.0.  Default = 5000.0" },
    { 1.0f, 127.0f, 1.0f, "Resonance", "", "Lowpass resonance Q value. 0.0 to 127.0.  Default = 1.0" }
};

FMOD_RESULT DSPLowPass2::createInternal() {
    Plugin::init();

    for (int32_t i = 0; i < this->mDescription.numparameters; i++) {
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

FMOD_RESULT DSPLowPass2::resetInternal() {
    for (int32_t i = 0; i < DSP_MAXLEVELS_MAX; i++) {
        this->mFilter_Y[i][0] = 0.0f;
        this->mFilter_Y[i][1] = 0.0f;
    }

    return FMOD_OK;
}

FMOD_RESULT DSPLowPass2::readInternal(float* inbuffer, float* outbuffer, uint32_t length, int32_t inchannels, int32_t outchannels) {
    static float dc = static_cast<float>(1E-20);

    if (this->mResonance != this->mResonanceUpdate || this->mCutoffHz != this->mCutoffHzUpdate) {
        this->mResonance = this->mResonanceUpdate;
        this->mCutoffHz = this->mCutoffHzUpdate;
        this->updateCoefficients(this->mResonance, this->mCutoffHz);
    }

    if (this->mFilter_A0 == 0.0f && this->mFilter_B0 == 2.0f && this->mFilter_B1 == -1.0f) {
        FMOD_memset(outbuffer, 0, sizeof(float) * length * inchannels);

        for (int32_t i = 0; i < inchannels; i++) {
            this->mFilter_Y[i][0] = 0.0f;
            this->mFilter_Y[i][1] = 0.0f;
        }
        return FMOD_OK;
    }

    if (inchannels == 1) {
        float fy1 = this->mFilter_Y[0][0];
        float fy2 = this->mFilter_Y[0][1];

        while (length) {
            outbuffer[0] = (inbuffer[0] + dc) * this->mFilter_A0 + fy1 * this->mFilter_B0 + fy2 * this->mFilter_B1;
            fy2 = fy1;
            fy1 = outbuffer[0];

            dc = -dc;
            length--;
            outbuffer++;
            inbuffer++;
        }

        this->mFilter_Y[0][0] = fy1;
        this->mFilter_Y[0][1] = fy2;
    } else if (inchannels == 2) {
        float fyL1 = this->mFilter_Y[0][0];
        float fyL2 = this->mFilter_Y[0][1];
        float fyR1 = this->mFilter_Y[1][0];
        float fyR2 = this->mFilter_Y[1][1];

        while (length) {
            float fyL, fyR;

            fyL = (inbuffer[0] + dc) * this->mFilter_A0 + fyL1 * this->mFilter_B0 + fyL2 * this->mFilter_B1;
            fyR = (inbuffer[1] + dc) * this->mFilter_A0 + fyR1 * this->mFilter_B0 + fyR2 * this->mFilter_B1;
            fyL2 = fyL1;
            fyL1 = fyL;
            fyR2 = fyR1;
            fyR1 = fyR;

            outbuffer[0] = fyL;
            outbuffer[1] = fyR;

            dc = -dc;
            length--;
            outbuffer += 2;
            inbuffer += 2;
        }

        this->mFilter_Y[0][0] = fyL1;
        this->mFilter_Y[0][1] = fyL2;
        this->mFilter_Y[1][0] = fyR1;
        this->mFilter_Y[1][1] = fyR2;
    } else if (inchannels == 6) {
        float fyA1 = this->mFilter_Y[0][0];
        float fyA2 = this->mFilter_Y[0][1];
        float fyB1 = this->mFilter_Y[1][0];
        float fyB2 = this->mFilter_Y[1][1];
        float fyC1 = this->mFilter_Y[2][0];
        float fyC2 = this->mFilter_Y[2][1];
        float fyD1 = this->mFilter_Y[3][0];
        float fyD2 = this->mFilter_Y[3][1];
        float fyE1 = this->mFilter_Y[4][0];
        float fyE2 = this->mFilter_Y[4][1];
        float fyF1 = this->mFilter_Y[5][0];
        float fyF2 = this->mFilter_Y[5][1];

        while (length) {
            float fyA;
            float fyB;
            float fyC;
            float fyD;
            float fyE;
            float fyF;

            fyA = (inbuffer[0] + dc) * this->mFilter_A0 + fyA1 * this->mFilter_B0 + fyA2 * this->mFilter_B1;
            fyB = (inbuffer[1] + dc) * this->mFilter_A0 + fyB1 * this->mFilter_B0 + fyB2 * this->mFilter_B1;
            fyC = (inbuffer[2] + dc) * this->mFilter_A0 + fyC1 * this->mFilter_B0 + fyC2 * this->mFilter_B1;
            fyD = (inbuffer[3] + dc) * this->mFilter_A0 + fyD1 * this->mFilter_B0 + fyD2 * this->mFilter_B1;
            fyE = (inbuffer[4] + dc) * this->mFilter_A0 + fyE1 * this->mFilter_B0 + fyE2 * this->mFilter_B1;
            fyF = (inbuffer[5] + dc) * this->mFilter_A0 + fyF1 * this->mFilter_B0 + fyF2 * this->mFilter_B1;
            fyA2 = fyA1;
            fyA1 = fyA;
            fyB2 = fyB1;
            fyB1 = fyB;
            fyC2 = fyC1;
            fyC1 = fyC;
            fyD2 = fyD1;
            fyD1 = fyD;
            fyE2 = fyE1;
            fyE1 = fyE;
            fyF2 = fyF1;
            fyF1 = fyF;
            outbuffer[0] = fyA;
            outbuffer[1] = fyB;
            outbuffer[2] = fyC;
            outbuffer[3] = fyD;
            outbuffer[4] = fyE;
            outbuffer[5] = fyF;

            dc = -dc;
            length--;
            outbuffer += 6;
            inbuffer += 6;
        }

        this->mFilter_Y[0][0] = fyA1;
        this->mFilter_Y[0][1] = fyA2;
        this->mFilter_Y[1][0] = fyB1;
        this->mFilter_Y[1][1] = fyB2;
        this->mFilter_Y[2][0] = fyC1;
        this->mFilter_Y[2][1] = fyC2;
        this->mFilter_Y[3][0] = fyD1;
        this->mFilter_Y[3][1] = fyD2;
        this->mFilter_Y[4][0] = fyE1;
        this->mFilter_Y[4][1] = fyE2;
        this->mFilter_Y[5][0] = fyF1;
        this->mFilter_Y[5][1] = fyF2;
    } else {
        for (int32_t i = 0; i < inchannels; i++) {
            float* in = inbuffer + i;
            float* out = outbuffer + i;
            int len;

            float fy1 = this->mFilter_Y[i][0];
            float fy2 = this->mFilter_Y[i][1];
            len = length;
            while (len) {
                out[0] = (in[0] + dc) * this->mFilter_A0 + fy1 * this->mFilter_B0 + fy2 * this->mFilter_B1;
                fy2 = fy1;
                fy1 = out[0];

                dc = -dc;
                len--;
                in += inchannels;
                out += inchannels;
            }

            this->mFilter_Y[i][0] = fy1;
            this->mFilter_Y[i][1] = fy2;
        }
    }

    return FMOD_OK;
}

FMOD_RESULT DSPLowPass2::setParameterInternal(int32_t index, float value) {
    switch (index) {
    case FMOD_DSP_LOWPASS_CUTOFF: {
        this->mCutoffHzUpdate = value;
        break;
    }
    case FMOD_DSP_LOWPASS_RESONANCE: {
        this->mResonanceUpdate = value;
        break;
    }
    }

    return FMOD_OK;
}

FMOD_RESULT DSPLowPass2::getParameterInternal(int32_t index, float* value, char* valuestr) {
    switch (index) {
    case FMOD_DSP_LOWPASS_CUTOFF: {
        *value = this->mCutoffHzUpdate;
        sprintf(valuestr, "%.02f", this->mCutoffHzUpdate);
        break;
    }
    case FMOD_DSP_LOWPASS_RESONANCE: {
        *value = this->mResonanceUpdate;
        sprintf(valuestr, "%.02f", this->mResonanceUpdate);
        break;
    }
    }

    return FMOD_OK;
}

FMOD_RESULT DSPLowPass2::updateCoefficients(float resonance, float cutoff) {
    int32_t outputrate;
    float fc;
    float fs;
    float fg;
    float fb0;
    float fb1;

    this->mSystem->getSoftwareFormat(&outputrate, nullptr, nullptr, nullptr, nullptr, nullptr);

    fc = static_cast<float>(cutoff);
    fs = static_cast<float>(outputrate);

    fc *= static_cast<float>(2.0f * 3.14159265358f / fs);

    float dmpfac = FMOD_POW(10.0f, -((24.0f / 128.0f) * resonance) / 20.0f);
    float d = (1.0f - 2.0f * dmpfac) * fc;

    if (d > 2.0f) {
        d = 2.0f;
    }

    d = (2.0f * dmpfac - d) / fc;
    float e = FMOD_POW(1.0f / fc, 2.0f);

    fg = 1.0f / (1.0f + d + e);
    fb0 = (d + e + e) / (1 + d + e);
    fb1 = -e / (1 + d + e);

    if (fg < 0.00001f) {
        fg = 0;
    }

    if (fb0 > 1.999f && fb1 < -0.999f) {
        fb0 = 2.0f;
        fb1 = -1.0f;
    }

    this->mFilter_A0 = fg;
    this->mFilter_B0 = fb0;
    this->mFilter_B1 = fb1;

    return FMOD_OK;
}

FMOD_RESULT F_CALLBACK DSPLowPass2::createCallback(FMOD_DSP_STATE* dsp) {
    auto lowpass2 = static_cast<DSPLowPass2*>(dsp);
    return lowpass2->createInternal();
}

FMOD_RESULT F_CALLBACK DSPLowPass2::readCallback(FMOD_DSP_STATE* dsp, float* inbuffer, float* outbuffer, uint32_t length, int32_t inchannels, int32_t outchannels) {
    auto lowpass2 = static_cast<DSPLowPass2*>(dsp);
    return lowpass2->readInternal(inbuffer, outbuffer, length, inchannels, outchannels);
}

FMOD_RESULT F_CALLBACK DSPLowPass2::resetCallback(FMOD_DSP_STATE* dsp) {
    auto lowpass2 = static_cast<DSPLowPass2*>(dsp);
    return lowpass2->resetInternal();
}

FMOD_RESULT F_CALLBACK DSPLowPass2::setParameterCallback(FMOD_DSP_STATE* dsp, int32_t index, float value) {
    auto lowpass2 = static_cast<DSPLowPass2*>(dsp);
    return lowpass2->setParameterInternal(index, value);
}

FMOD_RESULT F_CALLBACK DSPLowPass2::getParameterCallback(FMOD_DSP_STATE* dsp, int32_t index, float* value, char* valuestr) {
    auto lowpass2 = static_cast<DSPLowPass2*>(dsp);
    return lowpass2->getParameterInternal(index, value, valuestr);
}

FMOD_DSP_DESCRIPTION_EX* DSPLowPass2::getDescriptionEx() {
    FMOD_memset(&dsplowpass2, 0, sizeof(FMOD_DSP_DESCRIPTION_EX));

    FMOD_strcpy(dsplowpass2.name, "FMOD IT Lowpass");
    dsplowpass2.version = 0x10100;

    dsplowpass2.create = DSPLowPass2::createCallback;
    dsplowpass2.reset = DSPLowPass2::resetCallback;

    dsplowpass2.read = DSPLowPass2::readCallback;

    dsplowpass2.numparameters = (sizeof(dsplowpass2_param) / sizeof(dsplowpass2_param[0]));
    dsplowpass2.paramdesc = dsplowpass2_param;
    dsplowpass2.setparameter = DSPLowPass2::setParameterCallback;
    dsplowpass2.getparameter = DSPLowPass2::getParameterCallback;

    dsplowpass2.mType = FMOD_DSP_TYPE_ITLOWPASS;
    dsplowpass2.mCategory = FMOD_DSP_CATEGORY_FILTER;
    dsplowpass2.mSize = sizeof(DSPLowPass2);

    return &dsplowpass2;
}

} // namespace FMOD
