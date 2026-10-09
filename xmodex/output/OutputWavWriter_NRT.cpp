#include "xmodex/output/OutputWavWriter_NRT.hpp"
#include "xmodex/Memory.hpp"
#include "xmodex/SystemI.hpp"
#include "xmodex/codec/CodecWav.hpp"
#include "xmodex/output/Output.hpp"

namespace FMOD {

static const FMOD_GUID KSDATAFORMAT_SUBTYPE_IEEE_FLOAT = { 0x00000003, 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 } };
static const FMOD_GUID KSDATAFORMAT_SUBTYPE_PCM = { 0x00000001, 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 } };

FMOD_OUTPUT_DESCRIPTION_EX wavwriteroutput_nrt;

FMOD_RESULT OutputWavWriter_NRT::writeWavHeader() {
    if (!this->mFP) {
        return FMOD_ERR_INVALID_PARAM;
    }

    fseek(this->mFP, 0, SEEK_SET);

    {
        bool extensible = false;

        WAVE_CHUNK FmtHdr = {
            { 'f', 'm', 't', ' ' },
            sizeof(WAVE_FORMATEXTENSIBLE)
        };

        if (this->mFormat == FMOD_SOUND_FORMAT_PCMFLOAT && this->mChannels > 2) {
            extensible = true;
        }

        WAVE_FORMATEXTENSIBLE Fmt;
        FMOD_memset(&Fmt, 0, sizeof(WAVE_FORMATEXTENSIBLE));
        Fmt.Format.wFormatTag = static_cast<uint16_t>(
            extensible ? WAVE_FORMAT_EXTENSIBLE : this->mFormat == FMOD_SOUND_FORMAT_PCMFLOAT ? WAVE_FORMAT_IEEE_FLOAT
                                                                                              : WAVE_FORMAT_PCM);
        Fmt.Format.nChannels = static_cast<uint16_t>(this->mChannels);
        Fmt.Format.nSamplesPerSec = static_cast<uint32_t>(this->mRate);
        Fmt.Format.nAvgBytesPerSec = static_cast<uint32_t>(this->mRate * this->mChannels * this->mBits / 8);
        Fmt.Format.nBlockAlign = static_cast<uint16_t>(this->mChannels * this->mBits / 8);
        Fmt.Format.wBitsPerSample = static_cast<uint16_t>(this->mBits);

        if (Fmt.Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
            Fmt.Format.cbSize = 22;
            Fmt.Samples.wValidBitsPerSample = this->mBits;
            Fmt.dwChannelMask = 0;
            if (this->mFormat == FMOD_SOUND_FORMAT_PCMFLOAT) {
                FMOD_memcpy(&Fmt.SubFormat, &KSDATAFORMAT_SUBTYPE_IEEE_FLOAT, sizeof(FMOD_GUID));
            } else {
                FMOD_memcpy(&Fmt.SubFormat, &KSDATAFORMAT_SUBTYPE_PCM, sizeof(FMOD_GUID));
            }
        }

        WAVE_CHUNK DataChunk = {
            { 'd', 'a', 't', 'a' },
            this->mLengthBytes
        };

        WAVE_CHUNK WavHeader = {
            { 'R', 'I', 'F', 'F' },
            static_cast<uint32_t>(sizeof(FmtHdr) + sizeof(Fmt) + sizeof(DataChunk) + this->mLengthBytes),
        };

#if defined(XMODEX_CPU_BIGENDIAN)
        FmtHdr.size = FMOD_SWAPENDIAN_DWORD(FmtHdr.size);
        DataChunk.size = FMOD_SWAPENDIAN_DWORD(DataChunk.size);
        WavHeader.size = FMOD_SWAPENDIAN_DWORD(WavHeader.size);

        Fmt.Format.wFormatTag = FMOD_SWAPENDIAN_WORD(Fmt.Format.wFormatTag);
        Fmt.Format.nChannels = FMOD_SWAPENDIAN_WORD(Fmt.Format.nChannels);
        Fmt.Format.nSamplesPerSec = FMOD_SWAPENDIAN_DWORD(Fmt.Format.nSamplesPerSec);
        Fmt.Format.nAvgBytesPerSec = FMOD_SWAPENDIAN_DWORD(Fmt.Format.nAvgBytesPerSec);
        Fmt.Format.nBlockAlign = FMOD_SWAPENDIAN_WORD(Fmt.Format.nBlockAlign);
        Fmt.Format.wBitsPerSample = FMOD_SWAPENDIAN_WORD(Fmt.Format.wBitsPerSample);
        Fmt.Format.cbSize = FMOD_SWAPENDIAN_WORD(Fmt.Format.cbSize);

        Fmt.Samples.wValidBitsPerSample = FMOD_SWAPENDIAN_WORD(Fmt.Samples.wValidBitsPerSample);
        Fmt.dwChannelMask = FMOD_SWAPENDIAN_DWORD(Fmt.dwChannelMask);

        Fmt.SubFormat.Data1 = FMOD_SWAPENDIAN_DWORD(Fmt.SubFormat.Data1);
        Fmt.SubFormat.Data2 = FMOD_SWAPENDIAN_WORD(Fmt.SubFormat.Data2);
        Fmt.SubFormat.Data3 = FMOD_SWAPENDIAN_WORD(Fmt.SubFormat.Data3);
#endif

        int32_t written = 0;
        written = fwrite(&WavHeader, sizeof(WavHeader), 1, this->mFP);
        written = fwrite("WAVE", 4, 1, this->mFP);
        written = fwrite(&FmtHdr, sizeof(FmtHdr), 1, this->mFP);
        written = fwrite(&Fmt, sizeof(Fmt), 1, this->mFP);
        written = fwrite(&DataChunk, sizeof(DataChunk), 1, this->mFP);
    }

    return FMOD_OK;
}

