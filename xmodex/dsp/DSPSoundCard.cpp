#include "xmodex/dsp/DSPSoundCard.hpp"
#include "xmodex/Math.hpp"
#include "xmodex/Memory.hpp"
#include "xmodex/SystemI.hpp"

namespace FMOD {

DSPSoundCard::DSPSoundCard() {
}

FMOD_RESULT DSPSoundCard::alloc(FMOD_DSP_DESCRIPTION_EX* description) {
    FMOD_RESULT result;

    result = DSPI::alloc(description);
    if (result != FMOD_OK) {
        return result;
    }

    if (description->mFormat == FMOD_SOUND_FORMAT_PCMFLOAT) {
        this->mConvertBuffer = nullptr;
    } else {
        this->mConvertBufferMemory = static_cast<float*>(FMOD_Memory_Calloc((this->mSystem->mDSPBlockSize * description->channels * sizeof(float)) + 16));
        if (!this->mConvertBufferMemory) {
            return FMOD_ERR_MEMORY;
        }

        this->mConvertBuffer = reinterpret_cast<float*>(FMOD_ALIGNPOINTER(this->mConvertBufferMemory, 16));
    }

    this->updateTreeLevel(0);

    return FMOD_OK;
}

FMOD_RESULT DSPSoundCard::execute(void* inbuffer, void** outbuffer, uint32_t* length, int32_t inchannels, int32_t* outchannels, FMOD_SPEAKERMODE speakermode) {
    FMOD_RESULT result;
    float* readbuffer;

    if (!this->mConvertBuffer) {
        result = DSPFilter::execute(static_cast<float*>(inbuffer), reinterpret_cast<float**>(outbuffer), length, inchannels, outchannels, speakermode);
        if (result != FMOD_OK) {
            return result;
        }
        return FMOD_OK;
    }

    readbuffer = nullptr;

    result = DSPFilter::execute(this->mConvertBuffer, &readbuffer, length, inchannels, outchannels, speakermode);
    if (result != FMOD_OK) {
        return result;
    }

    result = this->convert(*outbuffer, readbuffer, this->mDescription.mFormat, FMOD_SOUND_FORMAT_PCMFLOAT, *length * *outchannels, 1, 1, 1.0f);
    if (result != FMOD_OK) {
        return result;
    }

    return FMOD_OK;
}

FMOD_RESULT DSPSoundCard::release(bool freethis) {
    if (this->mConvertBufferMemory) {
        FMOD_Memory_Free(this->mConvertBufferMemory);
        this->mConvertBuffer = nullptr;
        this->mConvertBufferMemory = nullptr;
    }

    return DSPFilter::release(freethis);
}

} // namespace FMOD
