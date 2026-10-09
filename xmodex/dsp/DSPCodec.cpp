#include "xmodex/dsp/DSPCodec.hpp"
#include "xmodex/Memory.hpp"
#include "xmodex/codec/Codec.hpp"

namespace FMOD {

FMOD_DSP_DESCRIPTION_EX dspcodec;

FMOD_RESULT DSPCodec::createInternal() {
    Plugin::init();

    this->mNewPosition = 0xFFFFFFFF;

    return FMOD_OK;
}

FMOD_RESULT DSPCodec::readInternal(int16_t* inbuffer, int16_t* outbuffer, uint32_t length, int32_t inchannels, int32_t outchannels) {
    FMOD_RESULT result;
    uint32_t size = length;
    uint32_t bytespersample;
    int32_t retries = 0;

    if (this->mNewPosition != 0xFFFFFFFF) {
        this->mPosition = this->mNewPosition;
        this->mCodec->reset();
        this->mCodec->setPosition(0, this->mPosition, FMOD_TIMEUNIT_PCM);
        this->mNewPosition = 0xFFFFFFFF;
    }

    SoundI::getBytesFromSamples(1, &bytespersample, this->mDescription.channels, this->mDescription.mFormat);

    while (size) {
        uint32_t endpoint;
        uint32_t toread;
        uint32_t read;

        if (this->mMode & FMOD_LOOP_NORMAL && this->mLoopCount) {
            endpoint = this->mLoopStart + this->mLoopLength - 1;
        } else {
            if (this->mCodec->mFlags & FMOD_CODEC_ACCURATELENGTH) {
                endpoint = this->mLength - 1;
            } else {
                endpoint = static_cast<uint32_t>(-1);
            }
        }

        toread = size;
        if (this->mPosition > endpoint) {
            toread = 0;
        } else if (this->mPosition + toread > endpoint) {
            toread = (endpoint - this->mPosition) + 1;
        }

        result = this->mCodec->read(outbuffer, toread * bytespersample, &read);
        if (result != FMOD_OK && result != FMOD_ERR_FILE_EOF) {
            return result;
        }

        read /= bytespersample;

        this->mPosition += read;
        size -= read;
        outbuffer += (read * inchannels);

        if (!read) {
            if (retries > 255) {
                result = FMOD_ERR_FILE_EOF;
            }
            retries++;
        }

        if (result == FMOD_ERR_FILE_EOF || this->mPosition > endpoint) {
            if (this->mMode & FMOD_LOOP_NORMAL && this->mLoopCount && retries <= 255) {
                this->mPosition = this->mLoopStart;
                this->mCodec->setPosition(0, this->mLoopStart, FMOD_TIMEUNIT_PCM);

                if (this->mLoopCount > 0) {
                    this->mLoopCount--;
                }
            } else {
                FMOD_memset(outbuffer, 0, size * bytespersample);
                return FMOD_ERR_FILE_EOF;
            }
        }
    }

    return FMOD_OK;
}

FMOD_RESULT DSPCodec::releaseInternal() {
    return FMOD_OK;
}

FMOD_RESULT DSPCodec::resetInternal() {
    return FMOD_OK;
}

FMOD_RESULT DSPCodec::setParameterInternal(int32_t index, float value) {
    return FMOD_OK;
}

FMOD_RESULT DSPCodec::getParameterInternal(int32_t index, float* value, char* valuestr) {
    return FMOD_OK;
}

FMOD_RESULT DSPCodec::setPositionInternal(uint32_t position) {
    this->mNewPosition = position;

    return FMOD_OK;
}

FMOD_RESULT DSPCodec::release(bool freethis) {
    FMOD_RESULT result;

    result = DSPResampler::release(false);
    if (freethis) {
        FMOD_Memory_FreeType(this, this->mWaveFormat.format == FMOD_SOUND_FORMAT_XMA ? FMOD_MEMORY_XBOX360_PHYSICAL : FMOD_MEMORY_NORMAL);
    }

    return result;
}

FMOD_RESULT F_CALLBACK DSPCodec::createCallback(FMOD_DSP_STATE* dsp) {
    auto dspcodec = static_cast<DSPCodec*>(dsp);
    return dspcodec->createInternal();
}

FMOD_RESULT F_CALLBACK DSPCodec::releaseCallback(FMOD_DSP_STATE* dsp) {
    auto dspcodec = static_cast<DSPCodec*>(dsp);
    return dspcodec->releaseInternal();
}

FMOD_RESULT F_CALLBACK DSPCodec::resetCallback(FMOD_DSP_STATE* dsp) {
    auto dspcodec = static_cast<DSPCodec*>(dsp);
    return dspcodec->resetInternal();
}

FMOD_RESULT F_CALLBACK DSPCodec::readCallback(FMOD_DSP_STATE* dsp, float* inbuffer, float* outbuffer, uint32_t length, int32_t inchannels, int32_t outchannels) {
    auto dspcodec = static_cast<DSPCodec*>(dsp);
    return dspcodec->readInternal(
        reinterpret_cast<int16_t*>(inbuffer),
        reinterpret_cast<int16_t*>(outbuffer),
        length,
        inchannels,
        outchannels);
}

FMOD_RESULT F_CALLBACK DSPCodec::setParameterCallback(FMOD_DSP_STATE* dsp, int32_t index, float value) {
    auto dspcodec = static_cast<DSPCodec*>(dsp);
    return dspcodec->setParameterInternal(index, value);
}

FMOD_RESULT F_CALLBACK DSPCodec::getParameterCallback(FMOD_DSP_STATE* dsp, int32_t index, float* value, char* valuestr) {
    auto dspcodec = static_cast<DSPCodec*>(dsp);
    return dspcodec->getParameterInternal(index, value, valuestr);
}

FMOD_RESULT F_CALLBACK DSPCodec::setPositionCallback(FMOD_DSP_STATE* dsp, uint32_t pos) {
    auto dspcodec = static_cast<DSPCodec*>(dsp);
    return dspcodec->setPositionInternal(pos);
}

FMOD_DSP_DESCRIPTION_EX* DSPCodec::getDescriptionEx() {
    FMOD_memset(&dspcodec, 0, sizeof(FMOD_DSP_DESCRIPTION_EX));
    FMOD_strcpy(dspcodec.name, "FMOD DSP Codec");
    dspcodec.version = 0x10100;

    dspcodec.create = DSPCodec::createCallback;
    dspcodec.release = DSPCodec::releaseCallback;
    dspcodec.reset = DSPCodec::resetCallback;
    dspcodec.read = DSPCodec::readCallback;
    dspcodec.setposition = DSPCodec::setPositionCallback;

    dspcodec.numparameters = 0;
    dspcodec.paramdesc = nullptr;
    dspcodec.setparameter = DSPCodec::setParameterCallback;
    dspcodec.getparameter = DSPCodec::getParameterCallback;

    dspcodec.mType = static_cast<FMOD_DSP_TYPE>(FMOD_DSP_TYPE_CODECREADER);
    dspcodec.mSize = sizeof(DSPCodec);

    return &dspcodec;
}

} // namespace FMOD
