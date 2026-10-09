#include "xmodex/output/OutputSoftware.hpp"
#include "xmodex/Math.hpp"
#include "xmodex/Memory.hpp"
#include "xmodex/SystemI.hpp"
#include "xmodex/channel/ChannelPool.hpp"
#include "xmodex/channel/ChannelSoftware.hpp"
#include "xmodex/sound/SampleSoftware.hpp"

namespace FMOD {

OutputSoftware::OutputSoftware() {
    FMOD_memset(&this->mDescription, 0, sizeof(FMOD_OUTPUT_DESCRIPTION_EX));

    this->mDescription.name = "FMOD Software Output";
    this->mDescription.version = 0x10100;
    this->mDescription.polling = 0;

    this->mDescription.getsamplemaxchannels = OutputSoftware::getSampleMaxChannelsCallback;
    this->mDescription.mType = FMOD_OUTPUTTYPE_SOFTWARE;
    this->mDescription.mSize = sizeof(OutputSoftware);
    this->mChannel = nullptr;
    this->mChannelPool = nullptr;
}

FMOD_RESULT OutputSoftware::init(int32_t maxchannels) {
    FMOD_RESULT result;

    if (!this->mSystem) {
        return FMOD_ERR_UNINITIALIZED;
    }

    if (maxchannels) {
        this->mChannelPool = this->mChannelPool3D = FMOD_Object_Alloc(ChannelPool);
        if (!this->mChannelPool) {
            return FMOD_ERR_MEMORY;
        }

        result = this->mChannelPool->init(this->mSystem, this, maxchannels);
        if (result != FMOD_OK) {
            return result;
        }

        this->mChannel = static_cast<ChannelSoftware*>(FMOD_Memory_Calloc(sizeof(ChannelSoftware) * maxchannels));
        if (!mChannel) {
            return FMOD_ERR_MEMORY;
        }

        for (int32_t i = 0; i < maxchannels; i++) {
            new (&this->mChannel[i]) ChannelSoftware;

            this->mChannelPool->setChannel(i, &this->mChannel[i], this->mSystem->mChannelGroup->mDSPHead);
        }
    }

    return FMOD_OK;
}

FMOD_RESULT OutputSoftware::release() {
    if (this->mChannelPool) {
        this->mChannelPool->release();
        this->mChannelPool = nullptr;
    }
    if (this->mChannel) {
        FMOD_Memory_Free(this->mChannel);
        this->mChannel = nullptr;
    }

    return Output::release();
}

FMOD_RESULT OutputSoftware::createSample(FMOD_MODE mode, FMOD_CODEC_WAVEFORMAT* waveformat, Sample** sample) {
    FMOD_RESULT result;
    int bits = 0;
    SampleSoftware* newsample;
    unsigned int overflowbytes = 0;
    bool allocated = false;

    if (!sample) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (waveformat) {
        result = SoundI::getBitsFromFormat(waveformat->format, &bits);
        if (result != FMOD_OK) {
            return result;
        }

        if (!bits &&
            waveformat->format != FMOD_SOUND_FORMAT_NONE &&
            waveformat->format != FMOD_SOUND_FORMAT_IMAADPCM &&
            waveformat->format != FMOD_SOUND_FORMAT_MPEG) {
            return FMOD_ERR_FORMAT;
        }
    }

    if (!*sample) {
        newsample = FMOD_Object_Calloc(SampleSoftware);
        if (!newsample) {
            return FMOD_ERR_MEMORY;
        }
        allocated = true;
    } else {
        newsample = static_cast<SampleSoftware*>(*sample);
    }

    if (!waveformat) {
        *sample = newsample;
        return FMOD_OK;
    }

    newsample->mFormat = waveformat->format;

    if (waveformat->format == FMOD_SOUND_FORMAT_IMAADPCM || waveformat->format == FMOD_SOUND_FORMAT_XMA || waveformat->format == FMOD_SOUND_FORMAT_MPEG) {
        newsample->mLengthBytes = waveformat->lengthbytes;
        newsample->mLoopPointDataEnd = 0;
        overflowbytes = 0;
    } else {
        result = SoundI::getBytesFromSamples(waveformat->lengthpcm, &newsample->mLengthBytes, waveformat->channels, waveformat->format);
        if (result != FMOD_OK) {
            return result;
        }

        result = SoundI::getBytesFromSamples(FMOD_DSP_RESAMPLER_OVERFLOWLENGTH, &overflowbytes, waveformat->channels, waveformat->format);
        if (result != FMOD_OK) {
            return result;
        }

        if (overflowbytes <= 8) {
            newsample->mLoopPointDataEnd = newsample->mLoopPointDataEndMemory;
        } else {
            newsample->mLoopPointDataEnd = (char*)FMOD_Memory_Calloc(overflowbytes);
            if (!newsample->mLoopPointDataEnd) {
                return FMOD_ERR_MEMORY;
            }
        }
    }

    if (mode & FMOD_OPENMEMORY_POINT) {
        newsample->mBufferMemory = 0;
        newsample->mBuffer = 0;
    } else {
        newsample->mBufferMemory = FMOD_Memory_Calloc(newsample->mLengthBytes + (overflowbytes * 2) + 16);
        if (!newsample->mBufferMemory) {
            if (allocated) {
                FMOD_Memory_Free(newsample);
            }
            return FMOD_ERR_MEMORY;
        }

        newsample->mBuffer = reinterpret_cast<char*>(FMOD_ALIGNPOINTER(reinterpret_cast<FMOD_UINT_NATIVE>(newsample->mBufferMemory) + overflowbytes, 16));
    }

    newsample->mFormat = waveformat->format;
    newsample->mLength = waveformat->lengthpcm;

    *sample = newsample;

    return FMOD_OK;
}

int32_t OutputSoftware::getSampleMaxChannels(FMOD_MODE mode, FMOD_SOUND_FORMAT format) {
    return 16;
}

int32_t F_CALLBACK OutputSoftware::getSampleMaxChannelsCallback(FMOD_OUTPUT_STATE* output, FMOD_MODE mode, FMOD_SOUND_FORMAT format) {
    auto outputsoftware = static_cast<OutputSoftware*>(output);
    return outputsoftware->getSampleMaxChannels(mode, format);
}

} // namespace FMOD
