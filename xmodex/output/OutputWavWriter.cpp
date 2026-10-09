#include "xmodex/output/OutputWavWriter.hpp"
#include "fmod.hpp"
#include "xmodex/Memory.hpp"
#include "xmodex/SystemI.hpp"
#include "xmodex/codec/CodecWav.hpp"
#include "xmodex/os/Time.hpp"

namespace FMOD {

static const FMOD_GUID _KSDATAFORMAT_SUBTYPE_IEEE_FLOAT = { 0x00000003, 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 } };
static const FMOD_GUID _KSDATAFORMAT_SUBTYPE_PCM = { 0x00000001, 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 } };

FMOD_OUTPUT_DESCRIPTION_EX wavwriteroutput;

FMOD_RESULT OutputWavWriter::getNumDrivers(int32_t* numdrivers) {
    *numdrivers = 1;
    return FMOD_OK;
}

FMOD_RESULT OutputWavWriter::getDriverName(int32_t id, char* name, int32_t namelen) {
    FMOD_strncpy(name, "fmodoutput.wav", namelen);
    return FMOD_OK;
}

FMOD_RESULT OutputWavWriter::getDriverCaps(int32_t id, FMOD_CAPS* caps) {
    *caps |= (FMOD_CAPS_OUTPUT_MULTICHANNEL |
              FMOD_CAPS_OUTPUT_FORMAT_PCM8 |
              FMOD_CAPS_OUTPUT_FORMAT_PCM16 |
              FMOD_CAPS_OUTPUT_FORMAT_PCM24 |
              FMOD_CAPS_OUTPUT_FORMAT_PCM32 |
              FMOD_CAPS_OUTPUT_FORMAT_PCMFLOAT);
    return FMOD_OK;
}

