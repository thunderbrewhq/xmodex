#include "xmodex/dsp/DSPCodecPool.hpp"
#include "xmodex/Memory.hpp"
#include "xmodex/PluginFactory.hpp"
#include "xmodex/codec/Codec.hpp"
#include "xmodex/dsp/DSPCodec.hpp"
#include "xmodex/dsp/DSPCodecADPCM.hpp"
#include "xmodex/dsp/DSPCodecMPEG.hpp"
#include "xmodex/dsp/DSPCodecRaw.hpp"

namespace FMOD {

FMOD_RESULT DSPCodecPool::close() {
    if (this->mPool) {
        for (int32_t i = 0; i < this->mNumDSPCodecs; i++) {
            auto dspcodec = this->mPool[i];

            dspcodec->mCodec->mFile = nullptr;
            dspcodec->mCodec->mReadBuffer = nullptr;
            dspcodec->mCodec->mWaveFormatMemory = nullptr;

            if (dspcodec->mCodec->mPCMBufferMemory) {
                FMOD_Memory_Free(dspcodec->mCodec->mPCMBufferMemory);
            }

            this->mPool[i]->release(true);
        }

        FMOD_Memory_Free(this->mPool);
        this->mPool = nullptr;
    }

    if (this->mReadBuffer) {
        FMOD_Memory_Free(this->mReadBuffer);
        this->mReadBuffer = nullptr;
    }

    return FMOD_OK;
}

FMOD_RESULT DSPCodecPool::alloc(DSPI** dsp) {
    for (int32_t i = 0; i < this->mNumDSPCodecs; i++) {
        bool finished;

        this->mPool[i]->getFinished(&finished);

        if (!this->mAllocated[i] && finished) {
            this->mAllocated[i] = true;
            *dsp = this->mPool[i];
            return FMOD_OK;
        }
    }

    return FMOD_ERR_CHANNEL_ALLOC;
}

FMOD_RESULT DSPCodecPool::areAnyFree() {
    for (int32_t i = 0; i < this->mNumDSPCodecs; i++) {
        bool finished;

        this->mPool[i]->getFinished(&finished);

        if (!this->mAllocated[i] && finished) {
            return FMOD_OK;
        }
    }

    return FMOD_ERR_CHANNEL_ALLOC;
}

FMOD_RESULT DSPCodecPool::init(FMOD_DSP_CATEGORY category, int32_t resamplerpcmblocksize, int32_t numdspcodecs) {
    FMOD_DSP_DESCRIPTION_EX descriptionex;

    if (!this->mSystem->mSoftware) {
        return FMOD_ERR_NEEDSSOFTWARE;
    }

    if (numdspcodecs >= FMOD_DSP_CODECPOOL_MAXCODECS) {
        return FMOD_ERR_INVALID_PARAM;
    }

    this->mNumDSPCodecs = numdspcodecs;

    this->mPool = static_cast<DSPCodec**>(FMOD_Memory_Calloc(sizeof(DSPCodec*) * this->mNumDSPCodecs));
    if (!this->mPool) {
        return FMOD_ERR_MEMORY;
    }

    FMOD_memcpy(&descriptionex, DSPCodec::getDescriptionEx(), sizeof(FMOD_DSP_DESCRIPTION_EX));

    for (int32_t i = 0; i < this->mNumDSPCodecs; i++) {
        FMOD_RESULT result;
        DSPCodec* dspcodec;

        descriptionex.channels = 2;
        descriptionex.mFormat = FMOD_SOUND_FORMAT_PCM16;
        descriptionex.mCategory = category;
        descriptionex.mResamplerBlockLength = resamplerpcmblocksize;

        if (category == FMOD_DSP_CATEGORY_DSPCODECMPEG) {
            descriptionex.mSize = sizeof(DSPCodecMPEG);
        } else if (category == FMOD_DSP_CATEGORY_DSPCODECADPCM) {
            descriptionex.mSize = sizeof(DSPCodecADPCM);
        } else if (category == FMOD_DSP_CATEGORY_DSPCODECRAW) {
            descriptionex.mResamplerBlockLength = 256;
            descriptionex.mSize = sizeof(DSPCodecRaw);
        } else {
            return FMOD_ERR_FORMAT;
        }

        result = this->mSystem->createDSP(&descriptionex, reinterpret_cast<DSPI**>(&dspcodec), true);
        if (result != FMOD_OK) {
            return result;
        }

        dspcodec->mFlags = 0;

        if (category == FMOD_DSP_CATEGORY_DSPCODECMPEG) {
            auto dspcodecmpeg = static_cast<DSPCodecMPEG*>(dspcodec);
            auto mpeg = &dspcodecmpeg->mCodecMemory;
            FMOD_CODEC_DESCRIPTION_EX* desc;

            result = this->mSystem->mPluginFactory->getCodec(this->mSystem->mMPEGPluginHandle, &desc);
            if (result != FMOD_OK) {
                return result;
            }

            FMOD_memcpy(&mpeg->mDescription, desc, sizeof(FMOD_CODEC_DESCRIPTION_EX));
            mpeg->mDescription.getwaveformat = Codec::defaultGetWaveFormat;

            dspcodec->mCodec = mpeg;
            dspcodec->mWaveFormat.format = FMOD_SOUND_FORMAT_MPEG;

            mpeg->mMemoryBlock = &dspcodecmpeg->mCodecMemoryBlock;
        } else if (category == FMOD_DSP_CATEGORY_DSPCODECADPCM) {
            auto dspcodecadpcm = static_cast<DSPCodecADPCM*>(dspcodec);
            auto wav = &dspcodecadpcm->mCodecMemory;
            FMOD_CODEC_DESCRIPTION_EX* desc;

            result = this->mSystem->mPluginFactory->getCodec(this->mSystem->mWAVPluginHandle, &desc);
            if (result != FMOD_OK) {
                return result;
            }

            FMOD_memcpy(&wav->mDescription, desc, sizeof(FMOD_CODEC_DESCRIPTION_EX));
            wav->mDescription.getwaveformat = Codec::defaultGetWaveFormat;

            dspcodec->mCodec = wav;
            dspcodec->mWaveFormat.format = FMOD_SOUND_FORMAT_IMAADPCM;
        } else {
            auto dspcodecraw = static_cast<DSPCodecRaw*>(dspcodec);
            auto raw = &dspcodecraw->mCodecMemory;

            FMOD_memcpy(&raw->mDescription, CodecRaw::getDescriptionEx(), sizeof(FMOD_CODEC_DESCRIPTION_EX));
            raw->mDescription.getwaveformat = Codec::defaultGetWaveFormat;

            dspcodec->mCodec = raw;
            dspcodec->mWaveFormat.format = FMOD_SOUND_FORMAT_PCM16;
        }

        dspcodec->mCodec->mFile = &dspcodec->mMemoryFile;
        dspcodec->mCodec->waveformat = &dspcodec->mWaveFormat;
        dspcodec->mCodec->mSrcDataOffset = 0;
        dspcodec->mCodec->mFlags |= FMOD_CODEC_ACCURATELENGTH;
        dspcodec->mPool = this;
        dspcodec->mPoolIndex = i;

        dspcodec->setFinished(true, true);

        this->mPool[i] = dspcodec;
        this->mAllocated[i] = false;
    }

    return FMOD_OK;
}

} // namespace FMOD