FMOD_RESULT OutputWavWriter_NRT::getNumDrivers(int32_t* numdrivers) {
    *numdrivers = 1;
    return FMOD_OK;
}

FMOD_RESULT OutputWavWriter_NRT::getDriverName(int32_t id, char* name, int32_t namelen) {
    FMOD_strncpy(name, "fmodoutput.wav", namelen);

    return FMOD_OK;
}

FMOD_RESULT OutputWavWriter_NRT::getDriverCaps(int32_t id, FMOD_CAPS* caps) {
    *caps |= (FMOD_CAPS_OUTPUT_MULTICHANNEL |
              FMOD_CAPS_OUTPUT_FORMAT_PCM8 |
              FMOD_CAPS_OUTPUT_FORMAT_PCM16 |
              FMOD_CAPS_OUTPUT_FORMAT_PCM24 |
              FMOD_CAPS_OUTPUT_FORMAT_PCM32 |
              FMOD_CAPS_OUTPUT_FORMAT_PCMFLOAT);
    return FMOD_OK;
}

FMOD_RESULT OutputWavWriter_NRT::init(int32_t selecteddriver, FMOD_INITFLAGS flags, int32_t* outputrate, int32_t outputchannels, FMOD_SOUND_FORMAT* outputformat, int32_t dspbufferlength, int32_t dspnumbuffers, void* extradriverdata) {
    FMOD_RESULT result;

    Plugin::init();

    result = this->mSystem->getSoftwareFormat(
        &this->mRate,
        &this->mFormat,
        &this->mChannels,
        nullptr,
        nullptr,
        &this->mBits);
    if (result != FMOD_OK) {
        return result;
    }

    this->mBufferLength = dspbufferlength;

    result = SoundI::getBytesFromSamples(
        this->mBufferLength,
        &this->mBufferLengthBytes,
        this->mChannels,
        this->mFormat);
    if (result != FMOD_OK) {
        return result;
    }

    this->mBuffer = FMOD_Memory_Calloc(this->mBufferLengthBytes);
    if (!this->mBuffer) {
        return FMOD_ERR_MEMORY;
    }

    if (!extradriverdata) {
        FMOD_strncpy(this->mFileName, "fmodoutput.wav", FMOD_STRING_MAXPATHLEN);
    } else {
        FMOD_strncpy(this->mFileName, static_cast<char*>(extradriverdata), FMOD_STRING_MAXPATHLEN);
    }

    return FMOD_OK;
}

FMOD_RESULT OutputWavWriter_NRT::close() {
    if (this->mBuffer) {
        FMOD_Memory_Free(this->mBuffer);
        this->mBuffer = nullptr;
    }

    return FMOD_OK;
}

FMOD_RESULT OutputWavWriter_NRT::getHandle(void** handle) {
    *handle = this->mFP;

    return FMOD_OK;
}

FMOD_RESULT OutputWavWriter_NRT::update() {
    FMOD_RESULT result;

    result = this->mix(this->mBuffer, this->mBufferLength);
    if (result != FMOD_OK) {
        return FMOD_OK;
    }

    fwrite(this->mBuffer, this->mBufferLengthBytes, 1, this->mFP);
    this->mLengthBytes += this->mBufferLengthBytes;

    return FMOD_OK;
}

FMOD_RESULT OutputWavWriter_NRT::start() {
    FMOD_RESULT result;

    this->mFP = fopen(this->mFileName, "wb");
    if (!this->mFP) {
        return FMOD_ERR_FILE_NOTFOUND;
    }

    result = this->writeWavHeader();
    if (result != FMOD_OK) {
        return result;
    }

    return FMOD_OK;
}

