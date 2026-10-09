#include "xmodex/dsp/DSPDistortion.hpp"
#include "xmodex/Math.hpp"
#include "xmodex/String.hpp"
#include <cstdio>

namespace FMOD {

FMOD_DSP_DESCRIPTION_EX dspdistortion;

FMOD_DSP_PARAMETERDESC dspdistortion_param[1] = {
    { 0.0f, 1.0f, 0.5, "Level", "", "Distortion value.  0.0 to 1.0.  Default = 0.5." },
};

FMOD_RESULT DSPDistortion::createInternal() {
    Plugin::init();

    // TODO:
    //
    // this->mSupportsSIMD = FMOD_OS_SupportsSIMD();

    this->mSupportsSIMD = false;

    for (int32_t i = 0; i < this->mDescription.numparameters; i++) {
        FMOD_RESULT result;

        result = this->setParameter(i, this->mDescription.paramdesc[i].defaultval);
        if (result != FMOD_OK) {
            return result;
        }
    }

    return FMOD_OK;
}

FMOD_RESULT DSPDistortion::releaseInternal() {
    return FMOD_OK;
}

FMOD_RESULT DSPDistortion::resetInternal() {
    return FMOD_OK;
}

FMOD_RESULT DSPDistortion::readInternal(float* inbuffer, float* outbuffer, uint32_t length, int32_t inchannels, int32_t outchannels) {
    float k;
    float level = this->mLevel;

    if (level < 1.0f) {
        k = 2.0f * level / (1.0f - level);
    } else {
        k = (2.0f * 0.9999f / (1.0f - 0.9999f));
    }

    if (this->mSupportsSIMD) {
        // TODO:
        //
        // FMOD_DSP_Distortion_SIMD(inbuffer, outbuffer, length, inchannels, outchannels, k);
    } else {
        uint32_t len;
        float* in = inbuffer;
        float* out = outbuffer;
        float kplus1 = k + 1.0f;

        len = (length * inchannels) >> 3;
        while (len) {
            out[0] = in[0] * kplus1 / (FMOD_FABS(in[0]) * k + 1.0f);
            out[1] = in[1] * kplus1 / (FMOD_FABS(in[1]) * k + 1.0f);
            out[2] = in[2] * kplus1 / (FMOD_FABS(in[2]) * k + 1.0f);
            out[3] = in[3] * kplus1 / (FMOD_FABS(in[3]) * k + 1.0f);
            out[4] = in[4] * kplus1 / (FMOD_FABS(in[4]) * k + 1.0f);
            out[5] = in[5] * kplus1 / (FMOD_FABS(in[5]) * k + 1.0f);
            out[6] = in[6] * kplus1 / (FMOD_FABS(in[6]) * k + 1.0f);
            out[7] = in[7] * kplus1 / (FMOD_FABS(in[7]) * k + 1.0f);
            in += 8;
            out += 8;
            len--;
        }

        len = (length * inchannels) & 7;
        while (len) {
            out[0] = in[0] * kplus1 / (FMOD_FABS(in[0]) * k + 1.0f);
            in++;
            out++;
            len--;
        }
    }

    return FMOD_OK;
}

FMOD_RESULT DSPDistortion::setParameterInternal(int32_t index, float value) {
    this->mLevel = value;

    return FMOD_OK;
}

FMOD_RESULT DSPDistortion::getParameterInternal(int32_t index, float* value, char* valuestr) {
    *value = this->mLevel;
    sprintf(valuestr, "%.02f", this->mLevel);

    return FMOD_OK;
}

FMOD_RESULT F_CALLBACK DSPDistortion::createCallback(FMOD_DSP_STATE* dsp) {
    auto distortion = static_cast<DSPDistortion*>(dsp);
    return distortion->createInternal();
}

FMOD_RESULT F_CALLBACK DSPDistortion::releaseCallback(FMOD_DSP_STATE* dsp) {
    auto distortion = static_cast<DSPDistortion*>(dsp);
    return distortion->releaseInternal();
}

FMOD_RESULT F_CALLBACK DSPDistortion::resetCallback(FMOD_DSP_STATE* dsp) {
    auto distortion = static_cast<DSPDistortion*>(dsp);
    return distortion->resetInternal();
}

FMOD_RESULT F_CALLBACK DSPDistortion::readCallback(FMOD_DSP_STATE* dsp, float* inbuffer, float* outbuffer, uint32_t length, int32_t inchannels, int32_t outchannels) {
    auto distortion = static_cast<DSPDistortion*>(dsp);
    return distortion->readInternal(inbuffer, outbuffer, length, inchannels, outchannels);
}

FMOD_RESULT F_CALLBACK DSPDistortion::setParameterCallback(FMOD_DSP_STATE* dsp, int32_t index, float value) {
    auto distortion = static_cast<DSPDistortion*>(dsp);
    return distortion->setParameterInternal(index, value);
}

FMOD_RESULT F_CALLBACK DSPDistortion::getParameterCallback(FMOD_DSP_STATE* dsp, int32_t index, float* value, char* valuestr) {
    auto distortion = static_cast<DSPDistortion*>(dsp);
    return distortion->getParameterInternal(index, value, valuestr);
}

FMOD_DSP_DESCRIPTION_EX* DSPDistortion::getDescriptionEx() {
    FMOD_memset(&dspdistortion, 0, sizeof(FMOD_DSP_DESCRIPTION_EX));

    FMOD_strcpy(dspdistortion.name, "FMOD Distortion");
    dspdistortion.version = 0x10100;

    dspdistortion.create = DSPDistortion::createCallback;
    dspdistortion.release = DSPDistortion::releaseCallback;
    dspdistortion.reset = DSPDistortion::resetCallback;
    dspdistortion.read = DSPDistortion::readCallback;

    dspdistortion.numparameters = (sizeof(dspdistortion_param) / sizeof(dspdistortion_param[0]));
    dspdistortion.paramdesc = dspdistortion_param;
    dspdistortion.setparameter = DSPDistortion::setParameterCallback;
    dspdistortion.getparameter = DSPDistortion::getParameterCallback;

    dspdistortion.mType = FMOD_DSP_TYPE_DISTORTION;
    dspdistortion.mCategory = FMOD_DSP_CATEGORY_FILTER;
    dspdistortion.mSize = sizeof(DSPDistortion);

    return &dspdistortion;
}

} // namespace FMOD
