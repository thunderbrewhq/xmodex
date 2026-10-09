#include "xmodex/dsp/DSPLowPassSimple.hpp"
#include "xmodex/Math.hpp"
#include "xmodex/String.hpp"
#include "xmodex/SystemI.hpp"
#include "xmodex/dsp/DSPConnection.hpp"
#include <cstdio>

#define CUTOFF_MIN 10.0f
#define CUTOFF_MAX 22000.0f

namespace FMOD {

FMOD_DSP_DESCRIPTION_EX dsplowpass_simple;

FMOD_DSP_PARAMETERDESC dsplowpass_simple_param[1] = {
    { CUTOFF_MIN, CUTOFF_MAX, 5000.0f, "Cutoff freq", "hz", "Lowpass cutoff frequency in hz.  1.0 to 22000.0.  Default = 5000.0" },
};

FMOD_RESULT DSPLowPassSimple::createInternal() {
    Plugin::init();

    for (int32_t i = 0; i < this->mDescription.numparameters; i++) {
        FMOD_RESULT result;

        result = this->setParameter(i, this->mDescription.paramdesc[i].defaultval);
        if (result != FMOD_OK) {
            return result;
        }
    }

    this->resetInternal();

    this->mCutoffHz = this->mCutoffHzUpdate;
    this->updateCoefficients(this->mCutoffHz);

    return FMOD_OK;
}

FMOD_RESULT DSPLowPassSimple::resetInternal() {
    for (int32_t i = 0; i < DSP_MAXLEVELS_MAX; i++) {
        this->mFilter_Y[i][0] = 0.0f;
        this->mFilter_Y[i][1] = 0.0f;
    }

    return FMOD_OK;
}

FMOD_RESULT DSPLowPassSimple::readInternal(float* inbuffer, float* outbuffer, uint32_t length, int32_t inchannels, int32_t outchannels) {
    static float dc = static_cast<float>(1E-20);

    if (this->mCutoffHz != this->mCutoffHzUpdate) {
        this->mCutoffHz = this->mCutoffHzUpdate;
        this->updateCoefficients(this->mCutoffHz);
    }

    if (this->mFilter_tc == 1.0f) {
        FMOD_memcpy(outbuffer, inbuffer, sizeof(float) * length * inchannels);
        return FMOD_OK;
    } else if (this->mFilter_tc == 0.0f) {
        FMOD_memset(outbuffer, 0, sizeof(float) * length * inchannels);

        for (int32_t i = 0; i < inchannels; i++) {
            this->mFilter_Y[i][0] = this->mFilter_Y[i][1] = 0;
        }
        return FMOD_OK;
    }

    if (inchannels == 1) {
        float fy1 = this->mFilter_Y[0][0];
        float fy2 = this->mFilter_Y[0][1];
        float fy;

        while (length) {
            fy = this->mFilter_tc * (inbuffer[0] + dc) + this->mFilter_oneminus_tc * fy1;
            fy1 = fy;
            fy = this->mFilter_tc * fy + this->mFilter_oneminus_tc * fy2;
            fy2 = fy;
            outbuffer[0] = fy;

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

        float fyL;
        float fyR;
        while (length) {
            fyL = this->mFilter_tc * (inbuffer[0] + dc) + this->mFilter_oneminus_tc * fyL1;
            fyR = this->mFilter_tc * (inbuffer[1] + dc) + this->mFilter_oneminus_tc * fyR1;

            fyL1 = fyL;
            fyR1 = fyR;

            fyL = this->mFilter_tc * fyL + this->mFilter_oneminus_tc * fyL2;
            fyR = this->mFilter_tc * fyR + this->mFilter_oneminus_tc * fyR2;

            fyL2 = fyL;
            fyR2 = fyR;

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

        float fyA;
        float fyB;
        float fyC;
        float fyD;
        float fyE;
        float fyF;

        while (length) {
            fyA = this->mFilter_tc * (inbuffer[0] + dc) + this->mFilter_oneminus_tc * fyA1;
            fyB = this->mFilter_tc * (inbuffer[1] + dc) + this->mFilter_oneminus_tc * fyB1;
            fyC = this->mFilter_tc * (inbuffer[2] + dc) + this->mFilter_oneminus_tc * fyC1;
            fyD = this->mFilter_tc * (inbuffer[3] + dc) + this->mFilter_oneminus_tc * fyD1;
            fyE = this->mFilter_tc * (inbuffer[4] + dc) + this->mFilter_oneminus_tc * fyE1;
            fyF = this->mFilter_tc * (inbuffer[5] + dc) + this->mFilter_oneminus_tc * fyF1;

            fyA1 = fyA;
            fyB1 = fyB;
            fyC1 = fyC;
            fyD1 = fyD;
            fyE1 = fyE;
            fyF1 = fyF;

            fyA = this->mFilter_tc * fyA + this->mFilter_oneminus_tc * fyA2;
            fyB = this->mFilter_tc * fyB + this->mFilter_oneminus_tc * fyB2;
            fyC = this->mFilter_tc * fyC + this->mFilter_oneminus_tc * fyC2;
            fyD = this->mFilter_tc * fyD + this->mFilter_oneminus_tc * fyD2;
            fyE = this->mFilter_tc * fyE + this->mFilter_oneminus_tc * fyE2;
            fyF = this->mFilter_tc * fyF + this->mFilter_oneminus_tc * fyF2;

            fyA2 = fyA;
            fyB2 = fyB;
            fyC2 = fyC;
            fyD2 = fyD;
            fyE2 = fyE;
            fyF2 = fyF;

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
            auto in = inbuffer + i;
            auto out = outbuffer + i;
            int len;

            float fy1 = this->mFilter_Y[i][0];
            float fy2 = this->mFilter_Y[i][1];
            float fy;
            len = length;
            while (len) {
                fy = this->mFilter_tc * (in[0] + dc) + this->mFilter_oneminus_tc * fy1;
                fy1 = fy;
                fy = this->mFilter_tc * fy + this->mFilter_oneminus_tc * fy2;
                fy2 = fy;
                out[0] = fy;

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

FMOD_RESULT DSPLowPassSimple::setParameterInternal(int32_t index, float value) {
    switch (index) {
    case FMOD_DSP_LOWPASS_SIMPLE_CUTOFF: {
        this->mCutoffHzUpdate = value;
        break;
    }
    }

    return FMOD_OK;
}

FMOD_RESULT DSPLowPassSimple::getParameterInternal(int32_t index, float* value, char* valuestr) {
    switch (index) {
    case FMOD_DSP_LOWPASS_CUTOFF: {
        *value = this->mCutoffHzUpdate;
        sprintf(valuestr, "%.02f", this->mCutoffHzUpdate);
        break;
    }
    }

    return FMOD_OK;
}

FMOD_RESULT DSPLowPassSimple::updateCoefficients(float cutoffhz) {
    int32_t outputrate;
    float RC;
    float dt;
    float threshold;

    this->mSystem->getSoftwareFormat(&outputrate, nullptr, nullptr, nullptr, nullptr, nullptr);

    dt = 1.0f / outputrate;
    threshold = outputrate / 3.14159265358979323846f;

    if (cutoffhz >= CUTOFF_MAX) {
        this->mFilter_tc = 1.0f;
        this->mFilter_oneminus_tc = 0.0f;
    } else if (cutoffhz <= threshold) {
        RC = 1.0f / (2.0f * FMOD_PI * this->mCutoffHz);
        this->mFilter_tc = dt / (RC + dt);
        this->mFilter_oneminus_tc = 1.0f - this->mFilter_tc;
    } else {
        this->mFilter_tc = 0.666666667f + (cutoffhz - threshold) / (3.0f * (CUTOFF_MAX - threshold));
        this->mFilter_oneminus_tc = 1.0f - this->mFilter_tc;
    }

    return FMOD_OK;
}

FMOD_RESULT F_CALLBACK DSPLowPassSimple::createCallback(FMOD_DSP_STATE* dsp) {
    auto lowpass_simple = static_cast<DSPLowPassSimple*>(dsp);
    return lowpass_simple->createInternal();
}

FMOD_RESULT F_CALLBACK DSPLowPassSimple::readCallback(FMOD_DSP_STATE* dsp, float* inbuffer, float* outbuffer, uint32_t length, int32_t inchannels, int32_t outchannels) {
    auto lowpass_simple = static_cast<DSPLowPassSimple*>(dsp);
    return lowpass_simple->readInternal(inbuffer, outbuffer, length, inchannels, outchannels);
}

FMOD_RESULT F_CALLBACK DSPLowPassSimple::resetCallback(FMOD_DSP_STATE* dsp) {
    auto lowpass_simple = static_cast<DSPLowPassSimple*>(dsp);
    return lowpass_simple->resetInternal();
}

FMOD_RESULT F_CALLBACK DSPLowPassSimple::setParameterCallback(FMOD_DSP_STATE* dsp, int32_t index, float value) {
    auto lowpass_simple = static_cast<DSPLowPassSimple*>(dsp);
    return lowpass_simple->setParameterInternal(index, value);
}

FMOD_RESULT F_CALLBACK DSPLowPassSimple::getParameterCallback(FMOD_DSP_STATE* dsp, int32_t index, float* value, char* valuestr) {
    auto lowpass_simple = static_cast<DSPLowPassSimple*>(dsp);
    return lowpass_simple->getParameterInternal(index, value, valuestr);
}

FMOD_DSP_DESCRIPTION_EX* DSPLowPassSimple::getDescriptionEx() {
    FMOD_memset(&dsplowpass_simple, 0, sizeof(FMOD_DSP_DESCRIPTION_EX));

    FMOD_strcpy(dsplowpass_simple.name, "FMOD Lowpass Simple");
    dsplowpass_simple.version = 0x00010100;

    dsplowpass_simple.create = DSPLowPassSimple::createCallback;
    dsplowpass_simple.reset = DSPLowPassSimple::resetCallback;

    dsplowpass_simple.read = DSPLowPassSimple::readCallback;

    dsplowpass_simple.numparameters = (sizeof(dsplowpass_simple_param) / sizeof(dsplowpass_simple_param[0]));
    dsplowpass_simple.paramdesc = dsplowpass_simple_param;
    dsplowpass_simple.setparameter = DSPLowPassSimple::setParameterCallback;
    dsplowpass_simple.getparameter = DSPLowPassSimple::getParameterCallback;

    dsplowpass_simple.mType = FMOD_DSP_TYPE_LOWPASS_SIMPLE;
    dsplowpass_simple.mCategory = FMOD_DSP_CATEGORY_FILTER;
    dsplowpass_simple.mSize = sizeof(DSPLowPassSimple);

    return &dsplowpass_simple;
}

} // namespace FMOD
