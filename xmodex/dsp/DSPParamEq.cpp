#include "xmodex/dsp/DSPParamEq.hpp"
#include "xmodex/Math.hpp"
#include "xmodex/String.hpp"
#include "xmodex/SystemI.hpp"
#include <cstdio>

namespace FMOD {

FMOD_DSP_DESCRIPTION_EX dspparameq;

FMOD_DSP_PARAMETERDESC dspparameq_param[3] = {
    { 20.0f, 22000.0f, 8000.0f, "Center freq", "hz", "Frequency center.  20.0 to 22000.0.  Default = 8000.0." },
    { 0.2f, 5.0f, 1.0f, "Octave range", "octaves", "Octave range around the center frequency to filter.  0.2 to 5.0.  Default = 1.0." },
    { 0.05f, 3.0f, 1.0f, "Frequency gain", "", "Frequency Gain.  0.05 to 3.0.  Default = 1.0." },
};

FMOD_RESULT DSPParamEq::createInternal() {
    Plugin::init();

    this->mBandwidth = 0.2f;
    this->mGain = 1.0f;

    for (int32_t i = 0; i < this->mDescription.numparameters; i++) {
        FMOD_RESULT result;

        result = this->setParameter(i, this->mDescription.paramdesc[i].defaultval);
        if (result != FMOD_OK) {
            return result;
        }
    }

    this->resetInternal();

    this->mCenter = this->mCenterUpdate;
    this->mBandwidth = this->mBandwidthUpdate;
    this->mGain = this->mGainUpdate;

    this->updateCoefficients(this->mCenter, this->mBandwidth, this->mGain);

    return FMOD_OK;
}

FMOD_RESULT DSPParamEq::resetInternal() {
    for (int32_t i = 0; i < DSP_MAXLEVELS_MAX; i++) {
        this->mFilterIn[i][0] = this->mFilterIn[i][1] = 0;
        this->mFilterOut[i][0] = this->mFilterOut[i][1] = 0;
    }

    return FMOD_OK;
}

FMOD_RESULT DSPParamEq::readInternal(float* inbuffer, float* outbuffer, uint32_t length, int32_t inchannels, int32_t outchannels) {
    static float dc = static_cast<float>(1E-20);

    if (this->mCenter != this->mCenterUpdate || this->mBandwidth != this->mBandwidthUpdate || this->mGain != this->mGainUpdate) {
        this->mCenter = this->mCenterUpdate;
        this->mBandwidth = this->mBandwidthUpdate;
        this->mGain = this->mGainUpdate;

        this->updateCoefficients(this->mCenter, this->mBandwidth, this->mGain);
    }

    float a0 = 1.0f / this->mCoefficient_a0;
    float a1 = this->mCoefficient_a1;
    float a2 = this->mCoefficient_a2;
    float b0 = this->mCoefficient_b0;
    float b1 = this->mCoefficient_b1;
    float b2 = this->mCoefficient_b2;

    if (inchannels == 1) {
        float in1 = this->mFilterIn[0][0];
        float in2 = this->mFilterIn[0][1];
        float out1 = this->mFilterOut[0][0];
        float out2 = this->mFilterOut[0][1];

        for (uint32_t i = 0; i < length; i++) {
            float in0;
            float out0;

            in0 = inbuffer[i] + dc;
            out0 = (b0 * in0 + b1 * in1 + b2 * in2 - a1 * out1 - a2 * out2) * a0;

            in2 = in1;
            in1 = in0;
            out2 = out1;
            out1 = out0;

            outbuffer[i] = out0;

            dc = -dc;
        }

        this->mFilterIn[0][0] = in1;
        this->mFilterIn[0][1] = in2;
        this->mFilterOut[0][0] = out1;
        this->mFilterOut[0][1] = out2;
    } else if (inchannels == 2) {
        float in1L = this->mFilterIn[0][0];
        float in1R = this->mFilterIn[1][0];
        float in2L = this->mFilterIn[0][1];
        float in2R = this->mFilterIn[1][1];
        float out1L = this->mFilterOut[0][0];
        float out1R = this->mFilterOut[1][0];
        float out2L = this->mFilterOut[0][1];
        float out2R = this->mFilterOut[1][1];

        while (length) {
            float in0L;
            float out0L;
            float in0R;
            float out0R;

            in0L = inbuffer[0] + dc;
            in0R = inbuffer[1] + dc;

            out0L = (b0 * in0L + b1 * in1L + b2 * in2L - a1 * out1L - a2 * out2L) * a0;
            out0R = (b0 * in0R + b1 * in1R + b2 * in2R - a1 * out1R - a2 * out2R) * a0;

            in2L = in1L;
            in1L = in0L;
            in2R = in1R;
            in1R = in0R;
            out2L = out1L;
            out1L = out0L;
            out2R = out1R;
            out1R = out0R;

            outbuffer[0] = out0L;
            outbuffer[1] = out0R;

            dc = -dc;
            outbuffer += 2;
            inbuffer += 2;
            length--;
        }

        this->mFilterIn[0][0] = in1L;
        this->mFilterIn[1][0] = in1R;
        this->mFilterIn[0][1] = in2L;
        this->mFilterIn[1][1] = in2R;
        this->mFilterOut[0][0] = out1L;
        this->mFilterOut[1][0] = out1R;
        this->mFilterOut[0][1] = out2L;
        this->mFilterOut[1][1] = out2R;
    } else if (inchannels == 6) {
        float in1A = this->mFilterIn[0][0];
        float in1B = this->mFilterIn[1][0];
        float in1C = this->mFilterIn[2][0];
        float in1D = this->mFilterIn[3][0];
        float in1E = this->mFilterIn[4][0];
        float in1F = this->mFilterIn[5][0];
        float in2A = this->mFilterIn[0][1];
        float in2B = this->mFilterIn[1][1];
        float in2C = this->mFilterIn[2][1];
        float in2D = this->mFilterIn[3][1];
        float in2E = this->mFilterIn[4][1];
        float in2F = this->mFilterIn[5][1];
        float out1A = this->mFilterOut[0][0];
        float out1B = this->mFilterOut[1][0];
        float out1C = this->mFilterOut[2][0];
        float out1D = this->mFilterOut[3][0];
        float out1E = this->mFilterOut[4][0];
        float out1F = this->mFilterOut[5][0];
        float out2A = this->mFilterOut[0][1];
        float out2B = this->mFilterOut[1][1];
        float out2C = this->mFilterOut[2][1];
        float out2D = this->mFilterOut[3][1];
        float out2E = this->mFilterOut[4][1];
        float out2F = this->mFilterOut[5][1];

        while (length) {
            float in0A;
            float out0A;
            float in0B;
            float out0B;
            float in0C;
            float out0C;
            float in0D;
            float out0D;
            float in0E;
            float out0E;
            float in0F;
            float out0F;

            in0A = inbuffer[0] + dc;
            in0B = inbuffer[1] + dc;
            in0C = inbuffer[2] + dc;
            in0D = inbuffer[3] + dc;
            in0E = inbuffer[4] + dc;
            in0F = inbuffer[5] + dc;

            out0A = (b0 * in0A + b1 * in1A + b2 * in2A - a1 * out1A - a2 * out2A) * a0;
            out0B = (b0 * in0B + b1 * in1B + b2 * in2B - a1 * out1B - a2 * out2B) * a0;
            out0C = (b0 * in0C + b1 * in1C + b2 * in2C - a1 * out1C - a2 * out2C) * a0;
            out0D = (b0 * in0D + b1 * in1D + b2 * in2D - a1 * out1D - a2 * out2D) * a0;
            out0E = (b0 * in0E + b1 * in1E + b2 * in2E - a1 * out1E - a2 * out2E) * a0;
            out0F = (b0 * in0F + b1 * in1F + b2 * in2F - a1 * out1F - a2 * out2F) * a0;

            in2A = in1A;
            in2B = in1B;
            in2C = in1C;
            in2D = in1D;
            in2E = in1E;
            in2F = in1F;
            in1A = in0A;
            in1B = in0B;
            in1C = in0C;
            in1D = in0D;
            in1E = in0E;
            in1F = in0F;

            out2A = out1A;
            out2B = out1B;
            out2C = out1C;
            out2D = out1D;
            out2E = out1E;
            out2F = out1F;
            out1A = out0A;
            out1B = out0B;
            out1C = out0C;
            out1D = out0D;
            out1E = out0E;
            out1F = out0F;

            outbuffer[0] = out0A;
            outbuffer[1] = out0B;
            outbuffer[2] = out0C;
            outbuffer[3] = out0D;
            outbuffer[4] = out0E;
            outbuffer[5] = out0F;

            dc = -dc;
            outbuffer += 6;
            inbuffer += 6;
            length--;
        }

        this->mFilterIn[0][0] = in1A;
        this->mFilterIn[1][0] = in1B;
        this->mFilterIn[2][0] = in1C;
        this->mFilterIn[3][0] = in1D;
        this->mFilterIn[4][0] = in1E;
        this->mFilterIn[5][0] = in1F;
        this->mFilterIn[0][1] = in2A;
        this->mFilterIn[1][1] = in2B;
        this->mFilterIn[2][1] = in2C;
        this->mFilterIn[3][1] = in2D;
        this->mFilterIn[4][1] = in2E;
        this->mFilterIn[5][1] = in2F;
        this->mFilterOut[0][0] = out1A;
        this->mFilterOut[1][0] = out1B;
        this->mFilterOut[2][0] = out1C;
        this->mFilterOut[3][0] = out1D;
        this->mFilterOut[4][0] = out1E;
        this->mFilterOut[5][0] = out1F;
        this->mFilterOut[0][1] = out2A;
        this->mFilterOut[1][1] = out2B;
        this->mFilterOut[2][1] = out2C;
        this->mFilterOut[3][1] = out2D;
        this->mFilterOut[4][1] = out2E;
        this->mFilterOut[5][1] = out2F;
    } else {
        for (int32_t i = 0; i < inchannels; i++) {
            uint32_t len;

            auto in = inbuffer + i;
            auto out = outbuffer + i;

            len = length;

            float in1 = this->mFilterIn[i][0];
            float in2 = this->mFilterIn[i][1];
            float out1 = this->mFilterOut[i][0];
            float out2 = this->mFilterOut[i][1];

            while (len) {
                float in0;
                float out0;

                in0 = in[0] + dc;
                out0 = (b0 * in0 + b1 * in1 + b2 * in2 - a1 * out1 - a2 * out2) * a0;
                in2 = in1;
                in1 = in0;
                out2 = out1;
                out1 = out0;
                out[0] = out0;

                in += inchannels;
                out += inchannels;
                len--;
                dc = -dc;
            }

            this->mFilterIn[i][0] = in1;
            this->mFilterIn[i][1] = in2;
            this->mFilterOut[i][0] = out1;
            this->mFilterOut[i][1] = out2;
        }
    }

    return FMOD_OK;
}

FMOD_RESULT DSPParamEq::setParameterInternal(int32_t index, float value) {
    FMOD_RESULT result;
    int32_t outputrate;

    result = this->mSystem->getSoftwareFormat(&outputrate, nullptr, nullptr, nullptr, nullptr, nullptr);
    if (result != FMOD_OK) {
        return result;
    }

    switch (index) {
    case FMOD_DSP_PARAMEQ_CENTER: {
        if (value >= (static_cast<float>(outputrate) / 2.0f) - 100.0f) {
            value = (static_cast<float>(outputrate) / 2.0f) - 100.0f;
        }
        this->mCenterUpdate = value;
        break;
    }
    case FMOD_DSP_PARAMEQ_BANDWIDTH: {
        this->mBandwidthUpdate = value;
        break;
    }
    case FMOD_DSP_PARAMEQ_GAIN: {
        this->mGainUpdate = value;
        break;
    }
    }

    return FMOD_OK;
}

FMOD_RESULT DSPParamEq::getParameterInternal(int32_t index, float* value, char* valuestr) {
    switch (index) {
    case FMOD_DSP_PARAMEQ_CENTER: {
        *value = this->mCenterUpdate;
        sprintf(valuestr, "%.02f", this->mCenterUpdate);
        break;
    }
    case FMOD_DSP_PARAMEQ_BANDWIDTH: {
        *value = this->mBandwidthUpdate;
        sprintf(valuestr, "%.02f", this->mBandwidthUpdate);
        break;
    }
    case FMOD_DSP_PARAMEQ_GAIN: {
        *value = this->mGainUpdate;
        sprintf(valuestr, "%.02f", this->mGainUpdate);
        break;
    }
    }

    return FMOD_OK;
}

FMOD_RESULT DSPParamEq::updateCoefficients(float center, float bandwidth, float gain) {
    int32_t outputrate;

    this->mSystem->getSoftwareFormat(&outputrate, nullptr, nullptr, nullptr, nullptr, nullptr);

    float w0 = 2.0f * FMOD_PI * center / static_cast<float>(outputrate);
    float Q = 1.0f / bandwidth;
    float A = gain;
    float alpha = FMOD_SIN(w0) / (2.0f * Q);

    this->mCoefficient_b0 = 1.0f + alpha * A;
    this->mCoefficient_b1 = -2.0f * FMOD_COS(w0);
    this->mCoefficient_b2 = 1.0f - alpha * A;
    this->mCoefficient_a0 = 1.0f + alpha / A;
    this->mCoefficient_a1 = -2.0f * FMOD_COS(w0);
    this->mCoefficient_a2 = 1.0f - alpha / A;

    return FMOD_OK;
}

FMOD_RESULT F_CALLBACK DSPParamEq::createCallback(FMOD_DSP_STATE* dsp) {
    auto parameq = static_cast<DSPParamEq*>(dsp);
    return parameq->createInternal();
}

FMOD_RESULT F_CALLBACK DSPParamEq::resetCallback(FMOD_DSP_STATE* dsp) {
    auto parameq = static_cast<DSPParamEq*>(dsp);
    return parameq->resetInternal();
}

FMOD_RESULT F_CALLBACK DSPParamEq::readCallback(FMOD_DSP_STATE* dsp, float* inbuffer, float* outbuffer, uint32_t length, int32_t inchannels, int32_t outchannels) {
    auto parameq = static_cast<DSPParamEq*>(dsp);
    return parameq->readInternal(inbuffer, outbuffer, length, inchannels, outchannels);
}

FMOD_RESULT F_CALLBACK DSPParamEq::setParameterCallback(FMOD_DSP_STATE* dsp, int32_t index, float value) {
    auto parameq = static_cast<DSPParamEq*>(dsp);
    return parameq->setParameterInternal(index, value);
}

FMOD_RESULT F_CALLBACK DSPParamEq::getParameterCallback(FMOD_DSP_STATE* dsp, int32_t index, float* value, char* valuestr) {
    auto parameq = static_cast<DSPParamEq*>(dsp);
    return parameq->getParameterInternal(index, value, valuestr);
}

FMOD_DSP_DESCRIPTION_EX* DSPParamEq::getDescriptionEx() {
    FMOD_memset(&dspparameq, 0, sizeof(FMOD_DSP_DESCRIPTION_EX));

    FMOD_strcpy(dspparameq.name, "FMOD ParamEQ");
    dspparameq.version = 0x10100;

    dspparameq.create = DSPParamEq::createCallback;
    dspparameq.reset = DSPParamEq::resetCallback;

    dspparameq.read = DSPParamEq::readCallback;

    dspparameq.numparameters = (sizeof(dspparameq_param) / sizeof(dspparameq_param[0]));
    dspparameq.paramdesc = dspparameq_param;
    dspparameq.setparameter = DSPParamEq::setParameterCallback;
    dspparameq.getparameter = DSPParamEq::getParameterCallback;

    dspparameq.mType = FMOD_DSP_TYPE_PARAMEQ;
    dspparameq.mCategory = FMOD_DSP_CATEGORY_FILTER;
    dspparameq.mSize = sizeof(DSPParamEq);

    return &dspparameq;
}

} // namespace FMOD
