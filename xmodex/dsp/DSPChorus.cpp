#include "xmodex/dsp/DSPChorus.hpp"
#include "xmodex/Math.hpp"
#include "xmodex/Memory.hpp"
#include "xmodex/String.hpp"
#include "xmodex/SystemI.hpp"
#include <cstdio>

namespace FMOD {

FMOD_DSP_DESCRIPTION_EX dspchorus;

FMOD_DSP_PARAMETERDESC dspchorus_param[8] = {
    { 0.0f, 1.0f, 0.50f, "Dry mix", "", "Volume of original signal to pass to output.  0.0 to 1.0. Default = 0.5." },
    { 0.0f, 1.0f, 0.50f, "Wet mix tap 1", "", "Volume of 1st chorus tap.  0.0 to 1.0.  Default = 0.5." },
    { 0.0f, 1.0f, 0.50f, "Wet mix tap 2", "", "Volume of 2nd chorus tap. This tap is 90 degrees out of phase of the first tap.  0.0 to 1.0.  Default = 0.5." },
    { 0.0f, 1.0f, 0.50f, "Wet mix tap 3", "", "Volume of 3rd chorus tap. This tap is 90 degrees out of phase of the second tap.  0.0 to 1.0.  Default = 0.5." },
    { 0.0f, 100.0f, 40.00f, "Delay", "ms", "Chorus delay in ms.  0.1 to 100.0.  Default = 40.0 ms." },
    { 0.0f, 20.0f, 0.80f, "Rate", "hz", "Chorus modulation rate in hz.  0.0 to 20.0.  Default = 0.8 hz." },
    { 0.0f, 1.0f, 0.03f, "Depth", "", "Chorus modulation depth.  0.0 to 1.0.  Default = 0.03." },
    { 0.0f, 1.0f, 0.00f, "Feedback", "", "Chorus feedback.  Controls how much of the wet signal gets fed back into the chorus buffer.  0.0 to 1.0.  Default = 0.0." }
};

FMOD_RESULT DSPChorus::createInternal() {
    FMOD_RESULT result;
    int32_t channels;

    Plugin::init();

    for (int32_t i = 0; i < DSP_CHORUS_COSTABSIZE; i++) {
        this->mCosTab[i] = FMOD_COS(FMOD_PI_2 * static_cast<float>(i) / static_cast<float>(DSP_CHORUS_COSTABSIZE));
    }

    result = this->mSystem->getSoftwareFormat(&this->mOutputRate, nullptr, &channels, nullptr, nullptr, nullptr);
    if (result != FMOD_OK) {
        return result;
    }

    this->mChorusBufferLengthBytes = static_cast<int32_t>(static_cast<float>(this->mOutputRate) * DSP_CHORUS_MAXBUFFERLENGTHMS) / 1000;
    this->mChorusBufferLengthBytes *= channels;
    this->mChorusBufferLengthBytes *= sizeof(int16_t);
    this->mChorusBufferLengthBytes += 1024;

    this->mChorusBuffer = static_cast<int16_t*>(FMOD_Memory_Calloc(this->mChorusBufferLengthBytes));
    if (!this->mChorusBuffer) {
        return FMOD_ERR_MEMORY;
    }

    this->mChorusTick = 0.0f;

    for (int32_t i = 0; i < this->mDescription.numparameters; i++) {
        result = this->setParameter(i, this->mDescription.paramdesc[i].defaultval);
        if (result != FMOD_OK) {
            return result;
        }
    }

    return FMOD_OK;
}

FMOD_RESULT DSPChorus::releaseInternal() {
    if (this->mChorusBuffer) {
        FMOD_Memory_Free(this->mChorusBuffer);
        this->mChorusBuffer = nullptr;
    }

    return FMOD_OK;
}

FMOD_RESULT DSPChorus::resetInternal() {
    this->mChorusBufferPosition = 0;

    if (this->mChorusBuffer) {
        FMOD_memset(this->mChorusBuffer, 0, this->mChorusBufferLengthBytes);
    }

    return FMOD_OK;
}

FMOD_RESULT DSPChorus::readInternal(float* inbuffer, float* outbuffer, uint32_t length, int32_t inchannels, int32_t outchannels) {
    int32_t halfchorusbufferlength = this->mChorusBufferLength / 2;
    float halfdepth = this->mDepth * 0.5f;

    for (uint32_t i = 0; i < length; i++) {
        uint32_t p1[3];
        uint32_t p2[3];
        float frac[3];

        p1[0] = (this->mChorusBufferPosition + static_cast<uint32_t>(this->mChorusPosition[0])) % this->mChorusBufferLength;
        p1[1] = (this->mChorusBufferPosition + static_cast<uint32_t>(this->mChorusPosition[1])) % this->mChorusBufferLength;
        p1[2] = (this->mChorusBufferPosition + static_cast<uint32_t>(this->mChorusPosition[2])) % this->mChorusBufferLength;
        p2[0] = p1[0] + 1;
        p2[1] = p1[1] + 1;
        p2[2] = p1[2] + 1;

        frac[0] = this->mChorusPosition[0] - static_cast<int32_t>(this->mChorusPosition[0]);
        frac[1] = this->mChorusPosition[1] - static_cast<int32_t>(this->mChorusPosition[1]);
        frac[2] = this->mChorusPosition[2] - static_cast<int32_t>(this->mChorusPosition[2]);

        for (int32_t j = 0; j < inchannels; j++) {
            int32_t offset = (i * inchannels) + j;
            float val;
            float chorusval;

            val = inbuffer[offset] * this->mDryMix * 32768.0f;

            val += ((this->mChorusBuffer[(p1[0] * inchannels) + j] * (1.0f - frac[0])) + (this->mChorusBuffer[(p2[0] * inchannels) + j] * frac[0])) * this->mWetMix1;
            val += ((this->mChorusBuffer[(p1[1] * inchannels) + j] * (1.0f - frac[1])) + (this->mChorusBuffer[(p2[1] * inchannels) + j] * frac[1])) * this->mWetMix2;
            val += ((this->mChorusBuffer[(p1[2] * inchannels) + j] * (1.0f - frac[2])) + (this->mChorusBuffer[(p2[2] * inchannels) + j] * frac[2])) * this->mWetMix3;

            chorusval = (inbuffer[offset] * 32767.0f) + val * this->mFeedback;

            this->mChorusBuffer[(this->mChorusBufferPosition * inchannels) + j] = chorusval < -32768.0f ? -32768 : chorusval > 32767.0f ? 32767
                                                                                                                                        : static_cast<int16_t>(chorusval);

            outbuffer[offset] = val / 32767.0f;
        }

        if (!this->mChorusBufferPosition) {
            for (int32_t j = 0; j < inchannels; j++) {
                this->mChorusBuffer[(this->mChorusBufferLength * inchannels) + j] = this->mChorusBuffer[j];
            }
        }

        this->mChorusBufferPosition++;
        if (this->mChorusBufferPosition >= this->mChorusBufferLength) {
            this->mChorusBufferPosition = 0;
        }

        this->mChorusPosition[0] = (1.0f + this->sine(this->mChorusTick + 0.00f)) * halfdepth;
        this->mChorusPosition[1] = (1.0f + this->sine(this->mChorusTick + 0.25f)) * halfdepth;
        this->mChorusPosition[2] = (1.0f + this->sine(this->mChorusTick + 0.50f)) * halfdepth;

        this->mChorusPosition[0] = halfchorusbufferlength + (this->mChorusPosition[0] * this->mChorusBufferLength);
        this->mChorusPosition[1] = halfchorusbufferlength + (this->mChorusPosition[1] * this->mChorusBufferLength);
        this->mChorusPosition[2] = halfchorusbufferlength + (this->mChorusPosition[2] * this->mChorusBufferLength);

        this->mChorusTick += this->mChorusSpeed;
    }

    return FMOD_OK;
}

FMOD_RESULT DSPChorus::setParameterInternal(int32_t index, float value) {
    float olddelay;

    olddelay = this->mDelay;

    this->mSystem->lockDSP();

    switch (index) {
    case FMOD_DSP_CHORUS_DRYMIX: {
        this->mDryMix = value;
        break;
    }
    case FMOD_DSP_CHORUS_WETMIX1: {
        this->mWetMix1 = value;
        break;
    }
    case FMOD_DSP_CHORUS_WETMIX2: {
        this->mWetMix2 = value;
        break;
    }
    case FMOD_DSP_CHORUS_WETMIX3: {
        this->mWetMix3 = value;
        break;
    }
    case FMOD_DSP_CHORUS_DELAY: {
        this->mDelay = value;
        break;
    }
    case FMOD_DSP_CHORUS_RATE: {
        this->mRateHz = value;
        break;
    }
    case FMOD_DSP_CHORUS_DEPTH: {
        this->mDepth = value;
        break;
    }
    case FMOD_DSP_CHORUS_FEEDBACK: {
        this->mFeedback = value;
        break;
    }
    }

    if (this->mDelay != olddelay) {
        this->mChorusBufferLength = static_cast<uint32_t>(static_cast<float>(this->mOutputRate) * this->mDelay / 1000.0f) * 2;
        if (this->mChorusBufferLength < 4) {
            this->mChorusBufferLength = 4;
        }

        this->resetInternal();
    }

    this->mChorusSpeed = this->mRateHz / static_cast<float>(this->mOutputRate);

    this->mSystem->unlockDSP();

    return FMOD_OK;
}

FMOD_RESULT DSPChorus::getParameterInternal(int32_t index, float* value, char* valuestr) {
    switch (index) {
    case FMOD_DSP_CHORUS_DRYMIX: {
        *value = this->mDryMix;
        sprintf(valuestr, "%.02f", this->mDryMix);
        break;
    }
    case FMOD_DSP_CHORUS_WETMIX1: {
        *value = this->mWetMix1;
        sprintf(valuestr, "%.02f", this->mWetMix1);
        break;
    }
    case FMOD_DSP_CHORUS_WETMIX2: {
        *value = this->mWetMix2;
        sprintf(valuestr, "%.02f", this->mWetMix2);
        break;
    }
    case FMOD_DSP_CHORUS_WETMIX3: {
        *value = this->mWetMix3;
        sprintf(valuestr, "%.02f", this->mWetMix3);
        break;
    }
    case FMOD_DSP_CHORUS_DELAY: {
        *value = this->mDelay;
        sprintf(valuestr, "%.02f", this->mDelay);
        break;
    }
    case FMOD_DSP_CHORUS_RATE: {
        *value = this->mRateHz;
        sprintf(valuestr, "%.02f", this->mRateHz);
        break;
    }
    case FMOD_DSP_CHORUS_DEPTH: {
        *value = this->mDepth;
        sprintf(valuestr, "%.02f", this->mDepth);
        break;
    }
    case FMOD_DSP_CHORUS_FEEDBACK: {
        *value = this->mFeedback;
        sprintf(valuestr, "%.02f", this->mFeedback);
        break;
    }
    }

    return FMOD_OK;
}

FMOD_RESULT F_CALLBACK DSPChorus::createCallback(FMOD_DSP_STATE* dsp) {
    auto chorus = static_cast<DSPChorus*>(dsp);
    return chorus->createInternal();
}

FMOD_RESULT F_CALLBACK DSPChorus::readCallback(FMOD_DSP_STATE* dsp, float* inbuffer, float* outbuffer, uint32_t length, int32_t inchannels, int32_t outchannels) {
    auto chorus = static_cast<DSPChorus*>(dsp);
    return chorus->readInternal(inbuffer, outbuffer, length, inchannels, outchannels);
}

FMOD_RESULT F_CALLBACK DSPChorus::releaseCallback(FMOD_DSP_STATE* dsp) {
    auto chorus = static_cast<DSPChorus*>(dsp);
    return chorus->releaseInternal();
}

FMOD_RESULT F_CALLBACK DSPChorus::resetCallback(FMOD_DSP_STATE* dsp) {
    auto chorus = static_cast<DSPChorus*>(dsp);
    return chorus->resetInternal();
}

FMOD_RESULT F_CALLBACK DSPChorus::setParameterCallback(FMOD_DSP_STATE* dsp, int32_t index, float value) {
    auto chorus = static_cast<DSPChorus*>(dsp);
    return chorus->setParameterInternal(index, value);
}

FMOD_RESULT F_CALLBACK DSPChorus::getParameterCallback(FMOD_DSP_STATE* dsp, int32_t index, float* value, char* valuestr) {
    auto chorus = static_cast<DSPChorus*>(dsp);
    return chorus->getParameterInternal(index, value, valuestr);
}

FMOD_DSP_DESCRIPTION_EX* DSPChorus::getDescriptionEx() {
    FMOD_memset(&dspchorus, 0, sizeof(FMOD_DSP_DESCRIPTION_EX));

    FMOD_strcpy(dspchorus.name, "FMOD Chorus");
    dspchorus.version = 0x10100;
    dspchorus.create = DSPChorus::createCallback;
    dspchorus.release = DSPChorus::releaseCallback;
    dspchorus.reset = DSPChorus::resetCallback;
    dspchorus.read = DSPChorus::readCallback;

    dspchorus.numparameters = (sizeof(dspchorus_param) / sizeof(dspchorus_param[0]));
    dspchorus.paramdesc = dspchorus_param;
    dspchorus.setparameter = DSPChorus::setParameterCallback;
    dspchorus.getparameter = DSPChorus::getParameterCallback;

    dspchorus.mType = FMOD_DSP_TYPE_CHORUS;
    dspchorus.mCategory = FMOD_DSP_CATEGORY_FILTER;
    dspchorus.mSize = sizeof(DSPChorus);

    return &dspchorus;
}

} // namespace FMOD
