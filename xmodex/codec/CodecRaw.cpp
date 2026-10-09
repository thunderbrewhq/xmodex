#include "xmodex/codec/CodecRaw.hpp"
#include "xmodex/codec/CodecWav.hpp"
#include "xmodex/dsp/DSPCodec.hpp"
#include "xmodex/file/File.hpp"

namespace FMOD {

FMOD_CODEC_DESCRIPTION_EX rawcodec;

FMOD_RESULT CodecRaw::closeInternal() {
    return FMOD_OK;
}

FMOD_RESULT CodecRaw::canPointInternal() {
    return FMOD_OK;
}

FMOD_RESULT CodecRaw::openInternal(FMOD_MODE usermode, FMOD_CREATESOUNDEXINFO* userexinfo) {
    FMOD_RESULT result = FMOD_OK;

    this->init(FMOD_SOUND_TYPE_RAW);

    result = this->mFile->seek(0, SEEK_SET);
    if (result != FMOD_OK) {
        return result;
    }

    this->waveformat = &this->mWaveFormat;

    result = this->mFile->getSize(&waveformat[0].lengthbytes);
    if (result != FMOD_OK) {
        return result;
    }

    this->mSrcDataOffset = 0;

    if (usermode & FMOD_SOFTWARE) {
        if (usermode & FMOD_CREATECOMPRESSEDSAMPLE) {
            if (userexinfo->format != FMOD_SOUND_FORMAT_IMAADPCM) {
                return FMOD_ERR_FORMAT;
            }
        } else if (userexinfo->format < FMOD_SOUND_FORMAT_PCM8 || userexinfo->format > FMOD_SOUND_FORMAT_PCMFLOAT) {
            return FMOD_ERR_FORMAT;
        }
    }

    waveformat[0].format = userexinfo->format;
    waveformat[0].channels = userexinfo->numchannels;
    waveformat[0].frequency = userexinfo->defaultfrequency;
    SoundI::getSamplesFromBytes(waveformat[0].lengthbytes, &waveformat[0].lengthpcm, userexinfo->numchannels, userexinfo->format);
    waveformat[0].blockalign = waveformat[0].channels * 16 / 8;

    this->numsubsounds = 0;

    if (waveformat[0].format == FMOD_SOUND_FORMAT_IMAADPCM) {
        if (waveformat[0].channels > 2) {
            return FMOD_ERR_TOOMANYCHANNELS;
        }

        this->mSamplesPerADPCMBlock = 64;
        this->mReadBufferLength = 36 * waveformat[0].channels;

        if (!this->mSystem->mDSPCodecPool_ADPCM.mNumDSPCodecs) {
            result = this->mSystem->mDSPCodecPool_ADPCM.init(
                FMOD_DSP_CATEGORY_DSPCODECADPCM,
                64,
                this->mSystem->mAdvancedSettings.maxADPCMcodecs ? this->mSystem->mAdvancedSettings.maxADPCMcodecs : 32);
            if (result != FMOD_OK) {
                return result;
            }

            for (int32_t i = 0; i < this->mSystem->mDSPCodecPool_ADPCM.mNumDSPCodecs; i++) {
                auto dspcodec = static_cast<DSPCodec*>(this->mSystem->mDSPCodecPool_ADPCM.mPool[i]);
                auto wav = reinterpret_cast<CodecWav*>(dspcodec->mCodec);

                wav->mSrcFormat = &wav->mSrcFormatMemory;
                wav->mReadBuffer = this->mSystem->mDSPCodecPool_ADPCM.mReadBuffer;
                wav->mSrcFormat->Format.wFormatTag = WAVE_FORMAT_IMA_ADPCM;
            }
        }
    }

    return result;
}

FMOD_RESULT CodecRaw::readInternal(void* buffer, uint32_t sizebytes, uint32_t* bytesread) {
    FMOD_RESULT result;

    if (this->waveformat[0].format == FMOD_SOUND_FORMAT_PCM16) {
        result = this->mFile->read(buffer, 2, sizebytes / 2, bytesread);

        *bytesread *= 2;
    } else {
        result = this->mFile->read(buffer, 1, sizebytes, bytesread);
    }

    return result;
}

FMOD_RESULT CodecRaw::setPositionInternal(int32_t subsound, uint32_t position, FMOD_TIMEUNIT postype) {
    FMOD_RESULT result;
    uint32_t raw;

    if (postype == FMOD_TIMEUNIT_RAWBYTES) {
        raw = position;
    } else {
        raw = static_cast<uint32_t>(static_cast<FMOD_UINT64>(position) * static_cast<FMOD_UINT64>(waveformat[0].lengthbytes) / static_cast<FMOD_UINT64>(waveformat[0].lengthpcm));
        raw /= waveformat[0].blockalign;
        raw *= waveformat[0].blockalign;
    }

    result = this->mFile->seek(this->mSrcDataOffset + raw, SEEK_SET);
    if (result != FMOD_OK) {
        return result;
    }

    return FMOD_OK;
}

FMOD_RESULT F_CALLBACK CodecRaw::closeCallback(FMOD_CODEC_STATE* codec) {
    auto wav = static_cast<CodecRaw*>(codec);
    return wav->closeInternal();
}

FMOD_RESULT F_CALLBACK CodecRaw::canPointCallback(FMOD_CODEC_STATE* codec) {
    auto wav = static_cast<CodecRaw*>(codec);
    return wav->canPointInternal();
}

FMOD_RESULT F_CALLBACK CodecRaw::openCallback(FMOD_CODEC_STATE* codec, FMOD_MODE usermode, FMOD_CREATESOUNDEXINFO* userexinfo) {
    auto wav = static_cast<CodecRaw*>(codec);
    return wav->openInternal(usermode, userexinfo);
}

FMOD_RESULT F_CALLBACK CodecRaw::readCallback(FMOD_CODEC_STATE* codec, void* buffer, uint32_t sizebytes, uint32_t* bytesread) {
    auto wav = static_cast<CodecRaw*>(codec);
    return wav->readInternal(buffer, sizebytes, bytesread);
}

FMOD_RESULT F_CALLBACK CodecRaw::setPositionCallback(FMOD_CODEC_STATE* codec, int32_t subsound, uint32_t position, FMOD_TIMEUNIT postype) {
    auto wav = static_cast<CodecRaw*>(codec);
    return wav->setPositionInternal(subsound, position, postype);
}

FMOD_CODEC_DESCRIPTION_EX* CodecRaw::getDescriptionEx() {
    FMOD_memset(&rawcodec, 0, sizeof(FMOD_CODEC_DESCRIPTION_EX));
    rawcodec.name = "FMOD Raw Codec";
    rawcodec.version = 0x10100;
    rawcodec.timeunits = FMOD_TIMEUNIT_PCM | FMOD_TIMEUNIT_RAWBYTES;

    rawcodec.open = CodecRaw::openCallback;
    rawcodec.close = CodecRaw::closeCallback;
    rawcodec.read = CodecRaw::readCallback;
    rawcodec.setposition = CodecRaw::setPositionCallback;
    rawcodec.canpoint = CodecRaw::canPointCallback;

    rawcodec.mType = FMOD_SOUND_TYPE_RAW;
    rawcodec.mSize = sizeof(CodecRaw);

    return &rawcodec;
}

} // namespace FMOD
