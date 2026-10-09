#include "xmodex/dsp/DSPLowPass.hpp"
#include "xmodex/Math.hpp"
#include "xmodex/String.hpp"
#include "xmodex/SystemI.hpp"
#include "xmodex/dsp/DSPConnection.hpp"
#include <cstdio>

namespace FMOD {

FMOD_DSP_DESCRIPTION_EX dsplowpass;

FMOD_DSP_PARAMETERDESC dsplowpass_param[2] = {
    { 10.0f, 22000.0f, 5000.0f, "Cutoff freq", "hz", "Lowpass cutoff frequency in hz.   1.0 to output 22000.0.  Default = 5000.0." },
    { 1.0f, 10.0f, 1.0f, "Resonance", "", "Lowpass resonance Q value. 1.0 to 10.0.  Default = 1.0." }
};

FMOD_RESULT DSPLowPass::bilinear(float a0, float a1, float a2, float b0, float b1, float b2, float* k, float fs, float* coef) {
    float ad = 4.0f * a2 * fs * fs + 2.0f * a1 * fs + a0;
    float bd = 4.0f * b2 * fs * fs + 2.0f * b1 * fs + b0;

    *k *= ad / bd;

    *coef++ = static_cast<float>((2.0f * b0 - 8.0f * b2 * fs * fs) / bd);
    *coef++ = static_cast<float>((4.0f * b2 * fs * fs - 2.0f * b1 * fs + b0) / bd);

    *coef++ = static_cast<float>((2.0f * a0 - 8.0f * a2 * fs * fs) / ad);
    *coef = static_cast<float>((4.0f * a2 * fs * fs - 2.0f * a1 * fs + a0) / ad);

    return FMOD_OK;
}

FMOD_RESULT DSPLowPass::szxform(float* a0, float* a1, float* a2, float* b0, float* b1, float* b2, float fc, float fs, float* k, float* coef) {
    this->prewarp(a0, a1, a2, fc, fs);
    this->prewarp(b0, b1, b2, fc, fs);
    this->bilinear(*a0, *a1, *a2, *b0, *b1, *b2, k, fs, coef);
    return FMOD_OK;
}

FMOD_RESULT DSPLowPass::prewarp(float* a0, float* a1, float* a2, float fc, float fs) {
    float pi = 4.0f * FMOD_ATAN(1.0f);
    float wp = 2.0f * fs * FMOD_TAN(pi * fc / fs);

    *a2 = (*a2) / (wp * wp);
    *a1 = (*a1) / wp;

    return FMOD_OK;
}

FMOD_RESULT DSPLowPass::updateState(float resonance, float cutoff) {
    FMOD_RESULT result;
    int32_t outputrate;

    result = this->mSystem->getSoftwareFormat(&outputrate, nullptr, nullptr, nullptr, nullptr, nullptr);
    if (result != FMOD_OK) {
        return result;
    }

    if (resonance >= 1.0f) {
        float a0;
        float a1;
        float a2;
        float b0;
        float b1;
        float b2;
        float fs;
        float k[2];
        float ktotal;
        float* coef;

        k[0] = 1.0f;
        k[1] = 1.0f;
        ktotal = 1.0f;
        coef = this->mCoefficients + 1;
        fs = static_cast<float>(outputrate);

        for (int32_t i = 0; i < LOWPASS_FILTER_SECTIONS; i++) {
            a0 = this->mProtoCoef[i].a0;
            a1 = this->mProtoCoef[i].a1;
            a2 = this->mProtoCoef[i].a2;

            b0 = this->mProtoCoef[i].b0;
            b1 = this->mProtoCoef[i].b1 / resonance;
            b2 = this->mProtoCoef[i].b2;

            this->szxform(&a0, &a1, &a2, &b0, &b1, &b2, static_cast<float>(cutoff), fs, &k[i], coef);

            if (this->mGainFactor[i] != 0.0f) {
                for (int32_t lout = 0; lout < DSP_MAXLEVELS_MAX; lout++) {
                    this->mHistory[lout][i] *= k[i] / this->mGainFactor[i];
                }
            }

            this->mGainFactor[i] = k[i];
            ktotal *= k[i];

            coef += 4;
        }

        this->mCoefficients[0] = ktotal;
    }

    return FMOD_OK;
}

FMOD_RESULT DSPLowPass::createInternal() {
    Plugin::init();

    this->mProtoCoef[0].a0 = 1.0f;
    this->mProtoCoef[0].a1 = 0;
    this->mProtoCoef[0].a2 = 0;
    this->mProtoCoef[0].b0 = 1.0f;
    this->mProtoCoef[0].b1 = 0.765367f;
    this->mProtoCoef[0].b2 = 1.0f;
    this->mGainFactor[0] = 1.0f;

    this->mProtoCoef[1].a0 = 1.0f;
    this->mProtoCoef[1].a1 = 0;
    this->mProtoCoef[1].a2 = 0;
    this->mProtoCoef[1].b0 = 1.0f;
    this->mProtoCoef[1].b1 = 1.847759f;
    this->mProtoCoef[1].b2 = 1.0f;
    this->mGainFactor[1] = 1.0f;

    for (int32_t i = 0; i < this->mDescription.numparameters; i++) {
        FMOD_RESULT result;

        result = this->setParameter(i, this->mDescription.paramdesc[i].defaultval);
        if (result != FMOD_OK) {
            return result;
        }
    }

    this->mResonance = this->mResonanceUpdate;
    this->mCutoffHz = this->mCutoffHzUpdate;

    this->updateState(this->mResonance, this->mCutoffHz);

    return FMOD_OK;
}

FMOD_RESULT DSPLowPass::readInternal(float* inbuffer, float* outbuffer, uint32_t length, int32_t inchannels, int32_t outchannels) {
    static float dc = static_cast<float>(1E-20);

    if (this->mResonance != this->mResonanceUpdate || this->mCutoffHz != this->mCutoffHzUpdate) {
        this->mResonance = this->mResonanceUpdate;
        this->mCutoffHz = this->mCutoffHzUpdate;

        this->updateState(this->mResonance, this->mCutoffHz);
    }

    if (inchannels == 1) {
        auto history = this->mHistory[0];

        while (length) {
            float a;
            float b;
            float c;

            a = ((inbuffer[0] + dc) * this->mCoefficients[0]) - (history[0] * this->mCoefficients[1]) - (history[1] * this->mCoefficients[2]);
            b = (history[0] * this->mCoefficients[3]) + (history[1] * this->mCoefficients[4]) - (history[2] * this->mCoefficients[5]) - (history[3] * this->mCoefficients[6]);
            c = (history[2] * this->mCoefficients[7]) + (history[3] * this->mCoefficients[8]);

            history[1] = history[0];
            history[3] = history[2];

            dc = -dc;
            length--;
            inbuffer++;
            outbuffer++;

            history[0] = a;
            history[2] = a + b;
            outbuffer[-1] = a + b + c;
        }
    } else if (inchannels == 2) {
        auto historyL = this->mHistory[0];
        auto historyR = this->mHistory[1];

        while (length) {
            float aL;
            float bL;
            float cL;
            float aR;
            float bR;
            float cR;

            aL = ((inbuffer[0] + dc) * this->mCoefficients[0]) - (historyL[0] * this->mCoefficients[1]) - (historyL[1] * this->mCoefficients[2]);
            aR = ((inbuffer[1] + dc) * this->mCoefficients[0]) - (historyR[0] * this->mCoefficients[1]) - (historyR[1] * this->mCoefficients[2]);
            bL = (historyL[0] * this->mCoefficients[3]) + (historyL[1] * this->mCoefficients[4]) - (historyL[2] * this->mCoefficients[5]) - (historyL[3] * this->mCoefficients[6]);
            bR = (historyR[0] * this->mCoefficients[3]) + (historyR[1] * this->mCoefficients[4]) - (historyR[2] * this->mCoefficients[5]) - (historyR[3] * this->mCoefficients[6]);
            cL = (historyL[2] * this->mCoefficients[7]) + (historyL[3] * this->mCoefficients[8]);
            cR = (historyR[2] * this->mCoefficients[7]) + (historyR[3] * this->mCoefficients[8]);

            dc = -dc;
            length--;
            inbuffer += 2;
            outbuffer += 2;

            historyL[1] = historyL[0];
            historyL[3] = historyL[2];
            historyL[0] = aL;
            historyL[2] = aL + bL;
            historyR[1] = historyR[0];
            historyR[3] = historyR[2];
            historyR[0] = aR;
            historyR[2] = aR + bR;

            outbuffer[-2] = aL + bL + cL;
            outbuffer[-1] = aR + bR + cR;
        }
    } else if (inchannels == 6) {
        auto history0 = this->mHistory[0];
        auto history1 = this->mHistory[1];
        auto history2 = this->mHistory[2];
        auto history3 = this->mHistory[3];
        auto history4 = this->mHistory[4];
        auto history5 = this->mHistory[5];

        while (length) {
            float a0;
            float b0;
            float c0;
            float a1;
            float b1;
            float c1;
            float a2;
            float b2;
            float c2;
            float a3;
            float b3;
            float c3;
            float a4;
            float b4;
            float c4;
            float a5;
            float b5;
            float c5;

            a0 = ((inbuffer[0] + dc) * this->mCoefficients[0]) - (history0[0] * this->mCoefficients[1]) - (history0[1] * this->mCoefficients[2]);
            a1 = ((inbuffer[1] + dc) * this->mCoefficients[0]) - (history1[0] * this->mCoefficients[1]) - (history1[1] * this->mCoefficients[2]);
            a2 = ((inbuffer[2] + dc) * this->mCoefficients[0]) - (history2[0] * this->mCoefficients[1]) - (history2[1] * this->mCoefficients[2]);
            a3 = ((inbuffer[3] + dc) * this->mCoefficients[0]) - (history3[0] * this->mCoefficients[1]) - (history3[1] * this->mCoefficients[2]);
            a4 = ((inbuffer[4] + dc) * this->mCoefficients[0]) - (history4[0] * this->mCoefficients[1]) - (history4[1] * this->mCoefficients[2]);
            a5 = ((inbuffer[5] + dc) * this->mCoefficients[0]) - (history5[0] * this->mCoefficients[1]) - (history5[1] * this->mCoefficients[2]);
            b0 = (history0[0] * this->mCoefficients[3]) + (history0[1] * this->mCoefficients[4]) - (history0[2] * this->mCoefficients[5]) - (history0[3] * this->mCoefficients[6]);
            b1 = (history1[0] * this->mCoefficients[3]) + (history1[1] * this->mCoefficients[4]) - (history1[2] * this->mCoefficients[5]) - (history1[3] * this->mCoefficients[6]);
            b2 = (history2[0] * this->mCoefficients[3]) + (history2[1] * this->mCoefficients[4]) - (history2[2] * this->mCoefficients[5]) - (history2[3] * this->mCoefficients[6]);
            b3 = (history3[0] * this->mCoefficients[3]) + (history3[1] * this->mCoefficients[4]) - (history3[2] * this->mCoefficients[5]) - (history3[3] * this->mCoefficients[6]);
            b4 = (history4[0] * this->mCoefficients[3]) + (history4[1] * this->mCoefficients[4]) - (history4[2] * this->mCoefficients[5]) - (history4[3] * this->mCoefficients[6]);
            b5 = (history5[0] * this->mCoefficients[3]) + (history5[1] * this->mCoefficients[4]) - (history5[2] * this->mCoefficients[5]) - (history5[3] * this->mCoefficients[6]);
            c0 = (history0[2] * this->mCoefficients[7]) + (history0[3] * this->mCoefficients[8]);
            c1 = (history1[2] * this->mCoefficients[7]) + (history1[3] * this->mCoefficients[8]);
            c2 = (history2[2] * this->mCoefficients[7]) + (history2[3] * this->mCoefficients[8]);
            c3 = (history3[2] * this->mCoefficients[7]) + (history3[3] * this->mCoefficients[8]);
            c4 = (history4[2] * this->mCoefficients[7]) + (history4[3] * this->mCoefficients[8]);
            c5 = (history5[2] * this->mCoefficients[7]) + (history5[3] * this->mCoefficients[8]);

            dc = -dc;
            length--;
            inbuffer += 6;
            outbuffer += 6;

            history0[1] = history0[0];
            history0[3] = history0[2];
            history0[0] = a0;
            history0[2] = a0 + b0;
            history1[1] = history1[0];
            history1[3] = history1[2];
            history1[0] = a1;
            history1[2] = a1 + b1;
            history2[1] = history2[0];
            history2[3] = history2[2];
            history2[0] = a2;
            history2[2] = a2 + b2;
            history3[1] = history3[0];
            history3[3] = history3[2];
            history3[0] = a3;
            history3[2] = a3 + b3;
            history4[1] = history4[0];
            history4[3] = history4[2];
            history4[0] = a4;
            history4[2] = a4 + b4;
            history5[1] = history5[0];
            history5[3] = history5[2];
            history5[0] = a5;
            history5[2] = a5 + b5;

            outbuffer[-6] = a0 + b0 + c0;
            outbuffer[-5] = a1 + b1 + c1;
            outbuffer[-4] = a2 + b2 + c2;
            outbuffer[-3] = a3 + b3 + c3;
            outbuffer[-2] = a4 + b4 + c4;
            outbuffer[-1] = a5 + b5 + c5;
        }
    } else {
        while (length) {
            for (uint32_t i = 0; i < static_cast<uint32_t>(inchannels); i++) {
                outbuffer[i] = this->filter(inbuffer[i], i);
            }

            length--;
            inbuffer += inchannels;
            outbuffer += inchannels;
        }
    }

    return FMOD_OK;
}

FMOD_RESULT DSPLowPass::setParameterInternal(int32_t index, float value) {
    if (index == FMOD_DSP_LOWPASS_CUTOFF) {
        this->mCutoffHzUpdate = value;
    } else if (index == FMOD_DSP_LOWPASS_RESONANCE) {
        this->mResonanceUpdate = value;
    }

    return FMOD_OK;
}

FMOD_RESULT DSPLowPass::getParameterInternal(int32_t index, float* value, char* valuestr) {
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

FMOD_RESULT F_CALLBACK DSPLowPass::createCallback(FMOD_DSP_STATE* dsp) {
    auto lowpass = static_cast<DSPLowPass*>(dsp);
    return lowpass->createInternal();
}

FMOD_RESULT F_CALLBACK DSPLowPass::readCallback(FMOD_DSP_STATE* dsp, float* inbuffer, float* outbuffer, uint32_t length, int32_t inchannels, int32_t outchannels) {
    auto lowpass = static_cast<DSPLowPass*>(dsp);
    return lowpass->readInternal(inbuffer, outbuffer, length, inchannels, outchannels);
}

FMOD_RESULT F_CALLBACK DSPLowPass::setParameterCallback(FMOD_DSP_STATE* dsp, int32_t index, float value) {
    auto lowpass = static_cast<DSPLowPass*>(dsp);
    return lowpass->setParameterInternal(index, value);
}

FMOD_RESULT F_CALLBACK DSPLowPass::getParameterCallback(FMOD_DSP_STATE* dsp, int32_t index, float* value, char* valuestr) {
    auto lowpass = static_cast<DSPLowPass*>(dsp);
    return lowpass->getParameterInternal(index, value, valuestr);
}

FMOD_DSP_DESCRIPTION_EX* DSPLowPass::getDescriptionEx() {
    FMOD_memset(&dsplowpass, 0, sizeof(FMOD_DSP_DESCRIPTION_EX));

    FMOD_strcpy(dsplowpass.name, "FMOD Lowpass");
    dsplowpass.version = 0x10100;

    dsplowpass.create = DSPLowPass::createCallback;
    dsplowpass.read = DSPLowPass::readCallback;

    dsplowpass.numparameters = (sizeof(dsplowpass_param) / sizeof(dsplowpass_param[0]));
    dsplowpass.paramdesc = dsplowpass_param;
    dsplowpass.setparameter = DSPLowPass::setParameterCallback;
    dsplowpass.getparameter = DSPLowPass::getParameterCallback;

    dsplowpass.mType = FMOD_DSP_TYPE_LOWPASS;
    dsplowpass.mCategory = FMOD_DSP_CATEGORY_FILTER;
    dsplowpass.mSize = sizeof(DSPLowPass);

    return &dsplowpass;
}

} // namespace FMOD
