#include "xmodex/output/OutputNoSound.hpp"
#include "fmod.hpp"
#include "xmodex/Memory.hpp"
#include "xmodex/SystemI.hpp"
#include "xmodex/os/Time.hpp"
#include "xmodex/output/Output.hpp"
#include "xmodex/sound/SoundI.hpp"

namespace FMOD {

FMOD_OUTPUT_DESCRIPTION_EX nosoundoutput;

FMOD_RESULT OutputNoSound::close() {
    if (this->mBuffer) {
        FMOD_Memory_Free(this->mBuffer);
        this->mBuffer = nullptr;
    }

    return FMOD_OK;
}

FMOD_RESULT OutputNoSound::getDriverCaps(int32_t id, FMOD_CAPS* caps) {
    *caps |= (FMOD_CAPS_OUTPUT_MULTICHANNEL |
              FMOD_CAPS_OUTPUT_FORMAT_PCM8 |
              FMOD_CAPS_OUTPUT_FORMAT_PCM16 |
              FMOD_CAPS_OUTPUT_FORMAT_PCM24 |
              FMOD_CAPS_OUTPUT_FORMAT_PCM32 |
              FMOD_CAPS_OUTPUT_FORMAT_PCMFLOAT);
    return FMOD_OK;
}

FMOD_RESULT OutputNoSound::getDriverName(int32_t driver, char* name, int32_t namelen) {
    FMOD_strncpy(name, "NoSound Driver", namelen);
    return FMOD_OK;
}

FMOD_RESULT OutputNoSound::getNumDrivers(int32_t* numdrivers) {
    *numdrivers = 1;
    return FMOD_OK;
}

FMOD_RESULT OutputNoSound::getPosition(uint32_t* pcm) {
    FMOD_RESULT result;
    uint32_t pos = 0;
    int32_t outputrate;

    result = this->mSystem->getSoftwareFormat(&outputrate, nullptr, nullptr, nullptr, nullptr, nullptr);
    if (result != FMOD_OK) {
        return result;
    }

    FMOD_OS_Time_GetMs(&pos);

    pos *= outputrate;
    pos /= 1000;

    *pcm = pos;

    return FMOD_OK;
}

FMOD_RESULT OutputNoSound::init(int32_t selecteddriver, FMOD_INITFLAGS flags, int32_t* outputrate, int32_t outputchannels, FMOD_SOUND_FORMAT* outputformat, int32_t dspbufferlength, int32_t dspnumbuffers, void* extradriverdata) {
    Plugin::init();

    SoundI::getBytesFromSamples(dspbufferlength * dspnumbuffers, &this->mBlockLengthBytes, outputchannels, *outputformat);

    this->mBuffer = reinterpret_cast<char*>(FMOD_Memory_Calloc(this->mBlockLengthBytes));
    if (!this->mBuffer) {
        return FMOD_ERR_MEMORY;
    }

    return FMOD_OK;
}

FMOD_RESULT OutputNoSound::lock(uint32_t offset, uint32_t length, void** ptr1, void** ptr2, uint32_t* len1, uint32_t* len2) {
    offset %= this->mBlockLengthBytes;

    *ptr1 = reinterpret_cast<char*>(this->mBuffer) + offset;

    if (offset + length > this->mBlockLengthBytes) {
        *ptr2 = this->mBuffer;
        *len1 = this->mBlockLengthBytes - offset;
        *len2 = length - (this->mBlockLengthBytes - offset);
    } else {
        *ptr2 = nullptr;
        *len1 = length;
        *len2 = 0;
    }

    return FMOD_OK;
}

FMOD_RESULT F_CALLBACK OutputNoSound::closeCallback(FMOD_OUTPUT_STATE* output) {
    auto nosound = static_cast<OutputNoSound*>(output);
    return nosound->close();
}

FMOD_RESULT F_CALLBACK OutputNoSound::getDriverCapsCallback(FMOD_OUTPUT_STATE* output, int32_t id, FMOD_CAPS* caps) {
    auto nosound = static_cast<OutputNoSound*>(output);
    return nosound->getDriverCaps(id, caps);
}

FMOD_RESULT F_CALLBACK OutputNoSound::getDriverNameCallback(FMOD_OUTPUT_STATE* output, int32_t id, char* name, int32_t namelen) {
    auto nosound = static_cast<OutputNoSound*>(output);
    return nosound->getDriverName(id, name, namelen);
}

FMOD_RESULT F_CALLBACK OutputNoSound::getNumDriversCallback(FMOD_OUTPUT_STATE* output, int32_t* numdrivers) {
    auto nosound = static_cast<OutputNoSound*>(output);
    return nosound->getNumDrivers(numdrivers);
}

FMOD_RESULT F_CALLBACK OutputNoSound::getPositionCallback(FMOD_OUTPUT_STATE* output, uint32_t* pcm) {
    auto nosound = static_cast<OutputNoSound*>(output);
    return nosound->getPosition(pcm);
}

FMOD_RESULT F_CALLBACK OutputNoSound::initCallback(FMOD_OUTPUT_STATE* output, int32_t selecteddriver, FMOD_INITFLAGS flags, int32_t* outputrate, int32_t outputchannels, FMOD_SOUND_FORMAT* outputformat, int32_t dspbufferlength, int32_t dspnumbuffers, void* extradriverdata) {
    auto nosound = static_cast<OutputNoSound*>(output);
    return nosound->init(selecteddriver,
                         flags,
                         outputrate,
                         outputchannels,
                         outputformat,
                         dspbufferlength,
                         dspnumbuffers,
                         extradriverdata);
}

FMOD_RESULT F_CALLBACK OutputNoSound::lockCallback(FMOD_OUTPUT_STATE* output, uint32_t offset, uint32_t length, void** ptr1, void** ptr2, uint32_t* len1, uint32_t* len2) {
    auto nosound = static_cast<OutputNoSound*>(output);
    return nosound->lock(offset, length, ptr1, ptr2, len1, len2);
}

FMOD_OUTPUT_DESCRIPTION_EX* OutputNoSound::getDescriptionEx() {
    FMOD_memset(&nosoundoutput, 0, sizeof(FMOD_OUTPUT_DESCRIPTION_EX));

    nosoundoutput.name = "FMOD NoSound Output";
    nosoundoutput.version = 0x10100;
    nosoundoutput.polling = 1;
    nosoundoutput.getnumdrivers = OutputNoSound::getNumDriversCallback;
    nosoundoutput.getdrivername = OutputNoSound::getDriverNameCallback;
    nosoundoutput.getdrivercaps = OutputNoSound::getDriverCapsCallback;
    nosoundoutput.init = OutputNoSound::initCallback;
    nosoundoutput.close = OutputNoSound::closeCallback;
    nosoundoutput.getposition = OutputNoSound::getPositionCallback;
    nosoundoutput.lock = OutputNoSound::lockCallback;

    nosoundoutput.mType = FMOD_OUTPUTTYPE_NOSOUND;
    nosoundoutput.mSize = sizeof(OutputNoSound);

    return &nosoundoutput;
}

} // namespace FMOD
