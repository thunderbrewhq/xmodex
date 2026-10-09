#include "xmodex/output/OutputNoSound_NRT.hpp"
#include "xmodex/Memory.hpp"
#include "xmodex/String.hpp"
#include "xmodex/SystemI.hpp"

namespace FMOD {

FMOD_OUTPUT_DESCRIPTION_EX nosoundoutput_nrt;

FMOD_RESULT OutputNoSound_NRT::getNumDrivers(int32_t* numdrivers) {
    *numdrivers = 1;
    return FMOD_OK;
}

FMOD_RESULT OutputNoSound_NRT::getDriverName(int32_t driver, char* name, int32_t namelen) {
    FMOD_strncpy(name, "NoSound Driver", namelen);
    return FMOD_OK;
}

FMOD_RESULT OutputNoSound_NRT::getDriverCaps(int32_t id, FMOD_CAPS* caps) {
    *caps |= (FMOD_CAPS_OUTPUT_MULTICHANNEL |
              FMOD_CAPS_OUTPUT_FORMAT_PCM8 |
              FMOD_CAPS_OUTPUT_FORMAT_PCM16 |
              FMOD_CAPS_OUTPUT_FORMAT_PCM24 |
              FMOD_CAPS_OUTPUT_FORMAT_PCM32 |
              FMOD_CAPS_OUTPUT_FORMAT_PCMFLOAT);
    return FMOD_OK;
}

FMOD_RESULT OutputNoSound_NRT::init(int32_t selecteddriver, FMOD_INITFLAGS flags, int32_t* outputrate, int32_t outputchannels, FMOD_SOUND_FORMAT* outputformat, int32_t dspbufferlength, int32_t dspnumbuffers, void* extradriverdata) {
    FMOD_RESULT result;
    FMOD_SOUND_FORMAT format;
    uint32_t bufferlengthbytes = 0;
    int32_t channels;

    Plugin::init();

    result = this->mSystem->getSoftwareFormat(
        nullptr,
        &format,
        &channels,
        nullptr,
        nullptr,
        nullptr);
    if (result != FMOD_OK) {
        return result;
    }

    this->mBufferLength = dspbufferlength;

    result = SoundI::getBytesFromSamples(this->mBufferLength, &bufferlengthbytes, channels, format);
    if (result != FMOD_OK) {
        return result;
    }

    this->mBuffer = FMOD_Memory_Calloc(bufferlengthbytes);
    if (!this->mBuffer) {
        return FMOD_ERR_MEMORY;
    }

    return FMOD_OK;
}

FMOD_RESULT OutputNoSound_NRT::close() {
    if (this->mBuffer) {
        FMOD_Memory_Free(this->mBuffer);
        this->mBuffer = nullptr;
    }

    return FMOD_OK;
}

FMOD_RESULT OutputNoSound_NRT::update() {
    FMOD_RESULT result;

    result = this->mix(this->mBuffer, this->mBufferLength);
    if (result != FMOD_OK) {
        return FMOD_OK;
    }

    return FMOD_OK;
}

FMOD_RESULT F_CALLBACK OutputNoSound_NRT::getNumDriversCallback(FMOD_OUTPUT_STATE* output, int32_t* numdrivers) {
    auto nosound = static_cast<OutputNoSound_NRT*>(output);
    return nosound->getNumDrivers(numdrivers);
}

FMOD_RESULT F_CALLBACK OutputNoSound_NRT::getDriverNameCallback(FMOD_OUTPUT_STATE* output, int32_t id, char* name, int32_t namelen) {
    auto nosound = static_cast<OutputNoSound_NRT*>(output);
    return nosound->getDriverName(id, name, namelen);
}

FMOD_RESULT F_CALLBACK OutputNoSound_NRT::getDriverCapsCallback(FMOD_OUTPUT_STATE* output, int32_t id, FMOD_CAPS* caps) {
    auto nosound = static_cast<OutputNoSound_NRT*>(output);
    return nosound->getDriverCaps(id, caps);
}

FMOD_RESULT F_CALLBACK OutputNoSound_NRT::initCallback(FMOD_OUTPUT_STATE* output, int32_t selecteddriver, FMOD_INITFLAGS flags, int32_t* outputrate, int32_t outputchannels, FMOD_SOUND_FORMAT* outputformat, int32_t dspbufferlength, int32_t dspnumbuffers, void* extradriverdata) {
    auto nosound = static_cast<OutputNoSound_NRT*>(output);
    return nosound->init(selecteddriver, flags, outputrate, outputchannels, outputformat, dspbufferlength, dspnumbuffers, extradriverdata);
}

FMOD_RESULT F_CALLBACK OutputNoSound_NRT::closeCallback(FMOD_OUTPUT_STATE* output) {
    auto nosound = static_cast<OutputNoSound_NRT*>(output);
    return nosound->close();
}

FMOD_RESULT F_CALLBACK OutputNoSound_NRT::updateCallback(FMOD_OUTPUT_STATE* output) {
    auto nosound = static_cast<OutputNoSound_NRT*>(output);
    return nosound->update();
}

FMOD_OUTPUT_DESCRIPTION_EX* OutputNoSound_NRT::getDescriptionEx() {
    FMOD_memset(&nosoundoutput_nrt, 0, sizeof(FMOD_OUTPUT_DESCRIPTION_EX));

    nosoundoutput_nrt.name = "FMOD NoSound Output - Non real-time";
    nosoundoutput_nrt.version = 0x10100;
    nosoundoutput_nrt.polling = false;

    nosoundoutput_nrt.getnumdrivers = OutputNoSound_NRT::getNumDriversCallback;
    nosoundoutput_nrt.getdrivername = OutputNoSound_NRT::getDriverNameCallback;
    nosoundoutput_nrt.getdrivercaps = OutputNoSound_NRT::getDriverCapsCallback;
    nosoundoutput_nrt.init = OutputNoSound_NRT::initCallback;
    nosoundoutput_nrt.close = OutputNoSound_NRT::closeCallback;
    nosoundoutput_nrt.update = OutputNoSound_NRT::updateCallback;

    nosoundoutput_nrt.mType = FMOD_OUTPUTTYPE_NOSOUND_NRT;
    nosoundoutput_nrt.mSize = sizeof(OutputNoSound_NRT);

    return &nosoundoutput_nrt;
}

} // namespace FMOD