FMOD_RESULT OutputWavWriter_NRT::stop() {
    this->writeWavHeader();

    if (this->mFP) {
        fclose(this->mFP);
        this->mFP = nullptr;
    }

    return FMOD_OK;
}

FMOD_RESULT F_CALLBACK OutputWavWriter_NRT::getNumDriversCallback(FMOD_OUTPUT_STATE* output, int32_t* numdrivers) {
    auto wavwriter = static_cast<OutputWavWriter_NRT*>(output);
    return wavwriter->getNumDrivers(numdrivers);
}

FMOD_RESULT F_CALLBACK OutputWavWriter_NRT::getDriverNameCallback(FMOD_OUTPUT_STATE* output, int32_t id, char* name, int32_t namelen) {
    auto wavwriter = static_cast<OutputWavWriter_NRT*>(output);
    return wavwriter->getDriverName(id, name, namelen);
}

FMOD_RESULT F_CALLBACK OutputWavWriter_NRT::getDriverCapsCallback(FMOD_OUTPUT_STATE* output, int32_t id, FMOD_CAPS* caps) {
    auto wavwriter = static_cast<OutputWavWriter_NRT*>(output);
    return wavwriter->getDriverCaps(id, caps);
}

FMOD_RESULT F_CALLBACK OutputWavWriter_NRT::initCallback(FMOD_OUTPUT_STATE* output, int32_t selecteddriver, FMOD_INITFLAGS flags, int32_t* outputrate, int32_t outputchannels, FMOD_SOUND_FORMAT* outputformat, int32_t dspbufferlength, int32_t dspnumbuffers, void* extradriverdata) {
    auto wavwriter = static_cast<OutputWavWriter_NRT*>(output);
    return wavwriter->init(selecteddriver, flags, outputrate, outputchannels, outputformat, dspbufferlength, dspnumbuffers, extradriverdata);
}

FMOD_RESULT F_CALLBACK OutputWavWriter_NRT::closeCallback(FMOD_OUTPUT_STATE* output) {
    auto wavwriter = static_cast<OutputWavWriter_NRT*>(output);
    return wavwriter->close();
}

FMOD_RESULT F_CALLBACK OutputWavWriter_NRT::startCallback(FMOD_OUTPUT_STATE* output) {
    auto wavwriter = static_cast<OutputWavWriter_NRT*>(output);
    return wavwriter->start();
}

FMOD_RESULT F_CALLBACK OutputWavWriter_NRT::stopCallback(FMOD_OUTPUT_STATE* output) {
    auto wavwriter = static_cast<OutputWavWriter_NRT*>(output);
    return wavwriter->stop();
}

FMOD_RESULT F_CALLBACK OutputWavWriter_NRT::updateCallback(FMOD_OUTPUT_STATE* output) {
    auto wavwriter = static_cast<OutputWavWriter_NRT*>(output);
    return wavwriter->update();
}

FMOD_RESULT F_CALLBACK OutputWavWriter_NRT::getHandleCallback(FMOD_OUTPUT_STATE* output, void** handle) {
    auto wavwriter = static_cast<OutputWavWriter_NRT*>(output);
    return wavwriter->getHandle(handle);
}

FMOD_OUTPUT_DESCRIPTION_EX* OutputWavWriter_NRT::getDescriptionEx() {
    FMOD_memset(&wavwriteroutput_nrt, 0, sizeof(FMOD_OUTPUT_DESCRIPTION_EX));
    wavwriteroutput_nrt.name = "FMOD WavWriter Output - Non real-time";

    wavwriteroutput_nrt.version = 0x10100;
    wavwriteroutput_nrt.polling = 0;

    wavwriteroutput_nrt.getnumdrivers = OutputWavWriter_NRT::getNumDriversCallback;
    wavwriteroutput_nrt.getdrivername = OutputWavWriter_NRT::getDriverNameCallback;
    wavwriteroutput_nrt.getdrivercaps = OutputWavWriter_NRT::getDriverCapsCallback;
    wavwriteroutput_nrt.init = OutputWavWriter_NRT::initCallback;
    wavwriteroutput_nrt.close = OutputWavWriter_NRT::closeCallback;
    wavwriteroutput_nrt.start = OutputWavWriter_NRT::startCallback;
    wavwriteroutput_nrt.stop = OutputWavWriter_NRT::stopCallback;
    wavwriteroutput_nrt.update = OutputWavWriter_NRT::updateCallback;
    wavwriteroutput_nrt.gethandle = OutputWavWriter_NRT::getHandleCallback;

    wavwriteroutput_nrt.mType = FMOD_OUTPUTTYPE_WAVWRITER_NRT;
    wavwriteroutput_nrt.mSize = sizeof(OutputWavWriter_NRT);

    return &wavwriteroutput_nrt;
}

} // namespace FMOD