FMOD_RESULT OutputWavWriter::init(int32_t selecteddriver, FMOD_INITFLAGS flags, int32_t* outputrate, int32_t outputchannels, FMOD_SOUND_FORMAT* outputformat, int32_t dspbufferlength, int32_t dspnumbuffers, void* extradriverdata) {
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

    result = SoundI::getBytesFromSamples(
        dspbufferlength,
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

FMOD_RESULT OutputWavWriter::close() {
    this->writeWavHeader();

    if (this->mFP) {
        fclose(this->mFP);
        this->mFP = nullptr;
    }

    if (this->mBuffer) {
        FMOD_Memory_Free(this->mBuffer);
        this->mBuffer = nullptr;
    }

    return FMOD_OK;
}

FMOD_RESULT OutputWavWriter::getHandle(void** handle) {
    *handle = this->mFP;
    return FMOD_OK;
}

FMOD_RESULT OutputWavWriter::getPosition(uint32_t* pcm) {
    FMOD_RESULT result;
    uint32_t pos = 0;
    int32_t outputrate;

    result = this->mSystem->getSoftwareFormat(
        &outputrate,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr);
    if (result != FMOD_OK) {
        return result;
    }

    FMOD_OS_Time_GetMs(&pos);

    pos *= outputrate;
    pos /= 1000;

    *pcm = pos;

    return FMOD_OK;
}

FMOD_RESULT OutputWavWriter::lock(uint32_t offset, uint32_t length, void** ptr1, void** ptr2, uint32_t* len1, uint32_t* len2) {
    offset %= this->mBufferLengthBytes;

    if (offset + length > this->mBufferLengthBytes) {
        *ptr1 = static_cast<char*>(this->mBuffer) + offset;
        *ptr2 = static_cast<char*>(this->mBuffer);
        *len1 = this->mBufferLengthBytes - offset;
        *len2 = length - (this->mBufferLengthBytes - offset);
    } else {
        *ptr1 = static_cast<char*>(this->mBuffer) + offset;
        *ptr2 = nullptr;
        *len1 = length;
        *len2 = 0;
    }

    return FMOD_OK;
}

FMOD_RESULT OutputWavWriter::unlock(void* ptr1, void* ptr2, uint32_t len1, uint32_t len2) {
    if (ptr1 && len1) {
        this->mLengthBytes += fwrite(ptr1, 1, len1, this->mFP);
    }

    if (ptr2 && len2) {
        this->mLengthBytes += fwrite(ptr2, 1, len2, this->mFP);
    }

    return FMOD_OK;
}

FMOD_RESULT OutputWavWriter::writeWavHeader() {
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
                FMOD_memcpy(&Fmt.SubFormat, &_KSDATAFORMAT_SUBTYPE_IEEE_FLOAT, sizeof(FMOD_GUID));
            } else {
                FMOD_memcpy(&Fmt.SubFormat, &_KSDATAFORMAT_SUBTYPE_PCM, sizeof(FMOD_GUID));
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

FMOD_RESULT F_CALLBACK OutputWavWriter::getNumDriversCallback(FMOD_OUTPUT_STATE* output, int32_t* numdrivers) {
    auto wavwriter = static_cast<OutputWavWriter*>(output);
    return wavwriter->getNumDrivers(numdrivers);
}

FMOD_RESULT F_CALLBACK OutputWavWriter::getDriverNameCallback(FMOD_OUTPUT_STATE* output, int32_t id, char* name, int32_t namelen) {
    auto wavwriter = static_cast<OutputWavWriter*>(output);
    return wavwriter->getDriverName(id, name, namelen);
}

FMOD_RESULT F_CALLBACK OutputWavWriter::getDriverCapsCallback(FMOD_OUTPUT_STATE* output, int32_t id, FMOD_CAPS* caps) {
    auto wavwriter = static_cast<OutputWavWriter*>(output);
    return wavwriter->getDriverCaps(id, caps);
}

FMOD_RESULT F_CALLBACK OutputWavWriter::initCallback(FMOD_OUTPUT_STATE* output, int32_t selecteddriver, FMOD_INITFLAGS flags, int32_t* outputrate, int32_t outputchannels, FMOD_SOUND_FORMAT* outputformat, int32_t dspbufferlength, int32_t dspnumbuffers, void* extradriverdata) {
    auto wavwriter = static_cast<OutputWavWriter*>(output);
    return wavwriter->init(selecteddriver, flags, outputrate, outputchannels, outputformat, dspbufferlength, dspnumbuffers, extradriverdata);
}

FMOD_RESULT F_CALLBACK OutputWavWriter::closeCallback(FMOD_OUTPUT_STATE* output) {
    auto wavwriter = static_cast<OutputWavWriter*>(output);
    return wavwriter->close();
}

FMOD_RESULT F_CALLBACK OutputWavWriter::getPositionCallback(FMOD_OUTPUT_STATE* output, uint32_t* pcm) {
    auto wavwriter = static_cast<OutputWavWriter*>(output);
    return wavwriter->getPosition(pcm);
}

FMOD_RESULT F_CALLBACK OutputWavWriter::lockCallback(FMOD_OUTPUT_STATE* output, uint32_t offset, uint32_t length, void** ptr1, void** ptr2, uint32_t* len1, uint32_t* len2) {
    auto wavwriter = static_cast<OutputWavWriter*>(output);
    return wavwriter->lock(offset, length, ptr1, ptr2, len1, len2);
}

FMOD_RESULT F_CALLBACK OutputWavWriter::unlockCallback(FMOD_OUTPUT_STATE* output, void* ptr1, void* ptr2, uint32_t len1, uint32_t len2) {
    auto wavwriter = static_cast<OutputWavWriter*>(output);
    return wavwriter->unlock(ptr1, ptr2, len1, len2);
}

FMOD_RESULT F_CALLBACK OutputWavWriter::getHandleCallback(FMOD_OUTPUT_STATE* output, void** handle) {
    auto wavwriter = static_cast<OutputWavWriter*>(output);
    return wavwriter->getHandle(handle);
}

FMOD_OUTPUT_DESCRIPTION_EX* OutputWavWriter::getDescriptionEx() {
    FMOD_memset(&wavwriteroutput, 0, sizeof(FMOD_OUTPUT_DESCRIPTION));

    wavwriteroutput.name = "FMOD WavWriter Output";
    wavwriteroutput.version = 0x10100;
    wavwriteroutput.polling = true;

    wavwriteroutput.getnumdrivers = OutputWavWriter::getNumDriversCallback;
    wavwriteroutput.getdrivername = OutputWavWriter::getDriverNameCallback;
    wavwriteroutput.getdrivercaps = OutputWavWriter::getDriverCapsCallback;
    wavwriteroutput.init = OutputWavWriter::initCallback;
    wavwriteroutput.close = OutputWavWriter::closeCallback;
    wavwriteroutput.getposition = OutputWavWriter::getPositionCallback;
    wavwriteroutput.lock = OutputWavWriter::lockCallback;
    wavwriteroutput.unlock = OutputWavWriter::unlockCallback;
    wavwriteroutput.gethandle = OutputWavWriter::getHandleCallback;

    wavwriteroutput.mType = FMOD_OUTPUTTYPE_WAVWRITER;
    wavwriteroutput.mSize = sizeof(OutputWavWriter);

    return &wavwriteroutput;
}

} // namespace FMOD
