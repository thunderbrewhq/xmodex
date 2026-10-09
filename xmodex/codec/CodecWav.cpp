#include "xmodex/codec/CodecWav.hpp"
#include "xmodex/Memory.hpp"
#include "xmodex/SyncPoint.hpp"
#include "xmodex/codec/IMAAdpcm.hpp"
#include "xmodex/dsp/DSPCodec.hpp"
#include "xmodex/file/File.hpp"

namespace FMOD {

static const FMOD_GUID _KSDATAFORMAT_SUBTYPE_PCM = {
    0x00000001,
    0x0000,
    0x0010,
    { 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9b, 0x71 }
};

static const FMOD_GUID _KSDATAFORMAT_SUBTYPE_IEEE_FLOAT = {
    0x00000003,
    0x0000,
    0x0010,
    { 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9b, 0x71 }
};

FMOD_CODEC_DESCRIPTION_EX wavcodec;

FMOD_RESULT CodecWav::openInternal(FMOD_MODE usermode, FMOD_CREATESOUNDEXINFO* userexinfo) {
    FMOD_RESULT result;
    int32_t tag;
    char wave[4];
    WAVE_CHUNK chunk;

    this->init(FMOD_SOUND_TYPE_WAV);

    result = this->mFile->seek(0, SEEK_SET);
    if (result != FMOD_OK) {
        return result;
    }

    result = this->mFile->read(&chunk, 1, sizeof(WAVE_CHUNK), nullptr);
    if (result != FMOD_OK) {
        return result;
    }

    if (FMOD_strncmp(reinterpret_cast<const char*>(chunk.id), "RIFF", 4)) {
        return FMOD_ERR_FORMAT;
    }

    result = this->mFile->read(wave, 1, 4, nullptr);
    if (result != FMOD_OK) {
        return result;
    }

    if (FMOD_strncmp(wave, "WAVE", 4)) {
        return FMOD_ERR_FORMAT;
    }

    this->mWaveFormatMemory = static_cast<FMOD_CODEC_WAVEFORMAT*>(FMOD_Memory_Calloc(sizeof(FMOD_CODEC_WAVEFORMAT)));
    if (!this->mWaveFormatMemory) {
        return FMOD_ERR_MEMORY;
    }

    this->waveformat = this->mWaveFormatMemory;

#if defined(XMODEX_CPU_BIGENDIAN)
    chunk.size = FMOD_SWAPENDIAN_DWORD(chunk.size);
#endif

    this->mSrcDataOffset = static_cast<uint32_t>(-1);
    this->mSyncPoint = 0;
    this->mNumSyncPoints = 0;

    result = this->parseChunk(chunk.size);
    if (result != FMOD_OK) {
        return result;
    }

    if (!this->mSrcFormat) {
        return FMOD_ERR_FORMAT;
    }

    if (this->mSrcDataOffset == static_cast<uint32_t>(-1)) {
        FMOD_Memory_Free(this->mSrcFormat);
        this->mSrcFormat = nullptr;
        this->mSrcDataOffset = 0;
        return FMOD_ERR_FORMAT;
    }

    FMOD_memset(&this->mDestFormat, 0, sizeof(WAVE_FORMATEXTENSIBLE));
    this->mDestFormat.Format.wFormatTag = WAVE_FORMAT_PCM;

    tag = this->mSrcFormat->Format.wFormatTag;

    if (tag == WAVE_FORMAT_MPEG || tag == WAVE_FORMAT_MPEGLAYER3) {
        return FMOD_ERR_FORMAT;
    }

    if (tag == WAVE_FORMAT_EXTENSIBLE) {
        if (!memcmp(&this->mSrcFormat->SubFormat, &_KSDATAFORMAT_SUBTYPE_PCM, sizeof(FMOD_GUID)) ||
            !memcmp(&this->mSrcFormat->SubFormat, &_KSDATAFORMAT_SUBTYPE_IEEE_FLOAT, sizeof(FMOD_GUID))) {
            FMOD_memcpy(&this->mDestFormat, this->mSrcFormat, sizeof(WAVE_FORMATEXTENSIBLE));

            waveformat[0].lengthpcm = static_cast<uint32_t>(static_cast<FMOD_UINT64>(waveformat[0].lengthbytes) * static_cast<FMOD_UINT64>(8) / static_cast<FMOD_UINT64>(this->mDestFormat.Format.wBitsPerSample) / static_cast<FMOD_UINT64>(this->mDestFormat.Format.nChannels));
            waveformat[0].channelmask = this->mDestFormat.dwChannelMask;

            if (tag == WAVE_FORMAT_IEEE_FLOAT || !memcmp(&this->mSrcFormat->SubFormat, &_KSDATAFORMAT_SUBTYPE_IEEE_FLOAT, sizeof(FMOD_GUID))) {
                waveformat[0].format = FMOD_SOUND_FORMAT_PCMFLOAT;
                if (this->mDestFormat.Format.wBitsPerSample != 32) {
                    return FMOD_ERR_FORMAT;
                }
            } else {
                result = SoundI::getFormatFromBits(this->mDestFormat.Format.wBitsPerSample, &waveformat[0].format);
                if (result != FMOD_OK) {
                    return result;
                }
            }
        } else {
            return FMOD_ERR_FORMAT;
        }
    } else if (tag == WAVE_FORMAT_PCM || tag == WAVE_FORMAT_IEEE_FLOAT) {
        FMOD_memcpy(&this->mDestFormat, this->mSrcFormat, sizeof(WAVE_FORMATEX));

        waveformat[0].lengthpcm = static_cast<uint32_t>(static_cast<FMOD_UINT64>(waveformat[0].lengthbytes) * static_cast<FMOD_UINT64>(8) / static_cast<FMOD_UINT64>(this->mDestFormat.Format.wBitsPerSample) / static_cast<FMOD_UINT64>(this->mDestFormat.Format.nChannels));

        if (tag == WAVE_FORMAT_IEEE_FLOAT) {
            waveformat[0].format = FMOD_SOUND_FORMAT_PCMFLOAT;
            if (this->mDestFormat.Format.wBitsPerSample != 32) {
                return FMOD_ERR_FORMAT;
            }
        } else {
            result = SoundI::getFormatFromBits(this->mDestFormat.Format.wBitsPerSample, &waveformat[0].format);
            if (result != FMOD_OK) {
                return result;
            }
        }
    } else if (tag == WAVE_FORMAT_IMA_ADPCM || tag == WAVE_FORMAT_XBOX_ADPCM) {
        auto formatadpcm = reinterpret_cast<WAVE_FORMAT_IMAADPCM*>(this->mSrcFormat);

        int32_t numblocks = waveformat[0].lengthbytes / formatadpcm->wfx.nBlockAlign;
        waveformat[0].lengthpcm = numblocks * formatadpcm->wSamplesPerBlock;

        FMOD_memcpy(&this->mDestFormat, this->mSrcFormat, sizeof(WAVE_FORMATEX));

        if (usermode & FMOD_CREATECOMPRESSEDSAMPLE) {
            waveformat[0].format = FMOD_SOUND_FORMAT_IMAADPCM;
        } else {
            waveformat[0].format = FMOD_SOUND_FORMAT_PCM16;
        }

        this->mDestFormat.Format.wFormatTag = WAVE_FORMAT_PCM;
        this->mDestFormat.Format.wBitsPerSample = 16;
        this->mDestFormat.Format.nBlockAlign = this->mDestFormat.Format.nChannels * this->mDestFormat.Format.wBitsPerSample / 8;
        this->mDestFormat.Format.nAvgBytesPerSec = this->mDestFormat.Format.nBlockAlign * this->mDestFormat.Format.nSamplesPerSec;

        this->mSamplesPerADPCMBlock = formatadpcm->wSamplesPerBlock;
        this->mPCMBufferLength = this->mSamplesPerADPCMBlock;
        if (usermode & FMOD_CREATECOMPRESSEDSAMPLE) {
            this->mPCMBufferLengthBytes = 0;
            this->mReadBufferLength = 0;
        } else {
            this->mPCMBufferLengthBytes = this->mPCMBufferLength * this->mDestFormat.Format.wBitsPerSample / 8 * this->mDestFormat.Format.nChannels;
            this->mReadBufferLength = this->mSrcFormat->Format.nBlockAlign;
        }
    } else {
        // MMRESULT hr;
        // HACMSTREAM has;
        //
        // hr = acmFormatSuggest(
        //     nullptr,
        //     reinterpret_cast<WAVEFORMATEX*>(this->mSrcFormat),
        //     reinterpret_cast<WAVEFORMATEX*>(this->mDestFormat),
        //     sizeof(WAVE_FORMATEX),
        //     ACM_FORMATSUGGESTF_WFORMATTAG);
        // if (hr) {
        //     return FMOD_ERR_FORMAT;
        // }
        //
        // hr = acmStreamOpen(
        //     &has,
        //     nullptr,
        //     reinterpret_cast<WAVEFORMATEX*>(this->mSrcFormat),
        //     reinterpret_cast<WAVEFORMATEX*>(&this->mDestFormat),
        //     nullptr,
        //     0,
        //     0,
        //     ACM_STREAMOPENF_NONREALTIME);
        // if (hr) {
        //     return FMOD_ERR_FORMAT;
        // }
        // this->mACMCodec = has;
        //
        // waveformat[0].format = FMOD_SOUND_FORMAT_PCM16;
        //
        // acmStreamSize(
        //     has,
        //     waveformat[0].lengthbytes,
        //     reinterpret_cast<LPDWORD>(&waveformat[0].lengthpcm),
        //     ACM_STREAMSIZEF_SOURCE);
        //
        // waveformat[0].lengthpcm = static_cast<uint32_t>(static_cast<FMOD_UINT64>(waveformat[0].lengthpcm) * static_cast<FMOD_UINT64>(8) / static_cast<FMOD_UINT64>(this->mDestFormat.Format.wBitsPerSample) / static_cast<FMOD_UINT64>(this->mDestFormat.Format.nChannels));
        //
        // if (this->mSrcFormat->Format.nBlockAlign) {
        //     this->mReadBufferLength = this->mSrcFormat->Format.nBlockAlign;
        //     if (this->mReadBufferLength <= 1) {
        //         this->mReadBufferLength = 1024;
        //     }
        //
        //     acmStreamSize(
        //         has,
        //         this->mReadBufferLength,
        //         reinterpret_cast<LPDWORD>(&this->mPCMBufferLengthBytes),
        //         ACM_STREAMSIZEF_SOURCE);
        //     if (!mPCMBufferLengthBytes) {
        //         return FMOD_ERR_MEMORY;
        //     }
        // }

        return FMOD_ERR_FORMAT;
    }

    if (this->mReadBufferLength) {
        this->mReadBuffer = static_cast<uint8_t*>(FMOD_Memory_Calloc(this->mReadBufferLength));
        if (!this->mReadBuffer) {
            return FMOD_ERR_MEMORY;
        }
    }

    if (this->mPCMBufferLengthBytes) {
        this->mPCMBufferMemory = static_cast<uint8_t*>(FMOD_Memory_Calloc(this->mPCMBufferLengthBytes));
        if (!this->mPCMBufferMemory) {
            return FMOD_ERR_MEMORY;
        }
        this->mPCMBuffer = this->mPCMBufferMemory;
    }

    waveformat[0].channels = this->mSrcFormat->Format.nChannels;
    waveformat[0].frequency = this->mSrcFormat->Format.nSamplesPerSec;
    waveformat[0].blockalign = this->mSrcFormat->Format.nBlockAlign;
    waveformat[0].loopstart = this->mLoopPoints[0];
    waveformat[0].loopend = this->mLoopPoints[1];

    if (this->mLoopPoints[1] > this->mLoopPoints[0]) {
        waveformat[0].mode = FMOD_LOOP_NORMAL;
    }

    if (waveformat[0].format == FMOD_SOUND_FORMAT_IMAADPCM) {
        if (waveformat[0].channels > 2) {
            return FMOD_ERR_TOOMANYCHANNELS;
        }

        this->mReadBufferLength = this->mSrcFormat->Format.nBlockAlign;

        if (!mSystem->mDSPCodecPool_ADPCM.mNumDSPCodecs) {
            result = mSystem->mDSPCodecPool_ADPCM.init(
                FMOD_DSP_CATEGORY_DSPCODECADPCM,
                64,
                this->mSystem->mAdvancedSettings.maxADPCMcodecs ? this->mSystem->mAdvancedSettings.maxADPCMcodecs : 32);
            if (result != FMOD_OK) {
                return result;
            }

            for (int32_t i = 0; i < this->mSystem->mDSPCodecPool_ADPCM.mNumDSPCodecs; i++) {
                auto dspcodec = static_cast<DSPCodec*>(this->mSystem->mDSPCodecPool_ADPCM.mPool[i]);
                auto wav = static_cast<CodecWav*>(dspcodec->mCodec);

                wav->mSrcFormat = &wav->mSrcFormatMemory;
                wav->mReadBuffer = this->mSystem->mDSPCodecPool_ADPCM.mReadBuffer;
                wav->mSrcFormat->Format.wFormatTag = WAVE_FORMAT_IMA_ADPCM;
            }
        }

        for (int32_t i = 0; i < this->mSystem->mDSPCodecPool_ADPCM.mNumDSPCodecs; i++) {
            auto dspcodec = static_cast<DSPCodec*>(this->mSystem->mDSPCodecPool_ADPCM.mPool[i]);
            auto wav = static_cast<CodecWav*>(dspcodec->mCodec);

            if (!wav->mPCMBufferMemory) {
                wav->mPCMBufferMemory = static_cast<uint8_t*>(FMOD_Memory_Calloc(this->mPCMBufferLength * this->mDestFormat.Format.wBitsPerSample / 8 * 2));
                if (!wav->mPCMBufferMemory) {
                    return FMOD_ERR_MEMORY;
                }

                wav->mPCMBuffer = wav->mPCMBufferMemory;
            }
        }
    }

    this->numsubsounds = 0;

    return result;
}

FMOD_RESULT CodecWav::closeInternal() {
    if (this->mSrcFormat && this->mSrcFormat != &this->mSrcFormatMemory) {
        FMOD_Memory_Free(this->mSrcFormat);
        this->mSrcFormat = nullptr;
    }

    if (this->mWaveFormatMemory) {
        FMOD_Memory_Free(this->mWaveFormatMemory);
        this->mWaveFormatMemory = nullptr;
    }

    if (this->mReadBuffer) {
        FMOD_Memory_Free(this->mReadBuffer);
        this->mReadBuffer = nullptr;
    }
    this->mReadBufferLength = 0;

    if (this->mSyncPoint) {
        FMOD_Memory_Free(this->mSyncPoint);
        this->mSyncPoint = nullptr;
    }
    this->mNumSyncPoints = 0;

    if (this->mPCMBuffer) {
        FMOD_Memory_Free(this->mPCMBuffer);
        this->mPCMBuffer = nullptr;
    }
    this->mPCMBufferLengthBytes = 0;

    return FMOD_OK;
}

FMOD_RESULT CodecWav::readInternal(void* buffer, uint32_t sizebytes, uint32_t* bytesread) {
    FMOD_RESULT result;
    bool finished = false;

    if (this->mSrcFormat->Format.wFormatTag == WAVE_FORMAT_PCM || this->mSrcFormat->Format.wFormatTag == WAVE_FORMAT_IEEE_FLOAT || this->mSrcFormat->Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
        uint32_t pos;

        this->mFile->tell(&pos);

        if (pos >= (this->mSrcDataOffset + waveformat[0].lengthbytes)) {
            return FMOD_ERR_FILE_EOF;
        }
        if (pos + sizebytes > this->mSrcDataOffset + waveformat[0].lengthbytes) {
            sizebytes = (this->mSrcDataOffset + waveformat[0].lengthbytes) - pos;
            finished = true;
        }

        if (waveformat[0].format == FMOD_SOUND_FORMAT_PCM8) {
            auto ptr = reinterpret_cast<uint8_t*>(buffer);
            uint32_t len;

            result = this->mFile->read(buffer, 1, sizebytes, bytesread);

            len = *bytesread >> 2;
            while (len) {
                ptr[0] ^= 128;
                ptr[1] ^= 128;
                ptr[2] ^= 128;
                ptr[3] ^= 128;
                ptr += 4;
                len--;
            }
            len = *bytesread & 3;
            while (len) {
                ptr[0] ^= 128;
                ptr++;
                len--;
            }
        } else if (waveformat[0].format == FMOD_SOUND_FORMAT_PCM16) {
            result = this->mFile->read(buffer, 2, sizebytes / 2, bytesread);

            *bytesread *= 2;
        } else {
            result = this->mFile->read(buffer, 1, sizebytes, bytesread);
        }

        if (finished) {
            result = FMOD_ERR_FILE_EOF;
        }

        if (result != FMOD_OK) {
            return result;
        }
    } else if (this->mSrcFormat->Format.wFormatTag == WAVE_FORMAT_IMA_ADPCM || this->mSrcFormat->Format.wFormatTag == WAVE_FORMAT_XBOX_ADPCM) {
        int32_t blockalign = waveformat[0].blockalign;
        auto destbuff = reinterpret_cast<int16_t*>(buffer);
        auto readbuff = reinterpret_cast<uint8_t*>(FMOD_alloca(this->mReadBufferLength));

        if (!readbuff) {
            return FMOD_ERR_MEMORY;
        }

        result = this->mFile->read(readbuff, 1, this->mReadBufferLength, nullptr);
        if (result != FMOD_OK) {
            return result;
        }

        if (waveformat[0].format == FMOD_SOUND_FORMAT_IMAADPCM) {
            if (waveformat[0].channels == 1) {
                result = IMAAdpcm_DecodeM16(readbuff, destbuff, 1, blockalign, this->mSamplesPerADPCMBlock, 1);
            } else if (waveformat[0].channels == 2) {
                result = IMAAdpcm_DecodeS16(readbuff, destbuff, 1, blockalign, this->mSamplesPerADPCMBlock);
            } else {
                blockalign /= waveformat[0].channels;

                for (int32_t i = 0; i < waveformat[0].channels; i++) {
                    int16_t tempin[4096];

                    for (int32_t j = 0; j < static_cast<int32_t>(this->mReadBufferLength) / waveformat[0].channels; j++) {
                        tempin[j] = reinterpret_cast<int16_t*>(readbuff)[(j * waveformat[0].channels) + i];
                    }

                    result = IMAAdpcm_DecodeM16(reinterpret_cast<uint8_t*>(tempin), destbuff + i, 1, blockalign, this->mSamplesPerADPCMBlock, waveformat[0].channels);
                }
            }

            *bytesread = this->mSamplesPerADPCMBlock * sizeof(int16_t) * waveformat[0].channels;
        } else {
            if (waveformat[0].channels == 1) {
                result = IMAAdpcm_DecodeM16(readbuff, destbuff, 1, blockalign, this->mSamplesPerADPCMBlock, 1);
            } else if (waveformat[0].channels == 2) {
                result = IMAAdpcm_DecodeS16(readbuff, destbuff, 1, blockalign, this->mSamplesPerADPCMBlock);
            } else {
                blockalign /= waveformat[0].channels;

                for (int32_t i = 0; i < waveformat[0].channels; i++) {
                    int16_t tempin[4096];

                    for (int32_t j = 0; j < static_cast<int32_t>(this->mReadBufferLength) / waveformat[0].channels; j++) {
                        tempin[j] = reinterpret_cast<int16_t*>(readbuff)[(j * waveformat[0].channels) + i];
                    }

                    result = IMAAdpcm_DecodeM16(reinterpret_cast<uint8_t*>(tempin), destbuff + i, 1, blockalign, this->mSamplesPerADPCMBlock, waveformat[0].channels);
                }
            }

            *bytesread = this->mSamplesPerADPCMBlock * sizeof(int16_t) * waveformat[0].channels;
        }
    }
    // else if (this->mACMCodec) {
    //     ACMSTREAMHEADER ash;
    //     MMRESULT mmresult;
    //
    //     result = this->mFile->read(this->mReadBuffer, 1, this->mReadBufferLength, nullptr);
    //     if (result != FMOD_OK) {
    //         return result;
    //     }
    //
    //     FMOD_memset(&ash, 0, sizeof(ACMSTREAMHEADER));
    //     ash.cbStruct = sizeof(ACMSTREAMHEADER);
    //     ash.pbSrc = this->mReadBuffer;
    //     ash.cbSrcLength = this->mReadBufferLength;
    //     ash.pbDst = reinterpret_cast<uint8_t*>(buffer);
    //     ash.cbDstLength = sizebytes;
    //
    //     mmresult = acmStreamPrepareHeader(this->mACMCodec, &ash, 0);
    //
    //     mmresult = acmStreamConvert(this->mACMCodec, &ash, ACM_STREAMCONVERTF_BLOCKALIGN);
    //
    //     *bytesread = ash.cbDstLengthUsed;
    //
    //     mmresult = acmStreamUnprepareHeader(this->mACMCodec, &ash, 0);
    // }
    else {
        return FMOD_ERR_PLUGIN_MISSING;
    }

    return FMOD_OK;
}

FMOD_RESULT CodecWav::soundCreateInternal(int32_t subsound, FMOD_SOUND* sound) {
    FMOD_RESULT result = FMOD_OK;
    auto s = reinterpret_cast<SoundI*>(sound);

    if (this->mNumSyncPoints && this->mSyncPoint) {
        for (int32_t i = 0; i < this->mNumSyncPoints; i++) {
            auto point = &this->mSyncPoint[i];
            s->addSyncPoint(
                point->mOffset,
                FMOD_TIMEUNIT_PCM,
                point->mName,
                reinterpret_cast<FMOD_SYNCPOINT**>(&point),
                0,
                false);
        }

        FMOD_Memory_Free(this->mSyncPoint);
        mSyncPoint = nullptr;
    }

    return result;
}

FMOD_RESULT CodecWav::canPointInternal() {
    // if (this->mACMCodec) {
    //     return FMOD_ERR_MEMORY_CANTPOINT;
    // }

    if (!this->mSrcFormat) {
        return FMOD_ERR_MEMORY_CANTPOINT;
    }

    if (this->mSrcFormat->Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
        if (memcmp(&this->mSrcFormat->SubFormat, &_KSDATAFORMAT_SUBTYPE_PCM, sizeof(FMOD_GUID)) &&
            memcmp(&this->mSrcFormat->SubFormat, &_KSDATAFORMAT_SUBTYPE_IEEE_FLOAT, sizeof(FMOD_GUID))) {
            return FMOD_ERR_MEMORY_CANTPOINT;
        }
    } else if (this->mSrcFormat->Format.wFormatTag != WAVE_FORMAT_PCM && this->mSrcFormat->Format.wFormatTag != WAVE_FORMAT_IEEE_FLOAT) {
        return FMOD_ERR_MEMORY_CANTPOINT;
    }

    if (this->mSrcFormat->Format.wBitsPerSample == 8) {
        return FMOD_ERR_MEMORY_CANTPOINT;
    }

    return FMOD_OK;
}

FMOD_RESULT CodecWav::setPositionInternal(int32_t subsound, uint32_t position, FMOD_TIMEUNIT postype) {
    FMOD_RESULT result;
    uint32_t raw = 0;
    uint32_t pcmbytes = 0;
    uint32_t pcmbytesaligned = 0;
    uint32_t pcmaligned = 0;
    uint32_t excessbytes = 0;

    if (postype == FMOD_TIMEUNIT_RAWBYTES) {
        result = this->mFile->seek(this->mSrcDataOffset + position, SEEK_SET);
        return result;
    }

    if (position) {
        raw = static_cast<uint32_t>(static_cast<FMOD_UINT64>(position) * static_cast<FMOD_UINT64>(waveformat[0].lengthbytes) / static_cast<FMOD_UINT64>(waveformat[0].lengthpcm));
        raw /= waveformat[0].blockalign;
        raw *= waveformat[0].blockalign;

        pcmaligned = static_cast<uint32_t>(static_cast<FMOD_UINT64>(raw) * static_cast<FMOD_UINT64>(waveformat[0].lengthpcm) / static_cast<FMOD_UINT64>(waveformat[0].lengthbytes));

        result = SoundI::getBytesFromSamples(position, &pcmbytes, waveformat[0].channels, waveformat[0].format);
        if (result != FMOD_OK) {
            return result;
        }

        result = SoundI::getBytesFromSamples(pcmaligned, &pcmbytesaligned, waveformat[0].channels, waveformat[0].format);
        if (result != FMOD_OK) {
            return result;
        }
    }

    result = this->mFile->seek(this->mSrcDataOffset + raw, SEEK_SET);
    if (result != FMOD_OK) {
        return result;
    }

    excessbytes = pcmbytes - pcmbytesaligned;

    while (excessbytes) {
        char buff[4096];
        uint32_t read = 0;
        uint32_t toread = 1000;

        if (toread > excessbytes) {
            toread = excessbytes;
        }

        result = Codec::read(buff, toread, &read);
        if (result != FMOD_OK) {
            break;
        }

        if (excessbytes >= read) {
            excessbytes -= read;
        } else {
            excessbytes = 0;
        }
    }

    return result;
}

FMOD_RESULT CodecWav::parseChunk(uint32_t chunksize) {
    uint32_t offset;
    uint32_t fileoffset;
    FMOD_RESULT result;
    bool done = false;

    result = this->mFile->tell(&fileoffset);
    if (result != FMOD_OK) {
        return result;
    }

    offset = 4;
    fileoffset -= sizeof(WAVE_CHUNK);

    do {
        WAVE_CHUNK chunk;

        result = this->mFile->seek(fileoffset + sizeof(WAVE_CHUNK), SEEK_SET);
        if (result != FMOD_OK) {
            break;
        }

        result = this->mFile->read(&chunk, 1, sizeof(WAVE_CHUNK), 0);
        if (result != FMOD_OK) {
            break;
        }

#if defined(XMODEX_CPU_BIGENDIAN)
        chunk.size = FMOD_SWAPENDIAN_DWORD(chunk.size);
#endif

        if (!FMOD_strncmp(reinterpret_cast<const char*>(chunk.id), "fmt ", 4)) {
            this->mSrcFormat = static_cast<WAVE_FORMATEXTENSIBLE*>(FMOD_Memory_Calloc(chunk.size < sizeof(WAVE_FORMATEXTENSIBLE) ? sizeof(WAVE_FORMATEXTENSIBLE) : chunk.size));
            if (!this->mSrcFormat) {
                return FMOD_ERR_MEMORY;
            }

            result = this->mFile->read(this->mSrcFormat, 1, chunk.size, 0);
            if (result != FMOD_OK) {
                break;
            }

#if defined(XMODEX_CPU_BIGENDIAN)
            this->mSrcFormat->Format.wFormatTag = FMOD_SWAPENDIAN_WORD(this->mSrcFormat->Format.wFormatTag);
            this->mSrcFormat->Format.nChannels = FMOD_SWAPENDIAN_WORD(this->mSrcFormat->Format.nChannels);
            this->mSrcFormat->Format.nSamplesPerSec = FMOD_SWAPENDIAN_DWORD(this->mSrcFormat->Format.nSamplesPerSec);
            this->mSrcFormat->Format.nAvgBytesPerSec = FMOD_SWAPENDIAN_DWORD(this->mSrcFormat->Format.nAvgBytesPerSec);
            this->mSrcFormat->Format.nBlockAlign = FMOD_SWAPENDIAN_WORD(this->mSrcFormat->Format.nBlockAlign);
            this->mSrcFormat->Format.wBitsPerSample = FMOD_SWAPENDIAN_WORD(this->mSrcFormat->Format.wBitsPerSample);
            this->mSrcFormat->Format.cbSize = FMOD_SWAPENDIAN_WORD(this->mSrcFormat->Format.cbSize);

            this->mSrcFormat->Samples.wValidBitsPerSample = FMOD_SWAPENDIAN_WORD(this->mSrcFormat->Samples.wValidBitsPerSample);
            this->mSrcFormat->dwChannelMask = FMOD_SWAPENDIAN_DWORD(this->mSrcFormat->dwChannelMask);

            if (this->mSrcFormat->Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
                this->mSrcFormat->SubFormat.Data1 = FMOD_SWAPENDIAN_DWORD(this->mSrcFormat->SubFormat.Data1);
                this->mSrcFormat->SubFormat.Data2 = FMOD_SWAPENDIAN_WORD(this->mSrcFormat->SubFormat.Data2);
                this->mSrcFormat->SubFormat.Data3 = FMOD_SWAPENDIAN_WORD(this->mSrcFormat->SubFormat.Data3);
            }
#endif
        } else if (!FMOD_strncmp(reinterpret_cast<const char*>(chunk.id), "cue ", 4)) {
            result = this->mFile->read(&this->mNumSyncPoints, 4, 1, 0);
            if (this->mNumSyncPoints) {
                if (this->mSyncPoint) {
                    FMOD_Memory_Free(this->mSyncPoint);
                }

                this->mSyncPoint = static_cast<SyncPoint*>(FMOD_Memory_Calloc(this->mNumSyncPoints * sizeof(SyncPoint)));
                if (!this->mSyncPoint) {
                    return FMOD_ERR_MEMORY;
                }

                for (int32_t i = 0; i < this->mNumSyncPoints; i++) {
                    WAVE_CUEPOINT cue;
                    auto point = &this->mSyncPoint[i];

                    result = this->mFile->read(&cue, 1, sizeof(WAVE_CUEPOINT), 0);
                    if (result != FMOD_OK) {
                        break;
                    }

#if defined(XMODEX_CPU_BIGENDIAN)
                    cue.dwSampleOffset = FMOD_SWAPENDIAN_DWORD(cue.dwSampleOffset);
                    cue.dwIdentifier = FMOD_SWAPENDIAN_DWORD(cue.dwIdentifier);
#endif

                    point->mOffset = cue.dwSampleOffset;
                    point->mIndex = cue.dwIdentifier;
                }
            }
        } else if (!FMOD_strncmp(reinterpret_cast<const char*>(chunk.id), "fact", 4)) {
            uint32_t fact;

            result = this->mFile->read(&fact, 4, 1, 0);
            if (result != FMOD_OK) {
                break;
            }

            waveformat[0].lengthpcm = fact;
        } else if (!FMOD_strncmp(reinterpret_cast<const char*>(chunk.id), "LIST", 4)) {
            char listid[4];

            result = this->mFile->read(listid, 1, 4, 0);
            if (result != FMOD_OK) {
                break;
            }

            result = this->parseChunk(chunk.size);
            if (result != FMOD_OK) {
                break;
            }
        } else if (!FMOD_strncmp(reinterpret_cast<const char*>(chunk.id), "labl", 4)) {
            if (this->mSyncPoint) {
                int32_t id;

                result = this->mFile->read(&id, 4, 1, 0);
                if (result != FMOD_OK) {
                    break;
                }

                for (int32_t i = 0; i < this->mNumSyncPoints; i++) {
                    auto p = &this->mSyncPoint[i];

                    if (p->mIndex == id) {
                        result = this->mFile->read(p->mName, 1, chunk.size - 4, nullptr);
                        if (result != FMOD_OK) {
                            break;
                        }
                        break;
                    }
                }
            }
        } else if (!FMOD_strncmp(reinterpret_cast<const char*>(chunk.id), "smpl", 4)) {
            WAVE_SMPLHEADER smpl;
            uint32_t rd;

            result = this->mFile->read(&smpl, 1, sizeof(WAVE_SMPLHEADER), &rd);
            if (result != FMOD_OK) {
                break;
            }

            if (rd == sizeof(WAVE_SMPLHEADER)) {
#if defined(XMODEX_CPU_BIGENDIAN)
                smpl.Manufacturer = FMOD_SWAPENDIAN_DWORD(smpl.Manufacturer);
                smpl.Product = FMOD_SWAPENDIAN_DWORD(smpl.Product);
                smpl.SamplePeriod = FMOD_SWAPENDIAN_DWORD(smpl.SamplePeriod);
                smpl.Note = FMOD_SWAPENDIAN_DWORD(smpl.Note);
                smpl.FineTune = FMOD_SWAPENDIAN_DWORD(smpl.FineTune);
                smpl.SMPTEFormat = FMOD_SWAPENDIAN_DWORD(smpl.SMPTEFormat);
                smpl.SMPTEOffset = FMOD_SWAPENDIAN_DWORD(smpl.SMPTEOffset);
                smpl.Loops = FMOD_SWAPENDIAN_DWORD(smpl.Loops);
                smpl.SamplerData = FMOD_SWAPENDIAN_DWORD(smpl.SamplerData);

                smpl.Loop.Identifier = FMOD_SWAPENDIAN_DWORD(smpl.Loop.Identifier);
                smpl.Loop.Type = FMOD_SWAPENDIAN_DWORD(smpl.Loop.Type);
                smpl.Loop.Start = FMOD_SWAPENDIAN_DWORD(smpl.Loop.Start);
                smpl.Loop.End = FMOD_SWAPENDIAN_DWORD(smpl.Loop.End);
                smpl.Loop.Fraction = FMOD_SWAPENDIAN_DWORD(smpl.Loop.Fraction);
                smpl.Loop.Count = FMOD_SWAPENDIAN_DWORD(smpl.Loop.Count);
#endif

                if (smpl.Loops) {
                    this->mLoopPoints[0] = smpl.Loop.Start;
                    this->mLoopPoints[1] = smpl.Loop.End;
                }
            }
        } else if (!FMOD_strncmp(reinterpret_cast<const char*>(chunk.id), "data", 4)) {
            if (this->mSrcDataOffset == static_cast<uint32_t>(-1)) {
                waveformat[0].lengthbytes = chunk.size;

                result = this->mFile->tell(&this->mSrcDataOffset);
                if (result != FMOD_OK) {
                    break;
                }
            }

            if (this->mFile->mSeekable) {
                result = this->mFile->seek(chunk.size, SEEK_CUR);
                if (result != FMOD_OK) {
                    break;
                }
            } else {
                done = true;
            }
        } else {
            this->mFile->seek(chunk.size, SEEK_CUR);
            if (result != FMOD_OK) {
                break;
            }
        }

        offset += (chunk.size + sizeof(WAVE_CHUNK));
        fileoffset += (chunk.size + sizeof(WAVE_CHUNK));

        if (chunk.size & 1) {
            offset++;
            fileoffset++;
        }

        if (chunk.size < 0) {
            break;
        }

    } while (offset < chunksize && offset > 0 && !done);

    if (result == FMOD_ERR_FILE_EOF) {
        result = FMOD_OK;
    }

    return result;
}

FMOD_RESULT F_CALLBACK CodecWav::openCallback(FMOD_CODEC_STATE* codec, FMOD_MODE usermode, FMOD_CREATESOUNDEXINFO* userexinfo) {
    auto wav = static_cast<CodecWav*>(codec);
    return wav->openInternal(usermode, userexinfo);
}

FMOD_RESULT F_CALLBACK CodecWav::closeCallback(FMOD_CODEC_STATE* codec) {
    auto wav = static_cast<CodecWav*>(codec);
    return wav->closeInternal();
}

FMOD_RESULT F_CALLBACK CodecWav::soundCreateCallback(FMOD_CODEC_STATE* codec, int32_t subsound, FMOD_SOUND* sound) {
    auto wav = static_cast<CodecWav*>(codec);
    return wav->soundCreateInternal(subsound, sound);
}

FMOD_RESULT F_CALLBACK CodecWav::canPointCallback(FMOD_CODEC_STATE* codec) {
    auto wav = static_cast<CodecWav*>(codec);
    return wav->canPointInternal();
}

FMOD_RESULT F_CALLBACK CodecWav::readCallback(FMOD_CODEC_STATE* codec, void* buffer, uint32_t sizebytes, uint32_t* bytesread) {
    auto wav = static_cast<CodecWav*>(codec);
    return wav->readInternal(buffer, sizebytes, bytesread);
}

FMOD_RESULT F_CALLBACK CodecWav::setPositionCallback(FMOD_CODEC_STATE* codec, int32_t subsound, uint32_t position, FMOD_TIMEUNIT postype) {
    auto wav = static_cast<CodecWav*>(codec);
    return wav->setPositionInternal(subsound, position, postype);
}

FMOD_CODEC_DESCRIPTION_EX* CodecWav::getDescriptionEx() {
    FMOD_memset(&wavcodec, 0, sizeof(FMOD_CODEC_DESCRIPTION_EX));

    wavcodec.name = "FMOD Wav Codec";
    wavcodec.version = 0x10100;
    wavcodec.timeunits = FMOD_TIMEUNIT_PCM | FMOD_TIMEUNIT_RAWBYTES;

    wavcodec.open = CodecWav::openCallback;
    wavcodec.close = CodecWav::closeCallback;
    wavcodec.read = CodecWav::readCallback;
    wavcodec.setposition = CodecWav::setPositionCallback;
    wavcodec.soundcreate = CodecWav::soundCreateCallback;
    wavcodec.canpoint = CodecWav::canPointCallback;

    wavcodec.mType = FMOD_SOUND_TYPE_WAV;
    wavcodec.mSize = sizeof(CodecWav);

    return &wavcodec;
}

} // namespace FMOD
